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
#ifndef NETWORKINFO_H
#define NETWORKINFO_H

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Core/WeaveEncoding.h>
#include <Weave/Profiles/WeaveProfiles.h>
#include "NetworkProvisioning.h"
#include <Weave/Profiles/common/CommonProfile.h>

/**
 * @file
 *    This file defines a utility class for serializing and
 *    deserializing NetworkProvisioning payloads.
 */
namespace nl {
namespace Weave {
namespace Profiles {
namespace NetworkProvisioning {


/**
 * A utility class for serializing and deserializing payloads
 * communicated via NetworkProvisioning profile: it encapsulates
 * information pertinent to the detecting and configuring networks.
 * The class relies on intermediate storage of network provisioning
 * information (intermediate between the ultimate store of the
 * information and the network payload) and uses dynamic memory
 * management to give the resulting object flexible runtime.  As such,
 * this class is not suitable for the most constrained environments,
 * but may be used on larger systems.
 */
class NL_DLL_EXPORT NetworkInfo
{
public:
    NetworkInfo();
    ~NetworkInfo();

    ::nl::Weave::Profiles::NetworkProvisioning::NetworkType
          NetworkType;              /**< The type of network. */
    int64_t NetworkId;              /**< The network id assigned to the network by the device, -1 if not specified. */

    // ---- WiFi-specific Fields ----
    char *WiFiSSID;                 /**< The WiFi SSID, or NULL if not specified. It is a NUL-terminated,
                                     * dynamically-allocated C-string, owned by the class.  Destroyed on any condition
                                     * that calls `Clear()` on the object. */
    ::nl::Weave::Profiles::NetworkProvisioning::WiFiMode
          WiFiMode;                 /**< The operating mode of the WiFi network.*/
    ::nl::Weave::Profiles::NetworkProvisioning::WiFiRole
          WiFiRole;                 /**< The role played by the device on the WiFi network. */
    ::nl::Weave::Profiles::NetworkProvisioning::WiFiSecurityType
          WiFiSecurityType;         /**< The WiFi security type. */
    uint8_t *WiFiKey;               /**< The WiFi key, or NULL if not specified. It is a dynamically allocated array
                                     * of arbitrary octets, owned by the class, with length specified by `WiFiKeyLen`.
                                     * Destroyed on any condition that calls `Clear()` on the object. */
    uint32_t WiFiKeyLen;            /**< The length in bytes of the WiFi key. */

    // ---- Thread-specific Fields ----
    char *ThreadNetworkName;        /**< The name of the Thread network, or NULL if not specified. It is a
                                     * NUL-terminated, dynamically-allocated C-string, owned by the class.  Destroyed
                                     * on any condition that calls `Clear()` on the object. */
    uint8_t *ThreadExtendedPANId;   /**< The Thread extended PAN id (8 bytes). */
    uint8_t *ThreadNetworkKey;      /**< The Thread master network key , or NULL if not specified. It is a dynamically
                                     * allocated array of arbitrary octets, owned by the class, with length specified
                                     * by `ThreadNetworkKeyLen`.  Destroyed on any condition that calls `Clear()` on
                                     * the object. */
    uint32_t ThreadNetworkKeyLen;   /**< The length in bytes of the Thread master network key. */

    // ---- General Fields ----
    int16_t WirelessSignalStrength; /**< The signal strength of the network, or INT16_MIN if not
                                     * available/applicable. */
    bool Hidden;                    /**< Whether or not the network is hidden. */


    /**
     * Replace the contents of this NetworkInfo object with the deep copy of the contents of the argument.
     *
     * @param[in] dest               NetworkInfo object containing information to be copied into this object.
     *
     * @retval #WEAVE_NO_ERROR        On success.
     *
     * @retval #WEAVE_ERROR_NOT_IMPLEMENTED When the platform does not support malloc or free.
     *
     * @retval #WEAVE_ERROR_NO_MEMORY On memory allocation failures.
     */
    WEAVE_ERROR CopyTo(NetworkInfo& dest);

    /**
     * Merge the contents of this NetworkInfo object with the deep copy of the contents of the argument. All the
     * non-default values from the argument object replace the values in this object.
     *
     * @param[in] dest               NetworkInfo object containing information to be copied into this object.
     *
     * @retval #WEAVE_NO_ERROR        On success.
     *
     * @retval #WEAVE_ERROR_NOT_IMPLEMENTED When the platform does not support malloc or free.
     *
     * @retval #WEAVE_ERROR_NO_MEMORY On memory allocation failures.
     */
    WEAVE_ERROR MergeTo(NetworkInfo& dest);

    /**
     * Reset to default and free all values within this NetworkInfo object
     */
    void Clear(void);

