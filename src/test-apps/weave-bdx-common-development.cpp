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
 * This file contains callbacks and helper functions common across the client and
 * server BDX test implementations.  They can serve, along with weave-bdx-client.cpp
 * and weave-bdx-server.cpp, as examples of how to use the BDX API to configure and run
 * a client/server.  Specifically, they handle transferring simple files using the C++
 * FILE API.
 */


#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

// This code uses DEVELOPMENT BDX namespace

#define WEAVE_CONFIG_BDX_NAMESPACE kWeaveManagedNamespace_Development

#include <Weave/Core/WeaveConfig.h>
#include <Weave/Support/logging/WeaveLogging.h>

#include "weave-bdx-common-development.h"

#include "ToolCommon.h"
// We can configure the server to download a requested URI using curl to represent
// grabbing an update image, but we disable this by default to not introduce the
// libcurl requirement to Weave.
#if HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif

#if HAVE_CURL_EASY_H
#include <curl/easy.h>
#endif

using nl::StatusReportStr;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Profiles::Common;
using namespace nl::Weave::Profiles::BulkDataTransfer;
using namespace nl::Weave::Logging;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(BDX, kWeaveManagedNamespaceDesignation_Development) {

static BdxAppState mAppStatePool[WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS];

// curled files go here
char TempFileLocation[FILENAME_MAX] = "/tmp/";
// bdx-received files go here
char ReceivedFileLocation[FILENAME_MAX] = "/tmp/";

BdxAppState *NewAppState()
{
    BdxAppState *appState;
    for (int i = 0; i < WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS; i++)
    {
        appState = &mAppStatePool[i];
        if (appState->mFile != NULL || !appState->mDone || appState->mBuffer != NULL)
        {
            continue;
        }

        return appState;
    }

    WeaveLogError(BDX, "BDX: Ran out of app states, maximum %d", WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS);
    return NULL;
}

void ResetAppStates()
{
    for (int i = 0; i < WEAVE_CONFIG_BDX_MAX_NUM_TRANSFERS; i++)
    {
        mAppStatePool[i].mFile = NULL;
        mAppStatePool[i].mDone = true;
        mAppStatePool[i].mBuffer = NULL;
    }
}

void SetReceivedFileLocation(const char *path)
{
    strncpy(ReceivedFileLocation, path, sizeof(ReceivedFileLocation));
    ReceivedFileLocation[sizeof(ReceivedFileLocation) - 1] = '\0';
    // End string with / if it doesn't already
    if (ReceivedFileLocation[strlen(ReceivedFileLocation) - 1] != '/')
    {
        ReceivedFileLocation[strlen(ReceivedFileLocation)] = '/';
    }

    ReceivedFileLocation[sizeof(ReceivedFileLocation) - 1] = '\0';
}

void SetTempLocation(const char *path)
{
    strncpy(TempFileLocation, path, sizeof(TempFileLocation));
    TempFileLocation[sizeof(TempFileLocation) - 1] = '\0';
    // End string with / if it doesn't already
    if (TempFileLocation[strlen(TempFileLocation) - 1] != '/')
    {
        TempFileLocation[strlen(TempFileLocation)] = '/';
    }

    TempFileLocation[sizeof(TempFileLocation) - 1] = '\0';
}

/** Helper function for use by libcurl. */
size_t WriteData(void *aPtr, size_t aSize, size_t aNmemb, FILE *aStream)
{
    size_t written = fwrite(aPtr, aSize, aNmemb, aStream);
    return written;
}

/** Helper function for writing data to a file. It is unclear as to why (or if) we
 * need the while loop in here (this code was inherited from an older branch).
 * Perhaps for handling streams that are being added to as we consume from them?
 */
size_t ReadData(char *aPtr, size_t aSize, size_t aNmemb, FILE *aStream)
{
    int nread = 0, left = aNmemb;

    WeaveLogDetail(BDX, "0 read_n entering\n");

    while (left > 0)
    {
        WeaveLogDetail(BDX, "1 read_n (left: %d)\n", left);

        if ((nread = fread(aPtr, aSize, aNmemb, aStream)) > 0)
        {
            left -= nread;
            aPtr += nread;
            WeaveLogDetail(BDX, "2 read_n (nread: %d, left: %d)\n", nread, left);
        }
        else
        {
            WeaveLogDetail(BDX, "3 read_n (nread: 0, left: %d)\n", left);
            return aNmemb - left;
        }
    }

    WeaveLogDetail(BDX, "4 read_n (n: %d, left: %d) exiting\n", nread, left);
    return nread;
}

/** This function uses libcurl to download the file specified by the URI aFileDesignator.
 * If successful, it modifies aFileDesignator to point to the location of where the
 * file was downloaded to (it gets stored in a directory specified by
 * TempFileLocation).
 *
 * @note
 *   If you want to get a local file, specify the file:// protocol
 */
#if HAVE_CURL_CURL_H && HAVE_CURL_EASY_H
int DownloadFile(char *aFileDesignator, size_t aFileDesignatorBufferSize)
{
    CURL *curl = NULL;
    FILE *fp;
    CURLcode res = CURLE_FAILED_INIT;
    char *pch = NULL, *file_name = NULL;
    char *downloadURL = (char*)malloc(1 + strlen(aFileDesignator));
    char outfilename[FILENAME_MAX];
    char fileTemplate[] = "/tmp/fileXXXXXX";
    int retval = 0;

    strncpy(outfilename, TempFileLocation, sizeof(outfilename));
    outfilename[sizeof(outfilename) - 1] = '\0';

    strcpy(downloadURL, aFileDesignator);

    // Extract the file name out of the download URL
    pch = strtok(aFileDesignator, "/");
    while (pch != NULL)
    {
        file_name = pch;
        pch = strtok(NULL, "/");
    }

    strncat(outfilename, file_name, FILENAME_MAX-1);

    if (mkstemp(fileTemplate) != -1)
    {
        curl = curl_easy_init();
    }

    if (curl)
    {
        fp = fopen(fileTemplate, "wb");

        WeaveLogDetail(BDX, "BDX: Downloading Image : |%s|\n", downloadURL);
        curl_easy_setopt(curl, CURLOPT_URL, downloadURL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteData);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);

        /* always cleanup */
        curl_easy_cleanup(curl);
        fclose(fp);

        if (res != CURLE_OK)
        {
            retval = remove(fileTemplate);
            if (retval != 0)
            {
                WeaveLogError(BDX, "BDX: Failed to remove the temporary file\n");
            }
            aFileDesignator = NULL;
        }
        else
        {
            retval = rename(fileTemplate, outfilename);
            if (retval != 0)
            {
                WeaveLogError(BDX, "BDX: Failed to rename the temporary file to %s\n", outfilename);
            }

            strncpy(aFileDesignator, outfilename, aFileDesignatorBufferSize-1);
            aFileDesignator[aFileDesignatorBufferSize-1] = '\0';
        }
    }
    else
    {
        WeaveLogError(BDX, "BDX: Failed to initialize curl\n");
        aFileDesignator = NULL;
    }

    free(downloadURL);
    downloadURL = NULL;

    return (int)res;
}
#endif // HAVE_CURL_CURL_H && HAVE_CURL_EASY_H

