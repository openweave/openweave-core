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
#      This file is the GNU automake template for the Nest Weave
#      Cocoa Device Manager library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                               = \
    NLWeaveBleDelegate_Protected.h         \
    $(NULL)

if WEAVE_WITH_COCOA
lib_LIBRARIES                            = libNLWeaveDeviceManager.a

libNLWeaveDeviceManager_a_CPPFLAGS       = \
    -I$(top_srcdir)/src/device-manager     \
    -I$(top_srcdir)/src/include            \
    $(LWIP_CPPFLAGS)                       \
    $(SOCKETS_CPPFLAGS)                    \
    $(NULL)

libNLWeaveDeviceManager_adir             = $(includedir)/NLWeaveDeviceManager

libNLWeaveDeviceManager_a_HEADERS        = \
    Base64Encoding.h                       \
    NLIdentifyDeviceCriteria.h             \
    NLLogging.h                            \
    NLNetworkInfo.h                        \
    NLPairingCodeUtils.h                   \
    NLProfileStatusError.h                 \
    NLServiceInfo.h                        \
    NLWeaveASN1ErrorCodes.h                \
    NLWeaveDeviceDescriptor.h              \
    NLWeaveDeviceManager.h                 \
    NLWeaveDeviceManagerTypes.h            \
    NLWeaveErrorCodes.h                    \
    NLWeaveError.h                         \
    NLWeaveInetErrorCodes.h                \
    NLWeaveBleDelegate.h                   \
    NLWeaveStack.h                         \
    NLWeaveConstants.h                     \
    NLNevisPairingCodeDecoding.h           \
    NLWeaveKeyIds.h                        \
    NLHKDF.h                               \
    NLWeavePasscodeEncryptionSupport.h     \
    NLWeaveKeyExportClient.h               \
    NLWeaveKeyExportSupport.h              \
    NLWdmClient.h                          \
    NLGenericTraitUpdatableDataSink.h      \
    NLResourceIdentifier.h                 \
    NLWdmClientFlushUpdateError.h          \
    NLWdmClientFlushUpdateDeviceStatusError.h       \
    $(NULL)

noinst_HEADERS                           = \
    NLIdentifyDeviceCriteria_Protected.h   \
    NLNetworkInfo_Protected.h              \
    NLWeaveDeviceDescriptor_Protected.h    \
    NLWeaveError_Protected.h               \
    NLWeaveDeviceManager_Protected.h       \
    NLWdmClient_Protected.h                \
    NLResourceIdentifier_Protected.h       \
    NLGenericTraitUpdatableDataSink_Protected.h     \
    $(NULL)

libNLWeaveDeviceManager_a_SOURCES        = \
    Base64Encoding.mm                      \
    NLIdentifyDeviceCriteria.mm            \
    NLNetworkInfo.mm                       \
    NLPairingCodeUtils.mm                  \
    NLProfileStatusError.mm                \
    NLServiceInfo.mm                       \
    NLWeaveDeviceDescriptor.mm             \
    NLWeaveDeviceManager.mm                \
    NLWeaveError.mm                        \
    NLWeaveLogging.mm                      \
    NLWeaveBleDelegate.mm                  \
    NLWeaveStack.mm                        \
    NLWeaveConstants.mm                    \
    NLNevisPairingCodeDecoding.mm          \
    NLWeaveKeyIds.mm                       \
    NLHKDF.mm                              \
    NLWeavePasscodeEncryptionSupport.mm    \
    NLWeaveKeyExportClient.mm              \
    NLWeaveKeyExportSupport.mm             \
    NLWdmClient.mm                         \
    NLGenericTraitUpdatableDataSink.mm     \
    NLResourceIdentifier.mm                \
    NLWdmClientFlushUpdateError.mm         \
    NLWdmClientFlushUpdateDeviceStatusError.mm                \
    $(NULL)

endif # WEAVE_WITH_COCOA

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
