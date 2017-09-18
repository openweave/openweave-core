/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      Utility functions for interacting with Weave Access Tokens.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include <Weave/Core/WeaveTLV.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/crypto/HashAlgos.h>
#include <Weave/Support/crypto/EllipticCurve.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeaveCert.h>
#include <Weave/Support/CodeUtils.h>
#include "WeaveAccessToken.h"

namespace nl {
namespace Weave {
namespace Profiles {
namespace Security {

using namespace nl::Weave::ASN1;
using namespace nl::Weave::TLV;
using namespace nl::Weave::Profiles;
using namespace nl::Weave::Crypto;

/**
 * @brief
 *   Load the certificates in an access token into a Weave certificate set.
 *
 * @details
 *   This function decodes a given Weave access token and loads the access token certificates into the
 *   specified Weave certificate set object.  If the access tokens contains one or more related certificate
 *   these are loaded into the certificate set as well.
 *
 * @param accessToken                       A pointer to a buffer containing an encoded Weave Access Token.
 * @param accessTokenLen                    The length of the encoded access token.
 * @param certSet                           The certificate set into which the access token certificates should
 *                                          be loaded.
 * @param devodeFlags                       The certificate decode flags that should be used when loading
 *                                          the certificates.
 * @param accessTokenCert                   A reference to a pointer that will be set to the Weave certificate
 *                                          data structure for the access token certificate. NOTE: This
 *                                          pointer will only be set if the function returns successfully.
 *
 * @retval #WEAVE_NO_ERROR                  If the access token certificates were successfully loaded.
 * @retval tlv-errors                       Weave errors related to reading TLV.
 * @retval cert-errors                      Weave errors related to decoding Weave certificates.
 * @retval platform-errors                  Other platform-specific errors.
 */
WEAVE_ERROR LoadAccessTokenCerts(const uint8_t *accessToken, uint32_t accessTokenLen, WeaveCertificateSet& certSet, uint16_t decodeFlags, WeaveCertificateData *& accessTokenCert)
{
    TLVReader reader;
    reader.Init(accessToken, accessTokenLen);
    return LoadAccessTokenCerts(reader, certSet, decodeFlags, accessTokenCert);
}

/**
 * @brief
 *   Load the certificates in an access token into a Weave certificate set.
 *
 * @details
 *   This function reads a Weave access token from a given TLVReader and loads the access token certificates
 *   into the specified Weave certificate set object.  If the access tokens contains one or more related
 *   certificate these are loaded into the certificate set as well.
 *
 * @param reader                            A TLVReader object that is position immediately before a Weave
 *                                          Access Token.
 * @param certSet                           The certificate set into which the access token certificates should
 *                                          be loaded.
 * @param devodeFlags                       The certificate decode flags that should be used when loading
 *                                          the certificates.
 * @param accessTokenCert                   A reference to a pointer that will be set to the Weave certificate
 *                                          data structure for the access token certificate. NOTE: This value
 *                                          is only set when the function returns successfully.
 *
 * @retval #WEAVE_NO_ERROR                  If the access token certificates were successfully loaded.
 * @retval tlv-errors                       Weave errors related to reading TLV.
 * @retval cert-errors                      Weave errors related to decoding Weave certificates.
 * @retval platform-errors                  Other platform-specific errors.
 */
WEAVE_ERROR LoadAccessTokenCerts(TLVReader& reader, WeaveCertificateSet& certSet, uint16_t decodeFlags, WeaveCertificateData *& accessTokenCert)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType tokenContainer;

    reader.ImplicitProfileId = kWeaveProfile_Security;

    // Advance the reader to the start of the access token structure.
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveAccessToken));
    SuccessOrExit(err);

    // Enter the structure.
    err = reader.EnterContainer(tokenContainer);
    SuccessOrExit(err);