/** Example implementation of a SendInitHandler that opens the requested file if possible
 * (in a directory specified by ReceivedFileLocation) and sets up the BDXTransfer
 * by attaching our AppState to store the open file handle and setting the appropriate
 * handlers.
 *
 * @err  #kStatus_ServerBadState        If the file to be written to couldn't be opened
 */
uint16_t BdxSendInitHandler(BDXTransfer *aXfer, SendInit *aSendInitMsg)
{
    uint16_t err = kStatus_NoError;
    BDXHandlers handlers =
    {
        NULL, NULL, NULL, NULL,
        BdxPutBlockHandler, BdxXferErrorHandler, BdxXferDoneHandler, BdxErrorHandler
    };

    // Copy file name onto C-string
    // NOTE: the original string is not NUL terminated, but we know its length.
    // TODO: put this in the aXfer's fileDesignator
    char fileDesignator[aSendInitMsg->mFileDesignator.theLength + strlen(ReceivedFileLocation) + 1];
    memcpy(fileDesignator, ReceivedFileLocation, strlen(ReceivedFileLocation));
    memcpy(fileDesignator + strlen(ReceivedFileLocation), aSendInitMsg->mFileDesignator.theString, aSendInitMsg->mFileDesignator.theLength);
    fileDesignator[aSendInitMsg->mFileDesignator.theLength + strlen(ReceivedFileLocation)] = '\0';

    WeaveLogDetail(BDX, "Send request for file: %s", fileDesignator);

    BdxAppState * mAppState = NewAppState();
    VerifyOrExit(mAppState != NULL, err = kStatus_ServerBadState);
    aXfer->mAppState = mAppState;

    // The client already handles Setting transfer mode, max block size, and start sending
    // We just need to open the file and allocate a buffer for reading blocks
    mAppState->mFile = fopen(fileDesignator, "w");
    VerifyOrExit(mAppState->mFile != NULL, err = kStatus_ServerBadState);

    //TODO: shouldn't be using dynamic memory allocation, but how to do that with dynamically negotiated maxBlockSize???
    //perhaps just go ahead and allocate our maximum size since we know the transfer won't go above that?
    mAppState->mBuffer = (uint8_t *)malloc(aSendInitMsg->mMaxBlockSize);

    // All seems good, so accept the transfer and set the handlers
    aXfer->mIsAccepted = true;
    aXfer->mTransferMode = aSendInitMsg->mSenderDriveSupported ? kMode_SenderDrive : kMode_ReceiverDrive;

    aXfer->SetHandlers(handlers);

exit:
    return err;
}

