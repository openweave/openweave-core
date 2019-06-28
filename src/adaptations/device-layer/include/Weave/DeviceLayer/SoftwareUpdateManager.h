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

namespace Internal {
    template<class ImplClass> class GenericSoftwareUpdateManagerImpl;
    template<class ImplClass> class GenericSoftwareUpdateManagerImpl_BDX;
};

class SoftwareUpdateManagerImpl;

class SoftwareUpdateManager
{
    typedef ::nl::Weave::TLV::TLVWriter TLVWriter;
    typedef ::nl::Weave::Profiles::ReferencedString ReferencedString;
    typedef ::nl::Weave::Profiles::SoftwareUpdate::UpdatePriority UpdatePriority;
    typedef ::nl::Weave::Profiles::SoftwareUpdate::UpdateCondition UpdateCondition;

    using ImplClass = SoftwareUpdateManagerImpl;


public:

    // ===== Members that define the public interface of the SoftwareUpdateManager

    enum State
    {
        kState_Idle                 = 1,
        kState_Scheduled_Holdoff    = 2,
        kState_Preparing            = 3,
        kState_Querying             = 4,
        kState_CheckingImageState   = 5,
        kState_Downloading          = 6,
        kState_CheckingIntegrity    = 7,
        kState_Installing           = 8,

        kState_MaxState
    };

    enum EventType
    {
        /**
         * Generated when a software update check has been triggered. Provides
         * an oppurtunity for the application to supply product related information
         * to the image query.
        */
        kEvent_PrepareQuery,

        /**
         * Provides an opputunity for the application to append additional metadata
         * to the software update query if needed. Generated when implementaion is ready
         * to get metadata from the application.
        */
        kEvent_PrepareQuery_Metadata,

        /**
         * Generated when the implementation encounters an error while
         * preparing to send out a software update query.
        */
        kEvent_QueryPrepareFailed,

        /**
         * Informational event to signal that a software update query
         * has been sent.
        */
        kEvent_QuerySent,

        /**
         * Generated when a ImageQueryResponse is received in response to
         * a query containing information of the available update.
        */
        kEvent_SoftwareUpdateAvailable,

        /**
         * Provides an oppurtunity for the application to disclose information
         * of a partial or full image previously downloaded so that the download
         * may be continued from the point where it last stopped as long as the URI
         * of the images are identical.
        */
        kEvent_FetchDownloadImageState,

        /**
         * Information event to signal the application to clear a previous partial
         * image download from their storage since a new but different image is
         * avaiable for download.
        */
        kEvent_ClearDownloadImageState,

        /**
         * Informational event to signal the start of an image download
         * transaction.
        */
        kEvent_StartImageDownload,

        /**
         * Generated whenever a data block is received from the
         * file download server. Parameters included with this event
         * provide the data and the length of the data.
        */
        kEvent_StoreDataBlock,

        /**
         * Informational event to signal the successful completion of a
         * download transaction.
        */
        kEvent_DownloadComplete,

        /**
         * Event to request application to compute image integrity
         * over the downloaded image. Sent after download is
         * complete and application is ready to compute integrity.
        */
        kEvent_ComputeImageIntegrity,

        /**
         * Informational event to signal that image is ready to be installed.
         * Sent when image integrity check was successful.
        */
        kEvent_ReadyToInstall,

        /**
         * Informational event to signal the start of an image install to the
         * application.
        */
        kEvent_StartInstallImage,

        /**
         * Generated when a software update check has finished with or without
         * errors. Parameters included with this event provide the reason for failure
         * if the attempt finished due to a failure.
        */
        kEvent_Finished,
    };


    /**
     * Incoming parameters sent with events generated directly from this component
     *
     */
    union InEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

        struct
        {
            WEAVE_ERROR Reason;
            StatusReport * Report;
        } QueryPrepareFailed;