    // Advance to the first element, which should be the access token certificate.
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_Certificate));
    SuccessOrExit(err);

    // Load the access token certificate into the certificate set.  Return a pointer to the
    // cert data structure to the caller.
    err = certSet.LoadCert(reader, decodeFlags, accessTokenCert);
    SuccessOrExit(err);

    // Advance to the private key field. (We will be ignoring this).
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_PrivateKey));
    SuccessOrExit(err);

    // Advance to the related certificates field.  If the field is present...
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_RelatedCertificates));
    if (err == WEAVE_NO_ERROR) {

        // Load the related certificates into the certificate set.
        err = certSet.LoadCerts(reader, decodeFlags);
        SuccessOrExit(err);
    }
    else {
        if (err == WEAVE_END_OF_TLV)
            err = WEAVE_NO_ERROR;
        SuccessOrExit(err);
    }

    // Verify there are no further fields in the access token.
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    // Exit the access token container.
    err = reader.ExitContainer(tokenContainer);
    SuccessOrExit(err);

exit:
    return err;
}

/**
 * @brief
 *   Reads a Weave Access Token and constructs a CASE Certificate Info TLV structure containing the
 *   certificates from the access token.
 *
 * @details
 *   This function decodes a given Weave Access Token and encodes the TLV for a Weave CASE Certificate
 *   Info structure.  The EntityCertificate field within the CertificateInfo structure is set to the
 *   access token certificate, and the RelatedCertificates field (if present) is set to the corresponding
 *   field within the access token.
 *
 * @param accessToken                       A pointer to a buffer containing an encoded Weave Access Token.
 * @param accessTokenLen                    The length of the encoded access token.
 * @param certInfoBuf                       A pointer to a buffer into which the CASE certificate info
 *                                          structure should be encoded.
 * @param certInfoBufSize                   The size of the buffer pointed to by certInfoBuf.
 * @param certInfoLen                       A reference to an integer will be set to the length of the
 *                                          encoded certificate info structure. NOTE: This value is only
 *                                          set when the function returns successfully.
 *
 * @retval #WEAVE_NO_ERROR                  If the access CASE certificate info structure was successfully
 *                                          encoded.
 * @retval tlv-errors                       Weave errors related to reading or writing TLV.
 * @retval cert-errors                      Weave errors related to decoding Weave certificates.
 * @retval platform-errors                  Other platform-specific errors.
 */
WEAVE_ERROR CASECertInfoFromAccessToken(const uint8_t *accessToken, uint32_t accessTokenLen, uint8_t *certInfoBuf, uint16_t certInfoBufSize, uint16_t& certInfoLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    TLVWriter writer;

    reader.Init(accessToken, accessTokenLen);

    writer.Init(certInfoBuf, certInfoBufSize);
    writer.ImplicitProfileId = kWeaveProfile_Security;

    err = CASECertInfoFromAccessToken(reader, writer);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    certInfoLen = writer.GetLengthWritten();

exit:
    return err;
}

/**
 * @brief
 *   Reads a Weave Access Token and writes a CASE Certificate Info TLV structure containing the
 *   certificates from the access token.
 *
 * @details
 *   This function reads a Weave Access Token from a given TLVReader and writes the TLV for a Weave CASE Certificate
 *   Info structure to a TLVWriter.  The EntityCertificate field within the CertificateInfo structure is set to the
 *   access token certificate, and the RelatedCertificates field (if present) is set to the corresponding
 *   field within the access token.
 *
 * @param accessToken                       A pointer to a buffer containing an encoded Weave Access Token.
 * @param accessTokenLen                    The length of the encoded access token.
 * @param certInfoBuf                       A pointer to a buffer into which the CASE certificate info
 *                                          structure should be encoded.
 * @param certInfoBufSize                   The size of the buffer pointed to by certInfoBuf.
 * @param certInfoLen                       A reference to an integer will be set to the length of the
 *                                          encoded certificate info structure. NOTE: This value is only
 *                                          set when the function returns successfully.
 *
 * @retval #WEAVE_NO_ERROR                  If the access CASE certificate info structure was successfully
 *                                          encoded.
 * @retval tlv-errors                       Weave errors related to reading or writing TLV.
 * @retval cert-errors                      Weave errors related to decoding Weave certificates.
 * @retval platform-errors                  Other platform-specific errors.
 */
