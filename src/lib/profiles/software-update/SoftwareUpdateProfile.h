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
 *          NotifyReponse--------------->
 *                                        (Client updates its firmware)
 *                 <----------------------UpdateNotify
 *          NotifyReponse--------------->
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
  /*
   * the protocol spelled out in the profile specification comprises
   * 7 messages as described above. the image announce is optional.
   * the message type identifiers for these messages are as follows:
   */
  enum {
    kMsgType_ImageAnnounce =			0,
    kMsgType_ImageQuery = 				1,
    kMsgType_ImageQueryResponse = 		2,
    kMsgType_DownloadNotify = 			3,
    kMsgType_NotifyRepsponse = 			4, //FIXME: check the spelling!
    kMsgType_UpdateNotify =				5,
    kMsgType_ImageQueryStatus =			6, // in case the image query fails
  };
  /*
   * the software update profile defines a number of profile-specific status
   * codes.
   *
   * - kStatus_NoUpdateAvailable: server => client, indicates that an
   * image query has been recieved and understood and that the server has no
   * update for the client at this time.
   *
   * - kStatus_UpdateFailed: client => server, indicates that an attempt
   * to install an image specified by the server has failed.
   *
   * - kStatus_InvalidInstructions: client => server, indicates that
   * the client was unable to download an image because the download
   * instructions contained in the ImageQueryResponse, i.e. URI, update scheme,
   * update condition, were poorly formed or inconsistent.
   *
   * - kStatus_DownloadFailed: client => server, indicates that an
   * attempted download failed.
   *
   * - kStatus_IntegrityCheckFailed: client => server, indicates that
   * an image was downloaded but it failed the subsequent integrity check.
   *
   * - kStatus_Abort: server => client, indicates that the client
   * should give up since the server is out of options.
   *
   * - kStatus_Retry: srever => client, indicates that the client
   * should submit another image query and restart/continue the update
   * process.
   */
  enum {
    kStatus_NoUpdateAvailable =			0x0001,
    kStatus_UpdateFailed =			0x0010,
    kStatus_InvalidInstructions =		0x0050,
    kStatus_DownloadFailed =			0x0051,
    kStatus_IntegrityCheckFailed =		0x0052,
    kStatus_Abort =				0x0053,
    kStatus_Retry =				0x0091,
  };
  /*
   * the frame control field of the image query frame has the following
   * control flags
   */
  enum {
    kFlag_PackageSpecPresent =                  1,
    kFlag_LocaleSpecPresent =                   2,
    kFlag_TargetNodeIdPresent =                 4,
  };
  /*
   * the (optional) update options field of the image query response (IQR) frame
   * defines a set of bitmasks.
   */
  enum {
    kMask_UpdatePriority =			0x03, // 0b00000011
    kMask_UpdateCondition =			0x1C, // 0b00011100
    kMask_ReportStatus =			0x20, // 0b00100000
  };
  /*
   * and shift offsets
   */
  enum {
    kOffset_UpdatePriority =                    0,
    kOffset_UpdateCondition =                   2,
    kOffset_ReportStatus =                      5,
  };
  /*
   * the image query frame contains information about which integrity checking
   * the client supports and the image query response contains an integrity type
   * and value for the image that the client is being instructed to download and
   * install. the supported types are:
   */
  enum {
    kIntegrityType_SHA160 = 			0, // 160-bit Secure Hash, aka SHA-1, required
    kIntegrityType_SHA256 =				1, // 256-bit Secure Hash (SHA-2)
    kIntegrityType_SHA512 =				2, // 512-bit, Secure Hash (SHA-2)
  };
  /*
   * the lengths in bytes for the integrity specification byte-strings are as follows.
   */
  enum {
    kLength_SHA160 =					20,
    kLength_SHA256 =					32,
    kLength_SHA512 =					64,
  };
  /*
   * similarly, the image query contains information about which update
   * schemes, i.e. download protocols, the client supports, and the response
   * contains a value for indicating the update scheme to use in downloading
   * the images. the supported schemes are:
   */
  enum {
    kUpdateScheme_HTTP =				0,
    kUpdateScheme_HTTPS	=				1,
    kUpdateScheme_SFTP =				2,
    kUpdateScheme_BDX =					3, // Nest Weave download protocol
  };

  /*
   * Data Element Tags for the Software Update Profile
   */
  enum
  {
    // ---- Top-level Tags ----
    //                                   Tag Type        Element Type      Disposition
    //                                   ----------------------------------------------
    kTag_InstalledLocales       = 0x00,    // Fully-Qualified  Array of strings   Required
    kTag_CertBodyId             = 0x01,    // Fully-Qualified  Integer            Required
    kTag_WirelessRegDom         = 0x02,    // Fully-Qualified  Integer            Optional
    kTag_SufficientBatterySWU   = 0x03     // Fully-Qualified  Bool               Required
  };



  /*
   * on the air and over the wire, the image announce looks like.
   *
   * | <Weave version> (==0) | <message type> (==0) | <exchange id> |
   * | <profile id> (==0x0000000C) |
   *
   * that is, it's a bare Weave application header with profile ID 0x0000000C
   * and message type 1. the structure provided here is not really used
   * except as a placeholder.
   */
  class ImageAnnounce { };
  /*
   * the image query frame has the following form.
   *
   * | <frame control> | <product specification> | <vendor specific data> (optional) |
   * | <version specification> | <locale specification> (optional) |
   * | <integrity type list> | <update scheme list > |
   *
   * where the frame control field has bit-fields as follows:
   * bit 0 - 1==vendor-specifc data present, 0==not present
   * bit 1 - 1==locale specification present, 0==not present
   *
   * the image query, as a structure reads slightly differently from the in-flight
   * representation. in particular, the version and locale are null-terminated
   * strings and both of the optional items are represented as nullable pointers
   * so there isn't a separate boolean to check.
   *
   * here's an auxiliary class that holds a list of integrity types as part of
   * the image query. these are pretty simple and are essentially wrappers
   * around a 3-element array of values drawn from the above enumeration.
   */
  class NL_DLL_EXPORT IntegrityTypeList {
  public:
    // constructor
    IntegrityTypeList();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t*);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator&);
    uint16_t packedLength();
    static WEAVE_ERROR parse(MessageIterator&, IntegrityTypeList&);
    // comparison
    bool operator == (const IntegrityTypeList&) const;
    // this is just a short list with specified length. if length is
    // 0 this indicates that the list is undefined since, to be useful,
    // one of these things has to caontain at least one element.
    uint8_t theLength;
    uint8_t theList[3];
  };
  /*
   * an auxiliary class that holds a list of update schemes as part of
   * the image query. these are pretty simple and are essenitlly wrappers
   * around a 4-element array of values drawn from the above enumeration.
   */
  class NL_DLL_EXPORT UpdateSchemeList {
  public:
    // constructor
    UpdateSchemeList();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t*);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator&);
    uint16_t packedLength();
    static WEAVE_ERROR parse(MessageIterator&, UpdateSchemeList&);
    // comparison
    bool operator == (const UpdateSchemeList&) const;
    // data members
    // this is just a short list with specified length. if length is
    // 0 this indicates that the list is undefined since, to be useful,
    // one of these things has to caontain at least one element.
    uint8_t theLength;
    uint8_t theList[4];
  };
  /*
   * an auxiliary class that represents a product specification, which
   * for our purposes will consist of:
   * - a 16-bit vendor identifer
   * - a 16-bit product identifier
   * - a 16-bit porduct revision number
   */
  class NL_DLL_EXPORT ProductSpec {
  public:
    // constructors
    ProductSpec(uint16_t, uint16_t, uint16_t);
    ProductSpec();
    // comparison
    bool operator == (const ProductSpec&) const;

    // data members
    uint16_t vendorId;
    uint16_t productId;
    uint16_t productRev;
  };
  /*
   * ImageQuery also declares a utility print method but obviously this
   * can only be defined is there's some way to do printing, e.g. on a test
   * platform or framework.
   */
  class NL_DLL_EXPORT ImageQuery {
  public:
    // constructor
    ImageQuery();
    // initializer
    WEAVE_ERROR init(ProductSpec &aProductSpec,
                     ReferencedString &aVersion, // 1 byte length
                     IntegrityTypeList &aTypeList,
                     UpdateSchemeList &aSchemeList,
                     ReferencedString *aPackage=NULL, // 1 byte length
                     ReferencedString *aLocale=NULL,
                     uint64_t aTargetNodeId=0,
                     ReferencedTLVData *aMetaData=NULL); // 2 byte length
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, ImageQuery&);
    // comparison
    bool operator == (const ImageQuery&) const;
    // utility
    void print(void);
    // data members
    ProductSpec productSpec;
    ReferencedString version;
    IntegrityTypeList integrityTypes;
    UpdateSchemeList updateSchemes;
    uint64_t targetNodeId;
    ReferencedString packageSpec;	// optional
    ReferencedString localeSpec;		// optional
    ReferencedTLVData theMetaData;	// optional
  };
  /*
   * the image query response message has the form:
   *
   * | <status report> | URI (optional) | | <version specification> (optional) |
   * | <integrity specification> (optional) | <update scheme> (optional) |
   * | <update options> (optional) |
   *
   * where all the the fields following the status report field are optional and
   * have values of significance only if the status report denotes success.
   *
   * the format of the (optional) update options field is as follows:
   *
   * |      b0...b2           b3...b4              b5        b6...b7 |
   * | <update priority> <update condition> <report status> reserved |
   *
   * there are only two update priorities:
   */
  enum UpdatePriority {
    Normal, 		// updated may be execute at leisure
    Critical, 	// update must be executed immediately
  };
  /*
   * there are 4 conditions governing update:
   *
   * - IfUnmatched, download and install the image if the version
   * specification in the response frame doesn't match the software version
   * currently installed.
   *
   * - IfLater, download and install the image if the version specification
   * in the response frame is later than the software version currently
   * installed.
   *
   * - Unconditionally, download and install the image, don't ask questions,
   * don't pass go, don't collect $200.
   *
   * - OnOptIn, download and install the image on some trigger provided
   * by an on-site user.
   */
  enum UpdateCondition {
    IfUnmatched,
    IfLater,
    Unconditionally,
    OnOptIn
  };
  /*
   * an integrity specification is a simple object that contains an
   * integrity type, as defined in the above enumeration, plus an integrity
   * check value.
   *
   * and the integrity specification object itself is sized to fit
   * the largest of these.
   */
  class NL_DLL_EXPORT IntegritySpec {
  public:
    //constructor
    IntegritySpec();
    // initializer
    WEAVE_ERROR init(uint8_t, uint8_t*);
    // packing and parsing
    WEAVE_ERROR pack(MessageIterator&);
    static WEAVE_ERROR parse(MessageIterator&, IntegritySpec&);
    // comparison
    bool operator == (const IntegritySpec&) const;
    // data members
    uint8_t type;
    uint8_t value[64];
  };
  /*
   * the image query response is only sent in the case where
   * the image query is processed successfully and produces an
   * image to download. it eszentially constitues download instructions
   * for the node the submitted the query.
   */
  class NL_DLL_EXPORT ImageQueryResponse {
  public:
    // constructor
    ImageQueryResponse();
    // initializers
    WEAVE_ERROR init (ReferencedString&, ReferencedString&, IntegritySpec&, uint8_t, UpdatePriority, UpdateCondition, bool);
    // packing and parsing
    WEAVE_ERROR pack(PacketBuffer*);
    uint16_t packedLength();
    static WEAVE_ERROR parse(PacketBuffer*, ImageQueryResponse&);
    // comparison
    bool operator == (const ImageQueryResponse&) const;
    // utility
    void print(void);
    // data members
    ReferencedString uri;
    ReferencedString versionSpec;
    IntegritySpec integritySpec;
    uint8_t updateScheme;
    UpdatePriority updatePriority;
    UpdateCondition updateCondition;
    bool reportStatus;
  };
} // namespace SoftwareUpdate
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif // _SW_UPDATE_PROFILE_H
