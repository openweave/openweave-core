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
 *      This file implements memory management functions for the Weave Security Manager.
 *      This implementation, known as the "Simple Allocator", is based on temporary
 *      network buffer allocation/release. This implementation is used when
 *      #WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_SIMPLE is enabled (1).
 *
 *      The simple allocator was designed for highly memory-constrained embedded systems.
 *      This allocator assumes that the number of memory blocks, their sizes and
 *      the order in which they are requested/released by the Weave Security Manager is known
 *      and doesn't change. If memory blocks sizes or their allocation order changes this
 *      implementation should be reviewed and adjusted accordingly.
 *
 *      Below is the list of memory objects that Simple Allocator was designed to work with:
 *
 *      |      Object           |    Type    |   Size    |    How to determine the size     |
 *      | :-------------------- | :--------- | :-------- | :------------------------------- |
 *      | CASE Engine           | Long Term  | 72  bytes | sizeof(WeaveCASEEngine)          |
 *      | PASE Engine           | Long Term  | 436 bytes | sizeof(WeavePASEEngine)          |
 *      | Private Key Buffer    | Short Term | 300 bytes | kMaxDevicePrivateKeySize         |
 *      | Cert. Data Buffer     | Short Term | 512 bytes | sizeof(WeaveCertificateData) * 4 |
 *      | Service Config Buffer | Short Term | 600 bytes | kMaxServiceConfigSize            |
 *      | Cert. Decode Buffer   | Short Term | 600 bytes | kCertDecodeBufferSize            |
 *
 *      Below diagram presents how Simple Allocator uses two network buffers to allocate
 *      memory blocks requested by Weave Security Manager:
 *
 *      | Inet Buffer #0                                        |
 *      | :---------------------------------------------------: |
 *      | CASE Engine                            \n (128 bytes) |
 *      | PASE Engine                            \n (512 bytes) |
 *      | Private Key Buffer / Cert. Data Buffer \n (512 bytes) |
 *
 *      | Inet Buffer #1                                        |
 *      | :---------------------------------------------------: |
 *      | Service Config Buffer                  \n (640 bytes) |
 *      | Cert. Decode Buffer                    \n (640 bytes) |
 *
 *      When use of small network buffers option is enabled
 *      (#WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS is set) minimum 4 buffers are needed
 *      and the diagram below presents how these buffers are used:
 *
 *      | Inet Buffer #0                                        |
 *      | :---------------------------------------------------: |
 *      | CASE Engine                            \n (128 bytes) |
 *      | PASE Engine                            \n (512 bytes) |
 *
 *      | Inet Buffer #1                                        |
 *      | :---------------------------------------------------: |
 *      | Private Key Buffer / Cert. Data Buffer \n (512 bytes) |
 *
 *      | Inet Buffer #2                                        |
 *      | :---------------------------------------------------: |
 *      | Service Config Buffer                  \n (640 bytes) |
 *
 *      | Inet Buffer #3                                        |
 *      | :---------------------------------------------------: |
 *      | Cert. Decode Buffer                    \n (640 bytes) |
 *
 *    @note All long term memory blocks are in the Inet Buffer #0, which guarantees
 *      that only one network buffer is allocated for a long period of PASE/CASE
 *      session duration. Also Private key Buffer and Cert. Data Buffer share the same memory
 *      block and it is guaranteed by design that their usage cannot overlap.
 *
 *      Where available, the library can also consume dedicated memory buffer during memory initialization
 *      with MemoryInit() function. In this case the network buffers allocation scheme
 *      described above is logically concatenated into single buffer allocation scheme
 *      and the memory blocks are allocated from dedicated buffer in a follow manner:
 *
 *      | Dedicated Buffer                                      |
 *      | :---------------------------------------------------: |
 *      | CASE Engine                            \n (128 bytes) |
 *      | PASE Engine                            \n (512 bytes) |
 *      | Private Key Buffer / Cert. Data Buffer \n (512 bytes) |
 *      | Service Config Buffer                  \n (640 bytes) |
 *      | Cert. Decode Buffer                    \n (640 bytes) |
 *
 *      The Simple Allocator design is highly parametrized and the number of
 *      buffers, number and sizes of memory blocks can be changed according to new
 *      requirements.
 *
 */

#include "WeaveConfig.h"
#include "WeaveSecurityMgr.h"

#if WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_SIMPLE

namespace nl {
namespace Weave {
namespace Platform {
namespace Security {

/**
 * This type is used to encode block set information. A block set is a group of memory
 * chunks that have same properties (size, type, and belong to the same buffer).
 * Diagram below presents how block set information encoded in an 8-bit format:
 *
 * | Is Long Term | Block Index | Number of Blocks | Block Size |
 * | :----------: | :---------: | :--------------: | :--------: |
 * |     [7]      |   [6:5]     |      [4:3]       |   [2:0]    |
 *
 * [2:0] - Size of memory block in a set. Size value is encoded with 128-byte granularity.
 * | [2:0] | Decoding  |
 * |-------|-----------|
 * |  000  | 0 bytes   |
 * |  001  | 128 bytes |
 * |  010  | 256 bytes |
 * |  ...  | -         |
 * |  111  | 896 bytes |
 *
 * [4:3] - Number of blocks in a set.
 * | [4:3] | Decoding |
 * |-------|----------|
 * |  00   | 4 blocks |
 * |  01   | 1 block  |
 * |  10   | 2 blocks |
 * |  11   | 3 blocks |
 *
 * [6:5] - Network buffer index where memory blocks reside. Note that when dedicated buffer
 *         is used all memory blocks are allocated from dedicated buffer and this index
 *         parameter is ignored.
 * | [6:5] | Decoding          |
 * |-------|-------------------|
 * |  00   | Network buffer #0 |
 * |  ...  | -                 |
 * |  11   | Network buffer #3 |
 *
 * [7] - Specifies if blocks are for long/short term storage.
 * |  [7]  | Decoding                 |
 * |-------|--------------------------|
 * |   0   | Short term memory blocks |
 * |   1   | Long term memory blocks  |
 *
 */
typedef uint8_t BlockSetParams_t;

/**
 * Block set parameter fields location in an 8-bit encoded form.
 */
enum BlockSetParamFields
{
    kBlockSetSizeMask                     = 0x07,    /**< Size field mask. */
    kBlockSetSizeShift                    = 0,       /**< Size field shift. */
    kBlockSetSizeValueShift               = 7,       /**< Size field decoding shift (multiply by 128). */
    kBlockSetCountMask                    = 0x18,    /**< Count field mask. */
    kBlockSetCountShift                   = 3,       /**< Count field shift. */
    kBlockSetBufferIndexMask              = 0x60,    /**< Buffer index field mask. */
    kBlockSetBufferIndexShift             = 5,       /**< Buffer index field shift. */
    kBlockSetIsLongTermMask               = 0x80,    /**< Type field mask. */
    kBlockSetIsLongTermShift              = 7,       /**< Type field shift. */
};

/**
 * Defines block set parameters.
 */
#if WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS
enum BlockSetParams
{
    // Simple Allocator parameters
    kNumberOfNetworkBuffers               = 4,       /**< Number of network buffers used by Simple Allocator. */
    kNumberOfBlockSets                    = 5,       /**< Number of block sets used by Simple Allocator. */
    kNetworkBuffer0BlockAllocationMask    = 0x03,    /**< This mask identifies all memory blocks allocated from network buffer #0. */
    kNetworkBuffer1BlockAllocationMask    = 0x04,    /**< This mask identifies all memory blocks allocated from network buffer #1. */
    kNetworkBuffer2BlockAllocationMask    = 0x08,    /**< This mask identifies all memory blocks allocated from network buffer #2. */
    kNetworkBuffer3BlockAllocationMask    = 0x10,    /**< This mask identifies all memory blocks allocated from network buffer #3. */
    // Block Set #1 (1 long-term blocks of 128 bytes)
    kBlockSet1Size                        = 1,       /**< Set #1 size is 128 bytes. */
    kBlockSet1Count                       = 1,       /**< Set #1 block count is 1. */
    kBlockSet1BufferIndex                 = 0,       /**< Set #1 buffer index is 0. */
    kBlockSet1isLongTerm                  = 1,       /**< Set #1 type is long term. */
    // Block Set #2 (1 long-term block of 512 bytes)
    kBlockSet2Size                        = 4,       /**< Set #2 size is 512 bytes. */
    kBlockSet2Count                       = 1,       /**< Set #2 block count is 1. */
    kBlockSet2BufferIndex                 = 0,       /**< Set #2 buffer index is 0. */
    kBlockSet2isLongTerm                  = 1,       /**< Set #2 type is long term. */
    // Block Set #3 (1 short-term block of 512 bytes)
    kBlockSet3Size                        = 4,       /**< Set #3 size is 512 bytes. */
    kBlockSet3Count                       = 1,       /**< Set #3 block count is 1. */
    kBlockSet3BufferIndex                 = 1,       /**< Set #3 buffer index is 1. */
    kBlockSet3isLongTerm                  = 0,       /**< Set #3 type is short term. */
    // Block Set #4 (1 short-term blocks of 640 bytes)
    kBlockSet4Size                        = 5,       /**< Set #4 size is 640 bytes. */
    kBlockSet4Count                       = 1,       /**< Set #4 block count is 1. */
    kBlockSet4BufferIndex                 = 2,       /**< Set #4 buffer index is 2. */
    kBlockSet4isLongTerm                  = 0,       /**< Set #4 type is short term. */
    // Block Set #5 (1 short-term blocks of 640 bytes)
    kBlockSet5Size                        = 5,       /**< Set #5 size is 640 bytes. */
    kBlockSet5Count                       = 1,       /**< Set #5 block count is 1. */
    kBlockSet5BufferIndex                 = 3,       /**< Set #5 buffer index is 3. */
    kBlockSet5isLongTerm                  = 0,       /**< Set #5 type is short term. */
    /** Total memory used by Simple Allocator. */
    kTotalMemorySize                      = (kBlockSet1Size * kBlockSet1Count +
                                             kBlockSet2Size * kBlockSet2Count +
                                             kBlockSet3Size * kBlockSet3Count +
                                             kBlockSet4Size * kBlockSet4Count +
                                             kBlockSet5Size * kBlockSet5Count) << kBlockSetSizeValueShift,
    /** Maximum block size supported by Simple Allocator. */
    kMaxBlockSize                         = 600,
    /** Minimum network buffer size required to support Simple Allocator use cases. */
    kMinBufferSize                        = 600,
};
#else // WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS
enum BlockSetParams
{
    // Simple Allocator parameters
    kNumberOfNetworkBuffers               = 2,       /**< Number of network buffers used by Simple Allocator. */
    kNumberOfBlockSets                    = 4,       /**< Number of block sets used by Simple Allocator. */
    kNetworkBuffer0BlockAllocationMask    = 0x07,    /**< This mask identifies all memory blocks allocated from network buffer #0. */
    kNetworkBuffer1BlockAllocationMask    = 0x18,    /**< This mask identifies all memory blocks allocated from network buffer #1. */
    // Block Set #1 (1 long-term blocks of 128 bytes)
    kBlockSet1Size                        = 1,       /**< Set #1 size is 128 bytes. */
    kBlockSet1Count                       = 1,       /**< Set #1 block count is 1. */
    kBlockSet1BufferIndex                 = 0,       /**< Set #1 buffer index is 0. */
    kBlockSet1isLongTerm                  = 1,       /**< Set #1 type is long term. */
    // Block Set #2 (1 long-term block of 512 bytes)
    kBlockSet2Size                        = 4,       /**< Set #2 size is 512 bytes. */
    kBlockSet2Count                       = 1,       /**< Set #2 block count is 1. */
    kBlockSet2BufferIndex                 = 0,       /**< Set #2 buffer index is 0. */
    kBlockSet2isLongTerm                  = 1,       /**< Set #2 type is long term. */
    // Block Set #3 (1 short-term block of 512 bytes)
    kBlockSet3Size                        = 4,       /**< Set #3 size is 512 bytes. */
    kBlockSet3Count                       = 1,       /**< Set #3 block count is 1. */
    kBlockSet3BufferIndex                 = 0,       /**< Set #3 buffer index is 0. */
    kBlockSet3isLongTerm                  = 0,       /**< Set #3 type is short term. */
    // Block Set #4 (2 short-term blocks of 640 bytes)
    kBlockSet4Size                        = 5,       /**< Set #4 size is 640 bytes. */
    kBlockSet4Count                       = 2,       /**< Set #4 block count is 2. */
    kBlockSet4BufferIndex                 = 1,       /**< Set #4 buffer index is 1. */
    kBlockSet4isLongTerm                  = 0,       /**< Set #4 type is short term. */
    /** Total memory used by Simple Allocator. */
    kTotalMemorySize                      = (kBlockSet1Size * kBlockSet1Count +
                                             kBlockSet2Size * kBlockSet2Count +
                                             kBlockSet3Size * kBlockSet3Count +
                                             kBlockSet4Size * kBlockSet4Count) << kBlockSetSizeValueShift,
    /** Maximum block size supported by Simple Allocator. */
    kMaxBlockSize                         = 600,
    /** Minimum network buffer size required to support Simple Allocator use cases. This parameter
     *  is derived from sizes of two memory block (640 + 600) allocated in the network buffer #1. */
    kMinBufferSize                        = 1240,
};
#endif // WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS

/**
 * Encoded block sets parameters.
 */
static const BlockSetParams_t sBlockSetParams[kNumberOfBlockSets] = {
    // Parameters for memory block set #1
    ((kBlockSet1Size << kBlockSetSizeShift) & kBlockSetSizeMask) | \
    ((kBlockSet1Count << kBlockSetCountShift) & kBlockSetCountMask) | \
    ((kBlockSet1BufferIndex << kBlockSetBufferIndexShift) & kBlockSetBufferIndexMask) | \
    ((kBlockSet1isLongTerm << kBlockSetIsLongTermShift) & kBlockSetIsLongTermMask),
    // Parameters for memory block set #2
    ((kBlockSet2Size << kBlockSetSizeShift) & kBlockSetSizeMask) | \
    ((kBlockSet2Count << kBlockSetCountShift) & kBlockSetCountMask) | \
    ((kBlockSet2BufferIndex << kBlockSetBufferIndexShift) & kBlockSetBufferIndexMask) | \
    ((kBlockSet2isLongTerm << kBlockSetIsLongTermShift) & kBlockSetIsLongTermMask),
    // Parameters for memory block set #3
    ((kBlockSet3Size << kBlockSetSizeShift) & kBlockSetSizeMask) | \
    ((kBlockSet3Count << kBlockSetCountShift) & kBlockSetCountMask) | \
    ((kBlockSet3BufferIndex << kBlockSetBufferIndexShift) & kBlockSetBufferIndexMask) | \
    ((kBlockSet3isLongTerm << kBlockSetIsLongTermShift) & kBlockSetIsLongTermMask),
    // Parameters for memory block set #4
    ((kBlockSet4Size << kBlockSetSizeShift) & kBlockSetSizeMask) | \
    ((kBlockSet4Count << kBlockSetCountShift) & kBlockSetCountMask) | \
    ((kBlockSet4BufferIndex << kBlockSetBufferIndexShift) & kBlockSetBufferIndexMask) | \
    ((kBlockSet4isLongTerm << kBlockSetIsLongTermShift) & kBlockSetIsLongTermMask),
#if WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS
    // Parameters for memory block set #5
    ((kBlockSet5Size << kBlockSetSizeShift) & kBlockSetSizeMask) | \
    ((kBlockSet5Count << kBlockSetCountShift) & kBlockSetCountMask) | \
    ((kBlockSet5BufferIndex << kBlockSetBufferIndexShift) & kBlockSetBufferIndexMask) | \
    ((kBlockSet5isLongTerm << kBlockSetIsLongTermShift) & kBlockSetIsLongTermMask),
#endif
};

/**
 * The type used to mark which block is currently allocated/unallocated.
 *   uint8_t  - supports upto 8 memory blocks total
 *   uint16_t - supports upto 16 memory blocks total
 *   uint32_t - supports upto 32 memory blocks total
 *
 */
typedef uint8_t BlockMark_t;

/**
 * Indicates which block is allocated/unallocated.
 * Initialized to 0 - means that all memory blocks and unallocated.
 *
 */
static BlockMark_t sMemBlocksAllocated = 0;

/**
 * Set of buffer block masks.
 * Identifies if all blocks in the buffer and unallocated and then the buffer can be relieazed.
 *
 */
static const BlockMark_t sBufferAllocationMask[kNumberOfNetworkBuffers] = {
                                                   kNetworkBuffer0BlockAllocationMask,
                                                   kNetworkBuffer1BlockAllocationMask,
#if WEAVE_CONFIG_SIMPLE_ALLOCATOR_USE_SMALL_BUFFERS
                                                   kNetworkBuffer2BlockAllocationMask,
                                                   kNetworkBuffer3BlockAllocationMask,
#endif
                                                                                       };

/**
 * A boolean indicating whether (true) or not (false) the network buffers are used by Simple Allocator.
 * When false - dedicated buffer provided with MemoryInit() function is used.
 *
 */
static bool sNetworkBuffersUsed = true;

/**
 * Pointers to memory buffers initialized to NULL.
 * When network buffers are used sMemBufs[] point to the PacketBuffer objects.
 * When dedicated buffer is used sMemBufs[0] points to that buffer and other sMemBufs[] pointers are ignored.
 *
 */
static void *sMemBufs[kNumberOfNetworkBuffers] = { NULL };


static void DecodeBlockSetParams(BlockSetParams_t blockSetParams, uint16_t& blockSize, uint8_t& blockCount, uint8_t& blockBufferIndex, bool& blockIsLongTerm)
{
    blockSize = ((blockSetParams & kBlockSetSizeMask) >> kBlockSetSizeShift) << kBlockSetSizeValueShift;
    blockCount = (blockSetParams & kBlockSetCountMask) >> kBlockSetCountShift;
    if (sNetworkBuffersUsed)
        blockBufferIndex = (blockSetParams & kBlockSetBufferIndexMask) >> kBlockSetBufferIndexShift;
    else
        blockBufferIndex = 0;
    blockIsLongTerm = (blockSetParams & kBlockSetIsLongTermMask) == kBlockSetIsLongTermMask;
}


WEAVE_ERROR MemoryInit(void *buf, size_t bufSize)
{
    if (buf != NULL)
    {
        // align on 4-byte boundary
        const uint8_t bufByteOffset = ((unsigned long) buf) % 4;

        if (bufByteOffset != 0)
        {
            const uint8_t bufBytePadding = 4 - bufByteOffset;

            buf = (void *)(((uint8_t *)buf) + bufBytePadding);
            bufSize -= bufBytePadding;
        }

        if (bufSize < kTotalMemorySize)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;

        sMemBufs[0] = buf;
        sNetworkBuffersUsed = false;
    }

    return WEAVE_NO_ERROR;
}

void MemoryShutdown()
{
    if (sNetworkBuffersUsed)
    {
        for (uint8_t i = 0; i < kNumberOfNetworkBuffers; i++)
            if (sMemBufs[i] != NULL)
            {
                PacketBuffer::Free((PacketBuffer *)sMemBufs[i]);
                sMemBufs[i] = NULL;
            }
    }
    else
    {
        sMemBufs[0] = NULL;
    }

    sNetworkBuffersUsed = true;
    sMemBlocksAllocated = 0;
}

void *MemoryAlloc(size_t size)
{
    return MemoryAlloc(size, false);
}

void *MemoryAlloc(size_t size, bool isLongTermAlloc)
{
    BlockMark_t blockIndex = 0x01;
    uint16_t bufferOffset[kNumberOfNetworkBuffers] = { 0 };
    uint16_t blockSize;
    uint8_t blockCount;
    uint8_t blockBufferIndex;
    bool blockIsLongTerm;
    uint8_t *memBufStart;

    if (size == 0 || size > kMaxBlockSize)
        return NULL;

    for (uint8_t i = 0; i < kNumberOfBlockSets; i++)
    {
        DecodeBlockSetParams(sBlockSetParams[i], blockSize, blockCount, blockBufferIndex, blockIsLongTerm);

        if (size <= blockSize && (isLongTermAlloc || !blockIsLongTerm))
        {
            for (uint8_t j = 0; j < blockCount; j++)
            {
                if (!(blockIndex & sMemBlocksAllocated))
                {
                    if (sNetworkBuffersUsed)
                    {
                        if (sMemBufs[blockBufferIndex] == NULL)
                        {
                            sMemBufs[blockBufferIndex] = PacketBuffer::NewWithAvailableSize(0, kMinBufferSize);
                            if (sMemBufs[blockBufferIndex] == NULL)
                                return NULL;
                        }
                        memBufStart = ((PacketBuffer *)sMemBufs[blockBufferIndex])->Start();
                    }
                    else
                    {
                        memBufStart = (uint8_t *)sMemBufs[blockBufferIndex];
                    }

                    // Mark memory block as allocated and return pointer to this block
                    sMemBlocksAllocated |= blockIndex;
                    return (memBufStart + bufferOffset[blockBufferIndex]);
                }
                else
                {
                    blockIndex <<= 1;
                    bufferOffset[blockBufferIndex] += blockSize;
                }
            }
        }
        else
        {
            blockIndex <<= blockCount;
            bufferOffset[blockBufferIndex] += blockSize * blockCount;
        }
    }

    return NULL;
}

void MemoryFree(void *p)
{
    BlockMark_t blockIndex = 0x01;
    uint16_t bufferOffset[kNumberOfNetworkBuffers] = { 0 };
    uint16_t blockSize;
    uint8_t blockCount;
    uint8_t blockBufferIndex;
    bool blockIsLongTerm;
    uint8_t *memBufStart;
    uint8_t i;

    for (i = 0; i < kNumberOfBlockSets; i++)
    {
        DecodeBlockSetParams(sBlockSetParams[i], blockSize, blockCount, blockBufferIndex, blockIsLongTerm);

        if (sMemBufs[blockBufferIndex] != NULL)
        {
            if (sNetworkBuffersUsed)
                memBufStart = ((PacketBuffer *)sMemBufs[blockBufferIndex])->Start();
            else
                memBufStart = (uint8_t *)sMemBufs[blockBufferIndex];

            if (p >= memBufStart + bufferOffset[blockBufferIndex] &&
                p <  memBufStart + bufferOffset[blockBufferIndex] + blockSize * blockCount)
            {
                for (uint8_t j = 0; j < blockCount; j++)
                {
                    if (p == memBufStart + bufferOffset[blockBufferIndex])
                    {
                        // Mark memory block as unallocated
                        sMemBlocksAllocated &= ~blockIndex;
                        // Release network buffer if all it's memory blocks are unallocated
                        if (sNetworkBuffersUsed && !(sMemBlocksAllocated & sBufferAllocationMask[blockBufferIndex]))
                        {
                            PacketBuffer::Free((PacketBuffer *)sMemBufs[blockBufferIndex]);
                            sMemBufs[blockBufferIndex] = NULL;
                        }
                        return;
                    }
                    else
                    {
                        blockIndex <<= 1;
                        bufferOffset[blockBufferIndex] += blockSize;
                    }
                }
            }
        }
        blockIndex <<= blockCount;
        bufferOffset[blockBufferIndex] += blockSize * blockCount;
    }
}

} // namespace Security
} // namespace Platform
} // namespace Weave
} // namespace nl

#endif // WEAVE_CONFIG_SECURITY_MGR_MEMORY_MGMT_SIMPLE
