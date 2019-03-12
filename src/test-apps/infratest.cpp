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
 *      This file implements test code for the Weave core
 *      infrastructure.
 *
 */

// library includes
#include <iostream>
#include <assert.h>

using namespace std;

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Support/CodeUtils.h>

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include "lwip/tcpip.h"
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;

// static test data

#define TEST_BYTE 0x81
#define SHORT_TEST_NUM 0x8421
#define LONG_TEST_NUM 0x87654321

#if !WEAVE_SYSTEM_CONFIG_USE_LWIP
#if INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
using nl::Inet::pbuf;
#else // INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
using nl::Weave::System::pbuf;
#endif // INET_CONFIG_PROVIDE_OBSOLESCENT_INTERFACES
#endif // !WEAVE_SYSTEM_CONFIG_USE_LWIP

/*
 * here's a little utility function that tries
 * to get the eference count from an inet buffer
 */

uint16_t refCount(PacketBuffer *pBuffer)
{
    pbuf *p=(pbuf *)pBuffer;
    return p->ref;
}

/*
 * this is a TLV writer function required to test the
 * referenced TLV object below.
 */

void testWriter(TLVWriter &w, void  *a)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType containerType;
    TLVWriter checkpoint = w;

    err = w.StartContainer(AnonymousTag, kTLVType_Structure, containerType);
    SuccessOrExit(err);

    err = w.Put(ProfileTag(kWeaveProfile_Common, 1), 1);
    SuccessOrExit(err);

    err = w.Put(ProfileTag(kWeaveProfile_Common, 2), 2);
    SuccessOrExit(err);

    err = w.Put(ProfileTag(kWeaveProfile_Common, 3), 3);
    SuccessOrExit(err);

    err = w.EndContainer(containerType);
    SuccessOrExit(err);

    err = w.Finalize();
    SuccessOrExit(err);

exit:
    if (err != WEAVE_NO_ERROR)
    {
        w = checkpoint;
    }
}

// and away we go

int main()
{
    // try byte, short and long packing and parsing macros

    uint8_t testString[5];
    uint8_t *p=testString;
    uint8_t *q=p;

    uint16_t shortnum;
    uint32_t longnum;

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
    tcpip_init(NULL, NULL);
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

    WRITEBYTE(p, TEST_BYTE);
    assert((READBYTE(q)==TEST_BYTE) && p-testString==1 && q-testString==1);

    p=q=testString;
    WRITE16(p, SHORT_TEST_NUM);
    READ16(q, shortnum);
    assert((shortnum==SHORT_TEST_NUM) && p-testString==2 && q-testString==2);

    p=q=testString;
    WRITE32(p, LONG_TEST_NUM);
    READ32(q, longnum);
    assert(longnum==LONG_TEST_NUM && p-testString==4 && q-testString==4);

    cout<<"basic READ/WRITE macros work"<<endl;

    // check if message iterators work
    {
        // creat a buffer and some iterators and make sure the
        // reference counting stuff is working OK

        PacketBuffer *buffer=PacketBuffer::New();
        assert(refCount(buffer)==1);

        MessageIterator i(buffer);
        assert(refCount(buffer)==2);

        MessageIterator j(buffer);
        assert(refCount(buffer)==3);

        MessageIterator x(buffer);
        assert(refCount(buffer)==4);

        uint8_t byte;
        uint16_t shortInt;
        uint32_t longInt;
        char outStr[8]="abcdefg";
        char inStr[8]="xxxxxxx";

        // i and j should be equal

        assert(i==j);

        // shouldn't be able to read an empty buffer

        assert(i.readByte(&byte)==WEAVE_ERROR_BUFFER_TOO_SMALL);

        // write and read a byte

        assert(i.writeByte(TEST_BYTE)==WEAVE_NO_ERROR);
        assert(x.hasData(1)==true);

        // OK, now the iterators shouldn't be equal

        assert(i!=j);
        assert(j.readByte(&byte)==WEAVE_NO_ERROR);

        // and now they should again

        assert(i==j);
        assert(byte==TEST_BYTE);

        // write and read a short

        assert(i.write16(SHORT_TEST_NUM)==WEAVE_NO_ERROR);
        assert(x.hasData(3)==true);
        assert(j.read16(&shortInt)==WEAVE_NO_ERROR);
        assert(shortInt==SHORT_TEST_NUM);

        // write and read a long

        assert(i.write32(LONG_TEST_NUM)==WEAVE_NO_ERROR);
        assert(x.hasData(7)==true);
        assert(j.read32(&longInt)==WEAVE_NO_ERROR);
        assert(longInt==LONG_TEST_NUM);

        // write and read a string

        assert(i.writeString(8, outStr)==WEAVE_NO_ERROR);
        assert(x.hasData(15)==true);
        assert(j.readString(8, inStr)==WEAVE_NO_ERROR);
        for (int k=0; k<8; k++) assert(inStr[k]==outStr[k]);

        // do some checking on the pointer manipulation stuff

        assert(i==j);
        assert(*i==*j);
        i=i-2;

        // and make sure self-assignment doesn't change the
        // reference count.

        assert(refCount(buffer)==4);
        assert(i!=j);
        assert(*i!=*j);
        j=j-3;
        assert(i!=j);
        assert(*i!=*j);
        ++j;
        assert(i==j);
        assert(*i==*j);

        // one final check on the reference count and then
        // free some stuff

        assert(refCount(buffer)==4);
        x.Release();
        assert(refCount(buffer)==3);
        i.Release();
        assert(refCount(buffer)==2);
        j.Release();
        assert(refCount(buffer)==1);

        // test the auto-destructor
        {
            MessageIterator k(buffer);
            assert(refCount(buffer)==2);
        }

        assert(refCount(buffer)==1);

        cout<<"message iterators seem to work"<<endl;

        PacketBuffer::Free(buffer);
    }
    // do some checking on referenced TLV
    {
        PacketBuffer *buffer=PacketBuffer::New();
        ReferencedTLVData out;
        ReferencedTLVData in;

        // do the same auto-destructor test as above

        {
            ReferencedTLVData k;

            k.init(buffer);

            assert(refCount(buffer)==2);
        }

        assert(refCount(buffer)==1);

        assert(out.init(testWriter, NULL)==WEAVE_NO_ERROR);
        assert(out.pack(buffer)==WEAVE_NO_ERROR);
        assert(out.theLength!=0);

        assert(ReferencedTLVData::parse(buffer, in)==WEAVE_NO_ERROR);
        assert(out==in);

        assert(refCount(buffer)==2);

        // check if free does the right thing here.

        out.free();
        in.free();
        assert(refCount(buffer)==1);

        assert(out.isFree());
        assert(in.isFree());

        cout<<"Referenced TLV pack and parse (with writer fcn) works"<<endl;

        PacketBuffer::Free(buffer);
    }
    return 0;
}