        struct
        {
            UpdatePriority Priority;
            UpdateCondition Condition;
            ReferencedString * Version;
            uint8_t IntegrityType;
        } SoftwareUpdateAvailable;

        struct
        {
            char* URI;
        } FetchDownloadImageState;

        struct
        {
            uint8_t *DataBlock;
            uint64_t DataBlockLenInBytes;
            uint64_t TotalImageLength;
        } StoreDataBlock;

        struct
        {
            uint8_t IntegrityType;
        } ClearDownloadImageState;

        struct
        {
            uint8_t IntegrityType;
        } ComputeImageIntegrity;

        struct
        {
            WEAVE_ERROR Reason;
            StatusReport * Report;
            uint64_t TimeTillNextQueryInMs;
        } Finished;
    };

   /**
     * Outgoing parameters sent with events generated directly from this component
     *
     */
    union OutEventParam
    {
        void Clear(void) { memset(this, 0, sizeof(*this)); }

        struct
        {
            char * PackageSpecification;
            char * DesiredLocale;
            bool HaveSufficientBattery;
            uint8_t CertBodyId;
        } PrepareQuery;

        struct
        {
            TLVWriter * MetaDataWriter;
        } PrepareQuery_Metadata;

        struct
        {
            bool CanDownload;
        } SoftwareUpdateAvailable;

        struct
        {
            bool IsPartialImageAvailable;
            bool IsFullImageAvailable;
            uint64_t PartialImageLenInBytes;
        } FetchDownloadImageState;

        struct
        {
            WEAVE_ERROR Reason;
        } StoreDataBlock;

        struct
        {
            bool CanInstallImage;
        } ReadyToInstall;

        struct
        {
            uint8_t *IntegrityValue;
            uint8_t IntegrityLength;
        } ComputeImageIntegrity;
    };

    struct RetryParam
    {
        uint32_t mNumRetries;     ///< Number of retries, reset on a successful attempt
    };

    typedef void (*EventCallback)(void *apAppState, EventType aEvent, const InEventParam& aInParam, OutEventParam& aOutParam);
    typedef void (*RetryPolicyCallback)(void *aAppState, RetryParam& aRetryParam, uint32_t& aOutIntervalMsec);

    WEAVE_ERROR Init(void);
    WEAVE_ERROR Abort(void);

    bool IsInProgress(void);
    bool IsDownloading(void);

    void CheckNow(void);
    void OnPlatformEvent(const WeaveDeviceEvent * event);
    void SetQueryIntervalWindow(uint32_t aMinWaitTimeInSecs, uint32_t aMaxWaitTimeInSecs);
    void SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback);
    void SetEventCallback(void * const aAppState, const EventCallback aEventCallback);

    State GetState(void);

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

inline WEAVE_ERROR SoftwareUpdateManager::Init(void)
{
    return static_cast<ImplClass*>(this)->_Init();
}

inline void SoftwareUpdateManager::CheckNow(void)
{
    static_cast<ImplClass*>(this)->_CheckNow();
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

inline bool SoftwareUpdateManager::IsDownloading(void)
{
    return static_cast<ImplClass*>(this)->_IsDownloading();
}

inline void SoftwareUpdateManager::SetQueryIntervalWindow(uint32_t aMinRangeSecs, uint32_t aMaxRangeSecs)
{
    static_cast<ImplClass*>(this)->_SetQueryIntervalWindow(aMinRangeSecs, aMaxRangeSecs);
}

inline void SoftwareUpdateManager::SetRetryPolicyCallback(const RetryPolicyCallback aRetryPolicyCallback)
{
    static_cast<ImplClass*>(this)->_SetRetryPolicyCallback(aRetryPolicyCallback);
}

inline void SoftwareUpdateManager::SetEventCallback(void * const aAppState, const EventCallback aEventCallback)
{
    static_cast<ImplClass*>(this)->_SetEventCallback(aAppState, aEventCallback);
}

} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER
#endif // SOFTWARE_UPDATE_MANAGER_H
