/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file implements a unit test suite for the Weave
 *      RetainedPacketBuffer object.
 *
 */

#include <SystemLayer/SystemConfig.h>
#include <SystemLayer/SystemStats.h>

#include <Weave/Profiles/common/WeaveMessage.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/tcpip.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#include <nltest.h>

using namespace nl::Weave;

static void PacketBufferAlloc(System::PacketBuffer *&aBuffer)
{
    aBuffer = System::PacketBuffer::New();
}

static void PacketBufferFree(System::PacketBuffer *&aBuffer)
{
    if (aBuffer != NULL)
    {
        System::PacketBuffer::Free(aBuffer);
        aBuffer = NULL;
    }
}

/**
 *  Test default construction and, implictly, destruction
 *
 */
static void CheckDefaultConstruction(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theExtent;
}

/**
 *  Test the GetBuffer accessor method
 *
 */
static void CheckGetBufferAccessor(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer;
    System::PacketBuffer *theBuffer;

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);
}

/**
 *  Test the IsRetaining accessor method.
 *
 */
static void CheckIsRetainingAccessor(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer;
    System::PacketBuffer *theBuffer;
    bool retaining;

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);
}

/**
 *  Test the copy constructor absent retaining a buffer.
 *
 */
static void CheckCopyConstructionWithoutRetainedBuffer(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    Profiles::RetainedPacketBuffer theRetainedBuffer_2(theRetainedBuffer_1);
    System::PacketBuffer *theBuffer;
    bool retaining;

    theBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theBuffer = theRetainedBuffer_2.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_2.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);
}

/**
 *  Test the assignment operator absent retaining a buffer.
 *
 */
void CheckAssignmentWithoutRetainedBuffer(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    Profiles::RetainedPacketBuffer theRetainedBuffer_2;
    System::PacketBuffer *theBuffer;
    bool retaining;

    theBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theBuffer = theRetainedBuffer_2.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_2.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theRetainedBuffer_2 = theRetainedBuffer_1;

    theBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theBuffer = theRetainedBuffer_2.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer_2.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);
}

/**
 *  Test the Release method absent a retained buffer
 *
 */
void CheckReleaseWithoutRetainedBuffer(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer;
    System::PacketBuffer *theBuffer;
    bool retaining;

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theRetainedBuffer.Release();

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);
}

/**
 *  Test the Retain method with a NULL pointer
 *
 */
void CheckRetainWithNullPointer(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer;
    System::PacketBuffer *theBuffer;
    bool retaining;

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theRetainedBuffer.Retain(NULL);

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    theRetainedBuffer.Release();

    theBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, theBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);
}

/**
 *  Test the Retain method with an allocated buffer and an implicit
 *  release via object destruction
 *
 */
static void CheckRetainAllocatedBufferWithImplicitRelease(nlTestSuite *inSuite, void *inContext)
{
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer;

        accessedBuffer = theRetainedBuffer.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // Allocate a new system packet buffer with reference count of one (1).

        PacketBufferAlloc(allocatedBuffer);
        NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

        theRetainedBuffer.Retain(allocatedBuffer);

        accessedBuffer = theRetainedBuffer.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the retained packet buffer object holds
        // another reference, leaving the reference count at two (2).
    }

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test the Retain method with an allocated buffer and an explicit
 *  release via the Release method
 *
 */
static void CheckRetainAllocatedBufferWithExplicitRelease(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer;
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    accessedBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // Allocate a new system packet buffer with reference count of one (1).

    PacketBufferAlloc(allocatedBuffer);
    NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

    theRetainedBuffer.Retain(allocatedBuffer);

    accessedBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, the retained packet buffer object holds
    // another reference, leaving the reference count at two (2).

    // Explicitly release the associated buffer.

    theRetainedBuffer.Release();

    accessedBuffer = theRetainedBuffer.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // At this point, the explicit release should have dereferenced
    // the buffer, leaving the buffer reference count at one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test copy construction with non-NULL buffer and implicitly release the target
 *  first via destruction and then implicitly the source second via
 *  destruction
 *
 */
static void CheckCopyConstructionWithAllocatedBufferImplicitTargetImplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // Allocate a new system packet buffer with reference count of one (1).

        PacketBufferAlloc(allocatedBuffer);
        NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

        theRetainedBuffer_1.Retain(allocatedBuffer);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the first retained packet buffer object holds
        // another reference, leaving the reference count at two (2).

        // Add a scope nesting level to get implicit allocated buffer
        // dereference via object destruction when it goes out of scope.

        {
            Profiles::RetainedPacketBuffer theRetainedBuffer_2(theRetainedBuffer_1);

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            // At this point, the second retained packet buffer object holds
            // another reference, leaving the reference count at three (3).
        }

        // theRetainedBuffer_2 is now destroyed. theRetainedBuffer_1
        // should still be retained.

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, object destruction should have implicitly
        // dereferenced the buffer, leaving the buffer reference count at
        // two (2).
    }

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test copy construction with non-NULL buffer and implicitly release the target
 *  first via destruction and then explicitly the source second.
 *
 */
