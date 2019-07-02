/*
 *
 *    Copyright (c) 2019 Google LLC.
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
 *      This file implements a process to effect a functional test for
 *      the Weave Elliptic Curve (EC) mathematical interfaces.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ToolCommon.h"
#include <Weave/Core/WeaveConfig.h>

#include <sys/types.h>

// ============================================================
// Pamareters Declaration for Micro-ECC
// ============================================================

#if WEAVE_CONFIG_USE_MICRO_ECC

extern uint32_t sNIST_P192_ECPointS[2][6];
extern uint32_t sNIST_P192_ECPointT[2][6];
extern uint32_t sNIST_P192_ScalarD[];
extern uint32_t sNIST_P192_ScalarE[];
extern uint32_t sNIST_P192_ECPointRadd[2][6];
extern uint32_t sNIST_P192_ECPointRsub[2][6];
extern uint32_t sNIST_P192_ECPointRdbl[2][6];
extern uint32_t sNIST_P192_ECPointRmul[2][6];
extern uint32_t sNIST_P192_ECPointRjsm[2][6];

extern uint32_t sNIST_P224_ECPointS[2][7];
extern uint32_t sNIST_P224_ECPointT[2][7];
extern uint32_t sNIST_P224_ScalarD[];
extern uint32_t sNIST_P224_ScalarE[];
extern uint32_t sNIST_P224_ECPointRadd[2][7];
extern uint32_t sNIST_P224_ECPointRsub[2][7];
extern uint32_t sNIST_P224_ECPointRdbl[2][7];
extern uint32_t sNIST_P224_ECPointRmul[2][7];
extern uint32_t sNIST_P224_ECPointRjsm[2][7];

extern uint32_t sNIST_P256_ECPointS[2][8];
extern uint32_t sNIST_P256_ECPointT[2][8];
extern uint32_t sNIST_P256_ScalarD[];
extern uint32_t sNIST_P256_ScalarE[];
extern uint32_t sNIST_P256_ECPointRadd[2][8];
extern uint32_t sNIST_P256_ECPointRsub[2][8];
extern uint32_t sNIST_P256_ECPointRdbl[2][8];
extern uint32_t sNIST_P256_ECPointRmul[2][8];
extern uint32_t sNIST_P256_ECPointRjsm[2][8];

#endif // WEAVE_CONFIG_USE_MICRO_ECC

// ============================================================
// Pamareters Declaration for OpenSSL
// ============================================================

#if WEAVE_CONFIG_USE_OPENSSL_ECC

extern BIGNUM *NIST_P192_ECPointS_X();
extern BIGNUM *NIST_P192_ECPointS_Y();
extern BIGNUM *NIST_P192_ECPointT_X();
extern BIGNUM *NIST_P192_ECPointT_Y();
extern BIGNUM *NIST_P192_D();
extern BIGNUM *NIST_P192_E();
extern BIGNUM *NIST_P192_ECPointRadd_X();
extern BIGNUM *NIST_P192_ECPointRadd_Y();
extern BIGNUM *NIST_P192_ECPointRsub_X();
extern BIGNUM *NIST_P192_ECPointRsub_Y();
extern BIGNUM *NIST_P192_ECPointRdbl_X();
extern BIGNUM *NIST_P192_ECPointRdbl_Y();
extern BIGNUM *NIST_P192_ECPointRmul_X();
extern BIGNUM *NIST_P192_ECPointRmul_Y();
extern BIGNUM *NIST_P192_ECPointRjsm_X();
extern BIGNUM *NIST_P192_ECPointRjsm_Y();

extern BIGNUM *NIST_P224_ECPointS_X();
extern BIGNUM *NIST_P224_ECPointS_Y();
extern BIGNUM *NIST_P224_ECPointT_X();
extern BIGNUM *NIST_P224_ECPointT_Y();
extern BIGNUM *NIST_P224_D();
extern BIGNUM *NIST_P224_E();
extern BIGNUM *NIST_P224_ECPointRadd_X();
extern BIGNUM *NIST_P224_ECPointRadd_Y();
extern BIGNUM *NIST_P224_ECPointRsub_X();
extern BIGNUM *NIST_P224_ECPointRsub_Y();
extern BIGNUM *NIST_P224_ECPointRdbl_X();
extern BIGNUM *NIST_P224_ECPointRdbl_Y();
extern BIGNUM *NIST_P224_ECPointRmul_X();
extern BIGNUM *NIST_P224_ECPointRmul_Y();
extern BIGNUM *NIST_P224_ECPointRjsm_X();
extern BIGNUM *NIST_P224_ECPointRjsm_Y();

extern BIGNUM *NIST_P256_ECPointS_X();
extern BIGNUM *NIST_P256_ECPointS_Y();
extern BIGNUM *NIST_P256_ECPointT_X();
extern BIGNUM *NIST_P256_ECPointT_Y();
extern BIGNUM *NIST_P256_D();
extern BIGNUM *NIST_P256_E();
extern BIGNUM *NIST_P256_ECPointRadd_X();
extern BIGNUM *NIST_P256_ECPointRadd_Y();
extern BIGNUM *NIST_P256_ECPointRsub_X();
extern BIGNUM *NIST_P256_ECPointRsub_Y();
extern BIGNUM *NIST_P256_ECPointRdbl_X();
extern BIGNUM *NIST_P256_ECPointRdbl_Y();
extern BIGNUM *NIST_P256_ECPointRmul_X();
extern BIGNUM *NIST_P256_ECPointRmul_Y();
extern BIGNUM *NIST_P256_ECPointRjsm_X();
extern BIGNUM *NIST_P256_ECPointRjsm_Y();

#endif // WEAVE_CONFIG_USE_OPENSSL_ECC

// ============================================================
// End of Pamareters Declaration
// ============================================================

using namespace nl::Weave::Crypto;
using namespace nl::Weave::ASN1;
using namespace nl::Weave::Encoding;

// ============================================================
// EC Math Fanctiodns Declaration for Micro-ECC library
// ============================================================

#if WEAVE_CONFIG_USE_MICRO_ECC

/* Compares points: returns 1 if left == right */
#define uECC_point_equal(left, right, num_words) \
        uECC_vli_equal((left), (right), 2 * (num_words))

