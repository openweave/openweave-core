/*
 *
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *          Defines the public interface for the Device Layer SoftwareUpdateManager object.
 */

#ifndef SOFTWARE_UPDATE_MANAGER_H
#define SOFTWARE_UPDATE_MANAGER_H

#if WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER

#include <Weave/Profiles/software-update/SoftwareUpdateProfile.h>
#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {

class SoftwareUpdateManagerImpl;

class SoftwareUpdateManager
{
    typedef ::nl::Weave::TLV::TLVWriter TLVWriter;
    typedef ::nl::Weave::Profiles::SoftwareUpdate::UpdatePriority UpdatePriority;
    typedef ::nl::Weave::Profiles::SoftwareUpdate::UpdateCondition UpdateCondition;

    using ImplClass = SoftwareUpdateManagerImpl;


public:

    // ===== Members that define the public interface of the SoftwareUpdateManager

    enum State
    {
        kState_Idle                 = 1,
        kState_ScheduledHoldoff     = 2,
        kState_Prepare              = 3,
        kState_Query                = 4,
        kState_Download             = 5,
        kState_Install              = 6,

        kState_ApplicationManaged   = 7,

        kState_MaxState
    };

    enum EventType
    {
        /**
         *  Generated when a software update check has been triggered. Provides
         *  an opportunity for the application to supply product related information
         *  to the image query.
         */
        kEvent_PrepareQuery,

        /**
         *  Provides an opportunity for the application to append additional meta-data
         *  to the software update query if needed. Generated when implementation is ready
         *  to get meta-data from the application.
         */
        kEvent_PrepareQuery_Metadata,

        /**
         *  Generated when the implementation encounters an error while
         *  preparing to send out a software update query.
         */
        kEvent_QueryPrepareFailed,

        /**
         *  Informational event to signal that a software update query
         *  has been sent.
         */
        kEvent_QuerySent,

        /**
         *  Generated when a ImageQueryResponse is received in response to
         *  a query containing information of the available update.
         */
        kEvent_SoftwareUpdateAvailable,

        /**
         *  Provides an opportunity for the application to disclose information
         *  of a partial image previously downloaded so that the download
         *  may be continued from the point where it last stopped. URI of the available
         *  software update is provided as an input parameter that the application can use
         *  to compare if the image being downloaded is the same as the partial image.
         *  Application can set output parameter PartialImageLenInBytes to 0 to indicate
         *  non-existence of a partial image for the URI provided as a input parameter.
         */
        kEvent_FetchPartialImageInfo,

        /**
         *  Information event to signal the application to clear a previous partial
         *  image download from their storage since a new but different image is
         *  available for download.
         */
        kEvent_ClearImageFromStorage,

        /**
         *  Informational event to signal the start of an image download
         *  transaction.
         */
        kEvent_StartImageDownload,

        /**
         *  Generated whenever a data block is received from the
         *  file download server. Parameters included with this event
         *  provide the data and the length of the data.
         */
        kEvent_StoreImageBlock,

        /**
         *  Event to request application to compute image integrity
         *  over the downloaded image. Sent after download is
         *  complete.
         */
        kEvent_ComputeImageIntegrity,

        /**
         *  Informational event to signal that image is ready to be installed.
         *  Sent when image integrity check was successful.
         */
        kEvent_ReadyToInstall,

        /**
         *  Informational event to signal the start of an image install to the
         *  application.
         */
        kEvent_StartInstallImage,

        /**
         *  Generated when a software update check has finished with or without
         *  errors. Parameters included with this event provide the reason for failure
         *  if the attempt finished due to a failure.
         */
        kEvent_Finished,

        /**
         *  Used to verify correct default event handling in the application.
         */
        kEvent_DefaultCheck                         = 100,

    };

    /**
     *  When a software update is available, the application can chose one of
     *  the following actions as part of the SoftwareUpdateAvailable API event
     *  callback. The default action will be set to kAction_Now.
     */
    enum ActionType
    {
        /**
         *  Ignore the download completely. A kEvent_Finished API event callback will
         *  be generated with error WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_CANCELLED if
         *  this option is selected and the retry logic will not be invoked.
         */
        kAction_Ignore,

