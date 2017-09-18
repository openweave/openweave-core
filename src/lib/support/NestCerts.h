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
 *      This file defines global symbols for Nest Weave public key
 *      infrastructure (PKI) certificates.
 *
 */

#ifndef NESTCERTS_H_
#define NESTCERTS_H_

#include <Weave/Support/NLDLLUtil.h>

/**
 *   @namespace nl::NestCerts
 *
 *   @brief
 *     This namespace includes global symbols for Nest Weave public key
 *     infrastructure (PKI) certificates.
 */

namespace nl {
namespace NestCerts {

    /**
     *   @namespace nl::NestCerts::Production
     *
     *   @brief
     *     This namespace includes global symbols for Nest Weave
     *     "Production" PKI certificates.
     *
     *     @note Production certificates are the only certificates
     *           that may be used in qualified, tested, shipping Weave
     *           products.
     */

    namespace Production {

        /**
         *   @namespace nl::NestCerts::Production::Root
         *
         *   @brief
         *     This namespace includes global symbols for Nest Weave
         *     Production Root PKI certificates.
         */

        namespace Root {

            extern const NL_DLL_EXPORT uint8_t Cert[];
            extern const NL_DLL_EXPORT uint16_t CertLength;

            extern const uint8_t PublicKey[];
            extern const uint16_t PublicKeyLength;

            extern const uint8_t SubjectKeyId[];
            extern const uint16_t SubjectKeyIdLength;

            extern const uint32_t CurveOID;

            extern const uint64_t CAId;
        }

        /**
         *   @namespace nl::NestCerts::Production::DeviceCA
         *
         *   @brief
         *     This namespace includes global symbols for Nest Weave
         *     Production Device Certificate Authority (CA) PKI
         *     certificates.
         */

        namespace DeviceCA {

            extern const NL_DLL_EXPORT uint8_t Cert[];
            extern const NL_DLL_EXPORT uint16_t CertLength;

            extern const uint8_t PublicKey[];
            extern const uint16_t PublicKeyLength;

            extern const uint8_t SubjectKeyId[];
            extern const uint16_t SubjectKeyIdLength;

            extern const uint32_t CurveOID;

            extern const uint64_t CAId;

        }

    } // namespace Production

    /**
     *   @namespace nl::NestCerts::Development
     *
     *   @brief
     *     This namespace includes global symbols for Nest Weave
     *     "Development" PKI certificates.
     *
     *     @note Development certificates are the only allowed for
     *           Weave product development and test and shall not be
     *           used for qualified, tested, shipping Weave products.
     */

    namespace Development {

        /**
         *   @namespace nl::NestCerts::Development::Root
         *
         *   @brief
         *     This namespace includes global symbols for Nest Weave
         *     Development Root PKI certificates.
         */

        namespace Root {

            extern const NL_DLL_EXPORT uint8_t Cert[];
            extern const NL_DLL_EXPORT uint16_t CertLength;

            extern const uint8_t PublicKey[];
            extern const uint16_t PublicKeyLength;

            extern const uint8_t SubjectKeyId[];
            extern const uint16_t SubjectKeyIdLength;

            extern const uint32_t CurveOID;

            extern const uint64_t CAId;

        }

        /**
         *   @namespace nl::NestCerts::Development::DeviceCA
         *
         *   @brief
         *     This namespace includes global symbols for Nest Weave
         *     Development Device Certificate Authority (CA) PKI
         *     certificates.
         */

        namespace DeviceCA {

            extern const NL_DLL_EXPORT uint8_t Cert[];
            extern const NL_DLL_EXPORT uint16_t CertLength;

            extern const uint8_t PublicKey[];
            extern const uint16_t PublicKeyLength;

            extern const uint8_t SubjectKeyId[];
            extern const uint16_t SubjectKeyIdLength;

            extern const uint32_t CurveOID;

            extern const uint64_t CAId;

        }

    } // namespace Development

} // namespace NestCerts
} // namespace nl


#endif /* NESTCERTS_H_ */
