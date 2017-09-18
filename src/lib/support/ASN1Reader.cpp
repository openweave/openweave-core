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
 *      This file implements an object for reading Abstract Syntax
 *      Notation One (ASN.1) encoded data.
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <Weave/Support/ASN1.h>

namespace nl {
namespace Weave {
namespace ASN1 {

void ASN1Reader::Init(const uint8_t *buf, uint32_t len)
{
    ResetElementState();
    mBuf = buf;
    mBufEnd = buf + len;
    mElemStart = buf;
    mContainerEnd = mBufEnd;
    mNumSavedContexts = 0;
}

ASN1_ERROR ASN1Reader::Next()
{
    if (IsEndOfContents)
        return ASN1_END;

    if (IsIndefiniteLen)
    {
        // TODO: handle skipping indefinite length
        return ASN1_ERROR_UNSUPPORTED_ENCODING;
    }
    else
        mElemStart += (mHeadLen + ValueLen);

    ResetElementState();

    if (mElemStart == mContainerEnd)
        return ASN1_END;

    return DecodeHead();
}

ASN1_ERROR ASN1Reader::EnterConstructedType()
{
    if (!IsConstructed)
        return ASN1_ERROR_INVALID_STATE;

    return EnterContainer(0);
}

ASN1_ERROR ASN1Reader::ExitConstructedType()
{
    return ExitContainer();
}

ASN1_ERROR ASN1Reader::EnterEncapsulatedType()
{
    if (Class != kASN1TagClass_Universal || (Tag != kASN1UniversalTag_OctetString && Tag != kASN1UniversalTag_BitString))
        return ASN1_ERROR_INVALID_STATE;

    if (IsConstructed)
        return ASN1_ERROR_UNSUPPORTED_ENCODING;

    return EnterContainer((Tag == kASN1UniversalTag_BitString) ? 1 : 0);
}

ASN1_ERROR ASN1Reader::ExitEncapsulatedType()
{
    return ExitContainer();
}

ASN1_ERROR ASN1Reader::EnterContainer(uint32_t offset)
{
    if (mNumSavedContexts == kMaxContextDepth)
        return ASN1_ERROR_MAX_DEPTH_EXCEEDED;

    mSavedContexts[mNumSavedContexts].ElemStart = mElemStart;
    mSavedContexts[mNumSavedContexts].HeadLen = mHeadLen;
    mSavedContexts[mNumSavedContexts].ValueLen = ValueLen;
    mSavedContexts[mNumSavedContexts].IsIndefiniteLen = IsIndefiniteLen;
    mSavedContexts[mNumSavedContexts].ContainerEnd = mContainerEnd;
    mNumSavedContexts++;

    mElemStart = Value + offset;
    if (!IsIndefiniteLen)
        mContainerEnd = Value + ValueLen;

    ResetElementState();

    return ASN1_NO_ERROR;
}

ASN1_ERROR ASN1Reader::ExitContainer()
{
    if (mNumSavedContexts == 0)
        return ASN1_ERROR_INVALID_STATE;

    ASN1ParseContext& prevContext = mSavedContexts[--mNumSavedContexts];

    if (prevContext.IsIndefiniteLen)
    {
        // TODO: implement me
        return ASN1_ERROR_UNSUPPORTED_ENCODING;
    }
    else
        mElemStart = prevContext.ElemStart + prevContext.HeadLen + prevContext.ValueLen;

    mContainerEnd = prevContext.ContainerEnd;

    ResetElementState();

    return ASN1_NO_ERROR;
}

bool ASN1Reader::IsContained() const
{
    return mNumSavedContexts > 0;
}

ASN1_ERROR ASN1Reader::GetInteger(int64_t& val)
{
    if (Value == NULL)
        return ASN1_ERROR_INVALID_STATE;
    if (ValueLen < 1)
        return ASN1_ERROR_INVALID_ENCODING;
    if (ValueLen > sizeof(int64_t))
        return ASN1_ERROR_VALUE_OVERFLOW;
    if (mElemStart + mHeadLen + ValueLen > mContainerEnd)
        return ASN1_ERROR_UNDERRUN;
    const uint8_t *p = Value;
    val = ((*p & 0x80) == 0) ? 0 : -1;
    for (uint32_t i = ValueLen; i > 0; i--, p++)
        val = (val << 8) | *p;
    return ASN1_NO_ERROR;
}

ASN1_ERROR ASN1Reader::GetBoolean(bool& val)
{
    if (Value == NULL)
        return ASN1_ERROR_INVALID_STATE;
    if (ValueLen != 1)
        return ASN1_ERROR_INVALID_ENCODING;
    if (mElemStart + mHeadLen + ValueLen > mContainerEnd)
        return ASN1_ERROR_UNDERRUN;
    if (*Value == 0)
        val = false;
    else if (*Value == 0xFF)
        val = true;
    else
        return ASN1_ERROR_INVALID_ENCODING;
    return ASN1_NO_ERROR;
}

ASN1_ERROR ASN1Reader::GetUTCTime(ASN1UniversalTime& outTime)
{
    // Supported Encoding: YYMMDDHHMMSSZ

    if (Value == NULL)
        return ASN1_ERROR_INVALID_STATE;
    if (ValueLen < 1)
        return ASN1_ERROR_INVALID_ENCODING;
    if (mElemStart + mHeadLen + ValueLen > mContainerEnd)
        return ASN1_ERROR_UNDERRUN;

    if (ValueLen != 13 || Value[12] != 'Z')
        return ASN1_ERROR_UNSUPPORTED_ENCODING;

    for (int i = 0; i < 12; i++)
        if (!isdigit(Value[i]))
            return ASN1_ERROR_INVALID_ENCODING;

    outTime.Year        = (Value[0]  - '0') * 10 +
                          (Value[1]  - '0');
    outTime.Month       = (Value[2]  - '0') * 10 +
                          (Value[3]  - '0');
    outTime.Day         = (Value[4]  - '0') * 10 +
                          (Value[5]  - '0');
    outTime.Hour        = (Value[6]  - '0') * 10 +
                          (Value[7]  - '0');
    outTime.Minute      = (Value[8]  - '0') * 10 +
                          (Value[9]  - '0');
    outTime.Second      = (Value[10] - '0') * 10 +
                          (Value[11] - '0');

    if (outTime.Year >= 50)
        outTime.Year += 1900;
    else
        outTime.Year += 2000;

    return ASN1_NO_ERROR;
}

ASN1_ERROR ASN1Reader::GetGeneralizedTime(ASN1UniversalTime& outTime)
{
    // Supported Encoding: YYYYMMDDHHMMSSZ

    if (Value == NULL)
        return ASN1_ERROR_INVALID_STATE;
    if (ValueLen < 1)
        return ASN1_ERROR_INVALID_ENCODING;
    if (mElemStart + mHeadLen + ValueLen > mContainerEnd)
        return ASN1_ERROR_UNDERRUN;

    if (ValueLen != 15 || Value[14] != 'Z')
        return ASN1_ERROR_UNSUPPORTED_ENCODING;

    for (int i = 0; i < 14; i++)
        if (!isdigit(Value[i]))
            return ASN1_ERROR_INVALID_ENCODING;

    outTime.Year        = (Value[0]  - '0') * 1000 +
                          (Value[1]  - '0') * 100 +
                          (Value[2]  - '0') * 10 +
                          (Value[3]  - '0');
    outTime.Month       = (Value[4]  - '0') * 10 +
                          (Value[5]  - '0');
    outTime.Day         = (Value[6]  - '0') * 10 +
                          (Value[7]  - '0');
    outTime.Hour        = (Value[8]  - '0') * 10 +
                          (Value[9]  - '0');
    outTime.Minute      = (Value[10] - '0') * 10 +
                          (Value[11] - '0');
    outTime.Second      = (Value[12] - '0') * 10 +
                          (Value[13] - '0');

    return ASN1_NO_ERROR;
}

static uint8_t ReverseBits(uint8_t v)
{
    // swap adjacent bits
    v = ((v >> 1) & 0x55) | ((v & 0x55) << 1);
    // swap adjacent bit pairs
    v = ((v >> 2) & 0x33) | ((v & 0x33) << 2);
    // swap nibbles
    v = (v >> 4) | (v << 4);
    return v;
}

ASN1_ERROR ASN1Reader::GetBitString(uint32_t& outVal)
{
    // NOTE: only supports DER encoding.

    if (Value == NULL)
        return ASN1_ERROR_INVALID_STATE;
    if (ValueLen < 1)
        return ASN1_ERROR_INVALID_ENCODING;
    if (ValueLen > 5)
        return ASN1_ERROR_UNSUPPORTED_ENCODING;
    if (mElemStart + mHeadLen + ValueLen > mContainerEnd)
        return ASN1_ERROR_UNDERRUN;

    if (ValueLen == 1)
        outVal = 0;
    else
    {
        outVal = ReverseBits(Value[1]);
        int shift = 8;
        for (uint32_t i = 2; i < ValueLen; i++, shift += 8)
            outVal |= (ReverseBits(Value[i]) << shift);
    }

    return ASN1_NO_ERROR;
}

ASN1_ERROR ASN1Reader::DecodeHead()
{
    const uint8_t *p = mElemStart;

    if (p >= mBufEnd)
        return ASN1_ERROR_UNDERRUN;

    Class = *p & 0xC0;
    IsConstructed = (*p & 0x20) != 0;

    Tag = *p & 0x1F;
    p++;
    if (Tag == 0x1F)
    {
        Tag = 0;
        do
        {
            if (p >= mBufEnd)
                return ASN1_ERROR_UNDERRUN;
            if ((Tag & 0xFE000000) != 0)
                return ASN1_ERROR_TAG_OVERFLOW;
            Tag = (Tag << 7) | (*p & 0x7F);
        } while ((*p & 0x80) != 0);
    }

    if (p >= mBufEnd)
        return ASN1_ERROR_UNDERRUN;

    if ((*p & 0x80) == 0)
    {
        ValueLen = *p & 0x7F;
        IsIndefiniteLen = false;
        p++;
    }
    else if (*p == 0x80)
    {
        ValueLen = 0;
        IsIndefiniteLen = true;
        p++;
    }
    else
    {
        ValueLen = 0;
        uint8_t lenLen = *p & 0x7F;
        p++;
        for (; lenLen > 0; lenLen--, p++)
        {
            if (p >= mBufEnd)
                return ASN1_ERROR_UNDERRUN;
            if ((ValueLen & 0xFF000000) != 0)
                return ASN1_ERROR_LENGTH_OVERFLOW;
            ValueLen = (ValueLen << 8) | *p;
        }
        IsIndefiniteLen = false;
    }

    mHeadLen = p - mElemStart;

    IsEndOfContents = (Class == 0 && Tag == 0 && IsConstructed == false && ValueLen == 0);

    Value = p;

    return ASN1_NO_ERROR;
}

void ASN1Reader::ResetElementState()
{
    Class = 0;
    Tag = 0;
    Value = NULL;
    ValueLen = 0;
    IsConstructed = false;
    IsIndefiniteLen = false;
    IsEndOfContents = false;
    mHeadLen = 0;
}

ASN1_ERROR DumpASN1(ASN1Reader& asn1Parser, const char *prefix, const char *indent)
{
    ASN1_ERROR err = ASN1_NO_ERROR;

    if (indent == NULL)
        indent = "  ";

    int nestLevel = 0;
    while (true)
    {
        err = asn1Parser.Next();
        if (err != ASN1_NO_ERROR)
        {
            if (err == ASN1_END)
            {
                if (asn1Parser.IsContained())
                {
                    err = asn1Parser.ExitConstructedType();
                    if (err != ASN1_NO_ERROR)
                    {
                        printf("ASN1Reader::ExitConstructedType() failed: %ld\n", (long)err);
                        return err;
                    }
                    nestLevel--;
                    continue;
                }
                else
                    break;
            }
            printf("ASN1Reader::Next() failed: %ld\n", (long)err);
            return err;
        }
        if (prefix != NULL)
            printf("%s", prefix);
        for (int i = nestLevel; i; i--)
            printf("%s", indent);
        if (asn1Parser.IsEndOfContents)
            printf("END-OF-CONTENTS ");
        else if (asn1Parser.Class == 0)
            switch (asn1Parser.Tag)
            {
            case 1:     printf("BOOLEAN "); break;
            case 2:     printf("INTEGER "); break;
            case 3:     printf("BIT STRING "); break;
            case 4:     printf("OCTET STRING "); break;
            case 5:     printf("NULL "); break;
            case 6:     printf("OBJECT IDENTIFIER "); break;
            case 7:     printf("OBJECT DESCRIPTOR "); break;
            case 8:     printf("EXTERNAL "); break;
            case 9:     printf("REAL "); break;
            case 10:    printf("ENUMERATED "); break;
            case 16:    printf("SEQUENCE "); break;
            case 17:    printf("SET "); break;
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 25:
            case 26:
            case 27:     printf("STRING "); break;
            case 23:
            case 24:     printf("TIME "); break;
            default:     printf("[UNIVERSAL %lu] ", (unsigned long)asn1Parser.Tag); break;
            }
        else if (asn1Parser.Class == 0x40)
            printf("[APPLICATION %lu] ", (unsigned long)asn1Parser.Tag);
        else if (asn1Parser.Class == 0x80)
            printf("[%lu] ", (unsigned long)asn1Parser.Tag);
        else if (asn1Parser.Class == 0xC0)
            printf("[PRIVATE %lu] ", (unsigned long)asn1Parser.Tag);

        if (asn1Parser.IsConstructed)
            printf("(constructed) ");

        if (asn1Parser.IsIndefiniteLen)
            printf("Length = indefinite\n");
        else
            printf("Length = %ld\n", (long)asn1Parser.ValueLen);

        if (asn1Parser.IsConstructed)
        {
            err = asn1Parser.EnterConstructedType();
            if (err != ASN1_NO_ERROR)
            {
                printf("ASN1Reader::EnterConstructedType() failed: %ld\n", (long)err);
                return err;
            }
            nestLevel++;
        }
    }

    return err;
}

} // namespace ASN1
} // namespace Weave
} // namespace nl
