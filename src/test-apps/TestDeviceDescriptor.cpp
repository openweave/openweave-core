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
 *    @file
 *      This file implements test code to functionally-test the Weave
 *      Common Device Description profile.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <Weave/Profiles/device-description/DeviceDescription.h>
#include <Weave/Core/WeaveError.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>

using namespace nl;
using namespace nl::Weave;
using namespace nl::Weave::Profiles::DeviceDescription;

void Abort()
{
    abort();
}

void TestAssert(bool assert, const char *msg)
{
    if (!assert)
    {

        printf("%s\n", msg);
        Abort();
    }
}

bool IsEqual(uint64_t v1, uint64_t v2)                         { return v1 == v2; }
bool IsEqual(uint32_t v1, uint32_t v2)                         { return v1 == v2; }
bool IsEqual(uint32_t v1, int32_t v2)                          { return v1 == (uint32_t)v2; }
bool IsEqual(int64_t v1, int64_t v2)                           { return v1 == v2; }
bool IsEqual(int32_t v1, int32_t v2)                           { return v1 == v2; }
bool IsEqual(int32_t v1, uint32_t v2)                          { return v1 == (int32_t)v2; }
bool IsEqual(const char *s1, const char *s2)                   { return s1 == s2 || strcmp(s1, s2) == 0; }
bool IsEqual(const uint8_t *s1, const uint8_t *s2, size_t len) { return s1 == s2 || memcmp(s1, s2, len) == 0; }

