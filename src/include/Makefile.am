#
#    Copyright (c) 2014-2017 Nest Labs, Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU automake template for Weave public
#      headers, which ensures that a linked shadow set of such headers
#      is always available before building the source such that the in-
#      tree source FOR the SDK can compile against public headers exactly
#      as out-of-tree source AGAINST the SDK would.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Build Configuration
#

include_HEADERS                    = \
    BuildConfig.h                    \
    $(NULL)


#
# Default Project-Specific Configuration Headers
#

# Arrange to install the selected project-specific configuration
# headers IF they are located in the build/config source directory. If the
# user has specified their own configuration headers, then those
# headers are NOT included in the installation directory.
#
# The 'dist' target appears to have issues with absolute paths, so add
# them with relative paths to include_HEADERS which, in turn, is
# included in DIST_COMMON which, in turn, is included in DISTFILES
# which is what the 'distdir' target depended on by the 'dist' target
# works against.

include_HEADERS += $(subst $(abspath $(abs_top_srcdir)),$(top_srcdir),$(filter $(abspath $(top_srcdir)/build/config/%),$(abspath $(NL_PROJECT_CONFIG_INCLUDES))))

#
# First-party Public Headers
#

#
# BleLayer
#
# At present, the BLE layer is partitioned in two: a production
# portion under ble/ and a test protion under device-manager/.
# 

#
# BleLayer : ble
#
# Production Portion
#
nl_public_BleLayer_ble_source_dirstem = ble

nl_public_BleLayer_ble_header_sources = \
$(nl_public_BleLayer_ble_source_dirstem)/BLEEndPoint.h \
$(nl_public_BleLayer_ble_source_dirstem)/Ble.h \
$(nl_public_BleLayer_ble_source_dirstem)/BleApplicationDelegate.h \
$(nl_public_BleLayer_ble_source_dirstem)/BleConfig.h \
$(nl_public_BleLayer_ble_source_dirstem)/BleError.h \
$(nl_public_BleLayer_ble_source_dirstem)/BleLayer.h \
$(nl_public_BleLayer_ble_source_dirstem)/BlePlatformDelegate.h \
$(nl_public_BleLayer_ble_source_dirstem)/BleUUID.h \
$(nl_public_BleLayer_ble_source_dirstem)/WoBle.h \
$(NULL)

dist_ble_ble_HEADERS = $(addprefix ../,$(nl_public_BleLayer_ble_header_sources))

nl_public_BleLayer_ble_destination_dirstem = BleLayer

ble_bledir = $(includedir)/$(nl_public_BleLayer_ble_destination_dirstem)

if CONFIG_NETWORK_LAYER_BLE
nl_public_BleLayer_ble_header_paths = $(subst $(nl_public_BleLayer_ble_source_dirstem)/,,$(nl_public_BleLayer_ble_header_sources))
nl_public_BleLayer_ble_header_destinations = $(addprefix $(nl_public_BleLayer_ble_destination_dirstem)/,$(nl_public_BleLayer_ble_header_paths))

nl_public_BleLayer_ble_header_dirs = $(sort $(dir $(nl_public_BleLayer_ble_header_destinations)))

$(nl_public_BleLayer_ble_header_dirs):
	$(call create-directory)

$(nl_public_BleLayer_ble_header_destinations): $(nl_public_BleLayer_ble_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_BleLayer_ble_source_dirstem)/%.h | $(nl_public_BleLayer_ble_header_dirs)
	$(call create-link)
endif # CONFIG_NETWORK_LAYER_BLE

#
# BleLayer : device-manager
#
# Test Portion
#
# The WoBLE interface is currently exercised via the Device Manager
# interface. Consequently, when WoBLE testing is enabled, include the
# test header as a publicly-exported header.
#
nl_public_BleLayer_device_manager_source_dirstem = device-manager

nl_public_BleLayer_device_manager_header_sources = \
$(nl_public_BleLayer_device_manager_source_dirstem)/WoBleTest.h \
$(NULL)

dist_ble_device_manager_HEADERS = $(addprefix ../,$(nl_public_BleLayer_device_manager_header_sources))

nl_public_BleLayer_device_manager_destination_dirstem = BleLayer

ble_device_managerdir = $(includedir)/$(nl_public_BleLayer_device_manager_destination_dirstem)

if CONFIG_NETWORK_LAYER_BLE
if WEAVE_ENABLE_WOBLE_TEST
nl_public_BleLayer_device_manager_header_paths = $(subst $(nl_public_BleLayer_device_manager_source_dirstem)/,,$(nl_public_BleLayer_device_manager_header_sources))
nl_public_BleLayer_device_manager_header_destinations = $(addprefix $(nl_public_BleLayer_device_manager_destination_dirstem)/,$(nl_public_BleLayer_device_manager_header_paths))

nl_public_BleLayer_device_manager_header_dirs = $(sort $(dir $(nl_public_BleLayer_device_manager_header_destinations)))

$(nl_public_BleLayer_device_manager_header_dirs):
	$(call create-directory)

$(nl_public_BleLayer_device_manager_header_destinations): $(nl_public_BleLayer_device_manager_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_BleLayer_device_manager_source_dirstem)/%.h | $(nl_public_BleLayer_device_manager_header_dirs)
	$(call create-link)
endif # WEAVE_ENABLE_WOBLE_TEST
endif # CONFIG_NETWORK_LAYER_BLE

#
# InetLayer
#

nl_public_InetLayer_source_dirstem = inet

nl_always_InetLayer_header_sources = \
$(nl_public_InetLayer_source_dirstem)/EndPointBasis.h \
$(nl_public_InetLayer_source_dirstem)/IANAConstants.h \
$(nl_public_InetLayer_source_dirstem)/Inet.h \
$(nl_public_InetLayer_source_dirstem)/InetBuffer.h \
$(nl_public_InetLayer_source_dirstem)/InetConfig.h \
$(nl_public_InetLayer_source_dirstem)/InetError.h \
$(nl_public_InetLayer_source_dirstem)/InetInterface.h \
$(nl_public_InetLayer_source_dirstem)/InetLayer.h \
$(nl_public_InetLayer_source_dirstem)/InetLayerBasis.h \
$(nl_public_InetLayer_source_dirstem)/InetLayerEvents.h \
$(nl_public_InetLayer_source_dirstem)/InetTimer.h \
$(nl_public_InetLayer_source_dirstem)/IPAddress.h \
$(nl_public_InetLayer_source_dirstem)/IPEndPointBasis.h \
$(nl_public_InetLayer_source_dirstem)/IPPrefix.h \
$(nl_public_InetLayer_source_dirstem)/InetFaultInjection.h \
$(NULL)