/* Points subtraction: result = left - right */
static void uECC_point_sub(uECC_word_t *result,
                           const uECC_word_t *left,
                           const uECC_word_t *right,
                           uECC_Curve curve)
{
    const wordcount_t num_words = uECC_curve_num_words(curve);

    /**
     * result = - right
     *    * result_x = right_x
     *    * result_y = curve_p - result_y
     */
    uECC_vli_set(result, right, num_words);
    uECC_vli_sub(result + num_words, uECC_curve_p(curve), right + num_words, num_words);
    /* result = left - right */
    uECC_point_add(result, left, result, curve);
}

/* Point double: result = 2 * point */
static void uECC_point_dbl(uECC_word_t *result,
                           const uECC_word_t *point,
                           uECC_Curve curve)
{
    uECC_word_t scalar[kuECC_MaxWordCount] = { 0x02 };

    /* result = 2 * point */
    uECC_point_mult(result, point, scalar, curve);
}

static uECC_Curve CurveOID2uECC_Curve(OID curveOID)
{
    switch (curveOID)
    {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP160R1
    case kOID_EllipticCurve_secp160r1:
        return uECC_secp160r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
    case kOID_EllipticCurve_prime192v1:
        return uECC_secp192r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
    case kOID_EllipticCurve_secp224r1:
        return uECC_secp224r1();
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
    case kOID_EllipticCurve_prime256v1:
        return uECC_secp256r1();
#endif
    default:
        return NULL;
    }
}