        /**
         *  Start the download right away. A kEvent_FetchPartialImageInfo API event
         *  callback will be generated right after.
         */
        kAction_DownloadNow,

        /**
         *  Pause download on start. Scheduled software update checks (if enabled) will be suspended.
         *  State machine will remain in Download state. When ready, application can
         *  call the resume download API to proceed with download or call Abort to cancel.
         */
        kAction_DownloadLater,

        /**
         *  Allows application to manage the rest of the phases of software update such as
         *  download, image integrity validation and install. Software update manager
         *  state machine will move to the ApplicationManaged state. Scheduled software update checks (if enabled)
         *  will be suspended till application calls Abort or InstallationComplete API.
         */
        kAction_Defer_To_Application,
    };

    /**
     *  Incoming parameters sent with events generated directly from this component
     *
     */
    union InEventParam;

   /**
     *  Outgoing parameters sent with events generated directly from this component
     *
     */
    union OutEventParam;

    struct RetryParam
    {
        /**
         *  Specifies the retry attempt number.
         *  It is reset on a successful software update attempt.
         */
        uint32_t NumRetries;
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);
    typedef void (*RetryPolicyCallback)(void *aAppState, RetryParam& aRetryParam, uint32_t& aOutIntervalMsec);

    WEAVE_ERROR Abort(void);
    WEAVE_ERROR CheckNow(void);
    WEAVE_ERROR ImageInstallComplete(void);
    WEAVE_ERROR SetEventCallback(void * const aAppState, const EventCallback aEventCallback);
    WEAVE_ERROR SetQueryIntervalWindow(uint32_t aMinWaitTimeMs, uint32_t aMaxWaitTimeMs);

    bool IsInProgress(void);

    void SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback);

    State GetState(void);

    static void DefaultEventHandler(void *apAppState, EventType aEvent,
                                    const InEventParam& aInParam,
                                    OutEventParam& aOutParam);

private:
    // ===== Members for internal use by the following friends.

    // friend class SoftwareUpdateManagerImpl;
    template<class> friend class Internal::GenericPlatformManagerImpl;

    WEAVE_ERROR Init(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);

protected:

    // Construction/destruction limited to subclasses.
    SoftwareUpdateManager() = default;
    ~SoftwareUpdateManager() = default;

    // No copy, move or assignment.
    SoftwareUpdateManager(const SoftwareUpdateManager &) = delete;
    SoftwareUpdateManager(const SoftwareUpdateManager &&) = delete;
    SoftwareUpdateManager & operator=(const SoftwareUpdateManager &) = delete;
};

/**
 * Returns a reference to the public interface of the SoftwareUpdateManager singleton object.
 *
 * Weave application should use this to access features of the SoftwareUpdateManager object
 * that are common to all platforms.
 */
extern SoftwareUpdateManager & SoftwareUpdateMgr(void);

/**
 * Returns the platform-specific implementation of the SoftwareUpdateManager singleton object.
 *
 * Weave applications can use this to gain access to features of the SoftwareUpdateManager
 * that are specific to the selected platform.
 */
extern SoftwareUpdateManagerImpl & SoftwareUpdateMgrImpl(void);

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

/* Include a header file containing the implementation of the SoftwareUpdateManager
 * object for the selected platform.
 */
#ifdef EXTERNAL_SOFTWAREUPDATEMANAGERIMPL_HEADER
#include EXTERNAL_SOFTWAREUPDATEMANAGERIMPL_HEADER
#else
#define SOFTWAREUPDATEMANAGERIMPL_HEADER <Weave/DeviceLayer/WEAVE_DEVICE_LAYER_TARGET/SoftwareUpdateManagerImpl.h>
#include SOFTWAREUPDATEMANAGERIMPL_HEADER
#endif

namespace nl {
namespace Weave {
namespace DeviceLayer {

union SoftwareUpdateManager::InEventParam
{
    void Clear(void) { memset(this, 0, sizeof(*this)); }

