/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
 *      memory-mapped I/O and byte reordering interfaces.
 *
 */

#include <stdlib.h>
#include <unistd.h>

#include <nlbyteorder.h>
#include <nltest.h>

#include <Weave/Core/WeaveEncoding.h>

#define MAGIC8          0x01U
#define MAGIC16         0x0123U
#define MAGIC32         0x01234567UL
#define MAGIC64         0x0123456789ABCDEFULL

#define MAGIC_SWAP8     0x01U
#define MAGIC_SWAP16    0x2301U
#define MAGIC_SWAP32    0x67452301UL
#define MAGIC_SWAP64    0xEFCDAB8967452301ULL

using namespace nl::Weave::Encoding;

static void CheckSwap(nlTestSuite *inSuite, void *inContext)
{
    // Check that the constant swap inline free functions work.

    NL_TEST_ASSERT(inSuite, Swap16(MAGIC16) == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, Swap32(MAGIC32) == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, Swap64(MAGIC64) == MAGIC_SWAP64);
}

static void CheckSwapBig(nlTestSuite *inSuite, void *inContext)
{
    uint16_t v16_in = MAGIC16;
    uint32_t v32_in = MAGIC32;
    uint64_t v64_in = MAGIC64;
    uint16_t v16_out;
    uint32_t v32_out;
    uint64_t v64_out;

    // Check swap by value of big endian values against the host
    // system ordering.

    v16_out = BigEndian::HostSwap16(v16_in);
    v32_out = BigEndian::HostSwap32(v32_in);
    v64_out = BigEndian::HostSwap64(v64_in);

#if NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v16_out == v16_in);
    NL_TEST_ASSERT(inSuite, v32_out == v32_in);
    NL_TEST_ASSERT(inSuite, v64_out == v64_in);

#elif NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#endif
}

static void CheckSwapLittle(nlTestSuite *inSuite, void *inContext)
{
    uint16_t v16_in = MAGIC16;
    uint32_t v32_in = MAGIC32;
    uint64_t v64_in = MAGIC64;
    uint16_t v16_out;
    uint32_t v32_out;
    uint64_t v64_out;

    // Check swap by value of little endian values against the host
    // system ordering.

    v16_out = LittleEndian::HostSwap16(v16_in);
    v32_out = LittleEndian::HostSwap32(v32_in);
    v64_out = LittleEndian::HostSwap64(v64_in);

#if NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v16_out == v16_in);
    NL_TEST_ASSERT(inSuite, v32_out == v32_in);
    NL_TEST_ASSERT(inSuite, v64_out == v64_in);

#endif
}