nl_public_InetLayer_header_sources = $(nl_always_InetLayer_header_sources)

nl_dist_InetLayer_header_sources = \
$(nl_always_InetLayer_header_sources) \
$(nl_public_InetLayer_source_dirstem)/DNSResolver.h \
$(nl_public_InetLayer_source_dirstem)/RawEndPoint.h \
$(nl_public_InetLayer_source_dirstem)/TCPEndPoint.h \
$(nl_public_InetLayer_source_dirstem)/UDPEndPoint.h \
$(nl_public_InetLayer_source_dirstem)/TunEndPoint.h \
$(nl_public_InetLayer_source_dirstem)/AsyncDNSResolverSockets.h \
$(NULL)

dist_inet_HEADERS = $(addprefix ../,$(nl_dist_InetLayer_header_sources))

if INET_WANT_ENDPOINT_DNS
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/DNSResolver.h
endif # INET_WANT_ENDPOINT_DNS

if INET_WANT_ENDPOINT_RAW
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/RawEndPoint.h
endif # INET_WANT_ENDPOINT_RAW

if INET_WANT_ENDPOINT_TCP
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/TCPEndPoint.h
endif # INET_WANT_ENDPOINT_TCP

if INET_WANT_ENDPOINT_UDP
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/UDPEndPoint.h
endif # INET_WANT_ENDPOINT_UDP

if INET_WANT_ENDPOINT_TUN
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/TunEndPoint.h
endif # INET_WANT_ENDPOINT_TUN

if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
if INET_WANT_ENDPOINT_DNS
if INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
nl_public_InetLayer_header_sources += $(nl_public_InetLayer_source_dirstem)/AsyncDNSResolverSockets.h
endif # INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS
endif # INET_WANT_ENDPOINT_DNS
endif # WEAVE_SYSTEM_CONFIG_USE_SOCKETS

nl_public_InetLayer_destination_dirstem = InetLayer

nl_public_InetLayer_header_paths = $(subst $(nl_public_InetLayer_source_dirstem)/,,$(nl_public_InetLayer_header_sources))
nl_public_InetLayer_header_destinations = $(addprefix $(nl_public_InetLayer_destination_dirstem)/,$(nl_public_InetLayer_header_paths))

nl_public_InetLayer_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_InetLayer_header_destinations))))

$(nl_public_InetLayer_header_dirs):
	$(call create-directory)

$(nl_public_InetLayer_header_destinations): $(nl_public_InetLayer_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_InetLayer_source_dirstem)/%.h | $(nl_public_InetLayer_header_dirs)
	$(call create-link)

inetdir = $(includedir)/$(nl_public_InetLayer_destination_dirstem)

#
# platform/ble/bluez
#

platform_ble_bluez_public_source_dirstem = platform/ble/bluez
platform_ble_bluez_public_destination_dirstem = PlatformLayer/Ble/Bluez

platform_ble_bluez_public_header_sources =

if CONFIG_NETWORK_LAYER_BLE
if CONFIG_BLE_PLATFORM_BLUEZ
platform_ble_bluez_public_header_sources += \
$(platform_ble_bluez_public_source_dirstem)/BluezBleApplicationDelegate.h \
$(platform_ble_bluez_public_source_dirstem)/BluezBlePlatformDelegate.h \
$(platform_ble_bluez_public_source_dirstem)/WoBluezLayer.h \
$(NULL)
endif # CONFIG_BLE_PLATFORM_BLUEZ
endif # CONFIG_NETWORK_LAYER_BLE

platform_ble_bluez_public_header_paths = $(subst $(platform_ble_bluez_public_source_dirstem)/,,$(platform_ble_bluez_public_header_sources))

platform_ble_bluez_public_header_destinations = $(addprefix $(platform_ble_bluez_public_destination_dirstem)/,$(platform_ble_bluez_public_header_paths))

platform_ble_bluez_public_header_dirs = $(call Deslashify,$(sort $(dir $(platform_ble_bluez_public_header_destinations))))

$(platform_ble_bluez_public_header_dirs):
	$(call create-directory)

$(platform_ble_bluez_public_header_destinations): $(platform_ble_bluez_public_destination_dirstem)/%.h: $(abs_srcdir)/../$(platform_ble_bluez_public_source_dirstem)/%.h | $(platform_ble_bluez_public_header_dirs)
	$(call create-link)

platform_ble_bluezdir = $(includedir)/$(platform_ble_bluez_public_destination_dirstem)
dist_platform_ble_bluez_HEADERS = $(addprefix ../,$(platform_ble_bluez_public_header_sources))

#
# SystemLayer
#

nl_public_SystemLayer_source_dirstem = system
nl_public_SystemLayer_destination_dirstem = SystemLayer

nl_public_SystemLayer_header_sources                              = \
    $(nl_public_SystemLayer_source_dirstem)/SystemAlignSize.h       \
    $(nl_public_SystemLayer_source_dirstem)/SystemClock.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemConfig.h          \
    $(nl_public_SystemLayer_source_dirstem)/SystemError.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemEvent.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemFaultInjection.h  \
    $(nl_public_SystemLayer_source_dirstem)/SystemStats.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemLayer.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemMutex.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemObject.h          \
    $(nl_public_SystemLayer_source_dirstem)/SystemTimer.h           \
    $(nl_public_SystemLayer_source_dirstem)/SystemPacketBuffer.h    \
    $(NULL)

nl_public_SystemLayer_header_paths = $(subst $(nl_public_SystemLayer_source_dirstem)/,,$(nl_public_SystemLayer_header_sources))

nl_public_SystemLayer_header_destinations = $(addprefix $(nl_public_SystemLayer_destination_dirstem)/,$(nl_public_SystemLayer_header_paths))

nl_public_SystemLayer_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_SystemLayer_header_destinations))))

$(nl_public_SystemLayer_header_dirs):
	$(call create-directory)

$(nl_public_SystemLayer_header_destinations): $(nl_public_SystemLayer_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_SystemLayer_source_dirstem)/%.h | $(nl_public_SystemLayer_header_dirs)
	$(call create-link)

systemdir = $(includedir)/$(nl_public_SystemLayer_destination_dirstem)
dist_system_HEADERS = $(addprefix ../,$(nl_public_SystemLayer_header_sources))

#
# Warm
#

nl_public_Warm_source_dirstem = warm
nl_public_Warm_destination_dirstem = Warm

nl_public_Warm_header_sources = \
$(nl_public_Warm_source_dirstem)/Warm.h \
$(nl_public_Warm_source_dirstem)/WarmConfig.h \
$(NULL)