    SoftwareUpdateManager * Source;
    struct
    {
        TLVWriter * MetaDataWriter;
    } PrepareQuery_Metadata;

    struct
    {
        WEAVE_ERROR Error;
        Profiles::StatusReporting::StatusReport *StatusReport;
    } QueryPrepareFailed;

    struct
    {
        UpdatePriority Priority;
        UpdateCondition Condition;
        uint8_t IntegrityType;
        const char *URI;
        const char *Version;
    } SoftwareUpdateAvailable;

    struct
    {
        const char *URI;
    } FetchPartialImageInfo;

    struct
    {
        uint8_t *DataBlock;
        uint32_t DataBlockLen;
    } StoreImageBlock;

    struct
    {
        uint8_t IntegrityType;
    } ClearImageFromStorage;

    struct
    {
        uint8_t IntegrityType;
        uint8_t *IntegrityValueBuf;     // Pointer to the buffer for the app to copy Integrity Value into.
        uint8_t IntegrityValueBufLen;   // Length of the provided buffer.
    } ComputeImageIntegrity;

    struct
    {
        WEAVE_ERROR Error;
        Profiles::StatusReporting::StatusReport *StatusReport;
    } Finished;
};

union SoftwareUpdateManager::OutEventParam
{
    void Clear(void) { memset(this, 0, sizeof(*this)); }

    bool DefaultHandlerCalled;
    struct
    {
        const char *PackageSpecification;
        const char *DesiredLocale;
        WEAVE_ERROR Error;
    } PrepareQuery;

    struct
    {
        WEAVE_ERROR Error;
    } PrepareQuery_Metadata;

    struct
    {
        ActionType Action;
    } SoftwareUpdateAvailable;

    struct
    {
        uint64_t PartialImageLen;
    } FetchPartialImageInfo;

    struct
    {
        WEAVE_ERROR Error;
    } StoreImageBlock;

    struct
    {
        WEAVE_ERROR Error;
    } ComputeImageIntegrity;
};

inline WEAVE_ERROR SoftwareUpdateManager::Init(void)
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline WEAVE_ERROR SoftwareUpdateManager::CheckNow(void)
{
    return static_cast<ImplClass*>(this)->_CheckNow();
}

inline WEAVE_ERROR SoftwareUpdateManager::ImageInstallComplete(void)
{
    return static_cast<ImplClass*>(this)->_ImageInstallComplete();
}

inline void SoftwareUpdateManager::OnPlatformEvent(const WeaveDeviceEvent * event)
{
    static_cast<ImplClass*>(this)->_OnPlatformEvent(event);
}

inline SoftwareUpdateManager::State SoftwareUpdateManager::GetState(void)
{
    return static_cast<ImplClass*>(this)->_GetState();
}

inline WEAVE_ERROR SoftwareUpdateManager::Abort(void)
{
    return static_cast<ImplClass*>(this)->_Abort();
}

inline bool SoftwareUpdateManager::IsInProgress(void)
{
    return static_cast<ImplClass*>(this)->_IsInProgress();
}

inline WEAVE_ERROR SoftwareUpdateManager::SetQueryIntervalWindow(uint32_t aMinRangeSecs, uint32_t aMaxRangeSecs)
{
    return static_cast<ImplClass*>(this)->_SetQueryIntervalWindow(aMinRangeSecs, aMaxRangeSecs);
}

inline void SoftwareUpdateManager::SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback)
{
    static_cast<ImplClass*>(this)->_SetRetryPolicyCallback(aRetryPolicyCallback);
}

inline WEAVE_ERROR SoftwareUpdateManager::SetEventCallback(void * const aAppState, const EventCallback aEventCallback)
{
    return static_cast<ImplClass*>(this)->_SetEventCallback(aAppState, aEventCallback);
}

inline void SoftwareUpdateManager::DefaultEventHandler(void *apAppState, EventType aEvent,
                                                       const InEventParam& aInParam,
                                                       OutEventParam& aOutParam)
{
    ImplClass::_DefaultEventHandler(apAppState, aEvent, aInParam, aOutParam);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER
#endif // SOFTWARE_UPDATE_MANAGER_H
