/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      The main class defined herein - WeaveServiceManager - defines
 *      an object, generally a singleton, that implements the Weave
 *      Service Directory Profile. This profile allows applications
 *      using Weave to request a connection to a particular Weave
 *      service using a predefined service endpoint. The connect()
 *      call takes callbacks that the service directory sub-layer
 *      invokes when the requested connection is complete or an error
 *      occurs.
 *
 *      The underlying protocol is described in the document:
 *
 *          Nest Weave - Service Directory Protocol
 *
 *      which currently defines two messages:
 *
 *      1) A service endpoint query may be sent by a Weave node when
 *         that node wishes to request directory information from
 *         another node or a service entity. The service endpoint
 *         query message has no fields beyond the Weave exchange
 *         header.
 *
 *      2) A service endpoint response containing directory
 *         information, which shall be sent by a node or service
 *         entity in response to a successful service endpoint query.
 *
 *      In addition to its primary function as a directory lookup
 *      protocol, the Service Directory Protocol supports time
 *      synchronization by allowing the Weave service to optionally
 *      insert time fields in the service endpoint response.
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>

#include "ServiceDirectory.h"

#include <Weave/Support/WeaveFaultInjection.h>
#include <SystemLayer/SystemStats.h>

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

#if HAVE_NEW
#include <new>
#else
inline void * operator new(size_t, void * p) throw()
{
    return p;
}
inline void * operator new[](size_t, void * p) throw()
{
    return p;
}
#endif // HAVE_NEW

using namespace ::nl::Inet;
using namespace ::nl::Weave;
using namespace ::nl::Weave::Encoding;
using namespace ::nl::Weave::Encoding::LittleEndian;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::ServiceDirectory;
using namespace ::nl::Weave::Profiles::StatusReporting;

/**
 *  @def ARRAY_SIZE(a)
 *
 *  @brief
 *    This macro calculates the number of elements in a the given array.
 *
 *  @param[in] a An array, which we need to know the size of.
 *
 */
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

/**
 *  @brief
 *    This method is a trampoline which calls the actual handler
 *    WeaveServiceManager::onConnectionComplete() .
 *
 *  @param[in] aConnection A pointer to the Weave connection object which has
 *    been completed.
 *
 *  @param[in] aError An Weave error code indicating if there was any error
 *    during connection setup.
 */
static void handleSDConnectionComplete(WeaveConnection *aConnection, WEAVE_ERROR aError)
{
    WeaveServiceManager *manager = (WeaveServiceManager *)aConnection->AppState;

    WeaveLogProgress(ServiceDirectory, "handleSDConnectionComplete() <= %s", nl::ErrorStr(aError));

    if (manager)
        manager->onConnectionComplete(aError);
}

/**
 *  @brief
 *    This method is a trampoline which calls the actual handler
 *    WeaveServiceManager::ConnectRequest::onConnectionComplete() .
 *
 *  @param[in] aConnection A pointer to the Weave connection object which has
 *    been completed.
 *
 *  @param[in] aError An Weave error code indicating if there was any error
 *    during connection setup.
 */
static void handleAppConnectionComplete(WeaveConnection *aConnection, WEAVE_ERROR aError)
{
    WeaveServiceManager::ConnectRequest *request = (WeaveServiceManager::ConnectRequest *)aConnection->AppState;

    WeaveLogProgress(ServiceDirectory, "handleAppConnectionComplete() <= %s", nl::ErrorStr(aError));

    if (request)
        request->onConnectionComplete(aError);
}

/**
 *  @brief
 *    This is the handler that's set in the WeaveConnection to handle closure.
 *
 *  Note that it is distinct from the "connection closed" handler that is set
 *  in the ExchangeContext during a conversation.
 *
 *  @param[in] aConnection A pointer to the Weave connection object which has
 *    been closed.
 *
 *  @param[in] aError An Weave error code indicating if there was any error
 *    causing the connection to be closed.
 */
static void handleConnectionClosed(WeaveConnection *aConnection, WEAVE_ERROR aError)
{
    aConnection->Close();

    WeaveLogProgress(ServiceDirectory, "handleConnectionClosed() <= %s", nl::ErrorStr(aError));
}

/**
 *  @brief
 *    This method is a trampoline which calls the actual handler
 *    WeaveServiceManager::onConnectionClosed() .
 *
 *  @param[in] aExchangeCtx A pointer to the exchange context object which is
 *    associated with this conversation.
 *
 *  @param[in] aConnection A pointer to the Weave connection object which has
 *    been closed.
 *
 *  @param[in] aError An Weave error code indicating if there was any error
 *    causing the connection to be closed.
 */
static void ecHandleConnectionClosed(ExchangeContext *aExchangeCtx,
                                   WeaveConnection *aConnection,
                                   WEAVE_ERROR aError)
{
    WeaveServiceManager *manager = (WeaveServiceManager *)aConnection->AppState;

    /*
     * Connection is just closed by peer, which is not really
     * expected.  This handler is called if the service closes the
     * connection instead of sending any response.  If the connection
     * is closed gracefully, the error code passed by the lower layers
     * can be WEAVE_NO_ERROR.  In that case the error code is replaced
     * with CONNECTION_CLOSED_UNEXPECTEDLY because this event is
     * considered as an error protocol-wise.
     */

    if (WEAVE_NO_ERROR == aError)
    {
        aError = WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY;
    }

    WeaveLogProgress(ServiceDirectory, "ecHandleConnectionClosed() <= %s", nl::ErrorStr(aError));

    if (manager)
        manager->onConnectionClosed(aError);
}

/**
 *  @brief
 *    This method is a trampoline which calls the actual handler
 *    WeaveServiceManager::onResponseReceived() .
 *
 *  @param[in] aExchangeCtx A pointer to the exchange context object which is
 *    associated with this conversation.
 *
 *  @param[in] anAddrInfo   A read-only pointer to the IP header of this message.
 *
 *  @param[in] aMsgInfo     A read-only pointer to the Weave header of this message.
 *
 *  @param[in] aProfileId   The ID of the Weave profile this message belongs to.
 *  @param[in] aMsgType     The profile-specific type of this message.
 *  @param[in] aMsg         A pointer to a buffer with the content of this message.
 */
static void handleResponseMsg(ExchangeContext *aExchangeCtx,
                              const IPPacketInfo *anAddrInfo,
                              const WeaveMessageInfo *aMsgInfo,
                              uint32_t aProfileId,
                              uint8_t aMsgType,
                              PacketBuffer *aMsg)
{
    WeaveServiceManager *manager = (WeaveServiceManager *)aExchangeCtx->AppState;

    WeaveLogProgress(ServiceDirectory, "handleResponseMsg()");

    if (manager)
        manager->onResponseReceived(aProfileId, aMsgType, aMsg);
}

/**
 *  @brief
 *    This method is a trampoline which calls the actual handler
 *    WeaveServiceManager::onResponseTimeout() .
 *
 *  @param[in] aExchangeCtx A pointer to the exchange context object which
 *    is associated with this conversation.
 */
static void handleResponseTimeout(ExchangeContext *aExchangeCtx)
{
    WeaveServiceManager *manager = (WeaveServiceManager *)aExchangeCtx->AppState;

    WeaveLogProgress(ServiceDirectory, "handleResponseTimeout()");

    if (manager)
        manager->onResponseTimeout();
}

