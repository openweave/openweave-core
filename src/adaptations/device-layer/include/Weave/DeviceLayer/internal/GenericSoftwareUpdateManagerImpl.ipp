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
 *          Contains non-inline method definitions for the
 *          GenericSoftwareUpdateManagerImpl<> template.
 */

#ifndef GENERIC_SOFTWARE_UPDATE_MANAGER_IMPL_IPP
#define GENERIC_SOFTWARE_UPDATE_MANAGER_IMPL_IPP

#if WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER

#include <Weave/Core/WeaveCore.h>
#include <Weave/DeviceLayer/PlatformManager.h>
#include <Weave/DeviceLayer/ConnectivityManager.h>
#include <Weave/DeviceLayer/SoftwareUpdateManager.h>
#include <Weave/DeviceLayer/internal/WeaveDeviceLayerInternal.h>
#include <Weave/DeviceLayer/internal/GenericSoftwareUpdateManagerImpl.h>

#include <Weave/Support/FibonacciUtils.h>
#include <Weave/Support/RandUtils.h>

namespace nl {
namespace Weave {
namespace DeviceLayer {
namespace Internal {

using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;
using namespace ::nl::Weave::Profiles::SoftwareUpdate;

// Fully instantiate the generic implementation class in whatever compilation unit includes this file.
template class GenericSoftwareUpdateManagerImpl<SoftwareUpdateManagerImpl>;

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DoInit()
{
    mShouldRetry = false;
    mScheduledCheckEnabled = false;

    mEventHandlerCallback = DefaultEventHandler;
    mRetryPolicyCallback = DefaultRetryPolicyCallback;

    mRetryCounter = 0;
    mMinWaitTimeInSecs = 0;
    mMaxWaitTimeInSecs = 0;

    mState = SoftwareUpdateManager::kState_Idle;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::_SetEventCallback(void * const aAppState,
                                                                   const SoftwareUpdateManager::EventCallback aEventCallback)
{
    mAppState = aAppState;
    mEventHandlerCallback = (aEventCallback != NULL) ? aEventCallback : DefaultEventHandler;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::_SetRetryPolicyCallback(const SoftwareUpdateManager::RetryPolicyCallback aRetryPolicyCallback)
{
    mRetryPolicyCallback = (aRetryPolicyCallback != NULL) ? aRetryPolicyCallback : DefaultRetryPolicyCallback;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DoPrepare(intptr_t arg)
{
    WEAVE_ERROR err;

    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();

    self->Cleanup();

    self->mBinding = ExchangeMgr.NewBinding(HandleServiceBindingEvent, NULL);
    VerifyOrExit(self->mBinding != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = self->mBinding->BeginConfiguration()
            .Target_ServiceEndpoint(WEAVE_DEVICE_CONFIG_SOFTWARE_UPDATE_ENDPOINT_ID)
            .Transport_UDP_WRM()
            .Exchange_ResponseTimeoutMsec(WEAVE_DEVICE_CONFIG_SOFTWARE_UPDATE_RESPOSNE_TIMEOUT)
            .Security_SharedCASESession()
            .PrepareBinding();

exit:
    if (err != WEAVE_NO_ERROR)
    {
        self->Impl()->SoftwareUpdateFailed(err, NULL);
    }
}

template<class ImplClass>
WEAVE_ERROR GenericSoftwareUpdateManagerImpl<ImplClass>::PrepareQuery(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    ImageQuery imageQuery;
    TLVWriter writer;
    TLVType containerType;

    char firmwareRev[ConfigurationManager::kMaxFirmwareRevisionLength+1];
    char desiredLocaleBuf[16];
    char packageSpecBuf[64];

    size_t firmwareRevLen;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    err = ConfigurationMgr().GetProductId(imageQuery.productSpec.productId);
    SuccessOrExit(err);

    err = ConfigurationMgr().GetVendorId(imageQuery.productSpec.vendorId);
    SuccessOrExit(err);

    err = ConfigurationMgr().GetProductRevision(imageQuery.productSpec.productRev);
    SuccessOrExit(err);

    err = ConfigurationMgr().GetFirmwareRevision(firmwareRev, sizeof(firmwareRev), firmwareRevLen);
    VerifyOrExit(err != WEAVE_DEVICE_ERROR_CONFIG_NOT_FOUND, err = WEAVE_NO_ERROR);

    err = Impl()->GetUpdateSchemeList(&imageQuery.updateSchemes);
    SuccessOrExit(err);

    err = Impl()->GetIntegrityTypeList(&imageQuery.integrityTypes);
    SuccessOrExit(err);

    outParam.PrepareQuery.PackageSpecification = packageSpecBuf;
    outParam.PrepareQuery.DesiredLocale = desiredLocaleBuf;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_PrepareQuery, inParam, outParam);

    err = imageQuery.version.init((uint8_t) firmwareRevLen, firmwareRev);
    SuccessOrExit(err);

    // Locale is an optional field. If one is not provided by the application,
    // then skip over and move to the next field.
    if (outParam.PrepareQuery.DesiredLocale != NULL)
    {
        err = imageQuery.localeSpec.init((uint8_t) strlen(desiredLocaleBuf), desiredLocaleBuf);
        SuccessOrExit(err);
    }

    // Package Specification is NOT an option field. Application must provide one.
    VerifyOrExit(outParam.PrepareQuery.PackageSpecification != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = imageQuery.packageSpec.init((uint8_t) strlen(packageSpecBuf), packageSpecBuf);
    SuccessOrExit(err);

    // Allocate a buffer to hold the image query.
    mImageQueryPacketBuffer = PacketBuffer::New();
    VerifyOrExit(mImageQueryPacketBuffer != NULL, err = WEAVE_ERROR_NO_MEMORY);

    err = imageQuery.pack(mImageQueryPacketBuffer);
    SuccessOrExit(err);

    writer.Init(mImageQueryPacketBuffer);

    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = writer.Put(ProfileTag(kWeaveProfile_SWU, kTag_CertBodyId), outParam.PrepareQuery.CertBodyId);
    SuccessOrExit(err);

    err = writer.PutBoolean(ProfileTag(kWeaveProfile_SWU, kTag_SufficientBatterySWU), outParam.PrepareQuery.HaveSufficientBattery);
    SuccessOrExit(err);

    outParam.PrepareQuery_Metadata.MetaDataWriter = &writer;

    // Call EventHandler Callback to allow application to write metadata.
    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_PrepareQuery_Metadata, inParam, outParam);

    err = writer.EndContainer(containerType);
    SuccessOrExit(err);

    writer.Finalize();

exit:
    return err;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::_CheckNow(void)
{
    if (!Impl()->IsInProgress())
    {
        if (mState == SoftwareUpdateManager::kState_Scheduled_Holdoff)
        {
            // Cancel scheduled hold off and trigger software update prepare.
            SystemLayer.CancelTimer(HandleHoldOffTimerExpired, NULL);
        }

        DriveState(SoftwareUpdateManager::kState_Preparing);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::SoftwareUpdateFailed(WEAVE_ERROR aReason,
                                                                       Profiles::StatusReporting::StatusReport * aStatusReport)
{
    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mShouldRetry = true;
    mRetryCounter++;

    if (mState == SoftwareUpdateManager::kState_Preparing)
    {
        inParam.QueryPrepareFailed.Reason = aReason;
        inParam.QueryPrepareFailed.Report = aStatusReport;
        mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_QueryPrepareFailed, inParam, outParam);
    }
    else
    {
        inParam.Finished.Reason = aReason;
        inParam.Finished.Report = aStatusReport;
        mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_Finished, inParam, outParam);
    }

    DriveState(SoftwareUpdateManager::kState_Idle);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::SoftwareUpdateFinished(WEAVE_ERROR aReason)
{
    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mShouldRetry = false;
    mRetryCounter = 0;

    inParam.Finished.Reason = aReason;
    inParam.Finished.Report = NULL;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_Finished, inParam, outParam);

    DriveState(SoftwareUpdateManager::kState_Idle);
}

template<class ImplClass>
bool GenericSoftwareUpdateManagerImpl<ImplClass>::_IsInProgress(void)
{
    return ((mState == SoftwareUpdateManager::kState_Idle || mState == SoftwareUpdateManager::kState_Scheduled_Holdoff) ?
            false : true );
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::_OnPlatformEvent(const WeaveDeviceEvent * event)
{
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();

     if (event->Type == DeviceEventType::kServiceConnectivityChange)
     {
        if (event->ServiceConnectivityChange.Overall.Result == kConnectivity_Established)
        {
            self->mHaveServiceConnectivity = true;
        }
        else if (event->ServiceConnectivityChange.Overall.Result == kConnectivity_Lost)
        {
            self->mHaveServiceConnectivity = false;
            SystemLayer.CancelTimer(HandleHoldOffTimerExpired, NULL);
        }

        self->DriveState(SoftwareUpdateManager::kState_Idle);
     }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::SendQuery(void)
{
    WEAVE_ERROR err;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    // Configure the context
    mExchangeCtx->AppState          = this;
    mExchangeCtx->OnMessageReceived = HandleResponse;
    mExchangeCtx->OnResponseTimeout = OnResponseTimeout;
    mExchangeCtx->OnKeyError        = OnKeyError;

    // Send the query
    err =  mExchangeCtx->SendMessage(kWeaveProfile_SWU,
                                     kMsgType_ImageQuery,
                                     mImageQueryPacketBuffer,
                                     ExchangeContext::kSendFlag_ExpectResponse | ExchangeContext::kSendFlag_RequestAck);

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_QuerySent, inParam, outParam);

    if (err != WEAVE_NO_ERROR)
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::OnSendError(ExchangeContext * aEC, WEAVE_ERROR aErrorCode, void * aMsgSpecificContext)
{
    IgnoreUnusedVariable(aMsgSpecificContext);

    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();
    self->Impl()->SoftwareUpdateFailed(aErrorCode, NULL);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::OnResponseTimeout(ExchangeContext * aEC)
{
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();
    self->Impl()->SoftwareUpdateFailed(WEAVE_ERROR_TIMEOUT, NULL);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::OnKeyError(ExchangeContext *aEc, WEAVE_ERROR aKeyError)
{
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();
    self->Impl()->SoftwareUpdateFailed(aKeyError, NULL);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::HandleResponse(ExchangeContext * ec, const IPPacketInfo * pktInfo, const WeaveMessageInfo * msgInfo,
                                                                 uint32_t profileId, uint8_t msgType, PacketBuffer * payload)
{
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();

    // We expect to receive one of two possible responses:
    // 1. An Image Query Response message under the weave software update profile indicating
    //    an update might be available or
    // 2. A status report indicating no software update available or a problem with the query.
    if (profileId == kWeaveProfile_SWU && msgType == kMsgType_ImageQueryResponse)
    {
        self->HandleImageQueryResponse(payload);
    }
    else
    {
        self->HandleStatusReport(payload);
    }

    // Release the payload pbuf
    PacketBuffer::Free(payload);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::HandleStatusReport(PacketBuffer * payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport statusReport;

    // Parse the status report to uncover the underlying errors.
    err = StatusReport::parse(payload, statusReport);
    SuccessOrExit(err);

    WeaveLogProgress(DeviceLayer, "Status report received [profile: %lx, status:%x]\n", statusReport.mProfileId, statusReport.mStatusCode);

    if ((statusReport.mProfileId == kWeaveProfile_SWU) && (statusReport.mStatusCode == kStatus_NoUpdateAvailable))
    {
        err = WEAVE_ERROR_NO_SW_UPDATE_AVAILABLE;
    }
    else
    {
        err = WEAVE_ERROR_STATUS_REPORT_RECEIVED;
    }

exit:
    if (err == WEAVE_ERROR_NO_SW_UPDATE_AVAILABLE)
    {
        Impl()->SoftwareUpdateFinished(err);
    }
    else if (err == WEAVE_ERROR_STATUS_REPORT_RECEIVED)
    {
        Impl()->SoftwareUpdateFailed(err, &statusReport);
    }
    else
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::HandleImageQueryResponse(PacketBuffer * payload)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ImageQueryResponse imageQueryResponse;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    // Parse out the query response
    err = ImageQueryResponse::parse(payload, imageQueryResponse);
    SuccessOrExit(err);

    imageQueryResponse.uri.theString[imageQueryResponse.uri.theLength] = 0;
    imageQueryResponse.versionSpec.theString[imageQueryResponse.versionSpec.theLength] = 0;

    inParam.SoftwareUpdateAvailable.Priority = imageQueryResponse.updatePriority;
    inParam.SoftwareUpdateAvailable.Condition = imageQueryResponse.updateCondition;
    inParam.SoftwareUpdateAvailable.Version = &imageQueryResponse.versionSpec;
    inParam.SoftwareUpdateAvailable.IntegrityType = imageQueryResponse.integritySpec.type;

    // Cache URI and IntegritySpec since the original payload will be freed after this.
    VerifyOrExit(imageQueryResponse.uri.theLength <= WEAVE_DEVICE_CONFIG_SOFTWARE_UPDATE_URI_LEN, err = WEAVE_ERROR_NO_MEMORY);
    strncpy(mURI, imageQueryResponse.uri.theString, imageQueryResponse.uri.theLength);

    mIntegritySpec = imageQueryResponse.integritySpec;

    // Allow proceeding with download as default. Application can overide by setting it to false during the event callback.
    outParam.SoftwareUpdateAvailable.CanDownload = true;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_SoftwareUpdateAvailable, inParam, outParam);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
    else if (!outParam.SoftwareUpdateAvailable.CanDownload)
    {
        Impl()->SoftwareUpdateFinished(WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_CANCELLED);
    }
    else
    {
        DriveState(SoftwareUpdateManager::kState_CheckingImageState);
    }
}



template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::HandleHoldOffTimerExpired(::nl::Weave::System::Layer * aLayer,
                                                                            void * aAppState,
                                                                            ::nl::Weave::System::Error aError)
{
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();

    self->DriveState(SoftwareUpdateManager::kState_Preparing);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DriveState(SoftwareUpdateManager::State aNextState)
{
    if (mState != SoftwareUpdateManager::kState_Idle &&
        (aNextState == mState || aNextState >= SoftwareUpdateManager::kState_MaxState))
    {
        ExitNow();
    }

    mState = aNextState;

    switch(mState)
    {
        case SoftwareUpdateManager::kState_Idle:
        {
            // If scheduled software update check is enabled and service connectivity is present,
            // compute the next wait time interval and start the timer. Software Update Check
            // will trigger on expiration of the timer unless service connectivity was lost or
            // the application request a manual software update check.
            if (mScheduledCheckEnabled && mHaveServiceConnectivity)
            {
                uint32_t timeToNextQueryMS = ComputeNextWaitTimeInterval();

                mState = SoftwareUpdateManager::kState_Scheduled_Holdoff;

                SystemLayer.StartTimer(timeToNextQueryMS, HandleHoldOffTimerExpired, NULL);
            }
            else if (mScheduledCheckEnabled && !mHaveServiceConnectivity)
            {
                WeaveLogProgress(DeviceLayer, "Scheduled Software Update Check Suspended - no service connectivity");
            }
        }
        break;

        case SoftwareUpdateManager::kState_Preparing:
        {
            PlatformMgr().ScheduleWork(DoPrepare);
        }
        break;

        case SoftwareUpdateManager::kState_Querying:
        {
            SendQuery();
        }
        break;

        case SoftwareUpdateManager::kState_CheckingImageState:
        {
            CheckImageState();
            break;
        }
        case SoftwareUpdateManager::kState_Downloading:
        {
            StartingDownload();
            break;
        }

        case SoftwareUpdateManager::kState_CheckingIntegrity:
        {
            CheckImageIntegrity();
            break;
        }

        case SoftwareUpdateManager::kState_Installing:
        {
            StartImageInstall();
            break;
        }

        default:
            break;
    }

exit:
    return;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::_SetQueryIntervalWindow(uint32_t aMinWaitTimeInSecs, uint32_t aMaxWaitTimeInSecs)
{
    if (aMaxWaitTimeInSecs == 0)
    {
        WeaveLogProgress(DeviceLayer, "Scheduled Software Update Check Disabled");
        mScheduledCheckEnabled = false;
    }
    else
    {
        mMinWaitTimeInSecs = aMinWaitTimeInSecs;
        mMaxWaitTimeInSecs = aMaxWaitTimeInSecs;

        mScheduledCheckEnabled = true;
    }

    DriveState(SoftwareUpdateManager::kState_Idle);
}

template<class ImplClass>
uint32_t GenericSoftwareUpdateManagerImpl<ImplClass>::ComputeNextWaitTimeInterval()
{
    uint32_t timeOutMsecs;

    if (mShouldRetry)
    {
        SoftwareUpdateManager::RetryParam param;
        param.mNumRetries = mRetryCounter;

        mRetryPolicyCallback(mAppState, param, timeOutMsecs);

        WeaveLogProgress(DeviceLayer, "Retrying Software Update Check in %ums RetryCounter: %u", timeOutMsecs, mRetryCounter);
    }
    else
    {
        timeOutMsecs = (mMinWaitTimeInSecs + (GetRandU32() % (mMaxWaitTimeInSecs - mMinWaitTimeInSecs))) * 1000;
        WeaveLogProgress(DeviceLayer, "Next Scheduled Software Update Check in %ums", timeOutMsecs);
    }

    return timeOutMsecs;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DefaultEventHandler(void *apAppState,
                                                                      SoftwareUpdateManager::EventType aEvent,
                                                                      const SoftwareUpdateManager::InEventParam& aInParam,
                                                                      SoftwareUpdateManager::OutEventParam& aOutParam)
{
    IgnoreUnusedVariable(aInParam);
    IgnoreUnusedVariable(aOutParam);

    WeaveLogDetail(DataManagement, "%s event: %d", __func__, aEvent);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::HandleServiceBindingEvent(void * appState, ::nl::Weave::Binding::EventType eventType,
    const ::nl::Weave::Binding::InEventParam & aInParam, ::nl::Weave::Binding::OutEventParam & aOutParam)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport *statusReport = NULL;
    GenericSoftwareUpdateManagerImpl<ImplClass> * self = &SoftwareUpdateMgrImpl();

    switch (eventType)
    {
        case nl::Weave::Binding::kEvent_PrepareFailed:
            WeaveLogProgress(DeviceLayer, "Failed to prepare Software Update binding: %s", ErrorStr(aInParam.PrepareFailed.Reason));
            statusReport = aInParam.PrepareFailed.StatusReport;
            err = aInParam.PrepareFailed.Reason;
            break;

        case nl::Weave::Binding::kEvent_BindingFailed:
            WeaveLogProgress(DeviceLayer, "Software Update binding failed: %s", ErrorStr(aInParam.BindingFailed.Reason));
            err = aInParam.PrepareFailed.Reason;
            break;

        case nl::Weave::Binding::kEvent_BindingReady:
            WeaveLogProgress(DeviceLayer, "Software Update binding ready");

            err = self->mBinding->NewExchangeContext(self->mExchangeCtx);
            SuccessOrExit(err);

            self->mBinding->Release();
            self->mBinding = NULL;

            err = self->PrepareQuery();
            SuccessOrExit(err);

            self->DriveState(SoftwareUpdateManager::kState_Querying);
            break;

        default:
            nl::Weave::Binding::DefaultEventHandler(appState, eventType, aInParam, aOutParam);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        self->Impl()->SoftwareUpdateFailed(err, statusReport);
    }
}

template<class ImplClass>
WEAVE_ERROR GenericSoftwareUpdateManagerImpl<ImplClass>::StoreDataBlock(uint64_t aLength, uint8_t *aData, uint64_t aTotalTransferLength)
{
    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    inParam.StoreDataBlock.DataBlockLenInBytes = aLength;
    inParam.StoreDataBlock.DataBlock = aData;
    inParam.StoreDataBlock.TotalImageLength = aTotalTransferLength;
    outParam.StoreDataBlock.Reason = WEAVE_NO_ERROR;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_StoreDataBlock, inParam, outParam);

    return outParam.StoreDataBlock.Reason;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::CheckImageState()
{
    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    inParam.FetchDownloadImageState.URI = mURI;

    outParam.FetchDownloadImageState.IsPartialImageAvailable = false;
    outParam.FetchDownloadImageState.IsFullImageAvailable = false;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_FetchDownloadImageState, inParam, outParam);

    if (outParam.FetchDownloadImageState.IsFullImageAvailable)
    {
        Impl()->SoftwareUpdateFinished(WEAVE_NO_ERROR);
    }
    else
    {
        if (outParam.FetchDownloadImageState.IsPartialImageAvailable)
        {
            // Starting offset for download will be one more than the length of the partial image.
            mStartOffset = outParam.FetchDownloadImageState.PartialImageLenInBytes + 1;
        }
        else
        {
            inParam.Clear();
            outParam.Clear();

            inParam.ClearDownloadImageState.IntegrityType = mIntegritySpec.type;

            // Inform the application to clear any previous partial image download since we are going to start downloading
            // a new image from scratch.
            mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_ClearDownloadImageState, inParam, outParam);

            mStartOffset = 0;
        }

        DriveState(SoftwareUpdateManager::kState_Downloading);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::StartingDownload(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_StartImageDownload, inParam, outParam);

    err = Impl()->StartImageDownload(mURI, mStartOffset);
    if (err != WEAVE_NO_ERROR)
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DownloadComplete()
{
    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_DownloadComplete, inParam, outParam);

    DriveState(SoftwareUpdateManager::kState_CheckingIntegrity);
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::CheckImageIntegrity(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    uint8_t computedIntegrityValue[64];

    inParam.Clear();
    outParam.Clear();

    inParam.ComputeImageIntegrity.IntegrityType = mIntegritySpec.type;
    outParam.ComputeImageIntegrity.IntegrityValue = computedIntegrityValue;
    outParam.ComputeImageIntegrity.IntegrityLength = 0;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_ComputeImageIntegrity, inParam, outParam);

    const uint8_t typeLength = (mIntegritySpec.type == kIntegrityType_SHA160) ? kLength_SHA160 :
                               (mIntegritySpec.type == kIntegrityType_SHA256) ? kLength_SHA256 :
                               (mIntegritySpec.type == kIntegrityType_SHA512) ? kLength_SHA512 :
                                0;

    VerifyOrExit(outParam.ComputeImageIntegrity.IntegrityLength == typeLength, err = WEAVE_ERROR_INTEGRITY_CHECK_FAILED);

    for (int index = 0; index < typeLength; index++)
    {
        if (computedIntegrityValue[index] != mIntegritySpec.value[index])
        {
            err = WEAVE_ERROR_INTEGRITY_CHECK_FAILED;
            break;
        }
    }
    SuccessOrExit(err);

    inParam.Clear();
    outParam.Clear();

    outParam.ReadyToInstall.CanInstallImage = true;

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_ReadyToInstall, inParam, outParam);

    if (outParam.ReadyToInstall.CanInstallImage)
    {
        DriveState(SoftwareUpdateManager::kState_Installing);
    }
    else
    {
        Impl()->SoftwareUpdateFinished(WEAVE_DEVICE_ERROR_SOFTWARE_UPDATE_CANCELLED);
    }

exit:
    if (err != WEAVE_NO_ERROR)
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::StartImageInstall(void)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    SoftwareUpdateManager::InEventParam inParam;
    SoftwareUpdateManager::OutEventParam outParam;

    inParam.Clear();
    outParam.Clear();

    mEventHandlerCallback(mAppState, SoftwareUpdateManager::kEvent_StartInstallImage, inParam, outParam);

    err = Impl()->InstallImage();
    if (err != WEAVE_NO_ERROR)
    {
        Impl()->SoftwareUpdateFailed(err, NULL);
    }
    else
    {
        Impl()->SoftwareUpdateFinished(WEAVE_NO_ERROR);
    }
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::Cleanup(void)
{
    if (mBinding)
    {
        mBinding->Close();
        mBinding = NULL;
    }

    // Shutdown the exchange if its active
    if (mExchangeCtx)
    {
        mExchangeCtx->Abort();
        mExchangeCtx = NULL;
    }
}

template<class ImplClass>
WEAVE_ERROR GenericSoftwareUpdateManagerImpl<ImplClass>::_Abort()
{
    Cleanup();

    DriveState(SoftwareUpdateManager::kState_Idle);

    return WEAVE_NO_ERROR;
}

template<class ImplClass>
void GenericSoftwareUpdateManagerImpl<ImplClass>::DefaultRetryPolicyCallback(void * const aAppState,
                                                                             SoftwareUpdateManager::RetryParam & aRetryParam,
                                                                             uint32_t & aOutIntervalMsec)
{
    IgnoreUnusedVariable(aAppState);

    uint32_t fibonacciNum      = 0;
    uint32_t maxWaitTimeInMsec = 0;
    uint32_t waitTimeInMsec    = 0;
    uint32_t minWaitTimeInMsec = 0;

    if (aRetryParam.mNumRetries <= WEAVE_DEVICE_CONFIG_SWU_RETRY_MAX_FIBONACCI_STEP_INDEX)
    {
        fibonacciNum      = GetFibonacciForIndex(aRetryParam.mNumRetries);
        maxWaitTimeInMsec = fibonacciNum * WEAVE_DEVICE_CONFIG_SWU_RETRY_WAIT_TIME_MULTIPLIER_MS;
    }
    else
    {
        maxWaitTimeInMsec = WEAVE_DEVICE_CONFIG_SWU_RETRY_MAX_WAIT_INTERVAL_MS;
    }

    if (maxWaitTimeInMsec != 0)
    {
        minWaitTimeInMsec = (WEAVE_DEVICE_CONFIG_SWU_RETRY_MIN_WAIT_TIME_INTERVAL_PERCENT_PER_STEP * maxWaitTimeInMsec) / 100;
        waitTimeInMsec    = minWaitTimeInMsec + (GetRandU32() % (maxWaitTimeInMsec - minWaitTimeInMsec));
    }

    aOutIntervalMsec = waitTimeInMsec;
}

} // namespace Internal
} // namespace DeviceLayer
} // namespace Weave
} // namespace nl

#endif // WEAVE_DEVICE_CONFIG_ENABLE_SOFTWARE_UPDATE_MANAGER
#endif // GENERIC_SOFTWARE_UPDATE_MANAGER_IMPL_IPP
