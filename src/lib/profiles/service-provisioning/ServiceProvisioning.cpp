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
 *      This file implements an object for a Weave Service Provisioning
 *      profile unsolicited initiator (client).
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "ServiceProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace ServiceProvisioning {

using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Encoding;
using namespace nl::Weave::TLV;

WEAVE_ERROR RegisterServicePairAccountMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgLen;
    uint8_t *p;

    msgLen = 2 + 2 + 2 + 2 + 8 + ServiceConfigLen + AccountIdLen + PairingTokenLen + PairingInitDataLen;
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_MESSAGE_TOO_LONG);

    p = msgBuf->Start();
    LittleEndian::Write16(p, AccountIdLen);
    LittleEndian::Write16(p, ServiceConfigLen);
    LittleEndian::Write16(p, PairingTokenLen);
    LittleEndian::Write16(p, PairingInitDataLen);
    LittleEndian::Write64(p, ServiceId);
    memcpy(p, AccountId, AccountIdLen); p += AccountIdLen;
    memcpy(p, ServiceConfig, ServiceConfigLen); p += ServiceConfigLen;
    memcpy(p, PairingToken, PairingTokenLen); p += PairingTokenLen;
    memcpy(p, PairingInitData, PairingInitDataLen);
    msgBuf->SetDataLength(msgLen);

exit:
    return err;
}

WEAVE_ERROR RegisterServicePairAccountMessage::Decode(PacketBuffer *msgBuf, RegisterServicePairAccountMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t dataLen = msgBuf->DataLength();
    const uint8_t *p = msgBuf->Start();

    VerifyOrExit(dataLen >= 2 + 2 + 2 + 2 + 8, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    msg.AccountIdLen = LittleEndian::Read16(p);
    msg.ServiceConfigLen = LittleEndian::Read16(p);
    msg.PairingTokenLen = LittleEndian::Read16(p);
    msg.PairingInitDataLen = LittleEndian::Read16(p);
    msg.ServiceId = LittleEndian::Read64(p);

    VerifyOrExit(dataLen == 2 + 2 + 2 + 2 + 8 + msg.AccountIdLen + msg.ServiceConfigLen + msg.PairingTokenLen + msg.PairingInitDataLen,
                 err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    msg.AccountId        = (const char *)p;
    msg.ServiceConfig    = p + msg.AccountIdLen;
    msg.PairingToken     = p + msg.AccountIdLen + msg.ServiceConfigLen;
    msg.PairingInitData  = p + msg.AccountIdLen + msg.ServiceConfigLen + msg.PairingTokenLen;

exit:
    return err;
}

WEAVE_ERROR UpdateServiceMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgLen;
    uint8_t *p;

    msgLen = 2 + 8 + ServiceConfigLen;
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_MESSAGE_TOO_LONG);

    p = msgBuf->Start();
    LittleEndian::Write16(p, ServiceConfigLen);
    LittleEndian::Write64(p, ServiceId);
    memcpy(p, ServiceConfig, ServiceConfigLen);
    msgBuf->SetDataLength(msgLen);

exit:
    return err;
}

WEAVE_ERROR UpdateServiceMessage::Decode(PacketBuffer *msgBuf, UpdateServiceMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t dataLen = msgBuf->DataLength();
    const uint8_t *p = msgBuf->Start();

    VerifyOrExit(dataLen >= 2 + 8, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    msg.ServiceConfigLen = LittleEndian::Read16(p);
    msg.ServiceId = LittleEndian::Read64(p);

    VerifyOrExit(dataLen == 2 + 8 + msg.ServiceConfigLen, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    msg.ServiceConfig = p;

exit:
    return err;
}

WEAVE_ERROR PairDeviceToAccountMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgLen;
    uint8_t *p;

    msgLen = 2 + 2 + 2 + 2 + 8 + 8 + AccountIdLen + PairingTokenLen + PairingInitDataLen + DeviceInitDataLen;
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_MESSAGE_TOO_LONG);

    p = msgBuf->Start();
    LittleEndian::Write16(p, AccountIdLen);
    LittleEndian::Write16(p, PairingTokenLen);
    LittleEndian::Write16(p, PairingInitDataLen);
    LittleEndian::Write16(p, DeviceInitDataLen);
    LittleEndian::Write64(p, ServiceId);
    LittleEndian::Write64(p, FabricId);
    memcpy(p, AccountId, AccountIdLen); p += AccountIdLen;
    memcpy(p, PairingToken, PairingTokenLen); p += PairingTokenLen;
    memcpy(p, PairingInitData, PairingInitDataLen); p += PairingInitDataLen;
    memcpy(p, DeviceInitData, DeviceInitDataLen);
    msgBuf->SetDataLength(msgLen);

exit:
    return err;
}