/**
 *  @brief
 *    This method initializes the WeaveServiceManager instance.
 *
 *  Note that init() must be called to further initialize this instance.
 *
 */
WeaveServiceManager::WeaveServiceManager()
{
    mExchangeManager = NULL;
    mCache.base = NULL;
    mCache.length = 0;
    mConnection = NULL;
    mExchangeContext = NULL;
    mServiceEndpointQueryBegin = NULL;
    mServiceEndpointQueryEndWithTimeInfo = NULL;

    freeConnectRequests();

    clearWorkingState();
    clearCacheState();
}

/**
 *  This method destructs the WeaveServiceManager instance.
 */
WeaveServiceManager::~WeaveServiceManager()
{
    mExchangeManager = NULL;
    mCache.base = NULL;
    mCache.length = 0;
    mServiceEndpointQueryBegin = NULL;
    mServiceEndpointQueryEndWithTimeInfo = NULL;

    reset();
}

/**
 *  @brief This method initializes the service manager object.
 *
 *  In order to be used, a service manager object must be
 *  initialized. After a successful call to this method, clients can start
 *  calling connect(), lookup(), and other methods.
 *
 *  @param [in] aExchangeMgr    A pointer to the exchange
 *    manager to use for all service directory profile exchanges.
 *
 *  @param [in] aCache          A pointer to a buffer which can be used to
 *    cache directory information.
 *
 *  @param [in] aCacheLen       The length in bytes of the cache.
 *
 *  @param [in] aAccessor       The callback, as defined in ServiceDirectory.h
 *    to invoke in order to load the root directory as a starting point for
 *    directory lookup.
 *
 *  @param [in] aDirAuthMode    The authentication mode to
 *  use when talking to the directory service.
 *
 *  @param [in] aServiceEndpointQueryBegin
 *    A function pointer of type OnServiceEndpointQueryBegin, that is called
 *    at the start of a service directory request and allows application code
 *    to mark the time if it wishes to use the time synchronization offered
 *    by the service directory protocol.
 *
 *  @param [in] aServiceEndpointQueryEndWithTimeInfo
 *   A function pointer of type OnServiceEndpointQueryEndWithTimeInfo, that is
 *   called on receipt of a service directory that allows applications to
 *   synchronize with the Weave service using the time fields given in
 *   the response. This callback would be made after the service manager
 *   receives a response with time information. The cache should already be
 *   filled successfully before the callback is made.
 *
 *  @param [in] aConnectBegin
 *    A function pointer of type OnConnectBegin, that is called immediately
 *    prior to connection establishment and allows applications to observe
 *    and optionally alter the arguments passed to #WeaveConnection::Connect().
 *    A value of NULL (the default) disables the callback.
 *
 *  @return #WEAVE_ERROR_INVALID_ARGUMENT if a function
 *    argument is invalid; otherwise, #WEAVE_NO_ERROR.
 */
WEAVE_ERROR WeaveServiceManager::init(WeaveExchangeManager *aExchangeMgr,
                                      uint8_t *aCache,
                                      uint16_t aCacheLen,
                                      RootDirectoryAccessor aAccessor,
                                      WeaveAuthMode aDirAuthMode,
                                      OnServiceEndpointQueryBegin aServiceEndpointQueryBegin,
                                      OnServiceEndpointQueryEndWithTimeInfo aServiceEndpointQueryEndWithTimeInfo,
                                      OnConnectBegin aConnectBegin)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    WeaveLogProgress(ServiceDirectory, "init()");

    VerifyOrExit(aExchangeMgr && aCache && aCacheLen > 0 && aAccessor, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mExchangeManager = aExchangeMgr;
    mCache.base = aCache;
    mCache.length = aCacheLen;
    mDirAndSuffTableSize = 0;

    mAccessor = aAccessor;
    mDirAuthMode = aDirAuthMode;
    mServiceEndpointQueryBegin = aServiceEndpointQueryBegin;
    mServiceEndpointQueryEndWithTimeInfo = aServiceEndpointQueryEndWithTimeInfo;
    mConnectBegin = aConnectBegin;

    cleanupExchangeContext();

    clearCacheState();

    finalizeConnectRequests();

exit:

    return err;
}

/**
 *  @brief This method requests connect to a Weave service.
 *
 *  This is the top-level connect call. It essentially produces a
 *  secure connection to the Weave service given a service endpoint and
 *  an authentication mode or dies trying.
 *
 *  This method can only be called after successful call to init(),
 *  and a connection request can be potentially canceled by cancel().
 *
 *  This method can be called before the local cache is filled with
 *  data from either default provisioned data or a trip to the directory
 *  service. Service manager would just queue the request before the cache
 *  content can be determined.
 *
 *  @param [in] aServiceEp      The service endpoint identifier,
 *    as defined in ServiceDirectory.h, for the service of interest.
 *
 *  @param [in] aAuthMode       The authentication mode to use
 *    when connecting to the service of interest.
 *
 *  @param [in] aAppState       A pointer to an application state object,
 *    passed to the callbacks as an argument.
 *
 *  @param [in] aStatusHandler  A callback to invoke in the case of an error
 *    that occurs before the connection is completed.
 *
 *  @param [in] aConnectionCompleteHandler
 *    A callback to invoke in the case where the requested connection is
 *    completed. Note that the connection may fail with an Weave error code.
 *
 *  @param [in] aConnectTimeoutMsecs
 *    The optional TCP connect timeout in milliseconds.
 *
 *  @param [in] aConnectIntf
 *    The optional interface over which the connection is to be established.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 */
