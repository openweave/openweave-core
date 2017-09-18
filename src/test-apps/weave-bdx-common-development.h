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
 * This file contains declarations of callbacks and helper functions common across the client and
 * server BDX test implementations.  They can serve, along with weave-bdx-client.cpp
 * and weave-bdx-server.cpp, as examples of how to use the BDX API to configure and run
 * a client/server.  Specifically, they handle transferring simple files using the C++
 * FILE API.
 */

#ifndef _WEAVE_BDX_COMMON_H_
#define _WEAVE_BDX_COMMON_H_

#include <stdio.h>

#include <Weave/Profiles/bulk-data-transfer/Development/BulkDataTransfer.h>

using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::BulkDataTransfer;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

// AppState object for holding application-specific info that is passed around to handlers
// This object is attached to a BDXTransfer via its mAppState member.
struct BdxAppState
{
    FILE *mFile;
    bool mDone;
    uint8_t *mBuffer; // buffer to store read blocks
};

// Returns a reference to a static BdxAppState so that handlers can grab one
// implementations can access it.  Production code should use some sort of pool and
// retrieve a reference to one of the objects when sending or receiving an Init
BdxAppState * NewAppState();
void ResetAppStates();

void SetReceivedFileLocation(const char *path);
void SetTempLocation(const char *path);

// Helper functions
size_t WriteData(void *aPtr, size_t aSize, size_t aNmemb, FILE *aStream);
size_t ReadData(char *aPtr, size_t aSize, size_t aNemb, FILE *aStream);
/** This function curls the given file (so it must be a URI, use file:// for local files),
 * saves it in TempFileLocation, modifies aFileDesignator to point to the newly saved
 * copy, and returns a status code indicating whether the curl was successful. */
int DownloadFile(char *aFileDesignator, size_t aFileDesignatorBufferSize);

// Application-defined callbacks passed to the BDX server for use in the protocol
uint16_t BdxSendInitHandler(BDXTransfer *aXfer, SendInit *aSendInitMsg);
uint16_t BdxReceiveInitHandler(BDXTransfer *aXfer, ReceiveInit *aReceiveInit);
WEAVE_ERROR BdxSendAcceptHandler(BDXTransfer *aXfer, SendAccept *aSendAcceptMsg);
WEAVE_ERROR BdxReceiveAcceptHandler(BDXTransfer *aXfer, ReceiveAccept *aReceiveAcceptMsg);
void BdxRejectHandler(BDXTransfer *aXfer, StatusReport *aReport);
void BdxGetBlockHandler(BDXTransfer *aXfer,
                        uint64_t *aLength,
                        uint8_t **aDataBlock,
                        bool *aIsLastBlock);
void BdxPutBlockHandler(BDXTransfer *aXfer, uint64_t aLength, uint8_t *aDataBlock, bool aIsLastBlock);
void BdxXferErrorHandler(BDXTransfer *aXfer, StatusReport *aXferError);
void BdxXferDoneHandler(BDXTransfer *aXfer);
void BdxErrorHandler(BDXTransfer *aXfer, WEAVE_ERROR aErrorCode);

//namespaces
}
}
}
}
#endif //_WEAVE_BDX_COMMON_H_