WEAVE_ERROR CASECertInfoFromAccessToken(TLVReader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType tokenContainer;
    TLVType certInfoContainer;

    reader.ImplicitProfileId = kWeaveProfile_Security;

    // Advance the reader to the start of the access token structure.
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveAccessToken));
    SuccessOrExit(err);

    // Enter the structure.
    err = reader.EnterContainer(tokenContainer);
    SuccessOrExit(err);

    // Advance the reader to the first element, which should be the access token certificate.
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_Certificate));
    SuccessOrExit(err);

    // Write the start of the CASE cert info structure to the writer
    err = writer.StartContainer(ProfileTag(kWeaveProfile_Security, kTag_WeaveCASECertificateInformation), kTLVType_Structure, certInfoContainer);
    SuccessOrExit(err);

    // Copy the access token certificate into the cert info structure, using the EntityCertificate tag.
    err = writer.CopyContainer(ContextTag(kTag_CASECertificateInfo_EntityCertificate), reader);
    SuccessOrExit(err);

    // Advance the reader to the private key field. (We will be ignoring this).
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_PrivateKey));
    SuccessOrExit(err);

    // Advance the reader to the related certificates field.  If the field is present...
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_RelatedCertificates));
    if (err == WEAVE_NO_ERROR) {

        // Copy the related certificates collection into the cert info structure, using the EntityCertificate tag.
        err = writer.CopyContainer(ContextTag(kTag_CASECertificateInfo_RelatedCertificates), reader);
        SuccessOrExit(err);
    }
    else {
        if (err == WEAVE_END_OF_TLV)
            err = WEAVE_NO_ERROR;
        SuccessOrExit(err);
    }

    // Verify there are no further fields in the access token.
    err = reader.VerifyEndOfContainer();
    SuccessOrExit(err);

    // Exit the access token container.
    err = reader.ExitContainer(tokenContainer);
    SuccessOrExit(err);

    // Finish writing the cert info container.
    err = writer.EndContainer(certInfoContainer);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR ExtractPrivateKeyFromAccessToken(const uint8_t *accessToken, uint32_t accessTokenLen, uint8_t *privKeyBuf, uint16_t privKeyBufSize, uint16_t& privKeyLen)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader reader;
    TLVWriter writer;

    reader.Init(accessToken, accessTokenLen);

    writer.Init(privKeyBuf, privKeyBufSize);

    err = ExtractPrivateKeyFromAccessToken(reader, writer);
    SuccessOrExit(err);

    err = writer.Finalize();
    SuccessOrExit(err);

    privKeyLen = writer.GetLengthWritten();

exit:
    return err;
}

WEAVE_ERROR ExtractPrivateKeyFromAccessToken(TLVReader& reader, TLVWriter& writer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVType tokenContainer;

    reader.ImplicitProfileId = kWeaveProfile_Security;

    // Advance the reader to the start of the access token structure.
    err = reader.Next(kTLVType_Structure, ProfileTag(kWeaveProfile_Security, kTag_WeaveAccessToken));
    SuccessOrExit(err);

    // Enter the structure.
    err = reader.EnterContainer(tokenContainer);
    SuccessOrExit(err);

    // Advance the reader to the first element, which should be the access token certificate.
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_Certificate));
    SuccessOrExit(err);

    // Advance the reader to the next element, which should be the private key field.
    err = reader.Next(kTLVType_Structure, ContextTag(kTag_AccessToken_PrivateKey));
    SuccessOrExit(err);

    // Copy the private key to the writer, changing the tag to EllipticCurvePrivateKey.
    err = writer.CopyContainer(ProfileTag(kWeaveProfile_Security, kTag_EllipticCurvePrivateKey), reader);
    SuccessOrExit(err);

    // Exit the access token container.
    err = reader.ExitContainer(tokenContainer);
    SuccessOrExit(err);

exit:
    return err;
}



} // namespace Security
} // namespace Profiles
} // namespace Weave
} // namespace nl