WEAVE_ERROR WeaveServiceManager::connect(uint64_t aServiceEp,
                                         WeaveAuthMode aAuthMode,
                                         void *aAppState,
                                         StatusHandler aStatusHandler,
                                         WeaveConnection::ConnectionCompleteFunct aConnectionCompleteHandler,
                                         const uint32_t aConnectTimeoutMsecs,
                                         const InterfaceId aConnectIntf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    ConnectRequest *req = NULL;

    WeaveLogProgress(ServiceDirectory, "connect(%llx...)", aServiceEp);

    if (mCacheState == kServiceMgrState_Initial)
    {
        WeaveLogProgress(ServiceDirectory, "initial");

        /*
         * when the service manager state is "initial" the state of the
         * service cache is assumed to be empty or unknown. in this case
         * the only way forward is to get the root directory from the
         * service config and install it.
         */

        err = mAccessor(mCache.base, mCache.length);
        SuccessOrExit(err);

        mDirectory.base = mCache.base;
        mDirectory.length = 1;

        mCacheState = kServiceMgrState_Resolving;
    }

    if (mCacheState == kServiceMgrState_Resolving)
    {
        WeaveLogProgress(ServiceDirectory, "resolving");

        /*
         * when the state is "resolving" it means that the cache at least
         * has something in it and is awaiting resolution. in this case we
         * have to fire off a service directory query.
         */

        mConnection = mExchangeManager->MessageLayer->NewConnection();
        VerifyOrExit(mConnection, err = WEAVE_ERROR_NO_MEMORY);

        err = lookupAndConnect(mConnection,
                               kServiceEndpoint_Directory,
                               mDirAuthMode,
                               this,
                               handleSDConnectionComplete,
                               WEAVE_CONFIG_SERVICE_DIR_CONNECT_TIMEOUT_MSECS);
        SuccessOrExit(err);
        /*
         * at this point, the possible values for mCacheState are:
         *
         * - kServiceMgrState_Resolving - if everything went well,
         *
         * - kServiceMgrState_Initial - if the lookupAndConnect
         *   invoked the callback synchronously with an error.  That
         *   case results in call chain
         *       WeaveServiceManager::handleSDConnectionComplete ->
         *       WeaveServiceManager::onConnectionComplete ->
         *       WeaveServiceManager::fail ->
         *       WeaveServiceManager::clearCacheState
         */

        if (mCacheState == kServiceMgrState_Resolving)
        {
            mCacheState = kServiceMgrState_Waiting;
        }
        else
        {
            ExitNow(err = WEAVE_ERROR_CONNECTION_ABORTED);
        }
    }

    /*
     * OK. here the state is either "waiting" in which case
     * we've kicked off an SD request and we can't do anything
     * else until the response comes back or it's "resolved"
     * in which case we're good to go. in either case, we
     * queue up a connect request.
     */

    req = getAvailableRequest();
    VerifyOrExit(req, err = WEAVE_ERROR_WELL_EMPTY);

    err = req->init(this,
                    aServiceEp,
                    aAuthMode,
                    aAppState,
                    aStatusHandler,
                    aConnectionCompleteHandler,
                    aConnectTimeoutMsecs,
                    aConnectIntf);
    SuccessOrExit(err);

    if (mCacheState == kServiceMgrState_Waiting)
    {
        WeaveLogProgress(ServiceDirectory, "waiting");
    }

    else if (mCacheState == kServiceMgrState_Resolved)
    {
        WeaveLogProgress(ServiceDirectory, "resolved");

        err = lookupAndConnect(req->mConnection,
                               req->mServiceEp,
                               req->mAuthMode,
                               req,
                               handleAppConnectionComplete,
                               req->mConnectTimeoutMsecs,
                               req->mConnIntf);
    }

    else
    {
        err = WEAVE_ERROR_INCORRECT_STATE;
    }


exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(ServiceDirectory, "connect: %s", ErrorStr(err));

        if (mConnection && mCacheState == kServiceMgrState_Resolving)
        {
            /*
             * Note that if the cache state is "waiting" we don't want
             * to close the connection exactly because the connection
             * is what we're waiting for.
             */

            cleanupExchangeContext(err);
        }
        if (req)
        {
            req->finalize();
        }
    }

    return err;
}

/**
 * @brief This method looks up directory information for a service endpoint.
 *
 * If the service directory has been resolved, i.e. if there has been a
 * successful connect() operation, then this method will populate the supplied
 * HostPortList object.
 *
 * Note: The HostPortList is bound to the WeaveServiceManager object; it remains
 * valid until the service directory cache is cleared or until another service
 * directory lookup occurs.
 *
 * @param [in] aServiceEp       The identifier of the service endpoint to
 *   look up.
 *
 * @param [out] outHostPortList The pointer to the HostPortList that will be
 *   populated on successful lookup of the directory entry.  Must not be NULL.
 *
 * @retval #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 *
 * @retval #WEAVE_ERROR_INVALID_SERVICE_EP if the given service endpoint is
 *   not found.
 *
 * @retval #WEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE if directory contains an
 *   unknown directory entry type.
 */

WEAVE_ERROR WeaveServiceManager::lookup(uint64_t aServiceEp, HostPortList *outHostPortList)
{
    WEAVE_ERROR err;
    uint8_t itemCount;
    uint8_t ctrlByte;
    uint8_t* entry = NULL;

    VerifyOrExit(outHostPortList != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = lookup(aServiceEp, &ctrlByte, &entry);
    SuccessOrExit(err);

    VerifyOrExit((ctrlByte & kMask_DirectoryEntryType) == kDirectoryEntryType_HostPortList, err = WEAVE_ERROR_HOST_PORT_LIST_EMPTY);

    itemCount = ctrlByte & kMask_HostPortListLen;

    new (outHostPortList) HostPortList(entry, itemCount,  mSuffixTable.base, mSuffixTable.length);

exit:
    return err;
}

/**
 * @brief This method looks up directory information for a service endpoint.
 *
 * If the service directory has been resolved, i.e. if there has been
 * a successful connect() operation, then this method will return a
 * directory entry given a service endpoint identifier.
 *
 * This method exposes the details of the internal implementation of the service
 * directory, implementations should strongly favor using the variant of this
 * method that generates the HostPortList.
 *
 * @param [in] aServiceEp       The identifier of the service endpoint to
 *   look up.
 *
 * @param [out] aControlByte    A pointer to the place to write the directory
 *   entry control byte.
 *
 * @param [out] aDirectoryEntry A pointer-pointer to be directed to the
 *   directory entry.
 *
 * @retval #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 *
 * @retval #WEAVE_ERROR_INVALID_SERVICE_EP if the given service endpoint is
 *   not found.
 *
 * @retval #WEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE if directory contains an
 *   unknown directory entry type.
 */
WEAVE_ERROR WeaveServiceManager::lookup(uint64_t aServiceEp, uint8_t *aControlByte, uint8_t **aDirectoryEntry)
{
    WEAVE_ERROR err = WEAVE_ERROR_INVALID_SERVICE_EP;
    uint8_t *p = mDirectory.base;
    uint16_t entryLen = 0;
    bool found = false;
    *aControlByte = 0;
    *aDirectoryEntry = NULL;

    WeaveLogProgress(ServiceDirectory, "lookup()");

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_ServiceManager_Lookup,
                       memset(&aServiceEp, 0x0F, sizeof(aServiceEp)));

    for (uint8_t i = 0; i < mDirectory.length; i++)
    {
        uint8_t  entryCtrlByte = Read8(p);
        uint64_t svcEp = Read64(p);

        if (svcEp == aServiceEp)
        {
            // found it.
            // break out of the loop

            WeaveLogProgress(ServiceDirectory, "found [%x,%llx]", entryCtrlByte, svcEp);

            *aControlByte = entryCtrlByte;
            *aDirectoryEntry = p;

            found = true;
            err = WEAVE_NO_ERROR;

            break;
        }

        // skip over this entry

        err = calculateEntryLength(p, entryCtrlByte, &entryLen);
        SuccessOrExit(err);

        p += entryLen;
    }

    if (!found)
    {
        err = WEAVE_ERROR_INVALID_SERVICE_EP;
    }

exit:
    WeaveLogProgress(ServiceDirectory, "lookup() => %s", ErrorStr(err));

    return err;
}

/**
 *  Add the overriding directory entry of a hostname and port id at the beginning
 *  of the directory list.
 */