static void CheckCopyConstructionWithAllocatedBufferImplicitTargetExplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // Allocate a new system packet buffer with reference count of one (1).

    PacketBufferAlloc(allocatedBuffer);
    NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

    theRetainedBuffer_1.Retain(allocatedBuffer);

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, the first retained packet buffer object holds
    // another reference, leaving the reference count at two (2).

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_2(theRetainedBuffer_1);

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the second retained packet buffer object holds
        // another reference, leaving the reference count at three (3).
    }

    // theRetainedBuffer_2 is now destroyed. theRetainedBuffer_1
    // should still be retained.

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // two (2).

    // Explicitly release the associated buffer.

    theRetainedBuffer_1.Release();

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // At this point, the explicit release should have dereferenced
    // the buffer, leaving the buffer reference count at one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test copy construction with non-NULL buffer and explicitly release the target
 *  first and then implicitly the source second via destruction
 *
 */
static void CheckCopyConstructionWithAllocatedBufferExplicitTargetImplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // Allocate a new system packet buffer with reference count of one (1).

        PacketBufferAlloc(allocatedBuffer);
        NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

        theRetainedBuffer_1.Retain(allocatedBuffer);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the first retained packet buffer object holds
        // another reference, leaving the reference count at two (2).

        // Add a scope nesting level to get implicit allocated buffer
        // dereference via object destruction when it goes out of scope.

        {
            Profiles::RetainedPacketBuffer theRetainedBuffer_2(theRetainedBuffer_1);

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            // At this point, the second retained packet buffer object holds
            // another reference, leaving the reference count at three (3).

            // Explicitly release the associated buffer.

            theRetainedBuffer_2.Release();

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, !retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);
        }


        // theRetainedBuffer_2 is now both released and
        // destroyed. theRetainedBuffer_1 should still be retained.

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the explicit release should have dereferenced
        // the buffer, leaving the buffer reference count at two (2).
    }

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test copy construction with non-NULL buffer and explicitly release the target
 *  first and then explicitly the source second
 *
 */
static void CheckCopyConstructionWithAllocatedBufferExplicitTargetExplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // Allocate a new system packet buffer with reference count of one (1).

    PacketBufferAlloc(allocatedBuffer);
    NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

    theRetainedBuffer_1.Retain(allocatedBuffer);

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, the first retained packet buffer object holds
    // another reference, leaving the reference count at two (2).

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_2(theRetainedBuffer_1);

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the second retained packet buffer object holds
        // another reference, leaving the reference count at three (3).

        // Explicitly release the associated buffer.

        theRetainedBuffer_2.Release();

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // At this point, the explicit release should have dereferenced
        // the buffer, leaving the buffer reference count at two (2).

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);
    }

    // theRetainedBuffer_2 is now both released and
    // destroyed. theRetainedBuffer_1 should still be retained.

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // Explicitly release the associated buffer.

    theRetainedBuffer_1.Release();

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // At this point, the explicit release should have dereferenced
    // the buffer, leaving the buffer reference count at one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test assignment with non-NULL buffer and implicitly release the target
 *  first via destruction and then implicitly the source second via
 *  destruction
 *
 */