/** Example implementation of a ReceiveInitHandler that downloads the requested file
 * if possible and configured to do so (see DownloadFile()) and sets up the BDXTransfer
 * by attaching our AppState to store the open file handle and setting the appropriate
 * handlers.
 *
 * @err  #kStatus_UnknownFile    If the file couldn't be found
 */
uint16_t BdxReceiveInitHandler(BDXTransfer *aXfer, ReceiveInit *aReceiveInit)
{
    uint16_t err = kStatus_NoError;
    int retval = 0;
    long fileSize = 0;
    BdxAppState *mAppState;
    char *fileDesignator = NULL;
#if !defined(HAVE_CURL_CURL_H) || !defined(HAVE_CURL_EASY_H)
    char *tempFileDesignator = NULL;
#endif
    FILE *targetFile = NULL;

    BDXHandlers handlers =
    {
        NULL, NULL, NULL, BdxGetBlockHandler,
        NULL, BdxXferErrorHandler, BdxXferDoneHandler, BdxErrorHandler
    };

    // TODO: put this in the aXfer's fileDesignator
    // Copy file name onto C-string
    // NOTE: the original string is not NUL terminated, but we know its length.
    fileDesignator = (char *)malloc(FILENAME_MAX);
    VerifyOrExit(fileDesignator != NULL, err = kStatus_ServerBadState);
    memcpy(fileDesignator, aReceiveInit->mFileDesignator.theString, aReceiveInit->mFileDesignator.theLength);
    fileDesignator[aReceiveInit->mFileDesignator.theLength] = '\0';

#if HAVE_CURL_CURL_H && HAVE_CURL_EASY_H
    // We download the file specified by URI and then open it
    WeaveLogDetail(BDX, "Download URI : %s", fileDesignator);

    // NOTE: DownloadFile will mutate the fileDesignator to no longer be a URI
    retval = DownloadFile(fileDesignator, FILENAME_MAX);

    VerifyOrExit(retval == CURLE_OK,
                 err = kStatus_UnknownFile;
                 WeaveLogError(BDX, "Unable to download the file : %d", err));
#else
    // If fileDesignator doesn't specify a local file with file:// and we
    // don't have curl, exit out.
    if (strncmp(fileDesignator, "file://", strlen("file://") != 0))
    {
        WeaveLogError(BDX, "Curl not found and we're given a non-local file path.");
        err = kStatus_XferMethodNotSupported;
        SuccessOrExit(err);
    }
    else
    {
        tempFileDesignator = (char *)malloc(strlen(fileDesignator));
        VerifyOrExit(tempFileDesignator != NULL, err = kStatus_ServerBadState);

        strcpy(tempFileDesignator, fileDesignator + strlen("file://"));
        strcpy(fileDesignator, tempFileDesignator);
        free(tempFileDesignator);
    }
#endif // HAVE_CURL_CURL_H && HAVE_CURL_EASY_H

    mAppState = NewAppState();
    VerifyOrExit(mAppState != NULL, err = kStatus_ServerBadState);

    aXfer->mAppState = mAppState;

    // The client already handles Setting transfer mode, max block size, and start sending
    // We just need to open the file and allocate a buffer for reading blocks
    targetFile = fopen(fileDesignator, "r");
    VerifyOrExit(targetFile != NULL,
                 err = kStatus_UnknownFile;
                 WeaveLogError(BDX, "Error opening file %s", fileDesignator));
    // Use fseek/ftell to find the size of the file.
    fseek(targetFile, 0, SEEK_END);
    // This will only work for files below 2 GB
    fileSize = ftell(targetFile);
    VerifyOrExit(fileSize >= 0, err = kStatus_Unknown);
    VerifyOrExit(static_cast<uint64_t>(fileSize) >= aReceiveInit->mStartOffset, err = kStatus_StartOffsetNotSupported);

    retval = fseek(targetFile, aReceiveInit->mStartOffset, SEEK_SET);
    VerifyOrExit(retval == 0, err = kStatus_StartOffsetNotSupported);

    mAppState->mFile = targetFile;

    targetFile = NULL;

    if (aReceiveInit->mLength == 0)
    {
        aXfer->mLength = fileSize - aReceiveInit->mStartOffset;
    }
    else
    {
        aXfer->mLength = (aReceiveInit->mLength + aReceiveInit->mStartOffset > static_cast<uint64_t>(fileSize)) ? (fileSize - aReceiveInit->mStartOffset) : (aReceiveInit->mLength);
    }

    //TODO: shouldn't be using dynamic memory allocation, but how to do that with dynamically negotiated maxBlockSize???
    //perhaps just go ahead and allocate our maximum size since we know the transfer won't go above that?
    mAppState->mBuffer = (uint8_t *)malloc(aReceiveInit->mMaxBlockSize);

    // All seems good, so accept the transfer and set the handlers
    aXfer->mIsAccepted = true;
    aXfer->mTransferMode = aReceiveInit->mReceiverDriveSupported ? kMode_ReceiverDrive : kMode_SenderDrive;
    aXfer->SetHandlers(handlers);

exit:
    if (fileDesignator != NULL)
    {
        free(fileDesignator);
    }

    if (targetFile != NULL)
    {
        fclose(targetFile);
        targetFile = NULL;
    }

    return err;
}