WEAVE_ERROR WeaveServiceManager::replaceOrAddCacheEntry(uint16_t port,
                                                        const char *hostName,
                                                        uint8_t hostLen,
                                                        uint64_t serviceEndpointId)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t *p = mDirectory.base;
    uint8_t listCtrl = 0;
    uint8_t itemCtrl = 0;
    // Parameters for lookup.
    uint8_t ctrlByte;
    uint8_t *entry = NULL;
    uint16_t entryLength = 0;
    uint8_t bottomPortionLen = 0;
    bool newEntryAdded = false;

    // Byte length for the overriding entry that needs to be inserted at the beginning of directory.
    // 1(host/port list length byte) + 8(Service Endpoint Id) + 1(hostId type byte) + 1(string len)
    // + hostLen(hostname string size) + 2(port Id)
    uint16_t overrideEntryTotalLen = 1 + 8 + 1 + 1 + hostLen + 2;

    // Inject a fault to return an error while replacing a directory entry.

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_ServiceDirectoryReplaceError,
                       ExitNow(err = WEAVE_ERROR_INCORRECT_STATE);
                       );

    // Return if the cache state is not in the kServiceMgrState_Resolved state.
    // This is to avoid having to add an entry into the cache when it has been
    // reset thereby clearing the cache state and, avoid interfering with the
    // FSM if a query to the service-directory is in progress.

    VerifyOrExit(mCacheState == kServiceMgrState_Resolved,
                 err = WEAVE_ERROR_INCORRECT_STATE);

    // Perform lookup of the Service endpoint to replace an existing entry.

    err = lookup(serviceEndpointId, &ctrlByte, &entry);
    if (err == WEAVE_NO_ERROR)
    {
        // Found an entry to replace. Calculate length of the entry.

        err = calculateEntryLength(entry, ctrlByte, &entryLength);
        SuccessOrExit(err);

        entryLength += 9; // Add the entry ctrl byte(1) and the endpoint id(8) bytes
                          // to the entryLength.
        entry -= 9; // Retract the entry start to the beginning of its control byte.

        // Add a check for the length incorporating the new entry.

        VerifyOrExit(overrideEntryTotalLen + mDirAndSuffTableSize - entryLength < mCache.length,
                     err = WEAVE_ERROR_NO_MEMORY);

        // Delete entry by moving up everything after the replaced entry to fill the created gap.
        bottomPortionLen = mDirAndSuffTableSize - (entry + entryLength - p);
        memmove(entry, entry + entryLength, bottomPortionLen);

        // Reduce the overall size by this entry length.

        mDirAndSuffTableSize -= entryLength;
    }
    else
    {
        WeaveLogProgress(ServiceDirectory, "%s : Lookup failed, adding entry at the top", __func__);

        newEntryAdded = true;

        err = WEAVE_NO_ERROR;
    }

    // Make space for the new entry by moving the directory down the cache by the appropriate length.

    memmove(p + overrideEntryTotalLen, p, mDirAndSuffTableSize);

    // Write the host portlist control byte.
    // Host port list length = 1, reserved = 0, entry type = 01;

    listCtrl = (1 & kMask_HostPortListLen) | ((1 << 6) & kMask_DirectoryEntryType);
    Write8(p, listCtrl);

    // Write the Service Endpoint ID

    Write64(p, serviceEndpointId);

    // Write the item control byte.
    // HostID type = Fully qualified(00), Suffix index present = 0, port Id present = 1;

    itemCtrl = (1 << 3) & kMask_PortIdPresent;
    Write8(p, itemCtrl);

    // Write the Host Id string length

    Write8(p, hostLen);

    // Write the Host Id string

    memcpy(p, hostName, hostLen);

    // Skip over the hostName

    p += hostLen;

    // Write the port Id

    Write16(p, port);

    if (newEntryAdded)
    {
        // Update the directory length by this new entry.

        mDirectory.length++;
    }

    // Update the directory and suffix table size

    mDirAndSuffTableSize += overrideEntryTotalLen;

exit:
    WeaveLogProgress(ServiceDirectory, "%s : %s", __func__, nl::ErrorStr(err));

    return err;
}

/**
 * @brief This method cancels a connect request.
 *
 * This method cancels a connect request given the service endpoint ID and the
 * application state object passed in at request time as identifiers.
 * If it is the last connect request, this method clears up any pending service
 * directory connection state as well.
 *
 * @param [in] aServiceEp   The service endpoint ID of the request being canceled.
 *
 * @param [in] anAppState   A pointer to the app state object given to the
 *   connect() call.
 */
void WeaveServiceManager::cancel(uint64_t aServiceEp, void *aAppState)
{
    int activeRequests = 0;
    ConnectRequest *req;

    WeaveLogProgress(ServiceDirectory, "cancel()");

    for (uint8_t i = 0; i < ARRAY_SIZE(mConnectRequestPool); i++)
    {
        req = &mConnectRequestPool[i];

        if (req->isAllocatedTo(aServiceEp, aAppState))
            req->finalize();

        else if (!req->isFree())
            activeRequests++;
    }

    if (activeRequests == 0)
    {
        /*
         * in principle we could be in one of two states here -
         * waiting or resolved. if we're waiting then we need to set
         * the state back to resolving so that next time around it
         * will send out a directory request. otherwise, just leave
         * the state alone.
         */

        if (mCacheState == kServiceMgrState_Waiting)
            mCacheState = kServiceMgrState_Resolving;

        /*
         * now clean up the exchange state being used to request
         * service directory info.
         */

        cleanupExchangeContext(WEAVE_ERROR_CONNECTION_CLOSED_UNEXPECTEDLY);
    }
}

/**
 * @brief This method invalidates the service directory cache.
 *
 * This method sets the service directory cache state so that on the next
 * request the service manager will issue a service directory query.
 *
 * This version of the method - here for backward compatibility -
 * takes and logs an error then calls unresolve(void) .
 *
 * @param [in] aError The error that triggered this operation.
 *
 * @sa unresolve(void)
 */
void WeaveServiceManager::unresolve(WEAVE_ERROR aError)
{
    WeaveLogProgress(ServiceDirectory, "unresolve: %s", ErrorStr(aError));

    unresolve();
}

/**
 * @brief This method invalidates the service directory cache.
 *
 * This method sets the service directory cache state so that on the next request
 * the service manager will issue a service directory query.
 *
 * @sa unresolve(WEAVE_ERROR)
 *
 */
void WeaveServiceManager::unresolve(void)
{
    WeaveLogProgress(ServiceDirectory, "unresolve()");

    /*
     * we should only do this if the cache state has advanced beyond
     * "resolving". otherwise there's a chance of putting the service
     * directory in an inconsistent state.
     */

    if (mCacheState > kServiceMgrState_Resolving)
    {
        cleanupExchangeContext();

        mCacheState = kServiceMgrState_Resolving;

        finalizeConnectRequests();
    }
}

/**
 * @brief This method resets the service manager to its initial state.
 *
 * This method resets all service manager states including communications
 * state, cache state, and the state of any pending connect requests.
 *
 * This version of the method - here for backwards compatibility -
 * takes and logs an error then calls reset(void) .
 *
 * @param [in] aError The error that triggered this operation.
 *
 * @sa reset(void)
 */
void WeaveServiceManager::reset(WEAVE_ERROR aError)
{
    WeaveLogProgress(ServiceDirectory, "reset: %s", ErrorStr(aError));

    reset();
}