nl_public_Warm_header_paths = $(subst $(nl_public_Warm_source_dirstem)/,,$(nl_public_Warm_header_sources))

nl_public_Warm_header_destinations = $(addprefix $(nl_public_Warm_destination_dirstem)/,$(nl_public_Warm_header_paths))

nl_public_Warm_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_Warm_header_destinations))))

$(nl_public_Warm_header_dirs):
	$(call create-directory)

$(nl_public_Warm_header_destinations): $(nl_public_Warm_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_Warm_source_dirstem)/%.h | $(nl_public_Warm_header_dirs)
	$(call create-link)

warmdir = $(includedir)/$(nl_public_Warm_destination_dirstem)
dist_warm_HEADERS = $(addprefix ../,$(nl_public_Warm_header_sources))

#
# Weave
#

# Weave - Top-level
#
# Right now, this exists exclusive to serve the auto-generated
# Weave/WeaveVersion.h header.

nl_public_Weave_source_dirstem = Weave
nl_public_Weave_destination_dirstem = Weave

nl_public_Weave_header_sources = \
$(nl_public_Weave_source_dirstem)/WeaveVersion.h \
$(NULL)

nl_public_Weave_header_paths = $(subst $(nl_public_Weave_source_dirstem)/,,$(nl_public_Weave_header_sources))

nl_public_Weave_header_destinations = $(addprefix $(nl_public_Weave_destination_dirstem)/,$(nl_public_Weave_header_paths))

nl_public_Weave_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_Weave_header_destinations))))

$(nl_public_Weave_header_dirs):
	$(call create-directory)

weavedir = $(includedir)/$(nl_public_Weave_destination_dirstem)
dist_weave_HEADERS = $(addprefix ./,$(nl_public_Weave_header_sources))

# Auto-generation targets to force the creation and subsequent check
# and re-generation of Weave/WeaveVersion.h.

.PHONY: force
$(nl_public_Weave_destination_dirstem)/WeaveVersion.h: force

define check-file-WeaveVersion.h
$(top_srcdir)/build/scripts/gen-weave-version "`cat $(1)`" -o "$(2)"
endef # check-file-WeaveVersion.h

$(nl_public_Weave_destination_dirstem)/WeaveVersion.h: $(top_builddir)/.local-version | $(nl_public_Weave_destination_dirstem)
	$(call check-file,WeaveVersion.h)

# Weave - Support

nl_public_WeaveSupport_source_dirstem = lib/support
nl_public_WeaveSupport_destination_dirstem = Weave/Support

nl_public_WeaveSupport_base_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/ASN1.h \
$(nl_public_WeaveSupport_source_dirstem)/ASN1Config.h \
$(nl_public_WeaveSupport_source_dirstem)/ASN1Error.h \
$(nl_public_WeaveSupport_source_dirstem)/ASN1Macros.h \
$(nl_public_WeaveSupport_source_dirstem)/ASN1OID.h \
$(nl_public_WeaveSupport_source_dirstem)/Base64.h \
$(nl_public_WeaveSupport_source_dirstem)/CodeUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/ErrorStr.h \
$(nl_public_WeaveSupport_source_dirstem)/FibonacciUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/FlagUtils.hpp \
$(nl_public_WeaveSupport_source_dirstem)/ManagedNamespace.hpp \
$(nl_public_WeaveSupport_source_dirstem)/MathUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/NLDLLUtil.h \
$(nl_public_WeaveSupport_source_dirstem)/NestCerts.h \
$(nl_public_WeaveSupport_source_dirstem)/PersistedCounter.h \
$(nl_public_WeaveSupport_source_dirstem)/ProfileStringSupport.hpp \
$(nl_public_WeaveSupport_source_dirstem)/RandUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/SerialNumberUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/SerializationUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/TimeUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/TraitEventUtils.h \
$(nl_public_WeaveSupport_source_dirstem)/WeaveCounter.h \
$(nl_public_WeaveSupport_source_dirstem)/WeaveFaultInjection.h \
$(nl_public_WeaveSupport_source_dirstem)/WeaveNames.h \
$(nl_public_WeaveSupport_source_dirstem)/nlargparser.hpp \
$(NULL)

nl_public_WeaveSupport_crypto_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/crypto/AESBlockCipher.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/CTRMode.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/DRBG.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/EllipticCurve.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/HKDF.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/HMAC.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/HashAlgos.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/WeaveRNG.h \
$(nl_public_WeaveSupport_source_dirstem)/crypto/WeaveCrypto.h \
$(NULL)

nl_public_WeaveSupport_logging_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/logging/WeaveLogging.h \
$(nl_public_WeaveSupport_source_dirstem)/logging/DecodedIPPacket.h \
$(NULL)

nl_public_WeaveSupport_verhoeff_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/verhoeff/Verhoeff.h \
$(NULL)

nl_public_WeaveSupport_pairing_code_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/pairing-code/PairingCodeUtils.h \
$(NULL)

nl_public_WeaveSupport_platform_header_sources = \
$(nl_public_WeaveSupport_source_dirstem)/platform/PersistedStorage.h \
$(NULL)

nl_public_WeaveSupport_header_sources = \
$(nl_public_WeaveSupport_base_header_sources) \
$(nl_public_WeaveSupport_crypto_header_sources) \
$(nl_public_WeaveSupport_logging_header_sources) \
$(nl_public_WeaveSupport_verhoeff_header_sources) \
$(nl_public_WeaveSupport_pairing_code_header_sources) \
$(nl_public_WeaveSupport_platform_header_sources) \
$(NULL)

nl_public_WeaveSupport_header_paths = $(subst $(nl_public_WeaveSupport_source_dirstem)/,,$(nl_public_WeaveSupport_header_sources))

nl_public_WeaveSupport_header_destinations = $(addprefix $(nl_public_WeaveSupport_destination_dirstem)/,$(nl_public_WeaveSupport_header_paths))

nl_public_WeaveSupport_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_WeaveSupport_header_destinations))))

$(nl_public_WeaveSupport_header_dirs):
	$(call create-directory)

$(filter %.h,$(nl_public_WeaveSupport_header_destinations)): $(nl_public_WeaveSupport_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_WeaveSupport_source_dirstem)/%.h | $(nl_public_WeaveSupport_header_dirs)
	$(call create-link)

$(filter %.hpp,$(nl_public_WeaveSupport_header_destinations)): $(nl_public_WeaveSupport_destination_dirstem)/%.hpp: $(abs_srcdir)/../$(nl_public_WeaveSupport_source_dirstem)/%.hpp | $(nl_public_WeaveSupport_header_dirs)
	$(call create-link)

