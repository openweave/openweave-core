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
 *      General Weave security support code.
 *
 */

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::ASN1;

bool IsSupportedCurve(uint32_t curveId)
{
    return false
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
           || curveId == kWeaveCurveId_secp160r1
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
           || curveId == kWeaveCurveId_prime192v1
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
           || curveId == kWeaveCurveId_secp224r1
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
           || curveId == kWeaveCurveId_prime256v1
#endif
           ;
}

bool IsCurveInSet(uint32_t curveId, uint8_t curveSet)
{
    uint8_t curveFlag;

    switch (curveId)
    {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    case kWeaveCurveId_secp160r1:
        curveFlag = kWeaveCurveSet_secp160r1;
        break;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    case kWeaveCurveId_prime192v1:
        curveFlag = kWeaveCurveSet_prime192v1;
        break;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    case kWeaveCurveId_secp224r1:
        curveFlag = kWeaveCurveSet_secp224r1;
        break;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1

#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    case kWeaveCurveId_prime256v1:
        curveFlag = kWeaveCurveSet_prime256v1;
        break;
#endif // WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1

    default:
        return false;
    }

    return (curveSet & curveFlag) != 0;
}

NL_DLL_EXPORT OID WeaveCurveIdToOID(uint32_t weaveCurveId)
{
    if ((weaveCurveId & kWeaveCurveId_VendorMask) != (kWeaveVendor_NestLabs << kWeaveCurveId_VendorShift))
        return kOID_Unknown;
    return (OID)ASN1::kOIDCategory_EllipticCurve | (OID)(kWeaveCurveId_CurveNumMask & weaveCurveId);
}

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