/**
 * @brief This method resets the service manager to its initial state.
 *
 * This method resets all service manager states including communications
 * state, cache state and the state of any pending connect requests.
 *
 * @sa reset(WEAVE_ERROR)
 *
 */
void WeaveServiceManager::reset(void)
{
    WeaveLogProgress(ServiceDirectory, "reset()");

    cleanupExchangeContext();

    clearWorkingState();
    clearCacheState();

    finalizeConnectRequests();
}

/**
 * @brief This method relocates the service directory cache.
 *
 * When a service endpoint returns a status report with status code
 * kStatus_Relocated, the application could call unresolve() to clear
 * up the cache and cancel connection requests. This method simplifies
 * error handling by calling unresolve() in the first time, and reset()
 * if the problem is not resolved yet.
 *
 * This version of the method - here for backwards compatibility -
 * takes and logs an error then calls relocate(void) .
 *
 * @param [in] aError an error to log.
 *
 * @sa relocate(void)
 */
void WeaveServiceManager::relocate(WEAVE_ERROR aError)
{
    WeaveLogProgress(ServiceDirectory, "relocate: %s", ErrorStr(aError));

    relocate();
}

/**
 * @brief This method relocates the service directory cache.
 *
 * When a service endpoint returns a status report with status code
 * kStatus_Relocated, the application could call unresolve() to clear
 * up the cache and cancel connection requests. This method simplifies
 * error handling by calling unresolve() in the first time, and reset()
 * if the problem is not resolved yet.
 *
 * @sa relocate(WEAVE_ERROR)
 */
void WeaveServiceManager::relocate(void)
{
    WeaveLogProgress(ServiceDirectory, "relocate()");

    if (mWasRelocated)
    {
        reset();
    }
    else
    {
        mWasRelocated = !mWasRelocated;

        unresolve();
    }
}

/**
 *  @brief
 *    This method handles the connect completed event for service endpoint
 *    query transaction.
 *
 *  There are a couple of possibilities. First, the connection could
 *  have failed in which case we're done. Otherwise, the connection is
 *  actually complete and what we want to do is open an exchange
 *  context and send a directory query.
 *
 *  @param [in] aError An Weave error if there is any error during the
 *    connection setup.
 */
void WeaveServiceManager::onConnectionComplete(WEAVE_ERROR aError)
{
    WEAVE_ERROR err = aError;
    PacketBuffer *buf = NULL;

    WeaveLogProgress(ServiceDirectory, "onConnectionComplete() <= %s", ErrorStr(aError));

    SuccessOrExit(err);

    // get an exchange context from EM

    mExchangeContext = mExchangeManager->NewContext(mConnection, this);
    VerifyOrExit(mExchangeContext, err = WEAVE_ERROR_NO_MEMORY);

    // get a buffer to send our message

    buf = PacketBuffer::NewWithAvailableSize(WEAVE_SYSTEM_CONFIG_HEADER_RESERVE_SIZE, 0);
    VerifyOrExit(buf, err = WEAVE_ERROR_NO_MEMORY);

    if (mServiceEndpointQueryBegin)
        mServiceEndpointQueryBegin();

    /*
     * put a "default" close callback in the connection in case it
     * gets closed from the other end.
     */

    mConnection->OnConnectionClosed = handleConnectionClosed;

    mExchangeContext->AppState = this;
    mExchangeContext->OnMessageReceived = handleResponseMsg;
    mExchangeContext->OnConnectionClosed = ecHandleConnectionClosed;
    mExchangeContext->OnResponseTimeout = handleResponseTimeout;
    mExchangeContext->ResponseTimeout = kWeave_DefaultSendTimeout;

    err = mExchangeContext->SendMessage(kWeaveProfile_ServiceDirectory,
                                        kMsgType_ServiceEndpointQuery,
                                        buf,
                                        ExchangeContext::kSendFlag_ExpectResponse);
    buf = NULL;

exit:

    if (err != WEAVE_NO_ERROR)
    {
        if (buf)
            PacketBuffer::Free(buf);

        fail(err);
    }
}

/**
 *  This method handles the connection closed event reported by the associated Weave exchange context.
 *
 *  @param [in] aError An Weave error indicating the reason for this connection to be closed.
 */
void WeaveServiceManager::onConnectionClosed(WEAVE_ERROR aError)
{
    WeaveLogProgress(ServiceDirectory, "onConnectionClosed() <= %s", ErrorStr(aError));

    fail(aError);
}

/**
 *  @brief
 *    This method handles any response message in the conversation with
 *    directory service.
 *
 *  @param [in] aProfileId   The profile ID for this incoming message.
 *  @param [in] aMsgType     The profile-specific type for this message.
 *  @param [in] aMsg         The content of this message.
 */