static void CheckGetBig(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in  = MAGIC8;
    const uint16_t v16_in = MAGIC16;
    const uint32_t v32_in = MAGIC32;
    const uint64_t v64_in = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Get8(&v8_in);
    v16_out = BigEndian::Get16(reinterpret_cast<const uint8_t *>(&v16_in));
    v32_out = BigEndian::Get32(reinterpret_cast<const uint8_t *>(&v32_in));
    v64_out = BigEndian::Get64(reinterpret_cast<const uint8_t *>(&v64_in));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckGetLittle(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in  = MAGIC8;
    const uint16_t v16_in = MAGIC16;
    const uint32_t v32_in = MAGIC32;
    const uint64_t v64_in = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Get8(&v8_in);
    v16_out = LittleEndian::Get16(reinterpret_cast<const uint8_t *>(&v16_in));
    v32_out = LittleEndian::Get32(reinterpret_cast<const uint8_t *>(&v32_in));
    v64_out = LittleEndian::Get64(reinterpret_cast<const uint8_t *>(&v64_in));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckPutBig(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in  = MAGIC8;
    const uint16_t v16_in = MAGIC16;
    const uint32_t v32_in = MAGIC32;
    const uint64_t v64_in = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    Put8(&v8_out,   v8_in);
    BigEndian::Put16(reinterpret_cast<uint8_t *>(&v16_out), v16_in);
    BigEndian::Put32(reinterpret_cast<uint8_t *>(&v32_out), v32_in);
    BigEndian::Put64(reinterpret_cast<uint8_t *>(&v64_out), v64_in);

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckPutLittle(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in  = MAGIC8;
    const uint16_t v16_in = MAGIC16;
    const uint32_t v32_in = MAGIC32;
    const uint64_t v64_in = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    Put8(&v8_out,   v8_in);
    LittleEndian::Put16(reinterpret_cast<uint8_t *>(&v16_out), v16_in);
    LittleEndian::Put32(reinterpret_cast<uint8_t *>(&v32_out), v32_in);
    LittleEndian::Put64(reinterpret_cast<uint8_t *>(&v64_out), v64_in);

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckReadBig(nlTestSuite *inSuite, void *inContext)
{
    uint8_t        v8_in   = MAGIC8;
    uint16_t       v16_in  = MAGIC16;
    uint32_t       v32_in  = MAGIC32;
    uint64_t       v64_in  = MAGIC64;
    uint8_t        *p8_in  = &v8_in;
    uint8_t        *p16_in = reinterpret_cast<uint8_t *>(&v16_in);
    uint8_t        *p32_in = reinterpret_cast<uint8_t *>(&v32_in);
    uint8_t        *p64_in = reinterpret_cast<uint8_t *>(&v64_in);
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Read8(p8_in);
    v16_out = BigEndian::Read16(p16_in);
    v32_out = BigEndian::Read32(p32_in);
    v64_out = BigEndian::Read64(p64_in);

    NL_TEST_ASSERT(inSuite, p8_in == (&v8_in + 1));
    NL_TEST_ASSERT(inSuite, p16_in == reinterpret_cast<uint8_t *>((&v16_in + 1)));
    NL_TEST_ASSERT(inSuite, p32_in == reinterpret_cast<uint8_t *>((&v32_in + 1)));
    NL_TEST_ASSERT(inSuite, p64_in == reinterpret_cast<uint8_t *>((&v64_in + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckReadLittle(nlTestSuite *inSuite, void *inContext)
{
    uint8_t        v8_in   = MAGIC8;
    uint16_t       v16_in  = MAGIC16;
    uint32_t       v32_in  = MAGIC32;
    uint64_t       v64_in  = MAGIC64;
    uint8_t        *p8_in  = &v8_in;
    uint8_t        *p16_in = reinterpret_cast<uint8_t *>(&v16_in);
    uint8_t        *p32_in = reinterpret_cast<uint8_t *>(&v32_in);
    uint8_t        *p64_in = reinterpret_cast<uint8_t *>(&v64_in);
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Read8(p8_in);
    v16_out = LittleEndian::Read16(p16_in);
    v32_out = LittleEndian::Read32(p32_in);
    v64_out = LittleEndian::Read64(p64_in);

    NL_TEST_ASSERT(inSuite, p8_in == (&v8_in + 1));
    NL_TEST_ASSERT(inSuite, p16_in == reinterpret_cast<uint8_t *>((&v16_in + 1)));
    NL_TEST_ASSERT(inSuite, p32_in == reinterpret_cast<uint8_t *>((&v32_in + 1)));
    NL_TEST_ASSERT(inSuite, p64_in == reinterpret_cast<uint8_t *>((&v64_in + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckConstReadBig(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in   = MAGIC8;
    const uint16_t v16_in  = MAGIC16;
    const uint32_t v32_in  = MAGIC32;
    const uint64_t v64_in  = MAGIC64;
    const uint8_t  *p8_in  = &v8_in;
    const uint8_t  *p16_in = reinterpret_cast<const uint8_t *>(&v16_in);
    const uint8_t  *p32_in = reinterpret_cast<const uint8_t *>(&v32_in);
    const uint8_t  *p64_in = reinterpret_cast<const uint8_t *>(&v64_in);
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Read8(p8_in);
    v16_out = BigEndian::Read16(p16_in);
    v32_out = BigEndian::Read32(p32_in);
    v64_out = BigEndian::Read64(p64_in);

    NL_TEST_ASSERT(inSuite, p8_in == (&v8_in + 1));
    NL_TEST_ASSERT(inSuite, p16_in == reinterpret_cast<const uint8_t *>((&v16_in + 1)));
    NL_TEST_ASSERT(inSuite, p32_in == reinterpret_cast<const uint8_t *>((&v32_in + 1)));
    NL_TEST_ASSERT(inSuite, p64_in == reinterpret_cast<const uint8_t *>((&v64_in + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckConstReadLittle(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in   = MAGIC8;
    const uint16_t v16_in  = MAGIC16;
    const uint32_t v32_in  = MAGIC32;
    const uint64_t v64_in  = MAGIC64;
    const uint8_t  *p8_in  = &v8_in;
    const uint8_t  *p16_in = reinterpret_cast<const uint8_t *>(&v16_in);
    const uint8_t  *p32_in = reinterpret_cast<const uint8_t *>(&v32_in);
    const uint8_t  *p64_in = reinterpret_cast<const uint8_t *>(&v64_in);
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;

    v8_out  = Read8(p8_in);
    v16_out = LittleEndian::Read16(p16_in);
    v32_out = LittleEndian::Read32(p32_in);
    v64_out = LittleEndian::Read64(p64_in);

    NL_TEST_ASSERT(inSuite, p8_in == (&v8_in + 1));
    NL_TEST_ASSERT(inSuite, p16_in == reinterpret_cast<const uint8_t *>((&v16_in + 1)));
    NL_TEST_ASSERT(inSuite, p32_in == reinterpret_cast<const uint8_t *>((&v32_in + 1)));
    NL_TEST_ASSERT(inSuite, p64_in == reinterpret_cast<const uint8_t *>((&v64_in + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckWriteBig(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in   = MAGIC8;
    const uint16_t v16_in  = MAGIC16;
    const uint32_t v32_in  = MAGIC32;
    const uint64_t v64_in  = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;
    uint8_t        *p8_out  = &v8_out;
    uint8_t        *p16_out = reinterpret_cast<uint8_t *>(&v16_out);
    uint8_t        *p32_out = reinterpret_cast<uint8_t *>(&v32_out);
    uint8_t        *p64_out = reinterpret_cast<uint8_t *>(&v64_out);

    Write8(p8_out, v8_in);
    BigEndian::Write16(p16_out, v16_in);
    BigEndian::Write32(p32_out, v32_in);
    BigEndian::Write64(p64_out, v64_in);

    NL_TEST_ASSERT(inSuite, p8_out == (&v8_out + 1));
    NL_TEST_ASSERT(inSuite, p16_out == reinterpret_cast<uint8_t *>((&v16_out + 1)));
    NL_TEST_ASSERT(inSuite, p32_out == reinterpret_cast<uint8_t *>((&v32_out + 1)));
    NL_TEST_ASSERT(inSuite, p64_out == reinterpret_cast<uint8_t *>((&v64_out + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static void CheckWriteLittle(nlTestSuite *inSuite, void *inContext)
{
    const uint8_t  v8_in   = MAGIC8;
    const uint16_t v16_in  = MAGIC16;
    const uint32_t v32_in  = MAGIC32;
    const uint64_t v64_in  = MAGIC64;
    uint8_t        v8_out;
    uint16_t       v16_out;
    uint32_t       v32_out;
    uint64_t       v64_out;
    uint8_t        *p8_out  = &v8_out;
    uint8_t        *p16_out = reinterpret_cast<uint8_t *>(&v16_out);
    uint8_t        *p32_out = reinterpret_cast<uint8_t *>(&v32_out);
    uint8_t        *p64_out = reinterpret_cast<uint8_t *>(&v64_out);

    Write8(p8_out, v8_in);
    LittleEndian::Write16(p16_out, v16_in);
    LittleEndian::Write32(p32_out, v32_in);
    LittleEndian::Write64(p64_out, v64_in);

    NL_TEST_ASSERT(inSuite, p8_out == (&v8_out + 1));
    NL_TEST_ASSERT(inSuite, p16_out == reinterpret_cast<uint8_t *>((&v16_out + 1)));
    NL_TEST_ASSERT(inSuite, p32_out == reinterpret_cast<uint8_t *>((&v32_out + 1)));
    NL_TEST_ASSERT(inSuite, p64_out == reinterpret_cast<uint8_t *>((&v64_out + 1)));

#if NLBYTEORDER == NLBYTEORDER_LITTLE_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC64);

#elif NLBYTEORDER == NLBYTEORDER_BIG_ENDIAN
    NL_TEST_ASSERT(inSuite, v8_out  == MAGIC_SWAP8);
    NL_TEST_ASSERT(inSuite, v16_out == MAGIC_SWAP16);
    NL_TEST_ASSERT(inSuite, v32_out == MAGIC_SWAP32);
    NL_TEST_ASSERT(inSuite, v64_out == MAGIC_SWAP64);

#else
    NL_TEST_ASSERT(inSuite, false);

#endif
}

static const nlTest sTests[] = {
    NL_TEST_DEF("swap",                CheckSwap),
    NL_TEST_DEF("swap big",            CheckSwapBig),
    NL_TEST_DEF("swap little",         CheckSwapLittle),
    NL_TEST_DEF("get big",             CheckGetBig),
    NL_TEST_DEF("get little",          CheckGetLittle),
    NL_TEST_DEF("put big",             CheckPutBig),
    NL_TEST_DEF("put little",          CheckPutLittle),
    NL_TEST_DEF("read big",            CheckReadBig),
    NL_TEST_DEF("read little",         CheckReadLittle),
    NL_TEST_DEF("const read big",      CheckConstReadBig),
    NL_TEST_DEF("const read little",   CheckConstReadLittle),
    NL_TEST_DEF("write big",           CheckWriteBig),
    NL_TEST_DEF("write little",        CheckWriteLittle),
    NL_TEST_SENTINEL()
};

int main(void)
{
    nlTestSuite theSuite = {
        "weave-encoding",
        &sTests[0]
    };

    nl_test_set_output_style(OUTPUT_CSV);

    nlTestRunner(&theSuite, NULL);

    return nlTestRunnerStats(&theSuite);
}