    enum
    {
        kEncodeFlag_EncodeCredentials   = ::nl::Weave::Profiles::NetworkProvisioning::kGetNetwork_IncludeCredentials,
        kEncodeFlag_All                 = 0xFF
    };

    /**
     * Deserialize the content of this NetworkInfo object from its TLV representation
     *
     * @param[in] reader   TLVReader positioned to the structure element containing network info.
     *
     * @returns #WEAVE_NO_ERROR On success, WEAVE_ERROR_INVALID_TLV_ELEMENT on any element not conforming to the
     *                     network provisioning profile, any of the TLV reader errors on incorrect decoding of
     *                     elements.
     */
    WEAVE_ERROR Decode(nl::Weave::TLV::TLVReader& reader);

    /**
     * Serialize the content of this NetworkInfo object into its TLV representation
     *
     * @param[in] writer TLVWriter positioned at the place where the object will be serialized to.  The function emits
     *                   an anonymous tag for this object when this object is a part of array of elements or a profile
     *                   tag for kTag_NetworkInformation when emitted as a standalone element.
     *
     * @param[in] flags  Flags controlling whether the credentials of the NetworkInfo should be serialized.
     *
     * @returns #WEAVE_NO_ERROR On success, WEAVE_ERROR_INVALID_TLV_ELEMENT on any element not conforming to the
     *                   network provisioning profile, any of the TLV reader errors on incorrect decoding of elements.
     */
    WEAVE_ERROR Encode(nl::Weave::TLV::TLVWriter& writer, uint8_t encodeFlags) const;

    /**
     * Deserialize a list of NetworkInfo elements from its TLV representation.
     *
     * @param[in] reader TLVReader positioned at the array start. On successful return, the writer is positioned after
     *                         the end of the array.
     *
     * @param[inout] elemCount On input, maximum number of elements to deserialize from the TLVReader.  On output, the
     *                         number of elements actually deserialized from the stream.
     *
     * @param[inout] elemArray A reference to the array of NetworkInfo elements that will contain the deserialized
     *                         NetworkInfo objects.  When the array is NULL, it is allocated internally by the
     *                         function below, otherwise it is assumed that the externally allocated array contains at
     *                         least elemCount objects.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     *
     * @retval #WEAVE_ERROR_NOT_IMPLEMENTED on platforms that do not support dynamic memory management.
     *
     * @retval other Errors returned from the `Decode()` function.
     */
    static WEAVE_ERROR DecodeList(nl::Weave::TLV::TLVReader& reader, uint16_t& elemCount, NetworkInfo *& elemArray);

    /**
     * Serialize an array of NetworkInfo objects into its TLV representation. The array will be an anonymous element
     * in the TLV representation.
     *
     * @param[in] writer      Appropriately positioned TLVWriter
     *
     * @param[in] arrayLen    Number of elements in the `elemArray`.
     *
     * @param[in] elemArray   The array of NetworkInfo objects to be serialized.
     *
     * @param[in] encodeFlags Flags controlling whether the credentials of the NetworkInfo should be serialized.
     *
     * @param[out] encodedElementCount The number of elements actually serialized.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     *
     * @retval Other           Errors returned from the `Encode()` function.
     *
     */
    static WEAVE_ERROR EncodeList(nl::Weave::TLV::TLVWriter& writer, uint16_t elemCount, const NetworkInfo *elemArray,
        uint8_t encodeFlags);
    /**
     * Serialize an array of NetworkInfo objects into its TLV representation selecting only networks of a specific type.
     *
     * @param[in] writer      Appropriately positioned TLVWriter
     *
     * @param[in] arrayLen    Number of elements in the `elemArray`.
     *
     * @param[in] elemArray   The array of NetworkInfo objects to be serialized.
     *
     * @param[in] networkType The type of NetworkInfo objects to serialize
     *
     * @param[in] encodeFlags Flags controlling whether the credentials of the NetworkInfo should be serialized.
     *
     * @param[out] encodedElementCount The number of elements actually serialized.
     *
     * @retval #WEAVE_NO_ERROR  On success.
     *
     * @retval Other           Errors returned from the `Encode()` function.
     *
     */
    static WEAVE_ERROR EncodeList(nl::Weave::TLV::TLVWriter& writer, uint16_t arrayLen, const NetworkInfo *elemArray,
        ::nl::Weave::Profiles::NetworkProvisioning::NetworkType networkType, uint8_t encodeFlags, uint16_t& encodedElemCount);
};

} // namespace NetworkProvisioning
} // namespace Profiles
} // namespace Weave
} // namespace nl
#endif //NETWORKINFO_H