void WeaveServiceManager::onResponseReceived(uint32_t aProfileId, uint8_t aMsgType, PacketBuffer *aMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StatusReport report;
    bool redir = false;

    WeaveLogProgress(ServiceDirectory, "onResponseReceived(0x%x, %d)", aProfileId, aMsgType);

    // start by closing the old exchange context and connection to free up resources

    cleanupExchangeContext();

    if (aProfileId == kWeaveProfile_StatusReport_Deprecated || aProfileId == kWeaveProfile_Common)
    {
        /*
         * OK. so we got a status report rather than a response. at this
         * point our handling for this case is pretty primitive. we need
         * a more sophisticated way of doing errors like this.
         */

        StatusReport::parse(aMsg, report);

        WeaveLogProgress(ServiceDirectory, "status: %lx, %x", report.mProfileId, report.mStatusCode);

        clearWorkingState();

        mCacheState = kServiceMgrState_Initial;

        transactionsReportStatus(report);
    }

    else
    {
        VerifyOrExit(aProfileId == kWeaveProfile_ServiceDirectory, err = WEAVE_ERROR_INVALID_PROFILE_ID);

        WeaveLogProgress(ServiceDirectory, "kWeaveProfile_ServiceDirectory");

        /*
         * here, we've got an actual query response. what we do below
         * depends on the state we're in, as follows.
         */

        VerifyOrExit(aMsgType == kMsgType_ServiceEndpointResponse, err = WEAVE_ERROR_INVALID_MESSAGE_TYPE);
        VerifyOrExit(mCacheState == kServiceMgrState_Waiting, err = WEAVE_ERROR_INCORRECT_STATE);

        /*
         * this block unpacks the service directory message.
         */

        {
            MessageIterator i(aMsg);

            uint16_t msgLen = aMsg->DataLength();
            uint8_t dirCtrl;
            uint8_t *writePtr;
            uint8_t dirLen;
            bool suffixesPresent;
            uint8_t aLength;
            bool timePresent = false;

            err = i.readByte(&dirCtrl);
            SuccessOrExit(err);

            dirLen = dirCtrl & kMask_DirectoryLen;
            redir = (dirCtrl & kMask_Redirect) != 0;
            suffixesPresent = (dirCtrl & kMask_SuffixTablePresent) != 0;
            timePresent = (dirCtrl & kMask_TimeFieldsPresent) != 0;

            if (((msgLen > mCache.length) && !timePresent) || (msgLen > (mCache.length + (sizeof(uint64_t) + sizeof(uint32_t)))))
            {
                WeaveLogProgress(ServiceDirectory, "message length error: %d m.len:%d", msgLen, mCache.length);

                err = WEAVE_ERROR_MESSAGE_TOO_LONG;
            }
            SuccessOrExit(err);

            /*
             * here we have directory information beyond the root directory but
             * we're not done yet.
             */

            mDirectory.length = dirLen;
            writePtr = mDirectory.base = mCache.base;

            err = cacheDirectory(i, mDirectory.length, writePtr);
            SuccessOrExit(err);

            if (suffixesPresent)
            {
                WeaveLogProgress(ServiceDirectory, "suffixesPresent");

                err = i.readByte(&aLength);
                SuccessOrExit(err);

                mSuffixTable.length = aLength;
                writePtr += 1;
                mSuffixTable.base = writePtr;

                mDirAndSuffTableSize++;

                err = cacheSuffixes(i, mSuffixTable.length, writePtr);
                SuccessOrExit(err);
            }

            else
            {
                mSuffixTable.length = 0;
                mSuffixTable.base = NULL;
            }

            if (timePresent)
            {
                WeaveLogProgress(ServiceDirectory, "timePresent");

                err = handleTimeInfo(i);
                SuccessOrExit(err);
            }
        }

        /*
         * Release the received message buffer so that any code we
         * call below can immediately re-use it.
         */

        PacketBuffer::Free(aMsg);
        aMsg = NULL;

        if (redir)
        {
            // send out yet another query using this directory server.

            mConnection = mExchangeManager->MessageLayer->NewConnection();
            VerifyOrExit(mConnection, err = WEAVE_ERROR_NO_MEMORY);

            WeaveLogProgress(ServiceDirectory, "onResponseReceived(): redirecting");

            err = lookupAndConnect(mConnection,
                                   kServiceEndpoint_Directory,
                                   mDirAuthMode,
                                   this,
                                   handleSDConnectionComplete,
                                   WEAVE_CONFIG_SERVICE_DIR_CONNECT_TIMEOUT_MSECS);
        }

        else
        {
            mCacheState = kServiceMgrState_Resolved;

            WeaveLogProgress(ServiceDirectory, "onResponseReceived(): ->resolved");

            // now we gotta process all the pending transactions (see below)

            for (uint8_t j = 0; j < ARRAY_SIZE(mConnectRequestPool); j++)
            {

                /*
                 * go through all the transactions here even if some
                 * of them err out and invoke a handler. this leaves
                 * openthe possiblity that hihger layer code can
                 * handle indiviual failures individually.
                 */

                WEAVE_ERROR conErr;
                ConnectRequest *req = &mConnectRequestPool[j];
                StatusHandler handler = req->mStatusHandler;
                void *appState = req->mAppState;

                if (req->mServiceEp != 0)
                {
                    WeaveLogProgress(ServiceDirectory, "onResponseReceived() txn = %llx", req->mServiceEp);

                    conErr = lookupAndConnect(req->mConnection,
                                              req->mServiceEp,
                                              req->mAuthMode,
                                              req,
                                              handleAppConnectionComplete,
                                              req->mConnectTimeoutMsecs,
                                              req->mConnIntf);

                    if (conErr != WEAVE_NO_ERROR)
                    {
                        req->finalize();

                        if (handler)
                            handler(appState, conErr, NULL);
                    }
                }
            }
        }
    }

exit:

    // Free the received message buffer if it hasn't been done already.

    PacketBuffer::Free(aMsg);

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogProgress(ServiceDirectory, "onResponseReceived: %s", ErrorStr(err));

        fail(err);
    }
}

/**
 *  @brief
 *    This method handles the timeout event, in which no response was received
 *    from directory service.
 */
void WeaveServiceManager::onResponseTimeout(void)
{
    fail(WEAVE_ERROR_TIMEOUT);
}

/**
 *  @brief
 *    This method initializes a ConnectRequest instance with the arguments passed in
 *
 *  @param [in] aManager             A pointer to the containing service manager.
 *  @param [in] aServiceEp           An ID to the intended service endpoint for
 *    this connect request.
 *  @param [in] aAuthMode            A descriptor for the authentication method
 *    which should be used for this connection.
 *  @param [in] aAppState            An arbitrary pointer which would be passed
 *    back in callbacks.
 *  @param [in] aStatusHandler       A pointer to callback function which handles
 *    a status report in response to service endpoint query.
 *  @param [in] aCompleteHandler     A pointer to callback function which handles
 *    the connection complete event.
 *  @param [in] aConnectTimeoutMsecs The timeout for the Connect call to succeed
 *    or return an error.
 *  @param [in] aConnectIntf         The interface over which the connection is
 *    to be established.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 */
WEAVE_ERROR WeaveServiceManager::ConnectRequest::init(WeaveServiceManager *aManager,
                                                      const uint64_t &aServiceEp,
                                                      WeaveAuthMode aAuthMode,
                                                      void *aAppState,
                                                      StatusHandler aStatusHandler,
                                                      WeaveConnection::ConnectionCompleteFunct aCompleteHandler,
                                                      const uint32_t aConnectTimeoutMsecs,
                                                      const InterfaceId aConnIntf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aServiceEp != 0 && aCompleteHandler, err = WEAVE_ERROR_INVALID_ARGUMENT);

    mConnection = aManager->mExchangeManager->MessageLayer->NewConnection();
    VerifyOrExit(mConnection, err = WEAVE_ERROR_NO_MEMORY);

    mServiceEp = aServiceEp;
    mAuthMode = aAuthMode;
    mAppState = aAppState;
    mStatusHandler = aStatusHandler;
    mConnectionCompleteHandler = aCompleteHandler;
    mConnectTimeoutMsecs = aConnectTimeoutMsecs;
    mConnIntf = aConnIntf;

exit:

    return err;
}

/**
 *  This method frees a connection request object, returning it to the pool
 */
void WeaveServiceManager::ConnectRequest::free(void)
{
    if (!isFree())
    {
        SYSTEM_STATS_DECREMENT(nl::Weave::System::Stats::kServiceMgr_NumRequests);

        memset(this, 0, sizeof(*this));
    }
}

/**
 *  This method cleans up internal state, including connection closure
 */
void WeaveServiceManager::ConnectRequest::finalize(void)
{
    WeaveConnection *con = mConnection;

    free();

    if (con)
        con->Close();
}

/**
 *  This method is a trampoline to application layer for the connection complete event. It calls the
 *  connection complete handler assigned at lookupAndConnect() .
 */
void WeaveServiceManager::ConnectRequest::onConnectionComplete(WEAVE_ERROR aError)
{
    WeaveConnection *con = mConnection;
    WeaveConnection::ConnectionCompleteFunct handler = mConnectionCompleteHandler;

    con->AppState = mAppState;

    free();

    handler(con, aError);
}

/**
 *  This method frees the entire connect request pool.
 */
void WeaveServiceManager::freeConnectRequests(void)
{
    memset(mConnectRequestPool, 0, sizeof(mConnectRequestPool));

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kServiceMgr_NumRequests);
}

/**
 *  This method frees connect requests and close any hanging connections.
 */