weave_support_basedir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)
dist_weave_support_base_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_base_header_sources))

weave_support_cryptodir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)/crypto
dist_weave_support_crypto_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_crypto_header_sources))

weave_support_loggingdir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)/logging
dist_weave_support_logging_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_logging_header_sources))

weave_support_verhoeffdir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)/verhoeff
dist_weave_support_verhoeff_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_verhoeff_header_sources))

weave_support_pairing_codedir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)/pairing-code
dist_weave_support_pairing_code_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_pairing_code_header_sources))

weave_support_platformdir = $(includedir)/$(nl_public_WeaveSupport_destination_dirstem)/platform
dist_weave_support_platform_HEADERS = $(addprefix ../,$(nl_public_WeaveSupport_platform_header_sources))

# Weave - Core

nl_public_WeaveCore_source_dirstem = lib/core
nl_public_WeaveCore_destination_dirstem = Weave/Core

nl_public_WeaveCore_header_sources = \
$(nl_public_WeaveCore_source_dirstem)/HostPortList.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveKeyIds.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveBinding.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveBDXConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveCore.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveDMConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveTimeConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveEncoding.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveError.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveEventLoggingConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveExchangeMgr.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveFabricState.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveGlobals.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveMessageLayer.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveSecurityMgr.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveServerBase.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveStats.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLV.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLVData.hpp \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLVDebug.hpp \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLVTags.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLVTypes.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveTLVUtilities.hpp \
$(nl_public_WeaveCore_source_dirstem)/WeaveCircularTLVBuffer.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveVendorIdentifiers.hpp \
$(nl_public_WeaveCore_source_dirstem)/WeaveTunnelConfig.h \
$(nl_public_WeaveCore_source_dirstem)/WeaveWRMPConfig.h \
$(NULL)

nl_public_WeaveCore_header_paths = $(subst $(nl_public_WeaveCore_source_dirstem)/,,$(nl_public_WeaveCore_header_sources))

nl_public_WeaveCore_header_destinations = $(addprefix $(nl_public_WeaveCore_destination_dirstem)/,$(nl_public_WeaveCore_header_paths))

nl_public_WeaveCore_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_WeaveCore_header_destinations))))

$(nl_public_WeaveCore_header_dirs):
	$(call create-directory)

$(filter %.h,$(nl_public_WeaveCore_header_destinations)): $(nl_public_WeaveCore_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_WeaveCore_source_dirstem)/%.h | $(nl_public_WeaveCore_header_dirs)
	$(call create-link)

$(filter %.hpp,$(nl_public_WeaveCore_header_destinations)): $(nl_public_WeaveCore_destination_dirstem)/%.hpp: $(abs_srcdir)/../$(nl_public_WeaveCore_source_dirstem)/%.hpp | $(nl_public_WeaveCore_header_dirs)
	$(call create-link)

weave_coredir = $(includedir)/$(nl_public_WeaveCore_destination_dirstem)
dist_weave_core_HEADERS = $(addprefix ../,$(nl_public_WeaveCore_header_sources))

# Weave - Profiles

nl_public_WeaveProfiles_source_dirstem = lib/profiles
nl_public_WeaveProfiles_destination_dirstem = Weave/Profiles

nl_public_WeaveProfiles_base_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/ProfileCommon.h \
$(nl_public_WeaveProfiles_source_dirstem)/WeaveProfiles.h \
$(NULL)

nl_public_WeaveProfiles_bulk_data_transfer_development_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXConstants.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXDelegate.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXManagedNamespace.hpp \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXMessages.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXNode.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXProtocol.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BDXTransferState.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/Development/BulkDataTransfer.h \
$(NULL)

nl_public_WeaveProfiles_bulk_data_transfer_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/BDXManagedNamespace.hpp \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/BulkDataTransfer.h \
$(nl_public_WeaveProfiles_source_dirstem)/bulk-data-transfer/WeaveBdxDelegate.h \
$(NULL)

nl_public_WeaveProfiles_common_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/common/CommonProfile.h \
$(nl_public_WeaveProfiles_source_dirstem)/common/WeaveMessage.h \
$(NULL)

nl_public_WeaveProfiles_data_management_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Binding.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/ClientDataManager.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/ClientNotifier.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/DMClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/DMConstants.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/DMPublisher.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/DataManagement.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/ProfileDatabase.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/ProtocolEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/PublisherDataManager.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/TopicIdentifier.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/WdmManagedNamespace.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/MessageDef.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/ViewClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/TraitData.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/TraitCatalog.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/TraitPathStore.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/SubscriptionEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/SubscriptionClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/SubscriptionHandler.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/NotificationEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Command.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/UpdateClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/UpdateEncoder.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/EventLogging.h		\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/EventLoggingTags.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/EventLoggingTypes.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/EventProcessor.h    \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/LogBDXUpload.h		\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/LoggingConfiguration.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/LoggingManagement.h	\
$(NULL)

nl_public_WeaveProfiles_data_management_legacy_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/Binding.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/ClientDataManager.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/ClientNotifier.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/DMClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/DMConstants.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/DMPublisher.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/DataManagement.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/ProfileDatabase.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/ProtocolEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/PublisherDataManager.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/TopicIdentifier.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Legacy/WdmManagedNamespace.h \
$(NULL)

nl_public_WeaveProfiles_data_management_current_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/DataManagement.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/MessageDef.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/ResourceIdentifier.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/ViewClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/TraitData.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/TraitCatalog.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/TraitPathStore.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/SubscriptionEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/SubscriptionClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/SubscriptionHandler.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/NotificationEngine.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/Command.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/UpdateClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/UpdateEncoder.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/WdmManagedNamespace.h \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/EventLogging.h		\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/EventLoggingTags.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/EventLoggingTypes.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/EventProcessor.h    \
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/LogBDXUpload.h		\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/LoggingConfiguration.h	\
$(nl_public_WeaveProfiles_source_dirstem)/data-management/Current/LoggingManagement.h	\
$(NULL)

nl_public_WeaveProfiles_device_control_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/device-control/DeviceControl.h \
$(NULL)

nl_public_WeaveProfiles_device_description_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/device-description/DeviceDescription.h \
$(NULL)

nl_public_WeaveProfiles_echo_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/echo/WeaveEcho.h \
$(NULL)

nl_public_WeaveProfiles_echo_current_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/echo/Current/WeaveEcho.h \
$(NULL)

nl_public_WeaveProfiles_echo_next_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/echo/Next/WeaveEcho.h \
$(nl_public_WeaveProfiles_source_dirstem)/echo/Next/WeaveEchoClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/echo/Next/WeaveEchoServer.h \
$(NULL)