/** Example implementation of a SendAccept handler that opens the file we previously
 * requested to send in a SendInit message and sets up the transfer by associating
 * the appropriate AppState for storing the file handle and sets the handlers for
 * the BDXTransfer.
 */
WEAVE_ERROR BdxSendAcceptHandler(BDXTransfer *aXfer, SendAccept *aSendAcceptMsg)
{
    WeaveLogDetail(BDX, "SendInit Accepted: %hd maxBlockSize, transfer mode is %hd", aSendAcceptMsg->mMaxBlockSize, aXfer->mTransferMode);
    BdxAppState* bdxState = static_cast<BdxAppState*>(aXfer->mAppState);
    WEAVE_ERROR error = WEAVE_NO_ERROR;

    // The client already handles Setting transfer mode, max block size, and start sending
    // We just need to open the file and allocate a buffer for reading blocks
    bdxState->mFile = fopen(aXfer->mFileDesignator.theString, "r");
    if (!bdxState->mFile)
    {
        printf("Error opening file %s\n", aXfer->mFileDesignator.theString);
        exit(-1);
    }

    //TODO: shouldn't be using dynamic memory allocation, but how to do that with dynamically negotiated maxBlockSize???
    //perhaps just go ahead and allocate our maximum size since we know the transfer won't go above that?
    bdxState->mBuffer = (uint8_t*)malloc(aSendAcceptMsg->mMaxBlockSize);

    return error;
}