static void CheckAssignmentWithAllocatedBufferImplicitTargetImplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // Allocate a new system packet buffer with reference count of one (1).

        PacketBufferAlloc(allocatedBuffer);
        NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

        theRetainedBuffer_1.Retain(allocatedBuffer);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the first retained packet buffer object holds
        // another reference, leaving the reference count at two (2).

        // Add a scope nesting level to get implicit allocated buffer
        // dereference via object destruction when it goes out of scope.

        {
            Profiles::RetainedPacketBuffer theRetainedBuffer_2;

            theRetainedBuffer_2 = theRetainedBuffer_1;

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            // At this point, the second retained packet buffer object holds
            // another reference, leaving the reference count at three (3).
        }

        // theRetainedBuffer_2 is now destroyed. theRetainedBuffer_1
        // should still be retained.

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, object destruction should have implicitly
        // dereferenced the buffer, leaving the buffer reference count at
        // two (2).
    }

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test assignment with non-NULL buffer and implicitly release the target
 *  first via destruction and then explicitly the source second.
 *
 */
static void CheckAssignmentWithAllocatedBufferImplicitTargetExplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // Allocate a new system packet buffer with reference count of one (1).

    PacketBufferAlloc(allocatedBuffer);
    NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

    theRetainedBuffer_1.Retain(allocatedBuffer);

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, the first retained packet buffer object holds
    // another reference, leaving the reference count at two (2).

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_2;

        theRetainedBuffer_2 = theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the second retained packet buffer object holds
        // another reference, leaving the reference count at three (3).
    }

    // theRetainedBuffer_2 is now destroyed. theRetainedBuffer_1
    // should still be retained.

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // two (2).

    // Explicitly release the associated buffer.

    theRetainedBuffer_1.Release();

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // At this point, the explicit release should have dereferenced
    // the buffer, leaving the buffer reference count at one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test assignment with non-NULL buffer and explicitly release the target
 *  first and then implicitly the source second via destruction
 *
 */
static void CheckAssignmentWithAllocatedBufferExplicitTargetImplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    // Add a scope nesting level to get implicit allocated buffer
    // dereference via object destruction when it goes out of scope.

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // Allocate a new system packet buffer with reference count of one (1).

        PacketBufferAlloc(allocatedBuffer);
        NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

        theRetainedBuffer_1.Retain(allocatedBuffer);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the first retained packet buffer object holds
        // another reference, leaving the reference count at two (2).

        // Add a scope nesting level to get implicit allocated buffer
        // dereference via object destruction when it goes out of scope.

        {
            Profiles::RetainedPacketBuffer theRetainedBuffer_2;

            theRetainedBuffer_2 = theRetainedBuffer_1;

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);

            // At this point, the second retained packet buffer object holds
            // another reference, leaving the reference count at three (3).

            // Explicitly release the associated buffer.

            theRetainedBuffer_2.Release();

            accessedBuffer = theRetainedBuffer_2.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

            retaining = theRetainedBuffer_2.IsRetaining();
            NL_TEST_ASSERT(inSuite, !retaining);

            accessedBuffer = theRetainedBuffer_1.GetBuffer();
            NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
            NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

            retaining = theRetainedBuffer_1.IsRetaining();
            NL_TEST_ASSERT(inSuite, retaining);
        }


        // theRetainedBuffer_2 is now both released and
        // destroyed. theRetainedBuffer_1 should still be retained.

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the explicit release should have dereferenced
        // the buffer, leaving the buffer reference count at two (2).
    }

    // At this point, object destruction should have implicitly
    // dereferenced the buffer, leaving the buffer reference count at
    // one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

/**
 *  Test assignment with non-NULL buffer and explicitly release the target
 *  first and then explicitly the source second
 *
 */
