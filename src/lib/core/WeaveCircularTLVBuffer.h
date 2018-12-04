/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
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
 *  @file
 *      This file defines the circular buffer for TLV
 *      elements. When used as the backing store for the TLVReader and
 *      TLVWriter, those classes will work with the wraparound of data
 *      within the buffer.  Additionally, the TLVWriter will be able
 *      to continually add top-level TLV elements by evicting
 *      pre-existing elements.
 */

#ifndef WEAVE_CIRCULAR_TLV_BUFFER_H_
#define WEAVE_CIRCULAR_TLV_BUFFER_H_

#include <stdlib.h>

#include <Weave/Support/NLDLLUtil.h>
#include <Weave/Core/WeaveError.h>
#include "WeaveTLVTags.h"
#include "WeaveTLVTypes.h"
#include "WeaveTLV.h"

namespace nl {
namespace Weave {
namespace TLV {

/**
 * @class WeaveCircularTLVBuffer
 *
 * @brief
 *    WeaveCircularTLVBuffer provides circular storage for the
 *    nl::Weave::TLV::TLVWriter and nl::Weave::TLVTLVReader.  nl::Weave::TLV::TLVWriter is able to write an
 *    unbounded number of TLV entries to the WeaveCircularTLVBuffer
 *    as long as each individual TLV entry fits entirely within the
 *    provided storage.  The nl::Weave::TLV::TLVReader will read at most the size of
 *    the buffer, but will accommodate the wraparound within the
 *    buffer.
 *
 */
class NL_DLL_EXPORT WeaveCircularTLVBuffer
{
public:
    WeaveCircularTLVBuffer(uint8_t *inBuffer, size_t inBufferLength);
    WeaveCircularTLVBuffer(uint8_t *inBuffer, size_t inBufferLength, uint8_t *inHead);

    WEAVE_ERROR GetNewBuffer(TLVWriter& ioWriter, uint8_t *& outBufStart, uint32_t& outBufLen);
    WEAVE_ERROR FinalizeBuffer(TLVWriter& ioWriter, uint8_t *inBufStart, uint32_t inBufLen);
    WEAVE_ERROR GetNextBuffer(TLVReader& ioReader, const uint8_t *& outBufStart, uint32_t& outBufLen);

    inline uint8_t *QueueHead(void) const { return mQueueHead; };
    inline uint8_t *QueueTail(void) const { return mQueue + (((mQueueHead-mQueue) + mQueueLength) % mQueueSize); };
    inline size_t DataLength(void) const { return mQueueLength; };
    inline size_t AvailableDataLength(void) const { return mQueueSize - mQueueLength; };
    inline size_t GetQueueSize(void) const { return mQueueSize; };
    inline uint8_t *GetQueue(void) const { return mQueue; };

    WEAVE_ERROR EvictHead(void);

    static WEAVE_ERROR GetNewBufferFunct(TLVWriter& ioWriter, uintptr_t& inBufHandle, uint8_t *& outBufStart, uint32_t& outBufLen);
    static WEAVE_ERROR FinalizeBufferFunct(TLVWriter& ioWriter, uintptr_t inBufHandle, uint8_t *inBufStart, uint32_t inBufLen);
    static WEAVE_ERROR GetNextBufferFunct(TLVReader& ioReader, uintptr_t &inBufHandle, const uint8_t *& outBufStart, uint32_t& outBufLen);

    /**
     *  @typedef WEAVE_ERROR (*ProcessEvictedElementFunct)(WeaveCircularTLVBuffer &inBuffer, void * inAppData, TLVReader &inReader)
     *
     *  A function that is called to process a TLV element prior to it
     *  being evicted from the nl::Weave::TLV::WeaveCircularTLVBuffer
     *
     *  Functions of this type are used to process a TLV element about
     *  to be evicted from the buffer.  The function will be given a
     *  nl::Weave::TLV::TLVReader positioned on the element about to be deleted, as
     *  well as void * context where the user may have provided
     *  additional environment for the callback.  If the function
     *  processed the element successfully, it must return
     *  #WEAVE_NO_ERROR ; this signifies to the WeaveCircularTLVBuffer
     *  that the element may be safely evicted.  Any other return
     *  value is treated as an error and will prevent the
     *  #WeaveCircularTLVBuffer from evicting the element under
     *  consideration.
     *
     *  Note: This callback may be used to force
     *  WeaveCircularTLVBuffer to not evict the element.  This may be
     *  useful in a number of circumstances, when it is desired to
     *  have an underlying circular buffer, but not to override any
     *  elements within it.
     *
     *  @param[in] inBuffer  A reference to the buffer from which the
     *                       eviction takes place
     *
     *  @param[in] inAppData A pointer to the user-provided structure
     *                       containing additional context for this
     *                       callback
     *
     *  @param[in] inReader  A TLVReader positioned at the element to
     *                       be evicted.
     *
     *  @retval #WEAVE_NO_ERROR On success. Element will be evicted.
     *
     *  @retval other        An error has occurred during the event
     *                       processing.  The element stays in the
     *                       buffer.  The write function that
     *                       triggered this element eviction will
     *                       fail.
     */
    typedef WEAVE_ERROR (*ProcessEvictedElementFunct)(WeaveCircularTLVBuffer &inBuffer, void * inAppData, TLVReader &inReader);

    uint32_t mImplicitProfileId;
    void * mAppData; /**< An optional, user supplied context to be used with the callback processing the evicted element. */
    ProcessEvictedElementFunct mProcessEvictedElement; /**< An optional, user-supplied callback that processes the element prior to evicting it from the circular buffer.  See the ProcessEvictedElementFunct type definition on additional information on implementing the mProcessEvictedElement function. */

private:
    uint8_t *mQueue;
    size_t mQueueSize;
    uint8_t *mQueueHead;
    size_t mQueueLength;
};

class NL_DLL_EXPORT CircularTLVReader : public TLVReader
{
public:
    void Init(WeaveCircularTLVBuffer *buf);
};

class NL_DLL_EXPORT CircularTLVWriter : public TLVWriter
{
public:
    void Init(WeaveCircularTLVBuffer *buf);
};

} // namespace TLV
} // namespace Weave
} // namespace nl

#endif //WEAVE_CIRCULAR_TLV_BUFFER_H_