nl_public_WeaveProfiles_fabric_provisioning_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/fabric-provisioning/FabricProvisioning.h \
$(NULL)

nl_public_WeaveProfiles_heartbeat_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/heartbeat/WeaveHeartbeat.h \
$(NULL)

nl_public_WeaveProfiles_locale_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/locale/LocaleProfile.hpp \
$(nl_public_WeaveProfiles_source_dirstem)/locale/LocaleStatus.hpp \
$(nl_public_WeaveProfiles_source_dirstem)/locale/LocaleTags.hpp \
$(NULL)

nl_public_WeaveProfiles_network_provisioning_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/network-provisioning/NetworkInfo.h         \
$(nl_public_WeaveProfiles_source_dirstem)/network-provisioning/NetworkProvisioning.h \
$(NULL)

nl_public_WeaveProfiles_security_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/security/ApplicationKeysStructSchema.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/ApplicationKeysTrait.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/ApplicationKeysTraitDataSink.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveAccessToken.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveAppGroupGlobalIds.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveApplicationKeys.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveCASE.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveCert.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveDummyGroupKeyStore.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeavePASE.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveTAKE.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveKeyExport.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveKeyExportClient.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeavePasscodes.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeavePrivateKey.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveProvBundle.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveProvHash.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveSecurity.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveSecurityDebug.h \
$(nl_public_WeaveProfiles_source_dirstem)/security/WeaveSig.h \
$(NULL)

nl_public_WeaveProfiles_service_directory_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/service-directory/ServiceDirectory.h \
$(NULL)

nl_public_WeaveProfiles_service_provisioning_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/service-provisioning/ServiceProvisioning.h \
$(NULL)

nl_public_WeaveProfiles_software_update_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/software-update/SoftwareUpdateProfile.h \
$(nl_public_WeaveProfiles_source_dirstem)/software-update/WeaveImageAnnounceServer.h \
$(NULL)

nl_public_WeaveProfiles_time_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/time/WeaveTime.h \
$(NULL)

nl_public_WeaveProfiles_token_pairing_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/token-pairing/TokenPairing.h \
$(NULL)

nl_public_WeaveProfiles_status_report_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/status-report/StatusReportProfile.h \
$(NULL)

nl_public_WeaveProfiles_weave_tunneling_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/weave-tunneling/WeaveTunnelAgent.h \
$(nl_public_WeaveProfiles_source_dirstem)/weave-tunneling/WeaveTunnelControl.h \
$(nl_public_WeaveProfiles_source_dirstem)/weave-tunneling/WeaveTunnelCommon.h \
$(nl_public_WeaveProfiles_source_dirstem)/weave-tunneling/WeaveTunnelConnectionMgr.h \
$(NULL)

nl_public_WeaveProfiles_nestlabs_device_description_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/vendor/nestlabs/device-description/NestProductIdentifiers.hpp \
$(NULL)


nl_public_WeaveProfiles_nestlabs_thermostat_header_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/vendor/nestlabs/thermostat/NestThermostatWeaveConstants.h \
$(NULL)

nl_public_WeaveProfiles_nestlabs_dropcam_legacy_pairing_sources = \
$(nl_public_WeaveProfiles_source_dirstem)/vendor/nestlabs/dropcam-legacy-pairing/DropcamLegacyPairing.h \
$(NULL)

nl_public_WeaveProfiles_header_sources = \
$(nl_public_WeaveProfiles_base_header_sources) \
$(nl_public_WeaveProfiles_bulk_data_transfer_development_header_sources) \
$(nl_public_WeaveProfiles_bulk_data_transfer_header_sources) \
$(nl_public_WeaveProfiles_common_header_sources) \
$(nl_public_WeaveProfiles_data_management_header_sources) \
$(nl_public_WeaveProfiles_data_management_current_header_sources) \
$(nl_public_WeaveProfiles_device_control_header_sources) \
$(nl_public_WeaveProfiles_device_description_header_sources) \
$(nl_public_WeaveProfiles_echo_header_sources) \
$(nl_public_WeaveProfiles_echo_current_header_sources) \
$(nl_public_WeaveProfiles_echo_next_header_sources) \
$(nl_public_WeaveProfiles_fabric_provisioning_header_sources) \
$(nl_public_WeaveProfiles_heartbeat_header_sources) \
$(nl_public_WeaveProfiles_locale_header_sources) \
$(nl_public_WeaveProfiles_network_provisioning_header_sources) \
$(nl_public_WeaveProfiles_security_header_sources) \
$(nl_public_WeaveProfiles_service_directory_header_sources) \
$(nl_public_WeaveProfiles_service_provisioning_header_sources) \
$(nl_public_WeaveProfiles_software_update_header_sources) \
$(nl_public_WeaveProfiles_status_report_header_sources) \
$(nl_public_WeaveProfiles_time_header_sources) \
$(nl_public_WeaveProfiles_token_pairing_header_sources) \
$(nl_public_WeaveProfiles_weave_tunneling_header_sources) \
$(nl_public_WeaveProfiles_nestlabs_device_description_header_sources) \
$(nl_public_WeaveProfiles_nestlabs_thermostat_header_sources) \
$(nl_public_WeaveProfiles_nestlabs_dropcam_legacy_pairing_sources) \
$(NULL)

if WEAVE_BUILD_LEGACY_WDM
nl_public_WeaveProfiles_header_sources += \
$(nl_public_WeaveProfiles_data_management_legacy_header_sources) \
$(NULL)
endif

nl_public_WeaveProfiles_header_paths = $(subst $(nl_public_WeaveProfiles_source_dirstem)/,,$(nl_public_WeaveProfiles_header_sources))

nl_public_WeaveProfiles_header_destinations = $(addprefix $(nl_public_WeaveProfiles_destination_dirstem)/,$(nl_public_WeaveProfiles_header_paths))

nl_public_WeaveProfiles_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_WeaveProfiles_header_destinations))))

$(nl_public_WeaveProfiles_header_dirs):
	$(call create-directory)

$(filter %.h,$(nl_public_WeaveProfiles_header_destinations)): $(nl_public_WeaveProfiles_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_WeaveProfiles_source_dirstem)/%.h | $(nl_public_WeaveProfiles_header_dirs)
	$(call create-link)

$(filter %.hpp,$(nl_public_WeaveProfiles_header_destinations)): $(nl_public_WeaveProfiles_destination_dirstem)/%.hpp: $(abs_srcdir)/../$(nl_public_WeaveProfiles_source_dirstem)/%.hpp | $(nl_public_WeaveProfiles_header_dirs)
	$(call create-link)