void WeaveServiceManager::finalizeConnectRequests(void)
{
    for (int i = 0; i < kConnectRequestPoolSize; i++)
    {
        ConnectRequest &r = mConnectRequestPool[i];

        r.finalize();
    }

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kServiceMgr_NumRequests);
}

/**
 *  @brief
 *    This method allocates and returns a new connect request instance or NULL.
 *
 *  The returned ConnectRequest object is not initialized. A call to init()
 *  is necessary to properly initialize this object.
 *
 *  @return A pointer to an uninitialized instance of ConnectRequest on success;
 *    NULL otherwise.
 */
WeaveServiceManager::ConnectRequest *WeaveServiceManager::getAvailableRequest(void)
{
    WeaveServiceManager::ConnectRequest *retval = NULL;

    WEAVE_FAULT_INJECT(nl::Weave::FaultInjection::kFault_ServiceManager_ConnectRequestNew, return retval);

    for (uint8_t i = 0; i < ARRAY_SIZE(mConnectRequestPool); i++)
    {
        if (mConnectRequestPool[i].mServiceEp == 0)
        {
            retval = &(mConnectRequestPool[i]);
            SYSTEM_STATS_INCREMENT(nl::Weave::System::Stats::kServiceMgr_NumRequests);
            break;
        }
    }

    return retval;
}

WEAVE_ERROR WeaveServiceManager::calculateEntryLength(uint8_t *entryStart,
                                                      uint8_t entryCtrlByte,
                                                      uint16_t *entryLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint8_t listLen = entryCtrlByte & kMask_HostPortListLen;
    uint8_t entryType = entryCtrlByte & kMask_DirectoryEntryType;
    uint8_t *p = entryStart;

    *entryLen = 0;

    switch (entryType)
    {
      case kDirectoryEntryType_SingleNode:
        *entryLen += 8;
        break;
      case kDirectoryEntryType_HostPortList:
        for (uint8_t j = 0; j < listLen; j++)
        {
            uint8_t itemCtrlByte = Read8(p);
            (*entryLen)++;
            // read the string length and skip the name string

            uint8_t itemLen = Read8(p);
            (*entryLen)++;

            *entryLen += itemLen;

            // then the optional fields if any

            if (itemCtrlByte & kMask_SuffixIndexPresent)
                (*entryLen)++;

            if (itemCtrlByte & kMask_PortIdPresent)
                *entryLen += 2;
        }
        break;
      default:
        // don't know what to do about other entry types
        err = WEAVE_ERROR_INVALID_DIRECTORY_ENTRY_TYPE;
        break;
    }

    return err;
}

ServiceConnectBeginArgs::ServiceConnectBeginArgs(
    uint64_t inServiceEndpoint,
    WeaveConnection *inConnection,
    HostPortList *inEndpointHostPortList,
    InterfaceId inConnectIntf,
    WeaveAuthMode inAuthMode,
    uint8_t inDNSOptions) :
    ServiceEndpoint(inServiceEndpoint),
    Connection(inConnection),
    EndpointHostPortList(inEndpointHostPortList),
    ConnectIntf(inConnectIntf),
    AuthMode(inAuthMode),
    DNSOptions(inDNSOptions)
{
}

/**
 *  @brief
 *    This method looks up the given service endpoint in the cache and sets up an
 *    Weave connection with completion callback.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 */
WEAVE_ERROR WeaveServiceManager::lookupAndConnect(WeaveConnection *aConnection,
                                                  uint64_t aServiceEp,
                                                  WeaveAuthMode aAuthMode,
                                                  void *aAppState,
                                                  WeaveConnection::ConnectionCompleteFunct aHandler,
                                                  const uint32_t aConnectTimeoutMsecs,
                                                  const InterfaceId aConnectIntf)
{
    WEAVE_ERROR err;

    HostPortList hostPortList;
    WeaveLogProgress(ServiceDirectory, "lookupAndConnect(%llx...)", aServiceEp);

    err = lookup(aServiceEp, &hostPortList);
    SuccessOrExit(err);

    aConnection->AppState = aAppState;
    aConnection->OnConnectionComplete = aHandler;

    aConnection->SetConnectTimeout(aConnectTimeoutMsecs);

    {
        ServiceConnectBeginArgs connectBeginArgs
            (
            aServiceEp,
            aConnection,
            &hostPortList,
            aConnectIntf,
            aAuthMode,
#if WEAVE_CONFIG_ENABLE_DNS_RESOLVER
            ::nl::Inet::kDNSOption_Default
#else
            0
#endif
            );

        if (mConnectBegin != NULL)
        {
            mConnectBegin(connectBeginArgs);
        }

        err = aConnection->Connect(aServiceEp,
                                   connectBeginArgs.AuthMode,
                                   hostPortList,
                                   connectBeginArgs.DNSOptions,
                                   connectBeginArgs.ConnectIntf);
        SuccessOrExit(err);
    }

exit:

    if (err != WEAVE_NO_ERROR)
        WeaveLogProgress(ServiceDirectory, "lookupAndConnect: %s", ErrorStr(err));

    return err;
}


/**
 *  @brief
 *    This method updates the local directory cache with the response we
 *    receive from the directory service.
 *
 *  @param [in] aIterator       An iterator which is used to scan the input.
 *  @param [in] aLength         The number of entries in the directory being
 *    cached.
 *  @param [inout] aWritePtr    A reference to pointer to where we're writing
 *    in the cache which would be updated along the process. At return, the
 *    pointer should point to the byte right after the area which has been
 *    filled.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 */
WEAVE_ERROR WeaveServiceManager::cacheDirectory(MessageIterator &aIterator, uint8_t aLength, uint8_t *&aWritePtr)
{
    WEAVE_ERROR retval = WEAVE_NO_ERROR;
    uint8_t *startWritePtr = aWritePtr;

    for (int index = 0; index < aLength; index++)
    {
        // write the control byte

        retval = aIterator.readByte(aWritePtr);

        if (retval != WEAVE_NO_ERROR)
            break;

        uint8_t listCtrl = *aWritePtr;
        uint8_t listLen = listCtrl & kMask_HostPortListLen;

        aWritePtr++;

        // and the service EP
        retval = aIterator.read64((uint64_t *)aWritePtr);

        if (retval != WEAVE_NO_ERROR)
            break;

        aWritePtr += 8;

        if (0 == (listCtrl & ~kMask_HostPortListLen))
        {
            // flags are zero
            // this means we're looking at a single node ID

            retval = aIterator.read64((uint64_t *)aWritePtr);

            if (retval != WEAVE_NO_ERROR)
                break;

            aWritePtr += 8;
        }
        else
        {
            // otherwise it's a host/port list

            for (int j = 0; j < listLen; j++)
            {
                // again, write the control byte

                retval = aIterator.readByte(aWritePtr);

                if (retval != WEAVE_NO_ERROR)
                    break;

                uint8_t itemCtrl = *aWritePtr;

                aWritePtr++;

                // now the string (with length)

                retval = aIterator.readByte(aWritePtr);

                if (retval != WEAVE_NO_ERROR)
                    break;

                uint8_t strLen = *aWritePtr;

                aWritePtr++;

                retval = aIterator.readBytes(strLen, aWritePtr);

                if (retval != WEAVE_NO_ERROR)
                    break;

                aWritePtr += strLen;

                // now the optional bits

                if ((itemCtrl & kMask_SuffixIndexPresent) != 0)
                {
                    retval = aIterator.readByte(aWritePtr);

                    if (retval != WEAVE_NO_ERROR)
                        break;

                    aWritePtr++;
                }

                if ((itemCtrl & kMask_PortIdPresent) != 0)
                {
                    retval = aIterator.read16((uint16_t *)aWritePtr);

                    if (retval != WEAVE_NO_ERROR)
                        break;

                    aWritePtr += 2;
                }
            }
        }
    }

    // Store the size of the directory in bytes.

    mDirAndSuffTableSize += aWritePtr - startWritePtr;

    return retval;
}

