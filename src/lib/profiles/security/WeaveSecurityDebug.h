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
 *      This file declares utility functions for outputting
 *      information related to Weave security.
 *
 *      @note These function symbols are only available when
 *            WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS has been
 *            asserted.
 *
 */

#ifndef WEAVESECURITYDEBUG_H_
#define WEAVESECURITYDEBUG_H_

#include <stdint.h>
#include <stdio.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

#if WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS

extern void PrintCert(FILE *out, const WeaveCertificateData& cert, const WeaveCertificateSet  *certSet, uint16_t indent = 0, bool verbose = false);
extern void PrintCertValidationResults(FILE *out, const WeaveCertificateSet& certSet, const ValidationContext& validContext, uint16_t indent = 0);
extern WEAVE_ERROR PrintWeaveDN(FILE * out, TLVReader & reader);
extern void PrintWeaveDN(FILE *out, const WeaveDN& dn);
extern void PrintPackedTime(FILE *out, uint32_t t);
extern void PrintPackedDate(FILE *out, uint16_t t);
extern const char *DescribeWeaveCertId(OID attrOID, uint64_t weaveCertId);
extern WEAVE_ERROR PrintCertArray(FILE * out, TLVReader & reader, uint16_t indent);
extern WEAVE_ERROR PrintECDSASignature(FILE * out, TLVReader & reader, uint16_t indent);
extern WEAVE_ERROR PrintCertReference(FILE * out, TLVReader & reader, uint16_t indent);
extern WEAVE_ERROR PrintWeaveSignature(FILE * out, TLVReader & reader, uint16_t indent);

#endif // WEAVE_CONFIG_ENABLE_SECURITY_DEBUG_FUNCS

} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl

#endif /* WEAVESECURITYDEBUG_H_ */