#define AssertValue(FIELD, VALUE)                                       \
do {                                                                    \
    if (!IsEqual(FIELD, VALUE))                                         \
    {                                                                   \
        printf("%s:%d : Invalid " #FIELD "\n", __FUNCTION__, __LINE__); \
        Abort();                                                        \
    }                                                                   \
} while (0)

#define AssertByteArrayValue(FIELD, EXPECTED)                           \
do {                                                                    \
    if (sizeof(FIELD) != sizeof(EXPECTED) ||                            \
        memcmp(FIELD, EXPECTED, sizeof(EXPECTED)) != 0)                 \
    {                                                                   \
        printf("%s:%d : Invalid " #FIELD "\n", __FUNCTION__, __LINE__); \
        Abort();                                                        \
    }                                                                   \
} while (0)




void TestTextDecoding(void)
{
    WeaveDeviceDescriptor devDesc;
    const char *textDevDesc1 = "1S:01AA01AB5011003W$";
    const char *textDevDesc2 = "1V:235A$P:1$R:1$D:140914$S:05BA01AC0313003G$L:18B43000000A91B3$W:18B43001D183$I:TOPAZZZ-91B3$C:07KP74$";
    const char *textDevDesc3 = "1V:235A$P:13$R:1$D:160805$S:15AA01ZZ01160101$E:18B4300400000101$";

    devDesc.Clear();
    WeaveDeviceDescriptor::DecodeText(textDevDesc1, strlen(textDevDesc1), devDesc);
    AssertValue(devDesc.VendorId, kWeaveVendor_NestLabs);
    AssertValue(devDesc.SerialNumber, "01AA01AB5011003W");
    AssertValue(devDesc.ManufacturingDate.Year, 2011);
    AssertValue(devDesc.ManufacturingDate.Month, 12);
    AssertValue(devDesc.ManufacturingDate.Day, 4);

    devDesc.Clear();
    WeaveDeviceDescriptor::DecodeText(textDevDesc2, strlen(textDevDesc2), devDesc);
    AssertValue(devDesc.VendorId, kWeaveVendor_NestLabs);
    AssertValue(devDesc.ProductId, 1);
    AssertValue(devDesc.ProductRevision, 1);
    AssertValue(devDesc.SerialNumber, "05BA01AC0313003G");
    AssertValue(devDesc.ManufacturingDate.Year, 2014);
    AssertValue(devDesc.ManufacturingDate.Month, 9);
    AssertValue(devDesc.ManufacturingDate.Day, 14);
    {
        uint8_t expected[] = { 0x18, 0xB4, 0x30, 0x00, 0x00, 0x0A, 0x91, 0xB3 };
        AssertByteArrayValue(devDesc.Primary802154MACAddress, expected);
    }
    {
        uint8_t expected[] = { 0x18, 0xB4, 0x30, 0x01, 0xD1, 0x83 };
        AssertByteArrayValue(devDesc.PrimaryWiFiMACAddress, expected);
    }
    AssertValue(devDesc.RendezvousWiFiESSID, "TOPAZZZ-91B3");
    AssertValue(devDesc.PairingCode, "07KP74");

    devDesc.Clear();
    WeaveDeviceDescriptor::DecodeText(textDevDesc3, strlen(textDevDesc3), devDesc);
    AssertValue(devDesc.VendorId, kWeaveVendor_NestLabs);
    AssertValue(devDesc.ProductId, 0x0013);
    AssertValue(devDesc.ProductRevision, 1);
    AssertValue(devDesc.SerialNumber, "15AA01ZZ01160101");
    AssertValue(devDesc.ManufacturingDate.Year, 2016);
    AssertValue(devDesc.ManufacturingDate.Month, 8);
    AssertValue(devDesc.ManufacturingDate.Day, 5);
    AssertValue(devDesc.DeviceId, 0x18B4300400000101UL);
}

void TestTextEncoding(void)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor devDesc;
    char textDevDesc[256];
    uint32_t textDevDescLen;

    devDesc.Clear();
    devDesc.VendorId = kWeaveVendor_NestLabs;
    devDesc.ProductId = 1;
    devDesc.ProductRevision = 1;
    strcpy(devDesc.SerialNumber, "08712459723451234");
    devDesc.ManufacturingDate.Year = 2014;
    devDesc.ManufacturingDate.Month = 9;
    devDesc.ManufacturingDate.Day = 14;
    {
        uint8_t value[] = { 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10 };
        memcpy(devDesc.Primary802154MACAddress, value, sizeof(devDesc.Primary802154MACAddress));
    }
    {
        uint8_t value[] = { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB };
        memcpy(devDesc.PrimaryWiFiMACAddress, value, sizeof(devDesc.PrimaryWiFiMACAddress));
    }
    strcpy(devDesc.RendezvousWiFiESSID, "NEST-91B3");
    strcpy(devDesc.PairingCode, "NSH923");

    err = WeaveDeviceDescriptor::EncodeText(devDesc, textDevDesc, sizeof(textDevDesc), textDevDescLen);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::EncodeText() returned error");

    {
        const char *expected = "1V:235A$P:1$R:1$D:140914$S:08712459723451234$L:FEDCBA9876543210$W:0123456789AB$I:NEST-91B3$C:NSH923$";
        TestAssert(strcmp(expected, textDevDesc) == 0, "Invalid text device descriptor");
    }

    devDesc.Clear();
    devDesc.VendorId = kWeaveVendor_NestLabs;
    devDesc.ProductId = 0x0013;
    devDesc.ProductRevision = 1;
    strcpy(devDesc.SerialNumber, "15AA01ZZ01160101");
    devDesc.ManufacturingDate.Year = 2016;
    devDesc.ManufacturingDate.Month = 8;
    devDesc.ManufacturingDate.Day = 5;
    devDesc.DeviceId = 0x18B4300400000101UL;

    err = WeaveDeviceDescriptor::EncodeText(devDesc, textDevDesc, sizeof(textDevDesc), textDevDescLen);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::EncodeText() returned error");

    {
        const char *expected = "1V:235A$P:13$R:1$D:160805$S:15AA01ZZ01160101$E:18B4300400000101$";
        TestAssert(strcmp(expected, textDevDesc) == 0, "Invalid text device descriptor");
    }
}

static uint8_t tlvDevDesc1[] =
{
    0xd5, 0x00, 0x00, 0x0e, 0x00, 0x01, 0x00, 0x25, 0x00, 0x5a, 0x23, 0x24, 0x01, 0x01, 0x24, 0x02,
    0x01, 0x25, 0x03, 0xce, 0x44, 0x2c, 0x04, 0x10, 0x30, 0x35, 0x42, 0x41, 0x30, 0x31, 0x41, 0x43,
    0x30, 0x33, 0x31, 0x33, 0x30, 0x30, 0x33, 0x47, 0x30, 0x05, 0x08, 0x18, 0xb4, 0x30, 0x00, 0x00,
    0x0a, 0x91, 0xb3, 0x30, 0x06, 0x06, 0x18, 0xb4, 0x30, 0x01, 0xd1, 0x83, 0x2c, 0x07, 0x0c, 0x54,
    0x4f, 0x50, 0x41, 0x5a, 0x5a, 0x5a, 0x2d, 0x39, 0x31, 0x42, 0x33, 0x2c, 0x08, 0x06, 0x30, 0x37,
    0x4b, 0x50, 0x37, 0x34, 0x18
};

static uint8_t tlvDevDesc2[] =
{
    0xd5, 0x00, 0x00, 0x0e, 0x00, 0x01, 0x00, 0x25, 0x00, 0x5a, 0x23, 0x24, 0x01, 0x13, 0x24, 0x02,
    0x01, 0x25, 0x03, 0x3c, 0x1a, 0x2c, 0x04, 0x10, 0x31, 0x35, 0x41, 0x41, 0x30, 0x31, 0x5a, 0x5a,
    0x30, 0x31, 0x31, 0x36, 0x30, 0x31, 0x30, 0x31, 0x27, 0x0a, 0x01, 0x01, 0x00, 0x00, 0x04, 0x30,
    0xb4, 0x18, 0x18
};

void TestTLVDecoding(void)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor devDesc;

    devDesc.Clear();
    err = WeaveDeviceDescriptor::DecodeTLV(tlvDevDesc1, sizeof(tlvDevDesc1), devDesc);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::DecodeTLV() returned error");

    AssertValue(devDesc.VendorId, kWeaveVendor_NestLabs);
    AssertValue(devDesc.ProductId, 1);
    AssertValue(devDesc.ProductRevision, 1);
    AssertValue(devDesc.SerialNumber, "05BA01AC0313003G");
    AssertValue(devDesc.ManufacturingDate.Year, 2014);
    AssertValue(devDesc.ManufacturingDate.Month, 9);
    AssertValue(devDesc.ManufacturingDate.Day, 14);
    {
        uint8_t expected[] = { 0x18, 0xB4, 0x30, 0x00, 0x00, 0x0A, 0x91, 0xB3 };
        AssertByteArrayValue(devDesc.Primary802154MACAddress, expected);
    }
    {
        uint8_t expected[] = { 0x18, 0xB4, 0x30, 0x01, 0xD1, 0x83 };
        AssertByteArrayValue(devDesc.PrimaryWiFiMACAddress, expected);
    }
    AssertValue(devDesc.RendezvousWiFiESSID, "TOPAZZZ-91B3");
    AssertValue(devDesc.PairingCode, "07KP74");

    devDesc.Clear();
    err = WeaveDeviceDescriptor::DecodeTLV(tlvDevDesc2, sizeof(tlvDevDesc2), devDesc);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::DecodeTLV() returned error");

    AssertValue(devDesc.VendorId, kWeaveVendor_NestLabs);
    AssertValue(devDesc.ProductId, 0x0013);
    AssertValue(devDesc.ProductRevision, 1);
    AssertValue(devDesc.SerialNumber, "15AA01ZZ01160101");
    AssertValue(devDesc.ManufacturingDate.Year, 2016);
    AssertValue(devDesc.ManufacturingDate.Month, 8);
    AssertValue(devDesc.ManufacturingDate.Day, 5);
    AssertValue(devDesc.DeviceId, 0x18B4300400000101UL);
}