weave_profiles_basedir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)
weave_profiles_base_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_base_header_sources))

weave_profiles_bulk_data_transferdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/bulk-data-transfer
weave_profiles_bulk_data_transfer_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_bulk_data_transfer_header_sources))

weave_profiles_bulk_data_transfer_currentdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/bulk-data-transfer/Current
weave_profiles_bulk_data_transfer_current_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_bulk_data_transfer_current_header_sources))

weave_profiles_bulk_data_transfer_developmentdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/bulk-data-transfer/Development
weave_profiles_bulk_data_transfer_development_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_bulk_data_transfer_development_header_sources))

weave_profiles_commondir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/common
weave_profiles_common_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_common_header_sources))

weave_profiles_data_managementdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/data-management
weave_profiles_data_management_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_data_management_header_sources))

weave_profiles_data_management_legacydir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/data-management/Legacy
weave_profiles_data_management_legacy_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_data_management_legacy_header_sources))

weave_profiles_data_management_currentdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/data-management/Current
weave_profiles_data_management_current_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_data_management_current_header_sources))

weave_profiles_device_controldir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/device-control
weave_profiles_device_control_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_device_control_header_sources))

weave_profiles_device_descriptiondir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/device-description
weave_profiles_device_description_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_device_description_header_sources))

weave_profiles_echodir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/echo
weave_profiles_echo_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_echo_header_sources))

weave_profiles_echo_currentdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/echo/Current
weave_profiles_echo_current_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_echo_current_header_sources))

weave_profiles_echo_nextdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/echo/Next
weave_profiles_echo_next_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_echo_next_header_sources))

weave_profiles_fabric_provisioningdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/fabric-provisioning
weave_profiles_fabric_provisioning_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_fabric_provisioning_header_sources))

weave_profiles_heartbeatdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/heartbeat
weave_profiles_heartbeat_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_heartbeat_header_sources))

weave_profiles_localedir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/locale
weave_profiles_locale_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_locale_header_sources))

weave_profiles_network_provisioningdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/network-provisioning
weave_profiles_network_provisioning_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_network_provisioning_header_sources))

weave_profiles_securitydir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/security
weave_profiles_security_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_security_header_sources))

weave_profiles_service_directorydir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/service-directory
weave_profiles_service_directory_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_service_directory_header_sources))

weave_profiles_service_provisioningdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/service-provisioning
weave_profiles_service_provisioning_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_service_provisioning_header_sources))

weave_profiles_software_updatedir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/software-update
weave_profiles_software_update_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_software_update_header_sources))

weave_profiles_status_reportdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/status-report
weave_profiles_status_report_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_status_report_header_sources))

weave_profiles_timedir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/time
weave_profiles_time_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_time_header_sources))

weave_profiles_token_pairingdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/token-pairing
weave_profiles_token_pairing_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_token_pairing_header_sources))

weave_profiles_weave_tunnelingdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/weave-tunneling
weave_profiles_weave_tunneling_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_weave_tunneling_header_sources))

weave_profiles_nestlabs_device_descriptiondir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/device-description
weave_profiles_nestlabs_device_description_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_device_description_header_sources))

weave_profiles_nestlabs_heat_linkdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/heat-link
weave_profiles_nestlabs_heat_link_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_heat_link_header_sources))

weave_profiles_nestlabs_protectdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/protect
weave_profiles_nestlabs_protect_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_protect_header_sources))

weave_profiles_nestlabs_thermostatdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/thermostat
weave_profiles_nestlabs_thermostat_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_thermostat_header_sources))

weave_profiles_nestlabs_time_variant_datadir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/time-variant-data
weave_profiles_nestlabs_time_variant_data_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_time_variant_data_header_sources))

weave_profiles_nestlabs_boilerdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/boiler
weave_profiles_nestlabs_boiler_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_boiler_header_sources))

weave_profiles_nestlabs_dropcam_legacy_pairingdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/dropcam-legacy-pairing
weave_profiles_nestlabs_dropcam_legacy_pairing_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_dropcam_legacy_pairing_sources))

weave_profiles_nestlabs_domestic_hot_waterdir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/domestic-hot-water
weave_profiles_nestlabs_domestic_hot_water_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_domestic_hot_water_header_sources))

weave_profiles_nestlabs_hvac_equipment_controldir = $(includedir)/$(nl_public_WeaveProfiles_destination_dirstem)/vendor/nestlabs/hvac/equipment-control
weave_profiles_nestlabs_hvac_equipment_control_HEADERS = $(addprefix ../,$(nl_public_WeaveProfiles_nestlabs_hvac_equipment_control_header_sources))

#
# Common defines from Schema, to be used across the ecosystem
#

nl_public_WeaveCommon_source_dirstem = lib/schema/common
nl_public_WeaveCommon_destination_dirstem = Weave/Common

nl_public_WeaveCommon_header_sources = \
$(nl_public_WeaveCommon_source_dirstem)/ResourceTypeEnum.h \
$(NULL)

nl_public_WeaveCommon_header_paths = $(subst $(nl_public_WeaveCommon_source_dirstem)/,,$(nl_public_WeaveCommon_header_sources))

nl_public_WeaveCommon_header_destinations = $(addprefix $(nl_public_WeaveCommon_destination_dirstem)/,$(nl_public_WeaveCommon_header_paths))

nl_public_WeaveCommon_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_WeaveCommon_header_destinations))))

$(nl_public_WeaveCommon_header_dirs):
	$(call create-directory)

$(filter %.h,$(nl_public_WeaveCommon_header_destinations)): $(nl_public_WeaveCommon_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_WeaveCommon_source_dirstem)/%.h | $(nl_public_WeaveCommon_header_dirs)
	$(call create-link)

$(filter %.hpp,$(nl_public_WeaveCommon_header_destinations)): $(nl_public_WeaveCommon_destination_dirstem)/%.hpp: $(abs_srcdir)/../$(nl_public_WeaveCommon_source_dirstem)/%.hpp | $(nl_public_WeaveCommon_header_dirs)
	$(call create-link)

weave_commondir = $(includedir)/$(nl_public_WeaveCommon_destination_dirstem)
dist_weave_common_HEADERS = $(addprefix ../,$(nl_public_WeaveCommon_header_sources))

#
# Weave Device Layer Headers
#

if WEAVE_DEVICE_LAYER_TARGET_ESP32