/**
 *  @brief
 *    This method updates suffix part of the local directory cache with the
 *    response we receive from the directory service.
 *
 *  @param [in] aIterator       An iterator which is used to scan the input.
 *  @param [in] aLength         The number of entries in the directory being
 *    cached.
 *  @param [inout] aWritePtr    A reference to pointer to where we're writing
 *    in the cache which would be updated along the process. At return, the
 *    pointer should point to the byte right after the area which has been
 *    filled.
 *
 *  @return #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 */
WEAVE_ERROR WeaveServiceManager::cacheSuffixes(MessageIterator &aIterator, uint8_t aLength, uint8_t *&aWritePtr)
{
    WEAVE_ERROR retval = WEAVE_NO_ERROR;
    uint8_t *startWritePtr = aWritePtr;

    for (uint8_t i = 0; i < aLength; i++)
    {
        // write the string (with length)

        retval = aIterator.readByte(aWritePtr);

        if (retval != WEAVE_NO_ERROR)
            break;

        uint8_t strLen = *aWritePtr;

        aWritePtr++;

        retval = aIterator.readBytes(strLen, aWritePtr);

        if (retval != WEAVE_NO_ERROR)
            break;

        aWritePtr += strLen;
    }

    // Add the suffix table to the length

    mDirAndSuffTableSize += aWritePtr - startWritePtr;

    return retval;
}

/**
 *  @brief
 *    This method cleans up after any failure by clearing the service
 *    manager's working state, calling all the appropriate handler methods and
 *    freeing any pending transactions.
 *
 *  @param[in] anError An error code indicating the cause of failure.
 */
void WeaveServiceManager::fail(WEAVE_ERROR aError)
{
    WeaveLogProgress(ServiceDirectory, "fail() <= %s", ErrorStr(aError));

    cleanupExchangeContext(aError);

    clearWorkingState();
    clearCacheState();

    transactionsErrorOut(aError);
}

/**
 *  @brief
 *    This method finalizes all connection requests, and calls
 *    status handler to allocated connection requests, with the error code.
 *
 *  @param[in] anError An error code indicating cause of failure.
 */
void WeaveServiceManager::transactionsErrorOut(WEAVE_ERROR aError)
{
    for (size_t i = 0; i < ARRAY_SIZE(mConnectRequestPool); i++)
    {
        ConnectRequest *req = &mConnectRequestPool[i];

        StatusHandler statusHndlr = req->mStatusHandler;
        void *appState = req->mAppState;

        req->finalize();

        if (statusHndlr && appState)
            statusHndlr(appState, aError, NULL);
    }

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kServiceMgr_NumRequests);
}

/**
 *  @brief
 *    This method finalizes all connection requests, and calls
 *    status handler to allocated connection requests, with the status report.
 *
 *  @param[in] aReport A reference to a status report received from a connection.
 */
void WeaveServiceManager::transactionsReportStatus(StatusReport &aReport)
{
    for (size_t i = 0; i < ARRAY_SIZE(mConnectRequestPool); i++)
    {
        ConnectRequest *req = &mConnectRequestPool[i];

        StatusHandler statusHndlr = req->mStatusHandler;
        void *appState = req->mAppState;

        req->finalize();

        if (statusHndlr && appState)
            statusHndlr(appState, WEAVE_NO_ERROR, &aReport);
    }

    SYSTEM_STATS_RESET(nl::Weave::System::Stats::kServiceMgr_NumRequests);
}

/**
 *  @brief
 *    This method clears the current exchange context and its associated
 *    connection.
 *
 *  @param[in] aErr WEAVE_NO_ERROR if this cleanup is not caused by
 *    any error. Otherwise, the connection would be aborted instead of
 *    gracefully closed.
 *
 *  @sa cleanupExchangeContext(void)
 */
void WeaveServiceManager::cleanupExchangeContext(WEAVE_ERROR aErr)
{
    if (mExchangeContext)
    {
        mExchangeContext->Close();
        mExchangeContext = NULL;
    }

    if (mConnection)
    {
        if (WEAVE_NO_ERROR == aErr)
        {
            mConnection->Close();
        }
        else
        {
            mConnection->Abort();
        }
        mConnection = NULL;
    }
}

/**
 *  @brief
 *    This method clears the current exchange context and its associated
 *    connection.
 *
 *  @sa cleanupExchangeContext(WEAVE_ERROR)
 */
void WeaveServiceManager::cleanupExchangeContext(void)
{
    cleanupExchangeContext(WEAVE_NO_ERROR);
}

/**
 *  @brief
 *    This method clears the working state of the manager leaving the cache state
 *    alone.
 */
void WeaveServiceManager::clearWorkingState(void)
{
    mDirectory.length = 0;
    mDirectory.base = NULL;
    mSuffixTable.length = 0;
    mSuffixTable.base = NULL;
    mDirAndSuffTableSize = 0;
}

/**
 *  @brief
 *    This private method parses the time related fields in response
 *    message. It calls mServiceEndpointQueryEndWithTimeInfo with the result
 *    if it's not NULL.
 *
 *  @pararm[inout] itMsg A message iterator pointing to the beginning of time
 *    information. If no error is reported, the message iterator would be
 *    advanced after the time-related fields
 *
 *  @retval #WEAVE_NO_ERROR on success; otherwise, a respective error code.
 *  @retval #WEAVE_ERROR_BUFFER_TOO_SMALL if the parsing fails because of
 *    buffer underrun
 */
WEAVE_ERROR WeaveServiceManager::handleTimeInfo(MessageIterator &itMsg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint64_t timeQueryReceiptMsec;
    uint32_t timeProcessMsec;

    SuccessOrExit(err = itMsg.read64(&timeQueryReceiptMsec));
    SuccessOrExit(err = itMsg.read32(&timeProcessMsec));

    if (NULL != mServiceEndpointQueryEndWithTimeInfo)
    {
        mServiceEndpointQueryEndWithTimeInfo(timeQueryReceiptMsec, timeProcessMsec);
    }

exit:
    WeaveLogFunctError(err);
    return err;
}

/**
 *  @brief
 *    This method clears the state and cache of the manager if the state is in
 *    the terminal kServiceMgrState_Resolved state, which means that response
 *    from Service Directory endpoint was received.
 */
void WeaveServiceManager::clearCache(void)
{
    WeaveLogProgress(ServiceDirectory, "clearCache(), state is %d", mCacheState);

    if (mCacheState == kServiceMgrState_Resolved)
    {
        clearWorkingState();
        clearCacheState();
    }
}
#endif //WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