WEAVE_ERROR PairDeviceToAccountMessage::Decode(PacketBuffer *msgBuf, PairDeviceToAccountMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t dataLen = msgBuf->DataLength();
    const uint8_t *p = msgBuf->Start();

    VerifyOrExit(dataLen >= 2 + 2 + 2 + 2 + 8 + 8, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    msg.AccountIdLen = LittleEndian::Read16(p);
    msg.PairingTokenLen = LittleEndian::Read16(p);
    msg.PairingInitDataLen = LittleEndian::Read16(p);
    msg.DeviceInitDataLen = LittleEndian::Read16(p);
    msg.ServiceId = LittleEndian::Read64(p);
    msg.FabricId = LittleEndian::Read64(p);

    VerifyOrExit(dataLen == 2 + 2 + 2 + 2 + 8 + 8 + msg.AccountIdLen + msg.PairingTokenLen + msg.PairingInitDataLen + msg.DeviceInitDataLen,
                 err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    msg.AccountId        = (const char *)p;
    msg.PairingToken     = p + msg.AccountIdLen;
    msg.PairingInitData  = p + msg.AccountIdLen + msg.PairingTokenLen;
    msg.DeviceInitData   = p + msg.AccountIdLen + msg.PairingTokenLen + msg.PairingInitDataLen;

exit:
    return err;
}

#if WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN
WEAVE_ERROR IFJServiceFabricJoinMessage::Encode(PacketBuffer *msgBuf)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint32_t msgLen;
    uint8_t *p;

    msgLen = 2 + 8 + 8 + DeviceInitDataLen;
    VerifyOrExit(msgBuf->AvailableDataLength() >= msgLen, err = WEAVE_ERROR_MESSAGE_TOO_LONG);

    p = msgBuf->Start();
    LittleEndian::Write16(p, DeviceInitDataLen);
    LittleEndian::Write64(p, ServiceId);
    LittleEndian::Write64(p, FabricId);
    memcpy(p, DeviceInitData, DeviceInitDataLen);
    msgBuf->SetDataLength(msgLen);

exit:
    return err;
}

WEAVE_ERROR IFJServiceFabricJoinMessage::Decode(PacketBuffer *msgBuf, IFJServiceFabricJoinMessage& msg)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t dataLen = msgBuf->DataLength();
    const uint8_t *p = msgBuf->Start();

    VerifyOrExit(dataLen >= 2 + 8 + 8, err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);
    msg.DeviceInitDataLen = LittleEndian::Read16(p);
    msg.ServiceId = LittleEndian::Read64(p);
    msg.FabricId = LittleEndian::Read64(p);

    VerifyOrExit(dataLen == 2 + 8 + 8 + msg.DeviceInitDataLen,
                 err = WEAVE_ERROR_INVALID_MESSAGE_LENGTH);

    msg.DeviceInitData   = p;

exit:
    return err;
}
#endif // WEAVE_CONFIG_ENABLE_IFJ_SERVICE_FABRIC_JOIN

NL_DLL_EXPORT WEAVE_ERROR EncodeServiceConfig(WeaveCertificateSet& certSet, const char *dirHostName, uint16_t dirPort, uint8_t *outBuf, uint16_t& outLen)
{
    WEAVE_ERROR err;
    TLVWriter writer;

    writer.Init(outBuf, outLen);

    {
        TLVType containingType1;

        err = writer.StartContainer(ProfileTag(kWeaveProfile_ServiceProvisioning, kTag_ServiceConfig), kTLVType_Structure, containingType1);
        SuccessOrExit(err);

        {
            TLVType containingType2;

            err = writer.StartContainer(ContextTag(kTag_ServiceConfig_CACerts), kTLVType_Array, containingType2);
            SuccessOrExit(err);

            err = certSet.SaveCerts(writer, NULL, true);
            SuccessOrExit(err);

            err = writer.EndContainer(containingType2);
            SuccessOrExit(err);
        }

#if WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY
        {
            TLVType containingType3;

            err = writer.StartContainer(ContextTag(kTag_ServiceEndPoint), kTLVType_Structure, containingType3);
            SuccessOrExit(err);

            err = writer.Put(ContextTag(kTag_ServiceEndPoint_Id), (uint64_t)kServiceEndpoint_Directory);
            SuccessOrExit(err);

            {
                TLVType containingType4;

                err = writer.StartContainer(ContextTag(kTag_ServiceEndPoint_Addresses), kTLVType_Array, containingType4);
                SuccessOrExit(err);

                {
                    TLVType containingType5;

                    err = writer.StartContainer(AnonymousTag, kTLVType_Structure, containingType5);
                    SuccessOrExit(err);

                    err = writer.PutString(ContextTag(kTag_ServiceEndPointAddress_HostName), dirHostName);
                    SuccessOrExit(err);

                    if (dirPort != WEAVE_PORT)
                    {
                        err = writer.Put(ContextTag(kTag_ServiceEndPointAddress_Port), dirPort);
                        SuccessOrExit(err);
                    }

                    err = writer.EndContainer(containingType5);
                    SuccessOrExit(err);
                }

                err = writer.EndContainer(containingType4);
                SuccessOrExit(err);
            }

            err = writer.EndContainer(containingType3);
            SuccessOrExit(err);
        }
#endif // WEAVE_CONFIG_ENABLE_SERVICE_DIRECTORY

        err = writer.EndContainer(containingType1);
        SuccessOrExit(err);
    }

    err = writer.Finalize();
    SuccessOrExit(err);

    outLen = writer.GetLengthWritten();

exit:
    return err;
}


} // ServiceProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl
