/*
 *    Copyright (c) 2019 Google LLC
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
 *      This file defines message descriptions for the messages in
 *      the software update profile.
 *
 *      The profile has a client and server role. the basic protocol
 *      can be diagrammed as follows:
 *
 *
 *          | Server |                    | Client |
 *          ----------------------------------------
 *          ImageAnnounce--------------->
 *                 <----------------------ImageQuery
 *          ImageQueryResponse---------->
 *                                        (Client downloads firmware image)
 *                 <----------------------DownloadNotify
 *          NotifyResponse--------------->
 *                                        (Client updates its firmware)
 *                 <----------------------UpdateNotify
 *          NotifyResponse--------------->
 *
 *      where the image announce message is optional.
 *
 */

#ifndef _SW_UPDATE_PROFILE_H
#define _SW_UPDATE_PROFILE_H

#include <Weave/Support/NLDLLUtil.h>

/**
 *   @namespace nl::Weave::Profiles::SoftwareUpdate
 *
 *   @brief
 *     This namespace includes all interfaces within Weave for the
 *     Weave Software Update profile, which includes the
 *     corresponding, protocol of the same name.
 */

namespace nl {
namespace Weave {
namespace Profiles {
namespace SoftwareUpdate {
/**
 * SoftwareUpdate Message types.
 */
// clang-format off
enum
{
    kMsgType_ImageAnnounce      = 0, /**< An optional unsolicited messsage, used by the server to announce availability of a software update.  The message carries no payload. */
    kMsgType_ImageQuery         = 1, /**< A query message sent by the client to the server.  Its format is defined with the ImageQuery class. */
    kMsgType_ImageQueryResponse = 2, /**< A message generated in response to a successful image query message. Its format is defined by the ImageQueryResponse class. */
    kMsgType_DownloadNotify     = 3, /**< An optional message from the client to the server used to notify the server about the download status.  Its payload is a nl::Weave::Profiles::StatusReporting::StatusReport with the additional status info drawn from @ref SoftwareUpdateStatusCodes.*/
    kMsgType_NotifyResponse     = 4, /**< A message generated in response to the download notify message.  Its payload is a nl::Weave::Profiles::StatusReporting::StatusReport with the additional status info drawn from @ref SoftwareUpdateStatusCodes. */
    kMsgType_UpdateNotify       = 5, /**< An optional message from the client to the server used to communicate the final status of the update.  Its payload is a nl::Weave::Profiles::StatusReporting::StatusReport with the additional status info drawn from @ref SoftwareUpdateStatusCodes.  As the message is expected to be generated post actual update, it is sent on a new ExchangeContext and is treated as an unsolicited message on the server. */
    kMsgType_ImageQueryStatus   = 6, /**< A message generated in response to a failed image query message. Its payload is a nl::Weave::Profiles::StatusReporting::StatusReport with the additional status info drawn from @ref SoftwareUpdateStatusCodes. */
};
// clang-format on

/**
 * @anchor SoftwareUpdateStatusCodes
 * SoftwareUpdate profile-specific status codes.
 */
// clang-format off
enum
{
    kStatus_NoUpdateAvailable    = 0x0001, /**< server => client, indicates that an image query has been received and understood and that the server has no update for the client at this time. */
    kStatus_UpdateFailed         = 0x0010, /**< client => server, indicates that an attempt to install an image specified by the server has failed. */
    kStatus_InvalidInstructions  = 0x0050, /**<  client => server, indicates that the client was unable to download an image because the download instructions contained in the ImageQueryResponse, i.e. URI, update scheme, update condition, were poorly formed or inconsistent. */
    kStatus_DownloadFailed       = 0x0051, /**< client => server, indicates that an attempted download failed.*/
    kStatus_IntegrityCheckFailed = 0x0052, /**< client => server, indicates that an image was downloaded but it failed the subsequent integrity check.*/
    kStatus_Abort                = 0x0053, /**< server => client, indicates that the client should give up since the server is out of options. */
    kStatus_Retry                = 0x0091, /**< server => client, indicates that the client should submit another image query and restart/continue the update */
};
// clang-format on

/**
 * Control flags for the control field of the ImageQuery frame
 */
enum
{
    kFlag_PackageSpecPresent  = 1, /**< Package specification is present in the ImageQuery. */
    kFlag_LocaleSpecPresent   = 2, /**< Locale specification is present in the ImageQuery. */
    kFlag_TargetNodeIdPresent = 4, /**< Target node ID is present in the ImageQuery. */
};

/**
 * Bitmasks for the optional update options field of the ImageQueryResponse.
 */
enum
{
    kMask_UpdatePriority  = 0x03, // 0b00000011
    kMask_UpdateCondition = 0x1C, // 0b00011100
    kMask_ReportStatus    = 0x20, // 0b00100000
};

/**
 * Shift offsets for the optional update options field of the ImageQueryResponse.
 */
enum
{
    kOffset_UpdatePriority  = 0,
    kOffset_UpdateCondition = 2,
    kOffset_ReportStatus    = 5,
};

/**
 * @anchor IntegrityTypes
 * Integrity types supported by the SoftwareUpdate profile.
 *
 * The image query frame contains information about which integrity checking
 * the client supports and the image query response contains an integrity type
 * and value for the image that the client is being instructed to download and
 * install. The supported types are:
 */
enum
{
    kIntegrityType_SHA160 = 0, /**< 160-bit Secure Hash, (SHA-1), required. */
    kIntegrityType_SHA256 = 1, /**< 256-bit Secure Hash (SHA-2). */
    kIntegrityType_SHA512 = 2, /**< 512-bit, Secure Hash (SHA-2). */
    kIntegrityType_Last   = 3, /**< Number of valid elements in the enumeration */
};

/**
 * Lengths, in bytes, for the integrity specification byte-strings.
 */
enum
{
    kLength_SHA160 = 20,
    kLength_SHA256 = 32,
    kLength_SHA512 = 64,
};

/**
 * @anchor UpdateSchemes
 * Update schemes supported by the SofwareUpdate profile.
 *
 * Similarly to the supported integrity types, the image query contains
 * information about which update schemes, i.e. download protocols, the client
 * supports, and the response contains a value for indicating the update scheme
 * to use in downloading the images. The supported schemes are:
 */
enum
{
    kUpdateScheme_HTTP  = 0, /**< HTTP shall be used as the download protocol. */
    kUpdateScheme_HTTPS = 1, /**< HTTPS shall be used as the download protocol. */
    kUpdateScheme_SFTP  = 2, /**< SFTP shall be used as the download protocol. */
    kUpdateScheme_BDX   = 3, /**< Weave Bulk data transfer shall be used as the download protocol. */
    kUpdateScheme_Last  = 4, /**< Number of valid elements in the enumeration */
};

/**
 * Data Element Tags for the SoftwareUpdate Profile
 */
enum
{
    // ---- Top-level Tags ----
    //                                   Tag Type        Element Type      Disposition
    //                                   ----------------------------------------------
    kTag_InstalledLocales     = 0x00, // Fully-Qualified  Array of strings   Required
    kTag_CertBodyId           = 0x01, // Fully-Qualified  Integer            Required
    kTag_WirelessRegDom       = 0x02, // Fully-Qualified  Integer            Optional
    kTag_SufficientBatterySWU = 0x03  // Fully-Qualified  Bool               Required
};

/**
 * Class describing the ImageAnnounce message.
 *
 * Over the wire and on the air, the message consists of a bare Weave
 * application header with profile ID 0x0000000C and message type 1. The
 * structure provided here is not really used except as a placeholder.
 */
class ImageAnnounce
{
};

/**
 * An auxiliary class to hold a list of integrity types as a part of the image query.
 *
 * A simple wrapper, sized to hold any subset of possible integrity types. In
 * order to accomplish this task, its size is equal to the number of elements in
 * the @ref IntegrityTypes.  It is used to generate the list of supported
 * integrity types in the ImageQuery message.
 */
class NL_DLL_EXPORT IntegrityTypeList
{
public:
    // constructor
    IntegrityTypeList();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t *);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator &);
    static WEAVE_ERROR parse(MessageIterator &, IntegrityTypeList &);
    // comparison
    bool operator ==(const IntegrityTypeList &) const;
    uint8_t theLength;  /**< Length of the supported element list.  Length of 0 indicates an empty list */
    uint8_t theList[kIntegrityType_Last]; /**< Container holding supported integrity types.  It is sized equal to the number of elements in @ref
                           IntegrityTypes */
};

/**
 * An auxiliary class to hold a list of update schemes as a part of the image query.
 *
 * A simple wrapper, sized to hold any subset of possible update schemes. In
 * order to accomplish this task, its size is equal to the number of elements in
 * the @ref UpdateSchemes is used to generate the list of supported
 * update schemes in the ImageQuery message.
 */
class NL_DLL_EXPORT UpdateSchemeList
{
public:
    // constructor
    UpdateSchemeList();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t *);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator &);
    static WEAVE_ERROR parse(MessageIterator &, UpdateSchemeList &);
    // comparison
    bool operator ==(const UpdateSchemeList &) const;
    uint8_t theLength;  /**< Length of the supported element list.  Length of 0 indicates an empty list */
    uint8_t theList[kUpdateScheme_Last]; /**< Container holding supported update schemes.  It is sized equal to the number of elements in @ref
                           UpdateSchemes */
};

/**
 * An auxiliary class that representing a product specification.
 */
class NL_DLL_EXPORT ProductSpec
{
public:
    // constructors
    ProductSpec(uint16_t aVendor, uint16_t aProduct, uint16_t aRevision);
    ProductSpec();
    // comparison
    bool operator ==(const ProductSpec &) const;

    // data members
    uint16_t vendorId;   /**< Weave Vendor ID drawn from the Weave Vendor Identifier Registry. */
    uint16_t productId;  /**< A 16-bit product ID drawn from a vendor-managed namespace. */
    uint16_t productRev; /**< A 16-bit product revision drawn from a vendor managed namespace. */
};

/**
 * A class to support creation and decoding of image query messages.
 *
 * The image query frame has the following form over the wire
 *
 *
 * Length | Field Name
 * -------|------------
 * 1 byte | frame control
 * 6 bytes| product specification
 * variable | version specification
 * 2..4 bytes | integrity type list
 * 2..5 bytes | update scheme list
 * variable | locale specification (optional)
 * 8 bytes | target node ID
 * variable | vendor specific data (optional)
 *
 * where the frame control field has bit-fields as follows:
 *
 * Bit | Meaning
 * ----|--------
 *  0 | 1 - vendor-specific data present, 0 - not present
 *  1 | 1 - locale specification present, 0 - not present
 *  2 | 1 - target node id present, 0 - not present
 *  3..7 | Reserved
 *
 * The ImageQuery, as a structure reads slightly differently from the in-flight
 * representation. In particular, the version and locale are null-terminated
 * c-strings (as opposed to (length, characters) tuples) and both of the optional
 * items are represented as nullable pointers so there isn't a separate boolean
 * to check.
 */
class NL_DLL_EXPORT ImageQuery
{
public:
    // constructor
    ImageQuery();
    // initializer
    WEAVE_ERROR init(ProductSpec & aProductSpec,
                     ReferencedString & aVersion, // 1 byte length
                     IntegrityTypeList & aTypeList, UpdateSchemeList & aSchemeList,
                     ReferencedString * aPackage = NULL, // 1 byte length
                     ReferencedString * aLocale = NULL, uint64_t aTargetNodeId = 0,
                     ReferencedTLVData * aMetaData = NULL); // 2 byte length
    WEAVE_ERROR pack(PacketBuffer *);
    static WEAVE_ERROR parse(PacketBuffer *, ImageQuery &);
    // comparison
    bool operator ==(const ImageQuery &) const;
    // utility
    void print(void);
    // data members
    ProductSpec productSpec;  /**< Product specification describing the device that is making the image query. */
    ReferencedString version; /**< A variable length UTF-8 string containing the vendor-specified software version of the device for
                                 which the query is being made. Must be of length 32 or smaller. */
    IntegrityTypeList integrityTypes; /**< Integrity types supported by the device. */
    UpdateSchemeList updateSchemes;   /**< Update schemes (download protocols) supported by the device. */
    uint64_t targetNodeId; /**< An optional node id of the device for which the query is being made.  The target node id field is
optional.  If absent, the target node id for the query is implicitly the node that was the source of the image query message.

The target node id field is typically used in instances where the node that is the source of the IMAGE QUERY message is serving as a
software update proxy for another node. */
    ReferencedString packageSpec; /**< A variable length UTF-8 string containing a vendor-specific package specification string. The
                       contents of the field describe the desired container type for the software image, such as ‘rpm’, ‘deb’,
                       ‘tgz’, ‘elf’, etc.  (NOTE: This field is unused in Nest implementations of the protocol) */
    ReferencedString localeSpec;  /**< A variable length UTF-8 string containing the POSIX locale in effect on the device for which
                                     the image query is being made.  The contents of the string must conform to the POSIX locale
                                     identifier format, as specified in ISO/IEC 15897, e.g. en_AU.UTF-8 for Australian English.*/
    ReferencedTLVData theMetaData; /**< The vendor-specific data field is variable in length and occupies the remainder of the Weave
                                      message payload, beyond the fields described above.  The field encodes vendor-specific
                                      information about the device for which the query is being made. The vendor-specific data field
                                      is optional.  If present, the field has a form of anonymous TLV-encoded structure. The tags
                                      presented within this structure shall be fully-qualified profile-specific tags. */
};

/**
 * Update priorities
 */
enum UpdatePriority
{
    Normal,   /**< Update may be execute at clients discretion. */
    Critical, /**< Update must be executed immediately. */
};

/**
 * Conditions governing update policy
 */
enum UpdateCondition
{
    IfUnmatched, /**< Download and install the image if the version specification in the response frame doesn't match the software
                    version currently installed. */
    IfLater,     /**< Download and install the image if the version specification in the response frame is later than the software
                    version currently installed. */
    Unconditionally, /**< Download and install the image regardless of the currently running software version. */
    OnOptIn          /**< download and install the image on some trigger provided by an on-site user. */
};

/**
 * An auxiliary class holding the integrity type and the actual hash of the software update image.
 *
 * The object holds the @ref IntegrityTypes field specifying the type of the
 * hash, and the actual hash of the software update image.  The length of the hash is
 * fixed based on the type of the hash.  The object is sized to hold the largest
 * of the supported hashes.
 */
class NL_DLL_EXPORT IntegritySpec
{
public:
    // constructor
    IntegritySpec();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t *);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator &);
    static WEAVE_ERROR parse(MessageIterator &, IntegritySpec &);
    // comparison
    bool operator ==(const IntegritySpec &) const;
    // data members
    uint8_t type; /**< Type of the hash, value to be drawn from @ref IntegrityTypes */
    uint8_t
        value[64]; /**< A variable length sequence of bytes containing the integrity value for the software image identified by the
                      URI field. The integrity value is computed by applying the integrity function specified by the integrity type
                      to the contents of the software update image accessed at the URI specified above.  The integrity specification
                      allows the client to confirm that the image downloaded matches the image specified in this response.*/
};

/**
 * A class to support creation and decoding of the image query response messages.
 *
 * The image query response message has the form:
 *
 * Length | Field Name
 * -------| ----------
 * variable | URI
 * variable | Version specification
 * variable | Integrity specification
 * 1 byte | Update scheme
 * 1 byte | Update options
 *
 * The format of the (optional) update options field is as follows:
 *
 * Bit | Meaning
 * ----|--------
 * 0..2 | Update priority
 * 3..4 | Update condition
 * 5 | Report status.  When set, the client is requested to generate the optional DownloadNotify and UpdateNotify messages.
 * 5..7 | Reserved
 *
 * The image query response is only sent in the case where the image query is
 * processed successfully and produces an image to download.  The message
 * constitutes download instructions for the node the submitted the query. Note
 * that in cases where the server fails to process the image query, it shall
 * generate an image query status.
 */
class NL_DLL_EXPORT ImageQueryResponse
{
public:
    // constructor
    ImageQueryResponse();
    // initializers
    WEAVE_ERROR init(ReferencedString &, ReferencedString &, IntegritySpec &, uint8_t, UpdatePriority, UpdateCondition, bool);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer *);
    static WEAVE_ERROR parse(PacketBuffer *, ImageQueryResponse &);
    // comparison
    bool operator ==(const ImageQueryResponse &) const;
    // utility
    void print(void);
    // data members
    ReferencedString uri; /**< A variable length UTF-8 string containing the location of the software image. The contents of this
                             string must conform to the RFC 3986 specification.  For update schemes corresponding to a well defined
                             Internet Protocol (HTTP, HTTPS, SFTP), the scheme element of the URI MUST conform to the canonical URL
                             encoding for that protocol scheme. The string length must not exceed 65565 and the the string must fit
                             within  a single Weave message, which may be subject to MTU limitations. */
    ReferencedString versionSpec; /**< A variable length UTF-8 string containing a vendor-specific a software version
                                     identification. The string length must not exceed 256 bytes. */
    IntegritySpec integritySpec;  /**< A field containing the integrity information (integrity type and a hash) for the software
                                     update image. */
    uint8_t updateScheme; /**< The update scheme to be used to download the software update image. Its value is drawn from @ref
                             UpdateSchemes. */
    UpdatePriority updatePriority;   /**< Instructions directing the device when to perform the software update. */
    UpdateCondition updateCondition; /**< Instructions as to the conditions under which to proceed with software update. */
    bool reportStatus; /**< Request to inform the server about the progress of software update via the optional DownloadNotify and
                          UpdateNotify messages. */
};
} // namespace SoftwareUpdate
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif // _SW_UPDATE_PROFILE_H
