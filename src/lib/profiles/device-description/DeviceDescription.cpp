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
 *      This file implements the Device Description profile, containing
 *      data semantics and methods to specify and query device specific
 *      characteristics that pertain to Weave.
 *
 *      The Device Description profile is used to communicate device
 *      specific characteristics between Weave nodes.  This information
 *      is communicated via the IdentifyRequest and IdentifyResponse
 *      message types, the former used to query for devices matching a
 *      filter, and the latter used to respond with a payload detailing
 *      some or all of the device specific characteristics.  Such
 *      characteristics include the device vendor, make and model, as
 *      well as network information including MAC addresses and connections.
 */

#include <string.h>
#include <ctype.h>

#include <Weave/Core/WeaveCore.h>
#include "DeviceDescription.h"
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Core/WeaveVendorIdentifiers.hpp>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/SerialNumberUtils.h>

using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Encoding;

namespace nl {
namespace Weave {
namespace Profiles {
namespace DeviceDescription {

enum
{
    kTextKey_VendorId                           = 'V',  // [ 1-4 hex digits ] Code identifying product vendor.
    kTextKey_ProductId                          = 'P',  // [ 1-4 hex digits ] Code identifying product.
    kTextKey_ProductRevision                    = 'R',  // [ 1-4 hex digits ] Code identifying product revision.
    kTextKey_ManufacturingDate                  = 'D',  // [ 4 or 6 decimal digits ] Calendar date of manufacture in YYMM or YYMMDD form.
    kTextKey_SerialNumber                       = 'S',  // [ 1-32 char string ] Device serial number.
    kTextKey_DeviceId                           = 'E',  // [ 8 hex digits ] Weave Device Id / device unique id.
    kTextKey_Primary802154MACAddress            = 'L',  // [ 8 hex digits ] MAC address for device's primary 802.15.4 interface.
    kTextKey_PrimaryWiFiMACAddress              = 'W',  // [ 6 hex digits ] MAC address for device's primary WiFi interface.
    kTextKey_RendezvousWiFiESSID                = 'I',  // [ 1-32 char string ] ESSID for device's WiFi rendezvous network.
    kTextKey_RendezvousWiFiESSIDSuffix          = 'H',  // [ 1-32 char string ] ESSID for device's WiFi rendezvous network.
    kTextKey_PairingCode                        = 'C',  // [ 6-16 char string ] The pairing code for the device.
    kTextKey_PairingCompatibilityVersionMajor   = 'J',  // [ 1-4 hex digits ] Pairing software compatibility major version.
    kTextKey_PairingCompatibilityVersionMinor   = 'N',  // [ 1-4 hex digits ] Pairing software compatibility minor version.

    kEncodingVersion                            = '1',
    kKeySeparator                               = ':',
    kValueTerminator                            = '$'
};


class TextDescriptorWriter
{
public:
    TextDescriptorWriter(char *buf, uint32_t bufSize)
    {
        mBuf = mWritePoint = buf;
        mBufEnd = buf + bufSize;
    }

    WEAVE_ERROR WriteHex(char fieldId, uint16_t val)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;
        uint16_t len;

        if (val < 0x0010)
            len = 1;
        else if (val < 0x0100)
            len = 2;
        else if (val < 0x1000)
            len = 3;
        else
            len = 4;

        VerifyOrExit(mWritePoint + len + 3 < mBufEnd, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

        *mWritePoint++ = fieldId;
        *mWritePoint++ = kKeySeparator;

        if (len == 4)
            *mWritePoint++ = HexDigit((val >> 12) & 0xF);
        if (len >= 3)
            *mWritePoint++ = HexDigit((val >> 8) & 0xF);
        if (len >= 2)
            *mWritePoint++ = HexDigit((val >> 4) & 0xF);
        *mWritePoint++ = HexDigit(val & 0xF);

        *mWritePoint++ = kValueTerminator;

    exit:
        return err;
    }

    WEAVE_ERROR WriteHex(char fieldId, const uint8_t *val, uint32_t valLen)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;

        VerifyOrExit(mWritePoint + valLen + 3 < mBufEnd, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

        *mWritePoint++ = fieldId;
        *mWritePoint++ = kKeySeparator;

        for (; valLen > 0; val++, valLen--)
        {
            *mWritePoint++ = HexDigit(*val >> 4);
            *mWritePoint++ = HexDigit(*val & 0xF);
        }

        *mWritePoint++ = kValueTerminator;

    exit:
        return err;
    }

    WEAVE_ERROR WriteString(char fieldId, const char *val)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;
        uint32_t len;

        len = strlen(val);

        VerifyOrExit(mWritePoint + len + 3 < mBufEnd, err = WEAVE_ERROR_BUFFER_TOO_SMALL);
        VerifyOrExit(strchr(val, '$') == NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

        *mWritePoint++ = fieldId;
        *mWritePoint++ = kKeySeparator;
        memcpy(mWritePoint, val, len);
        mWritePoint += len;

        *mWritePoint++ = kValueTerminator;

    exit:
        return err;
    }