/** Example implementation of a ReceiveAccept handler that opens the file we previously
 * requested to receive in a ReceiveInit message and sets up the transfer by associating
 * the appropriate AppState for storing the file handle and sets the handlers for
 * the BDXTransfer.
 *
 * @note
 *   The file will be saved inside the ReceivedFileLocation directory
 */
WEAVE_ERROR BdxReceiveAcceptHandler(BDXTransfer *aXfer, ReceiveAccept *aReceiveAcceptMsg)
{
    WeaveLogDetail(BDX, "ReceiveInit Accepted: %hd maxBlockSize, transfer mode is 0x%hx", aReceiveAcceptMsg->mMaxBlockSize, aXfer->mTransferMode);
    BdxAppState* bdxState = static_cast<BdxAppState*>(aXfer->mAppState);
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // The client already handles Setting transfer mode, max block size, and start sending
    // We just need to open the file and allocate a buffer for reading blocks
    // NOTE: we expect mFileDesignator to be a URI, so need to chop off the protocol.
    // We store the file in the ReceivedFileLocation with a different name so that running
    // both client and server on the same machine won't overwrite the source file.
    char * filename = strrchr(aXfer->mFileDesignator.theString, '/');
    if (filename == NULL)
    {
        filename = aXfer->mFileDesignator.theString;
    }
    else
    {
        filename++; //skip over '/'
    }

    char fileDesignator[strlen(ReceivedFileLocation) + strlen(filename) + 1];

    memcpy(fileDesignator, ReceivedFileLocation, strlen(ReceivedFileLocation));
    memcpy(fileDesignator + strlen(ReceivedFileLocation), filename, strlen(filename));

    fileDesignator[strlen(ReceivedFileLocation) + strlen(filename)] = '\0';

    WeaveLogDetail(BDX, "File being saved to: %s", fileDesignator);

    bdxState->mFile = fopen(fileDesignator, "w");
    if (!bdxState->mFile)
    {
        WeaveLogDetail(BDX, "Error opening file %s\n", fileDesignator);
        exit(-1);
    }

    return err;
}

/** Example implementation of a RejectHandler, which simply logs the StatusReport
 * and sets the transfer as complete so the example client knows to exit.
 */
void BdxRejectHandler(BDXTransfer *aXfer, StatusReport *aReport)
{
    WeaveLogProgress(BDX, "BDX Init message rejected: %d", aReport->mStatusCode);
    // mark as done to close client
    ((BdxAppState*)aXfer->mAppState)->mDone = true;
}

/** Example implementation of a GetBlockHandler that reads a block from the associated
 * open file handle, stores it in the AppState's buffer, and sets the parameters as
 * appropriate so the protocol can handle the block.
 */