void TestTLVEncoding(void)
{
    WEAVE_ERROR err;
    WeaveDeviceDescriptor devDesc;
    uint8_t testDevDesc[128];
    uint32_t testDevDescLen;

    devDesc.Clear();
    devDesc.VendorId = kWeaveVendor_NestLabs;
    devDesc.ProductId = 1;
    devDesc.ProductRevision = 1;
    strcpy(devDesc.SerialNumber, "05BA01AC0313003G");
    devDesc.ManufacturingDate.Year = 2014;
    devDesc.ManufacturingDate.Month = 9;
    devDesc.ManufacturingDate.Day = 14;
    {
        uint8_t value[] = { 0x18, 0xB4, 0x30, 0x00, 0x00, 0x0A, 0x91, 0xB3 };
        memcpy(devDesc.Primary802154MACAddress, value, sizeof(devDesc.Primary802154MACAddress));
    }
    {
        uint8_t value[] = { 0x18, 0xB4, 0x30, 0x01, 0xD1, 0x83 };
        memcpy(devDesc.PrimaryWiFiMACAddress, value, sizeof(devDesc.PrimaryWiFiMACAddress));
    }
    strcpy(devDesc.RendezvousWiFiESSID, "TOPAZZZ-91B3");
    strcpy(devDesc.PairingCode, "07KP74");

    err = WeaveDeviceDescriptor::EncodeTLV(devDesc, testDevDesc, sizeof(testDevDesc), testDevDescLen);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::EncodeTLV() returned error");

    TestAssert(testDevDescLen == sizeof(tlvDevDesc1), "Invalid length returned by WeaveDeviceDescriptor::EncodeTLV()");
    TestAssert(memcmp(testDevDesc, tlvDevDesc1, sizeof(tlvDevDesc1)) == 0, "Invalid value returned by WeaveDeviceDescriptor::EncodeTLV()");

    devDesc.Clear();
    devDesc.VendorId = kWeaveVendor_NestLabs;
    devDesc.ProductId = 0x0013;
    devDesc.ProductRevision = 1;
    strcpy(devDesc.SerialNumber, "15AA01ZZ01160101");
    devDesc.ManufacturingDate.Year = 2016;
    devDesc.ManufacturingDate.Month = 8;
    devDesc.ManufacturingDate.Day = 5;
    devDesc.DeviceId = 0x18B4300400000101UL;

    err = WeaveDeviceDescriptor::EncodeTLV(devDesc, testDevDesc, sizeof(testDevDesc), testDevDescLen);
    TestAssert(err == WEAVE_NO_ERROR, "WeaveDeviceDescriptor::EncodeTLV() returned error");

    TestAssert(testDevDescLen == sizeof(tlvDevDesc2), "Invalid length returned by WeaveDeviceDescriptor::EncodeTLV()");
    TestAssert(memcmp(testDevDesc, tlvDevDesc2, sizeof(tlvDevDesc2)) == 0, "Invalid value returned by WeaveDeviceDescriptor::EncodeTLV()");
}

int main()
{
    TestTextDecoding();
    TestTextEncoding();
    TestTLVDecoding();
    TestTLVEncoding();
    printf("All tests passed\n");
    return 0;
}
