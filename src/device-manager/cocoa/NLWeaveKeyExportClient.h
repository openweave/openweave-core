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
 *      This file defines a Wrapper for C++ implementation of key export functionality
 *      to support pin encryption.
 *
 */

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

extern NSString *const NLWeaveKeyExportClientErrorDomain;

// Error codes for NLWeaveKeyExportClientErrorDomain
typedef NS_ENUM (NSInteger, NLWeaveKeyExportClientErrorDomainCode){
    NLWeaveKeyExportClientErrorDomainInvalidExportBufferSize = 2,
    NLWeaveKeyExportClientErrorDomainKeyExportRequestFailure = 3,
    NLWeaveKeyExportClientErrorDomainKeyExportResponseFailure = 4,
    NLWeaveKeyExportClientErrorDomainProcessReconfiugreFailure = 5,
    NLWeaveKeyExportClientErrorDomainInvalidArgument = 6
};


/**
 * @class Wrapper for C++ implementation of key export functionality to
 * support pin encryption.
 */
@interface NLWeaveKeyExportClient : NSObject

/**
 * Generate a key export request given an access token.
 *
 * @param keyId[in]             The Weave key id of the key to be exported.
 * @param responderNodeId[in]   The Weave node id of the device to which the request will be forwarded; or
 *                              0 if the particular device id is unknown.
 * @param accessToken[in]       A buffer containing a Weave access token, in Weave TLV format.
 * @param errorOut[out]         Output error parameter, set in the event an error occurs and errOut is not null.
 * @return                      Binary buffer containing the generated key export request. Set to nil if error occurs.
 */
- (nullable NSData *) generateKeyExportRequest: (UInt32) keyId
                               responderNodeId: (UInt64) responderNodeId
                                   accessToken: (NSData *) accessToken
                                         error: (NSError **) errOut;

/**
 * Generate a key export request given a client certificate and private key.
 *
 * @param keyId[in]             The Weave key id of the key to be exported.
 * @param responderNodeId[in]   The Weave node id of the device to which the request will be forwarded; or
 *                              0 if the particular device id is unknown.
 * @param clientCert[in]        A buffer containing a Weave certificate identifying the client making the request.
 *                              The certificate is expected to be encoded in Weave TLV format.
 * @param clientKey[in]         A buffer containing the private key associated with the client certificate.
 *                              The private key is expected to be encoded in Weave TLV format.
 * @param errorOut[out]         Output error parameter, set in the event an error occurs and errOut is not null.
 * @return                      Binary buffer containing the generated key export request. Set to nil if error occurs.
 */
- (nullable NSData *) generateKeyExportRequest: (UInt32) keyId
                               responderNodeId: (UInt64) responderNodeId
                                    clientCert: (NSData *) clientCert
                                     clientKey: (NSData *) clientKey
                                         error: (NSError **) errOut;

/**
 * Process the response to a previously-generated key export request.
 *
 * @param responderNodeId[in]   The Weave node id of the device to which the request was forwarded; or
 *                              0 if the particular device id is unknown.
 * @param exportResp[in]        A buffer containing a Weave key export response, as returned by the device.
 * @param errorOut[out]         Output error parameter, set in the event an error occurs and errOut is not null.
 * @return                      Binary buffer containing exported key.  Set to nil if error occurs.
 */
- (nullable NSData *) processKeyExportResponse: (UInt64) responderNodeId
                                    exportResp: (NSData *) exportResp
                                         error: (NSError **) errOut;

/**
 * Process a reconfigure message received in response to a previously-generated key export request.
 *
 * @param reconfig[in]          A buffer containing a Weave key export reconfigure message, as returned
 *                              by the device.
 * @param errorOut[out]         Output error parameter, set in the event an error occurs and errOut is not null.
 * @return                      True on success, False on failure.
 */
- (BOOL) processKeyExportReconfigure: (NSData *) reconfig
                               error: (NSError **) errOut;

/**
 * Reset the key export client object, discarding any state associated with a pending key export request.
 */
- (void) reset;

/**
 * True if key export responses from Nest development devices will be allowed.
 */
- (BOOL) allowNestDevelopmentDevices;

/**
 * Allow or disallow key export responses from Nest development devices.
 */
- (void) setAllowNestDevelopmentDevices: (BOOL) nestDev;

/**
 * True if key export responses from devices with SHA1 certificates will be allowed.
 */
- (BOOL) allowSHA1DeviceCertificates;

/**
 * Allow or disallow key export responses from devices with SHA1 certificates.
 */
- (void) setAllowSHA1DeviceCertificates: (BOOL) nestDev;

/**
 *  Initializes NLWeaveKeyExportClient object.  Creates instance and initializes instace of
 *  internal C++ object for performing key export functionality.
 */
- (instancetype) init;


@end
NS_ASSUME_NONNULL_END