void BdxGetBlockHandler(BDXTransfer *aXfer,
                        uint64_t *aLength,
                        uint8_t **aDataBlock,
                        bool *aIsLastBlock)
{
    BdxAppState* bdxState = static_cast<BdxAppState*>(aXfer->mAppState);
    uint64_t blockSize = 0;

    if (aXfer->mLength != 0 && ((aXfer->mLength - aXfer->mBytesSent) < aXfer->mMaxBlockSize))
    {
        blockSize = aXfer->mLength - aXfer->mBytesSent;
    }
    else
    {
        blockSize = aXfer->mMaxBlockSize;
    }

    *aLength = fread(bdxState->mBuffer, 1, blockSize, bdxState->mFile);
    *aDataBlock = bdxState->mBuffer;
    aXfer->mBytesSent += blockSize;

    *aIsLastBlock = (*aLength < aXfer->mMaxBlockSize) ? true : false;
}

/** Example implementation of a PutBlockHandler that dumps the block to stdout for
 * debugging and then writes it to the file handle associated with the transfer.
 */
void BdxPutBlockHandler(BDXTransfer *aXfer, uint64_t aLength, uint8_t *aDataBlock, bool aIsLastBlock)
{
    BdxAppState* bdxState = static_cast<BdxAppState*>(aXfer->mAppState);

    DumpMemory(aDataBlock, aLength, "--> ", 16);

    if (bdxState->mFile)
    {
        // Write bulk data to disk.
        int wtd = fwrite(aDataBlock, 1, aLength, bdxState->mFile);
        WeaveLogDetail(BDX, "PutBlockHandler wrote %d bytes to disk", wtd);
    }
}

/** Example XferErrorHandler that simply logs the error and closes the entire program.
 * A real implemenation on a platform should obviously try to handle the error in a more
 * intelligent manner rather than hard exiting.
 */
//TODO: decide when we should be freeing any resources (esp. mallocs) associated with
//a transfer, including freeing up the BdxAppState object.
void BdxXferErrorHandler(BDXTransfer *aXfer, StatusReport *aXferError)
{
    BdxAppState *appState = (BdxAppState *)(aXfer->mAppState);

    WeaveLogProgress(BDX, "Transfer error: %d", aXferError->mStatusCode);

    if (appState->mFile)
    {
        if (fclose(appState->mFile))
        {
            printf("Error closing file! Permissions?\n");
        }
        appState->mFile = NULL;
    }

    appState->mDone = true;

    // app-defined state to tell main() to terminate client program
    if (appState->mBuffer != NULL)
    {
        free(appState->mBuffer);
    }

    appState->mBuffer = NULL;

    aXfer->Shutdown();
}

/** Example XferDoneHandler that closes the associated file handler, notifies our
 * AppState that the transfer is complete (so the client can shut down), and frees
 * any dynamically allocated resources for this transfer.
 */
void BdxXferDoneHandler(BDXTransfer *aXfer)
{
    WeaveLogDetail(BDX, "Transfer complete!");
    BdxAppState *appState = (BdxAppState *)(aXfer->mAppState);
    if (appState->mFile)
    {
        if (fclose(appState->mFile))
        {
            printf("Error closing file! Permissions?\n");
        }
        appState->mFile = NULL;
    }

    appState->mDone = true;

    // app-defined state to tell main() to terminate client program
    if (appState->mBuffer != NULL)
    {
        free(appState->mBuffer);
    }

    appState->mBuffer = NULL;

    aXfer->Shutdown();
}

/** Example ErrorHandler that logs the error and shuts down the transfer as well as the
 * client.
 */
void BdxErrorHandler(BDXTransfer *aXfer, WEAVE_ERROR aErrorCode)
{
    BdxAppState *appState = static_cast<BdxAppState *>(aXfer->mAppState);

    WeaveLogProgress(BDX, "BDX error: %s", ErrorStr(aErrorCode));

    // app-defined state to tell main() to terminate client program
    appState->mDone = true;

    if (appState->mFile)
    {
        fclose(appState->mFile);
        appState->mFile = NULL;
    }

    if (appState->mBuffer != NULL)
    {
        free(appState->mBuffer);
        appState->mBuffer = NULL;
    }


    aXfer->Shutdown();
}

// namespaces
}
}
}
}