static bool TestECMath_PointAddition(OID curveOID, uint32_t iterationCounter)
{
    uECC_Curve curve;
    uECC_word_t *ecPointS;
    uECC_word_t *ecPointT;
    uECC_word_t *ecPointR_Expected;
    EccPoint ecPointR;

    curve = CurveOID2uECC_Curve(curveOID);
    if (curve == NULL)
    {
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointS = (uECC_word_t*)sNIST_P192_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P192_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P192_ECPointRadd;
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointS = (uECC_word_t*)sNIST_P224_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P224_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P224_ECPointRadd;
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointS = (uECC_word_t*)sNIST_P256_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P256_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P256_ECPointRadd;
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    // Main Loop: ecPointR = ecPointS + ecPointT
    for (uint i = 0; i < iterationCounter; i++)
        uECC_point_add(ecPointR, ecPointS, ecPointT, curve);

    // Compare Result ecPointR == ecPointR_Expected
    if (!uECC_point_equal(ecPointR, ecPointR_Expected, uECC_curve_num_words(curve)))
    {
        printf("\tERROR: MicroECC point addition test failed !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointSubtraction(OID curveOID, uint32_t iterationCounter)
{
    uECC_Curve curve;
    uECC_word_t *ecPointS;
    uECC_word_t *ecPointT;
    uECC_word_t *ecPointR_Expected;
    EccPoint ecPointR;

    curve = CurveOID2uECC_Curve(curveOID);
    if (curve == NULL)
    {
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointS = (uECC_word_t*)sNIST_P192_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P192_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P192_ECPointRsub;
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointS = (uECC_word_t*)sNIST_P224_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P224_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P224_ECPointRsub;
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointS = (uECC_word_t*)sNIST_P256_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P256_ECPointT;
        ecPointR_Expected = (uECC_word_t*)sNIST_P256_ECPointRsub;
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    // Main Loop: ecPointR = ecPointS - ecPointT
    for (uint i = 0; i < iterationCounter; i++)
    {
        /* ecPointR = ecPointS - ecPointT */
        uECC_point_sub(ecPointR, ecPointS, ecPointT, curve);
    }

    // Compare Result ecPointR == ecPointR_Expected
    if (!uECC_point_equal(ecPointR, ecPointR_Expected, uECC_curve_num_words(curve)))
    {
        printf("\tERROR: MicroECC point subtraction test failed !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointDouble(OID curveOID, uint32_t iterationCounter)
{
    uECC_Curve curve;
    uECC_word_t *ecPointS;
    uECC_word_t *ecPointR_Expected;
    EccPoint ecPointR;

    curve = CurveOID2uECC_Curve(curveOID);
    if (curve == NULL)
    {
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointS = (uECC_word_t*)sNIST_P192_ECPointS;
        ecPointR_Expected = (uECC_word_t*)sNIST_P192_ECPointRdbl;
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointS = (uECC_word_t*)sNIST_P224_ECPointS;
        ecPointR_Expected = (uECC_word_t*)sNIST_P224_ECPointRdbl;
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointS = (uECC_word_t*)sNIST_P256_ECPointS;
        ecPointR_Expected = (uECC_word_t*)sNIST_P256_ECPointRdbl;
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    // Main Loop: ecPointR = 2 * ecPointS
    for (uint i = 0; i < iterationCounter; i++)
        uECC_point_dbl(ecPointR, ecPointS, curve);

    // Compare Result ecPointR == ecPointRExpected
    if (!uECC_point_equal(ecPointR, ecPointR_Expected, uECC_curve_num_words(curve)))
    {
        printf("\tERROR: MicroECC point double test failed !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointMultiply(OID curveOID, uint32_t iterationCounter)
{
    uECC_Curve curve;
    uECC_word_t *ecPointS;
    uECC_word_t *scalarD;
    uECC_word_t *ecPointR_Expected;
    EccPoint ecPointR;

    curve = CurveOID2uECC_Curve(curveOID);
    if (curve == NULL)
    {
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointS = (uECC_word_t*)sNIST_P192_ECPointS;
        scalarD = (uECC_word_t*)sNIST_P192_ScalarD;
        ecPointR_Expected = (uECC_word_t*)sNIST_P192_ECPointRmul;
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointS = (uECC_word_t*)sNIST_P224_ECPointS;
        scalarD = (uECC_word_t*)sNIST_P224_ScalarD;
        ecPointR_Expected = (uECC_word_t*)sNIST_P224_ECPointRmul;
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointS = (uECC_word_t*)sNIST_P256_ECPointS;
        scalarD = (uECC_word_t*)sNIST_P256_ScalarD;
        ecPointR_Expected = (uECC_word_t*)sNIST_P256_ECPointRmul;
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    // Main Loop: ecPointR = ScalarD * ecPointS
    for (uint i = 0; i < iterationCounter; i++)
        uECC_point_mult(ecPointR, ecPointS, scalarD, curve);

    // Compare Result ecPointR == ecPointRExpected
    if (!uECC_point_equal(ecPointR, ecPointR_Expected, uECC_curve_num_words(curve)))
    {
        printf("\tERROR: MicroECC point multiply test failed !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_JointScalarMultiply(OID curveOID, uint32_t iterationCounter)
{
    uECC_Curve curve;
    uECC_word_t *ecPointS;
    uECC_word_t *ecPointT;
    uECC_word_t *scalarD;
    uECC_word_t *scalarE;
    uECC_word_t *ecPointR_Expected;
    EccPoint ecPointR;
    EccPoint ecPointTmp1;
    EccPoint ecPointTmp2;

    curve = CurveOID2uECC_Curve(curveOID);
    if (curve == NULL)
    {
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    switch (curveOID) {

    case kOID_EllipticCurve_prime192v1:
        ecPointS = (uECC_word_t*)sNIST_P192_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P192_ECPointT;
        scalarD = (uECC_word_t*)sNIST_P192_ScalarD;
        scalarE = (uECC_word_t*)sNIST_P192_ScalarE;
        ecPointR_Expected = (uECC_word_t*)sNIST_P192_ECPointRjsm;
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointS = (uECC_word_t*)sNIST_P224_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P224_ECPointT;
        scalarD = (uECC_word_t*)sNIST_P224_ScalarD;
        scalarE = (uECC_word_t*)sNIST_P224_ScalarE;
        ecPointR_Expected = (uECC_word_t*)sNIST_P224_ECPointRjsm;
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointS = (uECC_word_t*)sNIST_P256_ECPointS;
        ecPointT = (uECC_word_t*)sNIST_P256_ECPointT;
        scalarD = (uECC_word_t*)sNIST_P256_ScalarD;
        scalarE = (uECC_word_t*)sNIST_P256_ScalarE;
        ecPointR_Expected = (uECC_word_t*)sNIST_P256_ECPointRjsm;
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        return false;
    }

    // Main Loop: ecPointR = ScalarD * ecPointS + ScalarE * ecPointT
    for (uint i = 0; i < iterationCounter; i++)
    {
        // ecPointTmp1 = ScalarD * ecPointS
        uECC_point_mult(ecPointTmp1, ecPointS, scalarD, curve);

        // ecPointTmp2 = ScalarE * ecPointT
        uECC_point_mult(ecPointTmp2, ecPointT, scalarE, curve);

        // ecPointR = ecPointTmp1 + ecPointTmp2
        uECC_point_add(ecPointR, ecPointTmp1, ecPointTmp2, curve);
    }

    // Compare Result ecPointR == ecPointRExpected
    if (!uECC_point_equal(ecPointR, ecPointR_Expected, uECC_curve_num_words(curve)))
    {
        printf("\tERROR: MicroECC joint scalar multiply test failed !!! \n");
        return false;
    }

    return true;
}
#endif // WEAVE_CONFIG_USE_MICRO_ECC

// ============================================================
// EC Math Fanctions Declaration for OpenSSL ECC Library
// ============================================================

#if WEAVE_CONFIG_USE_OPENSSL_ECC

static bool TestECMath_PointAddition(OID curveOID, uint32_t iterationCounter)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *ecPointS = NULL;
    BIGNUM *ecPointSx = NULL;
    BIGNUM *ecPointSy = NULL;
    EC_POINT *ecPointT = NULL;
    BIGNUM *ecPointTx = NULL;
    BIGNUM *ecPointTy = NULL;
    EC_POINT *ecPointR = NULL;
    EC_POINT *ecPointR_Expected = NULL;
    BIGNUM *ecPointRx_Expected = NULL;
    BIGNUM *ecPointRy_Expected = NULL;

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointSx = NIST_P192_ECPointS_X();
        ecPointSy = NIST_P192_ECPointS_Y();
        ecPointTx = NIST_P192_ECPointT_X();
        ecPointTy = NIST_P192_ECPointT_Y();
        ecPointRx_Expected = NIST_P192_ECPointRadd_X();
        ecPointRy_Expected = NIST_P192_ECPointRadd_Y();
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointSx = NIST_P224_ECPointS_X();
        ecPointSy = NIST_P224_ECPointS_Y();
        ecPointTx = NIST_P224_ECPointT_X();
        ecPointTy = NIST_P224_ECPointT_Y();
        ecPointRx_Expected = NIST_P224_ECPointRadd_X();
        ecPointRy_Expected = NIST_P224_ECPointRadd_Y();
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointSx = NIST_P256_ECPointS_X();
        ecPointSy = NIST_P256_ECPointS_Y();
        ecPointTx = NIST_P256_ECPointT_X();
        ecPointTy = NIST_P256_ECPointT_Y();
        ecPointRx_Expected = NIST_P256_ECPointRadd_X();
        ecPointRy_Expected = NIST_P256_ECPointRadd_Y();
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Get EC Group for Curve OID
    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    // Create EC_POINT object for Point S
    ecPointS = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointS, ecPointSx, ecPointSy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point T
    ecPointT = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointT != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointT, ecPointTx, ecPointTy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R_Expected (Expected Result)
    ecPointR_Expected = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR_Expected != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointR_Expected, ecPointRx_Expected, ecPointRy_Expected, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R (Actual Result)
    ecPointR = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Main Loop: ecPointR = ecPointS + ecPointT
    for (uint i = 0; i < iterationCounter; i++)
        if (!EC_POINT_add(ecGroup, ecPointR, ecPointS, ecPointT, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Compare Result ecPointR == ecPointR_Expected
    if ( EC_POINT_cmp(ecGroup, ecPointR, ecPointR_Expected, NULL)  != 0 )
    {
        printf("\tERROR: OpenSSL point addition test failed !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    BN_free(ecPointSx);
    BN_free(ecPointSy);
    BN_free(ecPointTx);
    BN_free(ecPointTy);
    BN_free(ecPointRx_Expected);
    BN_free(ecPointRy_Expected);
    EC_POINT_free(ecPointR);
    EC_POINT_free(ecPointR_Expected);
    EC_POINT_free(ecPointT);
    EC_POINT_free(ecPointS);
    EC_GROUP_free(ecGroup);
    if (err != WEAVE_NO_ERROR)
    {
        printf("\tERROR: Exiting with error !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointSubtraction(OID curveOID, uint32_t iterationCounter)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *ecPointS = NULL;
    BIGNUM *ecPointSx = NULL;
    BIGNUM *ecPointSy = NULL;
    EC_POINT *ecPointT = NULL;
    BIGNUM *ecPointTx = NULL;
    BIGNUM *ecPointTy = NULL;
    EC_POINT *ecPointR = NULL;
    EC_POINT *ecPointR_Expected = NULL;
    BIGNUM *ecPointRx_Expected = NULL;
    BIGNUM *ecPointRy_Expected = NULL;
    EC_POINT *ecPointTmp = NULL;

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointSx = NIST_P192_ECPointS_X();
        ecPointSy = NIST_P192_ECPointS_Y();
        ecPointTx = NIST_P192_ECPointT_X();
        ecPointTy = NIST_P192_ECPointT_Y();
        ecPointRx_Expected = NIST_P192_ECPointRsub_X();
        ecPointRy_Expected = NIST_P192_ECPointRsub_Y();
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointSx = NIST_P224_ECPointS_X();
        ecPointSy = NIST_P224_ECPointS_Y();
        ecPointTx = NIST_P224_ECPointT_X();
        ecPointTy = NIST_P224_ECPointT_Y();
        ecPointRx_Expected = NIST_P224_ECPointRsub_X();
        ecPointRy_Expected = NIST_P224_ECPointRsub_Y();
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointSx = NIST_P256_ECPointS_X();
        ecPointSy = NIST_P256_ECPointS_Y();
        ecPointTx = NIST_P256_ECPointT_X();
        ecPointTy = NIST_P256_ECPointT_Y();
        ecPointRx_Expected = NIST_P256_ECPointRsub_X();
        ecPointRy_Expected = NIST_P256_ECPointRsub_Y();
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Get EC Group for Curve OID
    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    // Create EC_POINT object for Point S
    ecPointS = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointS, ecPointSx, ecPointSy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point T
    ecPointT = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointT != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointT, ecPointTx, ecPointTy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R_Expected (Expected Result)
    ecPointR_Expected = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR_Expected != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointR_Expected, ecPointRx_Expected, ecPointRy_Expected, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R (Actual Result)
    ecPointR = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Create EC_POINT object for Temp Point
    ecPointTmp = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointTmp != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Main Loop: ecPointR = ecPointS - ecPointT
    for (uint i = 0; i < iterationCounter; i++)
    {
        ecPointTmp = EC_POINT_dup(ecPointT, ecGroup);
        VerifyOrExit(ecPointTmp != NULL, err = WEAVE_ERROR_NO_MEMORY);

        if (!EC_POINT_invert(ecGroup, ecPointTmp, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

        if (!EC_POINT_add(ecGroup, ecPointR, ecPointS, ecPointTmp, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Compare Result ecPointR == ecPointR_Expected
    if ( EC_POINT_cmp(ecGroup, ecPointR, ecPointR_Expected, NULL)  != 0 )
    {
        printf("\tERROR: OpenSSL point subtraction test failed !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    BN_free(ecPointSx);
    BN_free(ecPointSy);
    BN_free(ecPointTx);
    BN_free(ecPointTy);
    BN_free(ecPointRx_Expected);
    BN_free(ecPointRy_Expected);
    EC_POINT_free(ecPointTmp);
    EC_POINT_free(ecPointR);
    EC_POINT_free(ecPointR_Expected);
    EC_POINT_free(ecPointT);
    EC_POINT_free(ecPointS);
    EC_GROUP_free(ecGroup);
    if (err != WEAVE_NO_ERROR)
    {
        printf("\tERROR: Exiting with error !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointDouble(OID curveOID, uint32_t iterationCounter)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *ecPointS = NULL;
    BIGNUM *ecPointSx = NULL;
    BIGNUM *ecPointSy = NULL;
    EC_POINT *ecPointR = NULL;
    EC_POINT *ecPointR_Expected = NULL;
    BIGNUM *ecPointRx_Expected = NULL;
    BIGNUM *ecPointRy_Expected = NULL;

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointSx = NIST_P192_ECPointS_X();
        ecPointSy = NIST_P192_ECPointS_Y();
        ecPointRx_Expected = NIST_P192_ECPointRdbl_X();
        ecPointRy_Expected = NIST_P192_ECPointRdbl_Y();
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointSx = NIST_P224_ECPointS_X();
        ecPointSy = NIST_P224_ECPointS_Y();
        ecPointRx_Expected = NIST_P224_ECPointRdbl_X();
        ecPointRy_Expected = NIST_P224_ECPointRdbl_Y();
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointSx = NIST_P256_ECPointS_X();
        ecPointSy = NIST_P256_ECPointS_Y();
        ecPointRx_Expected = NIST_P256_ECPointRdbl_X();
        ecPointRy_Expected = NIST_P256_ECPointRdbl_Y();
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Get EC Group for Curve OID
    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    // Create EC_POINT object for Point S
    ecPointS = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointS, ecPointSx, ecPointSy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R_Expected (Expected Result)
    ecPointR_Expected = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR_Expected != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointR_Expected, ecPointRx_Expected, ecPointRy_Expected, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R (Actual Result)
    ecPointR = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Main Loop: ecPointR = 2 * ecPointS
    for (uint i = 0; i < iterationCounter; i++)
        if (!EC_POINT_dbl(ecGroup, ecPointR, ecPointS, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Compare Result ecPointR == ecPointR_Expected
    if ( EC_POINT_cmp(ecGroup, ecPointR, ecPointR_Expected, NULL)  != 0 )
    {
        printf("\tERROR: OpenSSL point double test failed !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    BN_free(ecPointSx);
    BN_free(ecPointSy);
    BN_free(ecPointRx_Expected);
    BN_free(ecPointRy_Expected);
    EC_POINT_free(ecPointR);
    EC_POINT_free(ecPointR_Expected);
    EC_POINT_free(ecPointS);
    EC_GROUP_free(ecGroup);
    if (err != WEAVE_NO_ERROR)
    {
        printf("\tERROR: Exiting with error !!! \n");
        return false;
    }

    return true;
}

static bool TestECMath_PointMultiply(OID curveOID, uint32_t iterationCounter)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *ecPointS = NULL;
    BIGNUM *ecPointSx = NULL;
    BIGNUM *ecPointSy = NULL;
    BIGNUM *scalarD = NULL;
    EC_POINT *ecPointR = NULL;
    EC_POINT *ecPointR_Expected = NULL;
    BIGNUM *ecPointRx_Expected = NULL;
    BIGNUM *ecPointRy_Expected = NULL;

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointSx = NIST_P192_ECPointS_X();
        ecPointSy = NIST_P192_ECPointS_Y();
        scalarD  = NIST_P192_D();
        ecPointRx_Expected = NIST_P192_ECPointRmul_X();
        ecPointRy_Expected = NIST_P192_ECPointRmul_Y();
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointSx = NIST_P224_ECPointS_X();
        ecPointSy = NIST_P224_ECPointS_Y();
        scalarD  = NIST_P224_D();
        ecPointRx_Expected = NIST_P224_ECPointRmul_X();
        ecPointRy_Expected = NIST_P224_ECPointRmul_Y();
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointSx = NIST_P256_ECPointS_X();
        ecPointSy = NIST_P256_ECPointS_Y();
        scalarD  = NIST_P256_D();
        ecPointRx_Expected = NIST_P256_ECPointRmul_X();
        ecPointRy_Expected = NIST_P256_ECPointRmul_Y();
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Get EC Group for Curve OID
    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    // Create EC_POINT object for Point S
    ecPointS = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointS, ecPointSx, ecPointSy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R_Expected (Expected Result)
    ecPointR_Expected = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR_Expected != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointR_Expected, ecPointRx_Expected, ecPointRy_Expected, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R (Actual Result)
    ecPointR = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    // Main Loop: ecPointR = scalarD * ecPointS
    for (uint i = 0; i < iterationCounter; i++)
        if (!EC_POINT_mul(ecGroup, ecPointR, NULL, ecPointS, scalarD, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Compare Result ecPointR == ecPointR_Expected
    if ( EC_POINT_cmp(ecGroup, ecPointR, ecPointR_Expected, NULL)  != 0 )
    {
        printf("\tERROR: OpenSSL point multiply test failed !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    BN_free(ecPointSx);
    BN_free(ecPointSy);
    BN_free(scalarD);
    BN_free(ecPointRx_Expected);
    BN_free(ecPointRy_Expected);
    EC_POINT_free(ecPointR);
    EC_POINT_free(ecPointR_Expected);
    EC_POINT_free(ecPointS);
    EC_GROUP_free(ecGroup);
    if (err != WEAVE_NO_ERROR)
    {
        printf("\tERROR: Exiting with error !!! \n");
        return false;
    }

    return true;
}

#if !defined(OPENSSL_IS_BORINGSSL)

static bool TestECMath_JointScalarMultiply(OID curveOID, uint32_t iterationCounter)
{
    WEAVE_ERROR err;
    EC_GROUP *ecGroup = NULL;
    EC_POINT *ecPointS = NULL;
    BIGNUM *ecPointSx = NULL;
    BIGNUM *ecPointSy = NULL;
    EC_POINT *ecPointT = NULL;
    BIGNUM *ecPointTx = NULL;
    BIGNUM *ecPointTy = NULL;
    BIGNUM *scalarD = NULL;
    BIGNUM *scalarE = NULL;
    EC_POINT *ecPointR = NULL;
    EC_POINT *ecPointR_Expected = NULL;
    BIGNUM *ecPointRx_Expected = NULL;
    BIGNUM *ecPointRy_Expected = NULL;
    const EC_POINT *ecPoints[2];
    const BIGNUM *scalars[2];

    switch (curveOID) {
    case kOID_EllipticCurve_prime192v1:
        ecPointSx = NIST_P192_ECPointS_X();
        ecPointSy = NIST_P192_ECPointS_Y();
        ecPointTx = NIST_P192_ECPointT_X();
        ecPointTy = NIST_P192_ECPointT_Y();
        scalarD   = NIST_P192_D();
        scalarE   = NIST_P192_E();
        ecPointRx_Expected = NIST_P192_ECPointRjsm_X();
        ecPointRy_Expected = NIST_P192_ECPointRjsm_Y();
        break;

    case kOID_EllipticCurve_secp224r1:
        ecPointSx = NIST_P224_ECPointS_X();
        ecPointSy = NIST_P224_ECPointS_Y();
        ecPointTx = NIST_P224_ECPointT_X();
        ecPointTy = NIST_P224_ECPointT_Y();
        scalarD   = NIST_P224_D();
        scalarE   = NIST_P224_E();
        ecPointRx_Expected = NIST_P224_ECPointRjsm_X();
        ecPointRy_Expected = NIST_P224_ECPointRjsm_Y();
        break;

    case kOID_EllipticCurve_prime256v1:
        ecPointSx = NIST_P256_ECPointS_X();
        ecPointSy = NIST_P256_ECPointS_Y();
        ecPointTx = NIST_P256_ECPointT_X();
        ecPointTy = NIST_P256_ECPointT_Y();
        scalarD   = NIST_P256_D();
        scalarE   = NIST_P256_E();
        ecPointRx_Expected = NIST_P256_ECPointRjsm_X();
        ecPointRy_Expected = NIST_P256_ECPointRjsm_Y();
        break;

    default:
        printf("\tERROR: Unsupported Elliptic Curve !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

    // Get EC Group for Curve OID
    err = GetECGroupForCurve(curveOID, ecGroup);
    SuccessOrExit(err);

    // Create EC_POINT object for Point S
    ecPointS = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointS != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointS, ecPointSx, ecPointSy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point T
    ecPointT = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointT != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointT, ecPointTx, ecPointTy, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R_Expected (Expected Result)
    ecPointR_Expected = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR_Expected != NULL, err = WEAVE_ERROR_NO_MEMORY);

    if (!EC_POINT_set_affine_coordinates_GFp(ecGroup, ecPointR_Expected, ecPointRx_Expected, ecPointRy_Expected, NULL))
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Create EC_POINT object for Point R (Actual Result)
    ecPointR = EC_POINT_new(ecGroup);
    VerifyOrExit(ecPointR != NULL, err = WEAVE_ERROR_NO_MEMORY);

    ecPoints[0] = ecPointS;
    ecPoints[1] = ecPointT;
    scalars[0]  = scalarD;
    scalars[1]  = scalarE;

    // Main Loop: ecPointR = scalarD * ecPointS + scalarE * ecPointT
    for (uint i = 0; i < iterationCounter; i++)
        if (!EC_POINTs_mul(ecGroup, ecPointR, NULL, 2, ecPoints, scalars, NULL))
            ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);

    // Compare Result ecPointR == ecPointR_Expected
    if ( EC_POINT_cmp(ecGroup, ecPointR, ecPointR_Expected, NULL)  != 0 )
    {
        printf("\tERROR: OpenSSL joint scalar multiply test failed !!! \n");
        ExitNow(err = WEAVE_ERROR_INVALID_ARGUMENT);
    }

exit:
    BN_free(ecPointSx);
    BN_free(ecPointSy);
    BN_free(ecPointTx);
    BN_free(ecPointTy);
    BN_free(scalarD);
    BN_free(scalarE);
    BN_free(ecPointRx_Expected);
    BN_free(ecPointRy_Expected);
    EC_POINT_free(ecPointR);
    EC_POINT_free(ecPointR_Expected);
    EC_POINT_free(ecPointT);
    EC_POINT_free(ecPointS);
    EC_GROUP_free(ecGroup);
    if (err != WEAVE_NO_ERROR)
    {
        printf("\tERROR: Exiting with error !!! \n");
        return false;
    }

    return true;
}

#endif // !defined(OPENSSL_IS_BORINGSSL)

#endif // WEAVE_CONFIG_USE_OPENSSL_ECC

// ============================================================
// Test Body
// ============================================================

#ifndef TEST_ECMATH_DEBUG_PRINT_ENABLE
#define TEST_ECMATH_DEBUG_PRINT_ENABLE          0
#endif

#ifndef TEST_ECMATH_NUMBER_OF_ITERATIONS
#define TEST_ECMATH_NUMBER_OF_ITERATIONS        1
#endif

int main(int argc, char *argv[])
{
    bool status = true;

    struct TestCurve {
        OID oid;
        char const *name;
    };

    TestCurve TestECMath_Curves[] = {
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP192R1
        { kOID_EllipticCurve_prime192v1, "PRIME192v1" },
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP224R1
        { kOID_EllipticCurve_secp224r1, "SECP224r1" },
#endif
#if WEAVE_CONFIG_SUPPORT_ELLIPTIC_CURVE_SECP256R1
        { kOID_EllipticCurve_prime256v1, "PRIME256v1" },
#endif
    };

    typedef bool (*TestECMath_Function)(OID curveOID, uint32_t iterationCounter);

    struct TestFunction {
        TestECMath_Function function;
        char const *name;
    };

    TestFunction TestECMath_Functions[] = {
        { TestECMath_PointAddition, "EC Point Addition" },
        { TestECMath_PointSubtraction, "EC Point Subtraction" },
        { TestECMath_PointDouble, "EC Point Double" },
        { TestECMath_PointMultiply, "EC Point Multiply" },
#if !defined(OPENSSL_IS_BORINGSSL)
        { TestECMath_JointScalarMultiply, "EC Joint Scalar Multiply" },
#endif
    };


    for (uint j = 0; j < sizeof(TestECMath_Curves)/sizeof(TestCurve); j++)
    {
        printf("Starting Elliptic Curve tests for %s curve (%d iterations)\n", TestECMath_Curves[j].name, TEST_ECMATH_NUMBER_OF_ITERATIONS);

        for (uint i = 0; i < sizeof(TestECMath_Functions)/sizeof(TestFunction); i++)
        {
#if TEST_ECMATH_DEBUG_PRINT_ENABLE
            printf("\tRunning %s test\n", TestECMath_Functions[i].name);
            time_t timeStart = time(NULL);
#endif

            status = TestECMath_Functions[i].function(TestECMath_Curves[j].oid, TEST_ECMATH_NUMBER_OF_ITERATIONS);
            VerifyOrExit(status == true, );

#if TEST_ECMATH_DEBUG_PRINT_ENABLE
            time_t timeEnd = time(NULL);
            printf("\tTotal Time = %ld sec\n", (timeEnd - timeStart));
#endif
        }
    }

exit:
    return ((status != false) ? EXIT_SUCCESS : EXIT_FAILURE);
}