nl_public_WeaveDeviceLayer_target_header_sources =                                                  \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/BLEManagerImpl.h                                 \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/ConfigurationManagerImpl.h                       \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/ConnectivityManagerImpl.h                        \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/ESP32Config.h                                    \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/ESP32Utils.h                                     \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/GroupKeyStoreImpl.h                              \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/NetworkProvisioningServerImpl.h                  \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/PlatformManagerImpl.h                            \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/WeaveDevicePlatformConfig.h                      \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ESP32/WeaveDevicePlatformEvent.h                       \
$(nl_public_WeaveDeviceLayer_source_dirstem)/FreeRTOS/GenericPlatformManagerImpl_FreeRTOS.h         \
$(nl_public_WeaveDeviceLayer_source_dirstem)/FreeRTOS/GenericPlatformManagerImpl_FreeRTOS.ipp       \
$(NULL)

endif # WEAVE_DEVICE_LAYER_TARGET_ESP32

if CONFIG_DEVICE_LAYER

nl_public_WeaveDeviceLayer_source_dirstem = adaptations/device-layer/include/Weave/DeviceLayer
nl_public_WeaveDeviceLayer_destination_dirstem = Weave/DeviceLayer

nl_public_WeaveDeviceLayer_base_header_sources =                                      \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ConfigurationManager.h                   \
$(nl_public_WeaveDeviceLayer_source_dirstem)/ConnectivityManager.h                    \
$(nl_public_WeaveDeviceLayer_source_dirstem)/GeneralUtils.h                           \
$(nl_public_WeaveDeviceLayer_source_dirstem)/PlatformManager.h                        \
$(nl_public_WeaveDeviceLayer_source_dirstem)/TimeSyncManager.h                        \
$(nl_public_WeaveDeviceLayer_source_dirstem)/TraitManager.h                           \
$(nl_public_WeaveDeviceLayer_source_dirstem)/WeaveDeviceConfig.h                      \
$(nl_public_WeaveDeviceLayer_source_dirstem)/WeaveDeviceError.h                       \
$(nl_public_WeaveDeviceLayer_source_dirstem)/WeaveDeviceEvent.h                       \
$(nl_public_WeaveDeviceLayer_source_dirstem)/WeaveDeviceLayer.h                       \
$(NULL)

nl_public_WeaveDeviceLayer_internal_header_sources =                                                \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/BLEManager.h                                  \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/DeviceControlServer.h                         \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/DeviceDescriptionServer.h                     \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/DeviceIdentityTraitDataSource.h               \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/EchoServer.h                                  \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/FabricProvisioningServer.h                    \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericConfigurationManagerImpl.h             \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericConfigurationManagerImpl.ipp           \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericNetworkProvisioningServerImpl.h        \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericNetworkProvisioningServerImpl.ipp      \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericPlatformManagerImpl.h                  \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/GenericPlatformManagerImpl.ipp                \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/NetworkInfo.h                                 \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/NetworkProvisioningServer.h                   \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/ServiceDirectoryManager.h                     \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/ServiceProvisioningServer.h                   \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/ServiceTunnelAgent.h                          \
$(nl_public_WeaveDeviceLayer_source_dirstem)/internal/WeaveDeviceLayerInternal.h                    \
$(NULL)

nl_public_WeaveDeviceLayer_header_sources = \
$(nl_public_WeaveDeviceLayer_base_header_sources) \
$(nl_public_WeaveDeviceLayer_internal_header_sources) \
$(nl_public_WeaveDeviceLayer_target_header_sources) \
$(NULL)

nl_public_WeaveDeviceLayer_header_paths = $(subst $(nl_public_WeaveDeviceLayer_source_dirstem)/,,$(nl_public_WeaveDeviceLayer_header_sources))

nl_public_WeaveDeviceLayer_header_destinations = $(addprefix $(nl_public_WeaveDeviceLayer_destination_dirstem)/,$(nl_public_WeaveDeviceLayer_header_paths))

nl_public_WeaveDeviceLayer_header_dirs = $(call Deslashify,$(sort $(dir $(nl_public_WeaveDeviceLayer_header_destinations))))

$(nl_public_WeaveDeviceLayer_header_dirs):
	$(call create-directory)

$(filter %.h,$(nl_public_WeaveDeviceLayer_header_destinations)): $(nl_public_WeaveDeviceLayer_destination_dirstem)/%.h: $(abs_srcdir)/../$(nl_public_WeaveDeviceLayer_source_dirstem)/%.h | $(nl_public_WeaveDeviceLayer_header_dirs)
	$(call create-link)

$(filter %.hpp,$(nl_public_WeaveDeviceLayer_header_destinations)): $(nl_public_WeaveDeviceLayer_destination_dirstem)/%.hpp: $(abs_srcdir)/../$(nl_public_WeaveDeviceLayer_source_dirstem)/%.hpp | $(nl_public_WeaveDeviceLayer_header_dirs)
	$(call create-link)

$(filter %.ipp,$(nl_public_WeaveDeviceLayer_header_destinations)): $(nl_public_WeaveDeviceLayer_destination_dirstem)/%.ipp: $(abs_srcdir)/../$(nl_public_WeaveDeviceLayer_source_dirstem)/%.ipp | $(nl_public_WeaveDeviceLayer_header_dirs)
	$(call create-link)

weave_device_layer_basedir = $(includedir)/$(nl_public_WeaveDeviceLayer_destination_dirstem)
dist_weave_device_layer_base_HEADERS = $(addprefix ../,$(nl_public_WeaveDeviceLayer_base_header_sources))

weave_device_layer_internaldir = $(includedir)/$(nl_public_WeaveDeviceLayer_destination_dirstem)/internal
dist_weave_device_layer_internal_HEADERS = $(addprefix ../,$(nl_public_WeaveDeviceLayer_internal_header_sources))

weave_device_layer_targetdir = $(includedir)/$(nl_public_WeaveDeviceLayer_destination_dirstem)/$(WEAVE_DEVICE_LAYER_TARGET)
dist_weave_device_layer_target_HEADERS = $(addprefix ../,$(nl_public_WeaveDeviceLayer_target_header_sources))

weave_device_layer_freertosdir = $(includedir)/$(nl_public_WeaveDeviceLayer_destination_dirstem)/FreeRTOS
dist_weave_device_layer_freertos_HEADERS = $(addprefix ../,$(nl_public_WeaveDeviceLayer_target_header_sources))

endif # CONFIG_DEVICE_LAYER


#
# Third-party Public Headers
#

#
# micro-ecc
#

tps_public_micro_ecc_source_dirstem = ../third_party/micro-ecc/micro-ecc
tps_public_micro_ecc_destination_dirstem = micro-ecc