    WEAVE_ERROR WriteDate(char fieldId, uint16_t year, uint8_t month, uint8_t day)
    {
        WEAVE_ERROR err = WEAVE_NO_ERROR;
        uint16_t len;

        len = (day != 0) ? 9 : 7;

        VerifyOrExit(mWritePoint + len < mBufEnd, err = WEAVE_ERROR_BUFFER_TOO_SMALL);

        *mWritePoint++ = fieldId;
        *mWritePoint++ = kKeySeparator;

        year -= 2000;
        VerifyOrExit(year < 100, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(month >= 1 && month <= 12, err = WEAVE_ERROR_INVALID_ARGUMENT);
        VerifyOrExit(day <= 31, err = WEAVE_ERROR_INVALID_ARGUMENT);

        *mWritePoint++ = '0' + (year / 10);
        *mWritePoint++ = '0' + (year % 10);

        *mWritePoint++ = '0' + (month / 10);
        *mWritePoint++ = '0' + (month % 10);

        if (day != 0)
        {
            *mWritePoint++ = '0' + (day / 10);
            *mWritePoint++ = '0' + (day % 10);
        }

        *mWritePoint++ = kValueTerminator;

    exit:
        return err;
    }

    WEAVE_ERROR WriteVersion()
    {
        if (mWritePoint + 1 > mBufEnd)
            return WEAVE_ERROR_INVALID_ARGUMENT;
        *mWritePoint++ = kEncodingVersion;
        return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR Finalize()
    {
        if (mWritePoint + 1 > mBufEnd)
            return WEAVE_ERROR_INVALID_ARGUMENT;
        *mWritePoint = 0;
        return WEAVE_NO_ERROR;
    }

    uint32_t GetLengthWritten()
    {
        return mWritePoint - mBuf;
    }

private:
    char *mBuf;
    char *mWritePoint;
    char *mBufEnd;

    char HexDigit(uint8_t val)
    {
        return (val < 10) ? '0' + val : 'A' + (val - 10);
    }
};

class TextDescriptorReader
{
public:
    TextDescriptorReader(const char *val, uint32_t valLen)
    {
        mVal = val;
        mValEnd = val + valLen;
        mReadPoint = val;

        while (mReadPoint < mValEnd && isspace(*mReadPoint))
            mReadPoint++;

        mFieldEnd = val;

        Version = (mReadPoint < mValEnd) ? *mReadPoint : 0;
        Key = 0;
    }

    char Version;
    char Key;

    WEAVE_ERROR Next()
    {
        mReadPoint = mFieldEnd + 1;

        while (mReadPoint < mValEnd && isspace(*mReadPoint))
            mReadPoint++;

        if (mReadPoint >= mValEnd)
        {
            Key = 0;
            return WEAVE_END_OF_INPUT;
        }

        mFieldEnd = (const char *)memchr(mReadPoint, '$', mValEnd - mReadPoint);
        if (mFieldEnd == NULL)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        Key = *mReadPoint;
        return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR ReadString(char *buf, uint32_t bufSize)
    {
        if (Key == 0)
            return WEAVE_ERROR_INCORRECT_STATE;
        uint32_t len = mFieldEnd - mReadPoint - 2;
        if (len > bufSize - 1)
            return WEAVE_ERROR_BUFFER_TOO_SMALL;
        memcpy(buf, mReadPoint + 2, len);
        buf[len] = 0;
        return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR ReadHex(uint16_t& val)
    {
        val = 0;
        if (Key == 0)
            return WEAVE_ERROR_INCORRECT_STATE;
        const char *p = mReadPoint + 2;
        if (p >= mFieldEnd)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        for (; p < mFieldEnd; p++)
        {
            int8_t digitVal = HexDigitValue(*p);
            if (digitVal < 0)
                return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
            val = (val << 4) | digitVal;
        }
        return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR ReadHex(uint8_t *buf, uint32_t bufLen)
    {
        if (Key == 0)
            return WEAVE_ERROR_INCORRECT_STATE;
        uint32_t len = mFieldEnd - mReadPoint - 2;
        if ((len & 1) != 0 || len / 2 != bufLen)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        for (const char *p = mReadPoint + 2; p < mFieldEnd; p += 2, buf++)
        {
            int8_t highDigitVal = HexDigitValue(p[0]);
            int8_t lowDigitVal = HexDigitValue(p[1]);
            if (highDigitVal < 0 || lowDigitVal < 0)
                return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
            *buf = (uint8_t)((highDigitVal << 4) | lowDigitVal);
        }
        return WEAVE_NO_ERROR;
    }

    WEAVE_ERROR ReadDate(uint16_t& year, uint8_t& month, uint8_t& day)
    {
        if (Key == 0)
            return WEAVE_ERROR_INCORRECT_STATE;
        uint32_t len = mFieldEnd - mReadPoint - 2;
        if (len != 4 && len != 6)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        int8_t v = DecimalDigitPairValue(mReadPoint[2], mReadPoint[3]);
        if (v < 0)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        year = 2000 + v;
        v = DecimalDigitPairValue(mReadPoint[4], mReadPoint[5]);
        if (v < 1 || v > 12)
            return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
        month = v;
        if (len == 6)
        {
            v = DecimalDigitPairValue(mReadPoint[6], mReadPoint[7]);
            if (v < 1 || v > 31)
                return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;
            day = v;
        }
        else
            day = 0;
        return WEAVE_NO_ERROR;
    }

private:
    const char *mVal;
    const char *mValEnd;
    const char *mReadPoint;
    const char *mFieldEnd;

    inline int8_t HexDigitValue(char digit)
    {
        if (digit >= '0' && digit <= '9')
            return digit - '0';
        if (digit >= 'A' && digit <= 'F')
            return (digit - 'A') + 10;
        if (digit >= 'a' && digit <= 'f')
            return (digit - 'a') + 10;
        return -1;
    }

    inline int8_t DecimalDigitPairValue(char digit1, char digit2)
    {
        if (digit1 < '0' || digit1 > '9' || digit2 < '0' || digit2 > '9')
            return -1;
        return (digit1 - '0') * 10 + (digit2 - '0');
    }
};


WeaveDeviceDescriptor::WeaveDeviceDescriptor()
{
    memset(this, 0, sizeof(*this));
}

/**
 * Clears the device description
 */
void WeaveDeviceDescriptor::Clear()
{
    memset(this, 0, sizeof(*this));
}

/**
 * Encodes the provided device descriptor as text written to the supplied buffer.
 *
 * @param[in]   desc            A reference to the Weave Device Descriptor to encode.
 * @param[out]  buf             A pointer to a buffer where the encoded text will be written.
 * @param[in]   bufLen          The length of the supplied buffer.
 * @param[out]  outEncodedLen   A reference to the length variable that will be overwritten
 *                              with the number of characters written to the buffer.
 *
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL    If the supplied buffer is too small for the generated
 *                                          text description.
 * @retval #WEAVE_ERROR_INVALID_ARGUMENT    If a descriptor field is invalid.
 * @retval #WEAVE_NO_ERROR                  On success.
 */
WEAVE_ERROR WeaveDeviceDescriptor::EncodeText(const WeaveDeviceDescriptor& desc, char *buf, uint32_t bufLen, uint32_t& outEncodedLen)
{
    WEAVE_ERROR err;
    TextDescriptorWriter writer(buf, bufLen);

    err = writer.WriteVersion();
    SuccessOrExit(err);

    if (desc.VendorId != 0)
    {
        err = writer.WriteHex(kTextKey_VendorId, desc.VendorId);
        SuccessOrExit(err);
    }

    if (desc.ProductId != 0)
    {
        err = writer.WriteHex(kTextKey_ProductId, desc.ProductId);
        SuccessOrExit(err);
    }

    if (desc.ProductRevision != 0)
    {
        err = writer.WriteHex(kTextKey_ProductRevision, desc.ProductRevision);
        SuccessOrExit(err);
    }

    if (desc.ManufacturingDate.Year != 0 && desc.ManufacturingDate.Month != 0)
    {
        err = writer.WriteDate(kTextKey_ManufacturingDate, desc.ManufacturingDate.Year, desc.ManufacturingDate.Month, desc.ManufacturingDate.Day);
        SuccessOrExit(err);
    }

    if (desc.SerialNumber[0] != 0)
    {
        err = writer.WriteString(kTextKey_SerialNumber, desc.SerialNumber);
        SuccessOrExit(err);
    }

    if (desc.DeviceId != 0)
    {
        uint64_t val = BigEndian::HostSwap64(desc.DeviceId);
        err = writer.WriteHex(kTextKey_DeviceId, (const uint8_t *)&val, sizeof(val));
        SuccessOrExit(err);
    }

    if (!IsZeroBytes(desc.Primary802154MACAddress, sizeof(desc.Primary802154MACAddress)))
    {
        err = writer.WriteHex(kTextKey_Primary802154MACAddress, desc.Primary802154MACAddress, sizeof(desc.Primary802154MACAddress));
        SuccessOrExit(err);
    }

    if (!IsZeroBytes(desc.PrimaryWiFiMACAddress, sizeof(desc.PrimaryWiFiMACAddress)))
    {
        err = writer.WriteHex(kTextKey_PrimaryWiFiMACAddress, desc.PrimaryWiFiMACAddress, sizeof(desc.PrimaryWiFiMACAddress));
        SuccessOrExit(err);
    }

    if (desc.RendezvousWiFiESSID[0] != 0)
    {
        const char fieldId = ((desc.Flags & kFlag_IsRendezvousWiFiESSIDSuffix) != 0)
                ? kTextKey_RendezvousWiFiESSIDSuffix
                : kTextKey_RendezvousWiFiESSID;

        err = writer.WriteString(fieldId, desc.RendezvousWiFiESSID);
        SuccessOrExit(err);
    }

    if (desc.PairingCode[0] != 0)
    {
        err = writer.WriteString(kTextKey_PairingCode, desc.PairingCode);
        SuccessOrExit(err);
    }

    if (desc.PairingCompatibilityVersionMajor != 0)
    {
        err = writer.WriteHex(kTextKey_PairingCompatibilityVersionMajor, desc.PairingCompatibilityVersionMajor);
        SuccessOrExit(err);
    }

    if (desc.PairingCompatibilityVersionMinor != 0)
    {
        err = writer.WriteHex(kTextKey_PairingCompatibilityVersionMinor, desc.PairingCompatibilityVersionMinor);
        SuccessOrExit(err);
    }

    err = writer.Finalize();
    SuccessOrExit(err);

    outEncodedLen = writer.GetLengthWritten();

exit:
    return err;
}

/**
 * Encodes the provided device descriptor as Weave TLV written to the supplied buffer.
 *
 * @param[in]   desc            A reference to the Weave Device Descriptor to encode.
 * @param[out]  buf             A pointer to a buffer where the encoded text will be written.
 * @param[in]   bufLen          The length of the supplied buffer.
 * @param[out]  outEncodedLen   A reference to the length variable that will be overwritten
 *                              with the number of characters written to the buffer.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 * @retval other            Other Weave or platform-specific error codes indicating that an error
 *                          occurred preventing the encoding of the TLV.
 */
WEAVE_ERROR WeaveDeviceDescriptor::EncodeTLV(const WeaveDeviceDescriptor& desc, uint8_t *buf, uint32_t bufLen, uint32_t& outEncodedLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;

    writer.Init(buf, bufLen);

    err = EncodeTLV(desc, writer);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    outEncodedLen = writer.GetLengthWritten();

exit:
    return err;
}

/**
 * Encodes the provided device descriptor as Weave TLV written using the provided
 * pre-initialized TLVWriter object.  This is used to add the device description to
 * larger TLV output.
 *
 * @param[in]   desc        A reference to the Weave Device Descriptor to encode.
 * @param[in]   writer      A reference to the pre-initialized TLVWriter object to be used.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 * @retval other            Other Weave or platform-specific error codes indicating that an error
 *                          occurred preventing the encoding of the TLV.
 */
WEAVE_ERROR WeaveDeviceDescriptor::EncodeTLV(const WeaveDeviceDescriptor& desc, nl::Weave::TLV::TLVWriter& writer)
{
    WEAVE_ERROR err;
    TLVType outerContainer;

    err = writer.StartContainer(ProfileTag(kWeaveProfile_DeviceDescription, kTag_WeaveDeviceDescriptor), kTLVType_Structure, outerContainer);
    SuccessOrExit(err);

    if (desc.VendorId != 0)
    {
        err = writer.Put(ContextTag(kTag_VendorId), desc.VendorId);
        SuccessOrExit(err);
    }

    if (desc.ProductId != 0)
    {
        err = writer.Put(ContextTag(kTag_ProductId), desc.ProductId);
        SuccessOrExit(err);
    }

    if (desc.ProductRevision != 0)
    {
        err = writer.Put(ContextTag(kTag_ProductRevision), desc.ProductRevision);
        SuccessOrExit(err);
    }

    if (desc.ManufacturingDate.Year != 0 && desc.ManufacturingDate.Month != 0)
    {
        uint16_t encodedDate;
        err = EncodeManufacturingDate(desc.ManufacturingDate.Year, desc.ManufacturingDate.Month, desc.ManufacturingDate.Day, encodedDate);
        SuccessOrExit(err);
        err = writer.Put(ContextTag(kTag_ManufacturingDate), encodedDate);
        SuccessOrExit(err);
    }

    if (desc.SerialNumber[0] != 0)
    {
        err = writer.PutString(ContextTag(kTag_SerialNumber), desc.SerialNumber);
        SuccessOrExit(err);
    }

    if (!IsZeroBytes(desc.Primary802154MACAddress, sizeof(desc.Primary802154MACAddress)))
    {
        err = writer.PutBytes(ContextTag(kTag_Primary802154MACAddress), desc.Primary802154MACAddress, sizeof(desc.Primary802154MACAddress));
        SuccessOrExit(err);
    }

    if (!IsZeroBytes(desc.PrimaryWiFiMACAddress, sizeof(desc.PrimaryWiFiMACAddress)))
    {
        err = writer.PutBytes(ContextTag(kTag_PrimaryWiFiMACAddress), desc.PrimaryWiFiMACAddress, sizeof(desc.PrimaryWiFiMACAddress));
        SuccessOrExit(err);
    }

    if (desc.RendezvousWiFiESSID[0] != 0)
    {
        const uint64_t tag = ((desc.Flags & kFlag_IsRendezvousWiFiESSIDSuffix) != 0)
                ? ContextTag(kTag_RendezvousWiFiESSIDSuffix)
                : ContextTag(kTag_RendezvousWiFiESSID);

        err = writer.PutString(tag, desc.RendezvousWiFiESSID);
        SuccessOrExit(err);
    }

    if (desc.PairingCode[0] != 0)
    {
        err = writer.PutString(ContextTag(kTag_PairingCode), desc.PairingCode);
        SuccessOrExit(err);
    }

    if (desc.DeviceId != 0)
    {
        err = writer.Put(ContextTag(kTag_DeviceId), desc.DeviceId);
        SuccessOrExit(err);
    }

    if (desc.FabricId != 0)
    {
        err = writer.Put(ContextTag(kTag_FabricId), desc.FabricId);
        SuccessOrExit(err);
    }

    if (desc.SoftwareVersion[0] != 0)
    {
        err = writer.PutString(ContextTag(kTag_SoftwareVersion), desc.SoftwareVersion);
        SuccessOrExit(err);
    }

    if (desc.PairingCompatibilityVersionMajor != 0)
    {
        err = writer.Put(ContextTag(kTag_PairingCompatibilityVersionMajor), desc.PairingCompatibilityVersionMajor);
        SuccessOrExit(err);
    }

    if (desc.PairingCompatibilityVersionMinor != 0)
    {
        err = writer.Put(ContextTag(kTag_PairingCompatibilityVersionMinor), desc.PairingCompatibilityVersionMinor);
        SuccessOrExit(err);
    }

    if ((desc.DeviceFeatures & kFeature_HomeAlarmLinkCapable) != 0)
    {
        err = writer.PutBoolean(ContextTag(kTag_DeviceFeature_HomeAlarmLinkCapable), true);
        SuccessOrExit(err);
    }

    if ((desc.DeviceFeatures & kFeature_LinePowered) != 0)
    {
        err = writer.PutBoolean(ContextTag(kTag_DeviceFeature_LinePowered), true);
        SuccessOrExit(err);
    }

    err = writer.EndContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * Decodes the contents of the provided data buffer into a Weave Device Descriptor object.
 *
 * @param[in]   data            A pointer to a buffer containing text or TLV encoded Weave Device
 *                              Descriptor data.
 * @param[in]   dataLen         The length of the provided buffer.
 * @param[out]  outDesc         A reference to the Device Descriptor object to be populated.
 *
 * @retval #WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR   If the provided buffer is invalid.
 * @retval #WEAVE_NO_ERROR                          On success.
 * @retval other                                    Other Weave or platform-specific error codes indicating that an error
 *                                                  occurred preventing the decoding of the TLV.
 */
WEAVE_ERROR WeaveDeviceDescriptor::Decode(const uint8_t *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc)
{
    if (dataLen == 0)
        return WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR;

    // Automatically detect a TLV-encoded descriptor by looking for the TLV structure encoding.  A proper TLV-encoded
    // device descriptor will begin with a structure having either a fully-qualified or implicit profile-specific tag
    // of 0000000E:1.  E.g.:
    //
    //     Fully-qualified: D5 00 00 0E 00 01 00
    //     Implicit:        95 01 00
    //
    if ((dataLen > 3 && data[0] == 0x95 && data[1] == 0x01 && data[2] == 0x00) ||
        (dataLen > 7 && data[0] == 0xD5 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x0E && data[4] == 0x00 && data[5] == 0x01 && data[6] == 0x00))
        return DecodeTLV(data, dataLen, outDesc);

    // If the descriptor is not TLV-encoded, assume its in text format.
    return DecodeText((const char *)data, dataLen, outDesc);
}

/**
 * Decodes the contents of the provided text data buffer into a Weave Device Descriptor object.
 *
 * @param[in]   data            A pointer to a buffer containing text encoded Weave Device
 *                              Descriptor data.
 * @param[in]   dataLen         The length of the provided buffer.
 * @param[out]  outDesc         A reference to the Device Descriptor object to be populated.
 *
 * @retval #WEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION If the encoded data version is
 *                                                  unsupported.
 * @retval #WEAVE_ERROR_INVALID_DEVICE_DESCRIPTOR   If the encoded data is not formatted correctly.
 * @retval #WEAVE_ERROR_INCORRECT_STATE             If an inconsistent state is encountered by the
 *                                                  decoder.
 * @retval #WEAVE_ERROR_BUFFER_TOO_SMALL            If the end of the buffer is reached during
 *                                                  decoding.
 * @retval #WEAVE_NO_ERROR                          On success.
 */
WEAVE_ERROR WeaveDeviceDescriptor::DecodeText(const char *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc)
{
    WEAVE_ERROR err;
    TextDescriptorReader reader(data, dataLen);
    bool vendorIdIncluded = false;
    bool mfgDateIncluded = false;
    bool serialNumIncluded = false;

    if (reader.Version != kEncodingVersion)
        return WEAVE_ERROR_UNSUPPORTED_DEVICE_DESCRIPTOR_VERSION;

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        switch (reader.Key)
        {
        case kTextKey_VendorId:
            err = reader.ReadHex(outDesc.VendorId);
            SuccessOrExit(err);
            vendorIdIncluded = true;
            break;
        case kTextKey_ProductId:
            err = reader.ReadHex(outDesc.ProductId);
            SuccessOrExit(err);
            break;
        case kTextKey_ProductRevision:
            err = reader.ReadHex(outDesc.ProductRevision);
            SuccessOrExit(err);
            break;
        case kTextKey_ManufacturingDate:
            err = reader.ReadDate(outDesc.ManufacturingDate.Year, outDesc.ManufacturingDate.Month, outDesc.ManufacturingDate.Day);
            SuccessOrExit(err);
            mfgDateIncluded = true;
            break;
        case kTextKey_SerialNumber:
            err = reader.ReadString(outDesc.SerialNumber, sizeof(outDesc.SerialNumber));
            SuccessOrExit(err);
            serialNumIncluded = true;
            break;
        case kTextKey_DeviceId:
        {
            uint64_t val;
            err = reader.ReadHex((uint8_t *)&val, sizeof(val));
            SuccessOrExit(err);
            outDesc.DeviceId = BigEndian::HostSwap64(val);
            break;
        }
        case kTextKey_Primary802154MACAddress:
            err = reader.ReadHex(outDesc.Primary802154MACAddress, sizeof(outDesc.Primary802154MACAddress));
            SuccessOrExit(err);
            break;
        case kTextKey_PrimaryWiFiMACAddress:
            err = reader.ReadHex(outDesc.PrimaryWiFiMACAddress, sizeof(outDesc.PrimaryWiFiMACAddress));
            SuccessOrExit(err);
            break;
        case kTextKey_RendezvousWiFiESSID:
            outDesc.Flags &= ~kFlag_IsRendezvousWiFiESSIDSuffix;
            goto readRendezvousWiFiESSID;
        case kTextKey_RendezvousWiFiESSIDSuffix:
            outDesc.Flags |= kFlag_IsRendezvousWiFiESSIDSuffix;
        readRendezvousWiFiESSID:
            err = reader.ReadString(outDesc.RendezvousWiFiESSID, sizeof(outDesc.RendezvousWiFiESSID));
            SuccessOrExit(err);
            break;
        case kTextKey_PairingCode:
            err = reader.ReadString(outDesc.PairingCode, sizeof(outDesc.PairingCode));
            SuccessOrExit(err);
            break;
        case kTextKey_PairingCompatibilityVersionMajor:
            err = reader.ReadHex(outDesc.PairingCompatibilityVersionMajor);
            SuccessOrExit(err);
            break;
        case kTextKey_PairingCompatibilityVersionMinor:
            err = reader.ReadHex(outDesc.PairingCompatibilityVersionMinor);
            SuccessOrExit(err);
            break;
        default:
            // ignore unknown keys
            break;
        }
    }

    if (err == WEAVE_END_OF_INPUT)
        err = WEAVE_NO_ERROR;

    // Absence of a vendor id in a *text* device descriptor implies Nest.
    if (!vendorIdIncluded)
        outDesc.VendorId = nl::Weave::kWeaveVendor_NestLabs;

    // If the device was manufactured by Nest and the manufacturing date was not included in the descriptor,
    // then extract the date from the serial number if included.  Since the serial number only includes the
    // week of manufacture, this date will correspond to the first day (Sunday) of the corresponding week.
    //
    // Note: we ignore any errors and just leave the manufacturing date field empty if the serial number
    // can't be parsed.
    //
    if (outDesc.VendorId == nl::Weave::kWeaveVendor_NestLabs && !mfgDateIncluded && serialNumIncluded)
        ExtractManufacturingDateFromSerialNumber(outDesc.SerialNumber, outDesc.ManufacturingDate.Year,
            outDesc.ManufacturingDate.Month, outDesc.ManufacturingDate.Day);

exit:
    return err;
}

/**
 * Decodes the contents of the provided TLV data buffer into a Weave Device Descriptor object.
 *
 * @param[in]   data            A pointer to a buffer containing text encoded Weave Device
 *                              Descriptor data.
 * @param[in]   dataLen         The length of the provided buffer.
 * @param[out]  outDesc         A reference to the Device Descriptor object to be populated.
 *
 * @retval #WEAVE_ERROR_WRONG_TLV_TYPE          If this is not Device Description TLV.
 * @retval #WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT  If more TLV data is encountered after the
 *                                              Device Description.
 * @retval #WEAVE_NO_ERROR                      On success.
 * @retval other                                Other Weave or platform-specific error codes indicating that an error
 *                                              occurred preventing the encoding of the TLV.
 */
WEAVE_ERROR WeaveDeviceDescriptor::DecodeTLV(const uint8_t *data, uint32_t dataLen, WeaveDeviceDescriptor& outDesc)
{
    WEAVE_ERROR err;
    TLVReader reader;

    reader.Init(data, dataLen);

    // Treat an implicit profile tag as specifying the Device Description profile.
    reader.ImplicitProfileId = kWeaveProfile_DeviceDescription;

    err = reader.Next();
    SuccessOrExit(err);

    VerifyOrExit(reader.GetTag() == ProfileTag(kWeaveProfile_DeviceDescription, kTag_WeaveDeviceDescriptor), err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = DecodeTLV(reader, outDesc);
    SuccessOrExit(err);

    err = reader.Next();
    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT);
    err = WEAVE_NO_ERROR;

exit:
    return err;
}

/**
 * Decodes the Device Description using the provided pre-initialized TLVReader.
 *
 * @param[in]   reader          A reference to the pre-initialized TLVReader.
 * @param[out]  outDesc         A reference to the Device Descriptor object to be populated.
 *
 * @retval #WEAVE_ERROR_INVALID_TLV_ELEMENT     If invalid Device Description information is found
 *                                              in the TLV data.
 * @retval #WEAVE_NO_ERROR                      On success.
 * @retval other                                Other Weave or platform-specific error codes indicating that an error
 *                                              occurred that prevented the decoding of the TLV.
 */
WEAVE_ERROR WeaveDeviceDescriptor::DecodeTLV(nl::Weave::TLV::TLVReader& reader, WeaveDeviceDescriptor& outDesc)
{
    WEAVE_ERROR err;
    TLVType outerContainer;

    outDesc.Clear();

    err = reader.EnterContainer(outerContainer);
    SuccessOrExit(err);

    while ((err = reader.Next()) == WEAVE_NO_ERROR)
    {
        uint64_t tag = reader.GetTag();
        if (tag == ContextTag(kTag_VendorId))
        {
            err = reader.Get(outDesc.VendorId);
            SuccessOrExit(err);
            VerifyOrExit(outDesc.VendorId != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_ProductId))
        {
            err = reader.Get(outDesc.ProductId);
            SuccessOrExit(err);
            VerifyOrExit(outDesc.ProductId != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_ProductRevision))
        {
            err = reader.Get(outDesc.ProductRevision);
            SuccessOrExit(err);
            VerifyOrExit(outDesc.ProductRevision != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_ManufacturingDate))
        {
            uint16_t encodedDate;
            err = reader.Get(encodedDate);
            SuccessOrExit(err);
            err = DecodeManufacturingDate(encodedDate, outDesc.ManufacturingDate.Year, outDesc.ManufacturingDate.Month, outDesc.ManufacturingDate.Day);
            SuccessOrExit(err);
        }
        else if (tag == ContextTag(kTag_SerialNumber))
        {
            err = reader.GetString(outDesc.SerialNumber, sizeof(outDesc.SerialNumber));
            SuccessOrExit(err);
            VerifyOrExit(outDesc.SerialNumber[0] != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_Primary802154MACAddress))
        {
            VerifyOrExit(reader.GetLength() == 8, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(outDesc.Primary802154MACAddress, sizeof(outDesc.Primary802154MACAddress));
            SuccessOrExit(err);
        }
        else if (tag == ContextTag(kTag_PrimaryWiFiMACAddress))
        {
            VerifyOrExit(reader.GetLength() == 6, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            err = reader.GetBytes(outDesc.PrimaryWiFiMACAddress, sizeof(outDesc.PrimaryWiFiMACAddress));
            SuccessOrExit(err);
        }
        else if (tag == ContextTag(kTag_RendezvousWiFiESSID) || tag == ContextTag(kTag_RendezvousWiFiESSIDSuffix))
        {
            err = reader.GetString(outDesc.RendezvousWiFiESSID, sizeof(outDesc.RendezvousWiFiESSID));
            SuccessOrExit(err);
            VerifyOrExit(outDesc.RendezvousWiFiESSID[0] != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
            if (tag == ContextTag(kTag_RendezvousWiFiESSID))
            {
                outDesc.Flags &= ~kFlag_IsRendezvousWiFiESSIDSuffix;
            }
            else
            {
                outDesc.Flags |= kFlag_IsRendezvousWiFiESSIDSuffix;
            }
        }
        else if (tag == ContextTag(kTag_PairingCode))
        {
            err = reader.GetString(outDesc.PairingCode, sizeof(outDesc.PairingCode));
            SuccessOrExit(err);
            VerifyOrExit(outDesc.PairingCode[0] != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_SoftwareVersion))
        {
            const uint8_t *swVer;
            uint32_t swVerLen = reader.GetLength();
            err = reader.GetDataPtr(swVer);
            SuccessOrExit(err);
            if (swVerLen > kMaxSoftwareVersionLength)
                swVerLen = kMaxSoftwareVersionLength;
            memcpy(outDesc.SoftwareVersion, swVer, swVerLen);
            outDesc.SoftwareVersion[swVerLen] = 0;
        }
        else if (tag == ContextTag(kTag_DeviceId))
        {
            err = reader.Get(outDesc.DeviceId);
            SuccessOrExit(err);
        }
        else if (tag == ContextTag(kTag_FabricId))
        {
            err = reader.Get(outDesc.FabricId);
            SuccessOrExit(err);
        }
        else if (tag == ContextTag(kTag_PairingCompatibilityVersionMajor))
        {
            err = reader.Get(outDesc.PairingCompatibilityVersionMajor);
            SuccessOrExit(err);
            VerifyOrExit(outDesc.PairingCompatibilityVersionMajor != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else if (tag == ContextTag(kTag_PairingCompatibilityVersionMinor))
        {
            err = reader.Get(outDesc.PairingCompatibilityVersionMinor);
            SuccessOrExit(err);
            VerifyOrExit(outDesc.PairingCompatibilityVersionMinor != 0, err = WEAVE_ERROR_INVALID_TLV_ELEMENT);
        }
        else {
            uint32_t flag;
            if (tag == ContextTag(kTag_DeviceFeature_HomeAlarmLinkCapable))
                flag = kFeature_HomeAlarmLinkCapable;
            else if (tag == ContextTag(kTag_DeviceFeature_LinePowered))
                flag = kFeature_LinePowered;
            else
                flag = 0;
            if (flag)
            {
                bool val;
                err = reader.Get(val);
                SuccessOrExit(err);
                if (val)
                    outDesc.DeviceFeatures |= flag;
            }
        }
        // Ignore unknown tags.
    }

    VerifyOrExit(err == WEAVE_END_OF_TLV, );

    err = reader.ExitContainer(outerContainer);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR WeaveDeviceDescriptor::EncodeManufacturingDate(uint16_t year, uint8_t month, uint8_t day, uint16_t& outEncodedDate)
{
    if (year < 2001 || year > 2099 || month < 1 || month > 12 || day > 31)
        return WEAVE_ERROR_INVALID_ARGUMENT;
    outEncodedDate =   (year - 2000)
                     + ((month - 1) * 100)
                     + (day * 12 * 100);
    return WEAVE_NO_ERROR;
}

WEAVE_ERROR WeaveDeviceDescriptor::DecodeManufacturingDate(uint16_t encodedDate, uint16_t& outYear, uint8_t& outMonth, uint8_t& outDay)
{
    outYear = (encodedDate % 100) + 2000;
    outMonth = ((encodedDate / 100) % 12) + 1;
    outDay = (encodedDate / 1200);
    if (outDay > 31)
        return WEAVE_ERROR_INVALID_ARGUMENT;
    return WEAVE_NO_ERROR;
}

/**
 * Check if the specified buffer contains only zeros.
 *
 * @param[in]   buf         A pointer to a buffer.
 * @param[in]   len         The length of the buffer.
 *
 * @retval  TRUE    If the buffer contains only zeros.
 * @retval  FALSE   If the buffer contains any non-zero values.
 */
bool WeaveDeviceDescriptor::IsZeroBytes(const uint8_t *buf, uint32_t len)
{
    for (; len > 0; len--, buf++)
        if (*buf != 0)
            return false;
    return true;
}

/**
 * Encodes this IdentifyRequestMessage object into the provided Inet buffer.
 *
 * @param[inout]    msgBuf      A pointer to the Inet buffer to write the Identify
 *                              Request message to.
 *
 * @retval  #WEAVE_NO_ERROR unconditionally.
 */
WEAVE_ERROR IdentifyRequestMessage::Encode(PacketBuffer *msgBuf) const
{
    uint8_t *p;

    p = msgBuf->Start();
    LittleEndian::Write64(p, TargetFabricId);
    LittleEndian::Write32(p, TargetModes);
    LittleEndian::Write16(p, TargetVendorId);
    LittleEndian::Write16(p, TargetProductId);
    msgBuf->SetDataLength(p - msgBuf->Start());

    return WEAVE_NO_ERROR;
}

/**
 * Decodes an Identify Request message from an Inet buffer into the provided
 * IdentifyRequestMessage object.
 *
 * @param[in]       msgBuf          A pointer to the Inet buffer to decode the Identify Request
 *                                  message from.
 * @param[in]       msgDestNodeId   The destination node ID of the message being decoded.
 * @param[inout]    msg             A reference to the IdentifyRequestMessage to populate.
 *
 * @retval #WEAVE_ERROR_INVALID_MESSAGE_LENGTH  If the provided buffer is an invalid length.
 * @retval #WEAVE_NO_ERROR                      On success.
 */
WEAVE_ERROR IdentifyRequestMessage::Decode(PacketBuffer *msgBuf, uint64_t msgDestNodeId, IdentifyRequestMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    const uint8_t *p;

    VerifyOrExit(msgBuf->DataLength() == 16, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    memset(&msg, 0, sizeof(msg));
    p = msgBuf->Start();
    msg.TargetFabricId = LittleEndian::Read64(p);
    msg.TargetModes = LittleEndian::Read32(p);
    msg.TargetVendorId = LittleEndian::Read16(p);
    msg.TargetProductId = LittleEndian::Read16(p);
    msg.TargetDeviceId = msgDestNodeId;

exit:
    return err;
}

/**
 * Encodes this IdentifyResponseMessage object into the provided message buffer.
 *
 * @param[inout]    msgBuf      A pointer to the Inet buffer to write the Identify
 *                              Response message to.
 *
 * @retval #WEAVE_NO_ERROR  On success.
 * @retval other            Other Weave or platform-specific error codes indicating that an error
 *                          occurred preventing the encoding of the IdentifyResponseMessage.
 */
WEAVE_ERROR IdentifyResponseMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err;
    uint32_t msgLen;

    err = WeaveDeviceDescriptor::EncodeTLV(DeviceDesc, msgBuf->Start(), msgBuf->AvailableDataLength(), msgLen);
    if (err == WEAVE_NO_ERROR)
        msgBuf->SetDataLength(msgLen);

    return err;
}

/**
 * Decodes an Identify Response message from an Inet buffer into the provided
 * IdentifyResponseMessage object.
 *
 * @param[in]       msgBuf          A pointer to the Inet buffer to decode the Identify Request
 *                                  message from.
 * @param[out]      msg             A reference to the IdentifyRequestMessage to populate.
 *
 * @retval #WEAVE_ERROR_WRONG_TLV_TYPE          If this is not Device Description TLV.
 * @retval #WEAVE_ERROR_UNEXPECTED_TLV_ELEMENT  If more TLV data is encountered after the
 *                                              Device Description.
 * @retval #WEAVE_NO_ERROR                      On success.
 * @retval other                                Other Weave or platform-specific error codes indicating that an error
 *                                              occurred preventing the decoding of the IdentifyResponseMessage.
 */
WEAVE_ERROR IdentifyResponseMessage::Decode(PacketBuffer *msgBuf, IdentifyResponseMessage& msg)
{
    return WeaveDeviceDescriptor::DecodeTLV(msgBuf->Start(), msgBuf->DataLength(), msg.DeviceDesc);
}

IdentifyDeviceCriteria::IdentifyDeviceCriteria()
{
    Reset();
}

/**
 * Resets this Identify Device Criteria object to be least restrictive,
 * that is, matching any.
 */
void IdentifyDeviceCriteria::Reset()
{
    TargetFabricId = kTargetFabricId_Any;
    TargetModes = kTargetDeviceMode_Any;
    TargetVendorId = 0xFFFF; // Any vendor
    TargetProductId = 0xFFFF; // Any product
    TargetDeviceId = kAnyNodeId;
}

/**
 * Compare two fabric IDs to determine if they match (considering wildcard values).
 *
 * @param[in]   fabricId        The fabric ID to test.
 * @param[in]   targetFabricId  The fabric ID to test against.
 *
 * @retval TRUE     If the fabric ids match.
 * @retval FALSE    If the fabric ids do not match.
 */
NL_DLL_EXPORT bool MatchTargetFabricId(uint64_t fabricId, uint64_t targetFabricId)
{
    if (targetFabricId == kTargetFabricId_Any)
        return true;

    if (targetFabricId == kTargetFabricId_NotInFabric)
        return (fabricId == kFabricIdNotSpecified);

    if (targetFabricId == kTargetFabricId_AnyFabric)
        return (fabricId != kFabricIdNotSpecified);

    return (targetFabricId == fabricId);
}


} // namespace DeviceDescription
} // namespace Profiles
} // namespace Weave
} // namespace nl