static void CheckAssignmentWithAllocatedBufferExplicitTargetExplicitSourceRelease(nlTestSuite *inSuite, void *inContext)
{
    Profiles::RetainedPacketBuffer theRetainedBuffer_1;
    System::PacketBuffer *allocatedBuffer;
    System::PacketBuffer *accessedBuffer;
    bool retaining;

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // Allocate a new system packet buffer with reference count of one (1).

    PacketBufferAlloc(allocatedBuffer);
    NL_TEST_ASSERT(inSuite, allocatedBuffer != NULL);

    theRetainedBuffer_1.Retain(allocatedBuffer);

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // At this point, the first retained packet buffer object holds
    // another reference, leaving the reference count at two (2).

    {
        Profiles::RetainedPacketBuffer theRetainedBuffer_2;

        theRetainedBuffer_2 = theRetainedBuffer_1;

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);

        // At this point, the second retained packet buffer object holds
        // another reference, leaving the reference count at three (3).

        // Explicitly release the associated buffer.

        theRetainedBuffer_2.Release();

        accessedBuffer = theRetainedBuffer_2.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

        retaining = theRetainedBuffer_2.IsRetaining();
        NL_TEST_ASSERT(inSuite, !retaining);

        // At this point, the explicit release should have dereferenced
        // the buffer, leaving the buffer reference count at two (2).

        accessedBuffer = theRetainedBuffer_1.GetBuffer();
        NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
        NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

        retaining = theRetainedBuffer_1.IsRetaining();
        NL_TEST_ASSERT(inSuite, retaining);
    }

    // theRetainedBuffer_2 is now both released and
    // destroyed. theRetainedBuffer_1 should still be retained.

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer != NULL);
    NL_TEST_ASSERT(inSuite, accessedBuffer == allocatedBuffer);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, retaining);

    // Explicitly release the associated buffer.

    theRetainedBuffer_1.Release();

    accessedBuffer = theRetainedBuffer_1.GetBuffer();
    NL_TEST_ASSERT(inSuite, accessedBuffer == NULL);

    retaining = theRetainedBuffer_1.IsRetaining();
    NL_TEST_ASSERT(inSuite, !retaining);

    // At this point, the explicit release should have dereferenced
    // the buffer, leaving the buffer reference count at one (1).

    PacketBufferFree(allocatedBuffer);

    // Assert that the buffer has been released to the pool, as expected.

    NL_TEST_ASSERT(inSuite, System::Stats::GetResourcesInUse()[System::Stats::kSystemLayer_NumPacketBufs] == 0);
}

static const nlTest sTests[] = {
    NL_TEST_DEF("default construction and destruction",                                                     CheckDefaultConstruction),
    NL_TEST_DEF("get buffer accessor",                                                                      CheckGetBufferAccessor),
    NL_TEST_DEF("is retaining accessor",                                                                    CheckIsRetainingAccessor),
    NL_TEST_DEF("copy construction with no retained buffer",                                                CheckCopyConstructionWithoutRetainedBuffer),
    NL_TEST_DEF("assignment with no retained buffer",                                                       CheckAssignmentWithoutRetainedBuffer),
    NL_TEST_DEF("release with no retained buffer",                                                          CheckReleaseWithoutRetainedBuffer),
    NL_TEST_DEF("retain with a null pointer",                                                               CheckRetainWithNullPointer),
    NL_TEST_DEF("retain with an allocated buffer and implicit release",                                     CheckRetainAllocatedBufferWithImplicitRelease),
    NL_TEST_DEF("retain with an allocated buffer and explicit release",                                     CheckRetainAllocatedBufferWithExplicitRelease),
    NL_TEST_DEF("copy construction with allocated buffer and implicit target then implicit source release", CheckCopyConstructionWithAllocatedBufferImplicitTargetImplicitSourceRelease),
    NL_TEST_DEF("copy construction with allocated buffer and implicit target then explicit source release", CheckCopyConstructionWithAllocatedBufferImplicitTargetExplicitSourceRelease),
    NL_TEST_DEF("copy construction with allocated buffer and explicit target then implicit source release", CheckCopyConstructionWithAllocatedBufferExplicitTargetImplicitSourceRelease),
    NL_TEST_DEF("copy construction with allocated buffer and explicit target then explicit source release", CheckCopyConstructionWithAllocatedBufferExplicitTargetExplicitSourceRelease),
    NL_TEST_DEF("assignment with allocated buffer and implicit target then implicit source release",        CheckAssignmentWithAllocatedBufferImplicitTargetImplicitSourceRelease),
    NL_TEST_DEF("assignment with allocated buffer and implicit target then explicit source release",        CheckAssignmentWithAllocatedBufferImplicitTargetExplicitSourceRelease),
    NL_TEST_DEF("assignment with allocated buffer and explicit target then implicit source release",        CheckAssignmentWithAllocatedBufferExplicitTargetImplicitSourceRelease),
    NL_TEST_DEF("assignment with allocated buffer and explicit target then explicit source release",        CheckAssignmentWithAllocatedBufferExplicitTargetExplicitSourceRelease),
    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "weave-retained-packet-buffer",
        &sTests[0]
    };

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