tps_public_micro_ecc_header_sources = \
$(tps_public_micro_ecc_source_dirstem)/uECC.h \
$(tps_public_micro_ecc_source_dirstem)/uECC_vli.h \
$(tps_public_micro_ecc_source_dirstem)/types.h \
$(NULL)

tps_public_micro_ecc_header_paths = $(subst $(tps_public_micro_ecc_source_dirstem)/,,$(tps_public_micro_ecc_header_sources))

tps_public_micro_ecc_header_destinations = $(addprefix $(tps_public_micro_ecc_destination_dirstem)/,$(tps_public_micro_ecc_header_paths))

tps_public_micro_ecc_header_dirs = $(call Deslashify,$(sort $(dir $(tps_public_micro_ecc_header_destinations))))

$(tps_public_micro_ecc_header_dirs):
	$(call create-directory)

$(tps_public_micro_ecc_header_destinations): $(tps_public_micro_ecc_destination_dirstem)/%.h: $(abs_srcdir)/../$(tps_public_micro_ecc_source_dirstem)/%.h | $(tps_public_micro_ecc_header_dirs)
	$(call create-link)

micro_eccdir = $(includedir)/$(tps_public_micro_ecc_destination_dirstem)
dist_micro_ecc_HEADERS = $(addprefix ../,$(tps_public_micro_ecc_header_sources))

#
# mincrypt
#

tps_public_mincrypt_source_dirstem = ../third_party/android/platform-system/core-mincrypt/include/mincrypt
tps_public_mincrypt_destination_dirstem = mincrypt

tps_public_mincrypt_header_sources = \
$(tps_public_mincrypt_source_dirstem)/sha.h \
$(tps_public_mincrypt_source_dirstem)/sha256.h \
$(tps_public_mincrypt_source_dirstem)/hash-internal.h \
$(NULL)

tps_public_mincrypt_header_paths = $(subst $(tps_public_mincrypt_source_dirstem)/,,$(tps_public_mincrypt_header_sources))

tps_public_mincrypt_header_destinations = $(addprefix $(tps_public_mincrypt_destination_dirstem)/,$(tps_public_mincrypt_header_paths))

tps_public_mincrypt_header_dirs = $(call Deslashify,$(sort $(dir $(tps_public_mincrypt_header_destinations))))

$(tps_public_mincrypt_header_dirs):
	$(call create-directory)

$(tps_public_mincrypt_header_destinations): $(tps_public_mincrypt_destination_dirstem)/%.h: $(abs_srcdir)/../$(tps_public_mincrypt_source_dirstem)/%.h | $(tps_public_mincrypt_header_dirs)
	$(call create-link)

mincryptdir = $(includedir)/$(tps_public_mincrypt_destination_dirstem)
dist_mincrypt_HEADERS = $(addprefix ../,$(tps_public_mincrypt_header_sources))

#
# All header destinations to create or shadow by symbolic link.
#

nl_public_headers                                           = \
    $(nl_public_BleLayer_ble_header_destinations)             \
    $(nl_public_BleLayer_device_manager_header_destinations)  \
    $(nl_public_InetLayer_header_destinations)                \
    $(nl_public_SystemLayer_header_destinations)              \
    $(nl_public_Warm_header_destinations)                     \
    $(nl_public_Weave_header_destinations)                    \
    $(nl_public_WeaveSupport_header_destinations)             \
    $(nl_public_WeaveCore_header_destinations)                \
    $(nl_public_WeaveCommon_header_destinations)              \
    $(nl_public_WeaveProfiles_header_destinations)            \
    $(nl_public_WeaveDeviceLayer_header_destinations)         \
    $(NULL)

platform_public_headers                                     = \
    $(platform_ble_bluez_public_header_destinations)          \
    $(NULL)

tps_public_headers                                          = \
    $(tps_public_micro_ecc_header_destinations)               \
    $(tps_public_mincrypt_header_destinations)                \
    $(NULL)

BUILT_SOURCES                                               = \
    $(nl_public_headers)                                      \
    $(platform_public_headers)                                \
    $(tps_public_headers)                                     \
    $(NULL)

CLEANFILES                                                  = \
    $(nl_public_headers)                                      \
    $(tps_public_headers)                                     \
    $(NULL)

#
# Do not attept to lint or prettyify third-party public
# headers.
#

#
# Headers to filter out of PRETTY_FILES
#

platform_public_header_sources                           = \
    $(dist_platform_ble_bluez_HEADERS)                     \
    $(NULL)

tps_public_header_sources                                = \
    $(dist_micro_ecc_HEADERS)                              \
    $(dist_mincrypt_HEADERS)                               \
    $(NULL)

nl_filtered_pretty_files                                 = \
    $(include_HEADERS)                                     \
    $(platform_public_header_sources)                      \
    $(tps_public_header_sources)                           \
    $(NULL)

PRETTY_FILES                                             = \
    $(filter-out $(nl_filtered_pretty_files),$(HEADERS))   \
    $(NULL)

nl_public_header_dirs                                    = \
    $(nl_public_BleLayer_ble_header_dirs)                  \
    $(nl_public_BleLayer_device_manager_header_dirs)       \
    $(nl_public_InetLayer_header_dirs)                     \
    $(nl_public_SystemLayer_header_dirs)                   \
    $(nl_public_Warm_header_dirs)                          \
    $(nl_public_WeaveCore_header_dirs)                     \
    $(nl_public_WeaveCommon_header_dirs)                     \
    $(nl_public_WeaveProfiles_header_dirs)                 \
    $(nl_public_WeaveSupport_header_dirs)                  \
    $(nl_public_Weave_header_dirs)                         \
    $(NULL)

platform_public_header_dirs                              = \
    $(platform_ble_bluez_public_header_dirs)               \
    $(NULL)

tps_public_header_dirs                                   = \
    $(tps_public_micro_ecc_header_dirs)                    \
    $(tps_public_mincrypt_header_dirs)                     \
    $(NULL)

# Ensure that auto-generated content such as BuildConfig.h and
# Weave/WeaveVersion.h, while public API headers, are scrubbed out of
# the source distribution.

dist-hook:
	$(AM_V_at)rm -f $(distdir)/BuildConfig.h
	$(AM_V_at)rm -f $(distdir)/$(nl_public_Weave_destination_dirstem)/WeaveVersion.h
	$(AM_V_at)rmdir $(distdir)/$(nl_public_Weave_destination_dirstem)

install-headers: install-data

# Ensure that any created parent directories from symlinking are also
# cleaned up.

clean-local:
	$(AM_V_at)rm -f -r $(nl_public_header_dirs) $(platform_public_header_dirs) $(tps_public_header_dirs)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
