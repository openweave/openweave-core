#
#    Copyright (c) 2018-2019 Google LLC
#    Copyright (c) 2014-2018 Nest Labs, Inc.
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
#      test applications
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Other files we do want to distribute with the Weave source SDK.
#
# XXX - Should these be sanitized out for external distribution? If so,
#       which?
#
EXTRA_DIST                                            = \
    AndroidTest.cpp					\
    ComplexMockTrait.proto				\
    SimpleMockTrait.proto				\
    TestDNS.cpp						\
    TestWeaveDeviceMangerJNI				\
    TestWeaveTool.py                                    \
    connect-BR-Svc-ns.sh				\
    happy/bin                                           \
    happy/lib                                           \
    happy/test-templates                                \
    happy/tests/service					\
    happy/tests/standalone				\
    happy/topologies     				\
    nlweavebdxclient-development.cpp			\
    nlweavebdxclient.cpp				\
    nlweavebdxserver-development.cpp			\
    nlweavebdxserver.h					\
    remove-tunnel-intf.sh				\
    remove-weave-devs.sh				\
    run-security-support-test.sh                        \
    schema/nest/test/trait/TestDTrait.cpp               \
    schema/nest/test/trait/TestFTrait.cpp               \
    setup-network-namespaces.sh				\
    setup-tunnel-intf.sh				\
    setup-weave-devs.sh					\
    test-Verhoeff.sh                                    \
    test-bdx-development.sh				\
    test-file-development.txt				\
    test-weave-device-descriptor-encode.sh		\
    weave-bdx-client.cpp				\
    weave-parallel-test-engine				\
    $(NULL)

SUBDIRS                                               = \
    wrapper-tests/jni                                   \
    $(NULL)

#
# Define a package-specific test driver (for 'make check').
#

LOG_DRIVER=$(abs_srcdir)/weave-parallel-test-engine

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                                             = \
    CASEOptions.h                                            \
    DMTestClient.h                                           \
    DeviceDescOptions.h                                      \
    KeyExportOptions.h                                       \
    MockDCLPServer.h                                         \
    MockDCServer.h                                           \
    MockDDServer.h                                           \
    MockDMPublisher.h                                        \
    MockEvents.h                                             \
    MockExternalEvents.h                                     \
    MockFPServer.h                                           \
    MockIAServer.h                                           \
    MockLoggingManager.h                                     \
    MockMismatchedSchemaSinkAndSource.h                      \
    MockNPServer.h                                           \
    MockOpActions.h                                          \
    MockPairingServer.h                                      \
    MockPlatformClocks.h                                     \
    MockSDServer.h                                           \
    MockSPServer.h                                           \
    MockSWUServer.h                                          \
    MockSinkTraits.h                                         \
    MockSourceTraits.h                                       \
    MockTestBTrait.h                                         \
    MockTimeSyncClient.h                                     \
    MockTimeSyncCoordinator.h                                \
    MockTimeSyncServer.h                                     \
    MockTimeSyncUtil.h                                       \
    MockTokenPairingServer.h                                 \
    MockWdmNodeOptions.h                                     \
    MockWdmSubscriptionInitiator.h                           \
    MockWdmSubscriptionResponder.h                           \
    MockWdmTestVerifier.h                                    \
    MockWdmViewClient.h                                      \
    MockWdmViewServer.h                                      \
    PASEEngineTest.h                                         \
    TAKEOptions.h                                            \
    TestDRBG.h                                               \
    TestEventLoggingSchemaExamples.h                         \
    TestGroupKeyStore.h                                      \
    TestInetLayerCommon.hpp                                  \
    TestPersistedStorageImplementation.h                     \
    TestProfile.h                                            \
    TestWRMP.h                                               \
    TestWdmOneWayCommand.h                                   \
    TestWdmSubscriptionlessNotification.h                    \
    TestWeaveCertData.h                                      \
    TestWeaveTunnel.h                                        \
    TestWeaveTunnelServer.h                                  \
    ToolCommon.h                                             \
    ToolCommonOptions.h                                      \
    WdmNextPerfUtility.h                                     \
    crypto-tests/WeaveCryptoTests.h                          \
    nlweavebdxclient-development.h                           \
    nlweavebdxclient.h                                       \
    nlweavebdxserver-development.h                           \
    nlweaveswuclient.h                                       \
    schema/nest/test/trait/TestATrait.h                      \
    schema/nest/test/trait/TestBTrait.h                      \
    schema/nest/test/trait/TestCTrait.h                      \
    schema/nest/test/trait/TestCommon.h                      \
    schema/nest/test/trait/TestCommonVersioned.h             \
    schema/nest/test/trait/TestCommonVersioned.cpp           \
    schema/nest/test/trait/TestDTrait.h                      \
    schema/nest/test/trait/TestETrait.h                      \
    schema/nest/test/trait/TestFTrait.h                      \
    schema/nest/test/trait/TestHTrait.h                      \
    schema/nest/test/trait/TestMismatchedCTrait.h            \
    schema/weave/common/DayOfWeekEnum.h                      \
    schema/weave/trait/locale/LocaleCapabilitiesTrait.h      \
    schema/weave/trait/locale/LocaleSettingsTrait.h          \
    schema/weave/trait/security/BoltLockSettingsTrait.h      \
    schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.h \
    weave-bdx-common-development.h                           \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

noinst_HEADERS                                += \
    MockBleApplicationDelegate.h                 \
    MockBlePlatformDelegate.h                    \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

if WEAVE_BUILD_TESTS
# A non-installed common library that may be used for all test
# applications in this directory.
#
# This will NOT be part of the externally-consumable binary SDK.

noinst_LIBRARIES                               = \
    libWeaveCryptoTests.a                        \
    libWeaveTestCommon.a                         \
    libWeaveTestGroupKeyStore.a                  \
    $(NULL)

# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                    = \
    -I$(top_srcdir)/src/include                  \
    $(LWIP_CPPFLAGS)                             \
    $(SOCKETS_CPPFLAGS)                          \
    $(PTHREAD_CFLAGS)                            \
    $(NULL)

libWeaveCryptoTests_a_SOURCES                  = \
    crypto-tests/WeaveCryptoAESTests.cpp         \
    crypto-tests/WeaveCryptoHKDFTests.cpp        \
    crypto-tests/WeaveCryptoHMACTests.cpp        \
    crypto-tests/WeaveCryptoSHATests.cpp         \
    $(NULL)

libWeaveTestCommon_a_SOURCES                   = \
    CASEOptions.cpp                              \
    KeyExportOptions.cpp                         \
    TAKEOptions.cpp                              \
    DeviceDescOptions.cpp                        \
    Certs.cpp                                    \
    TestGroupKeyStore.cpp                        \
    ToolCommon.cpp                               \
    ToolCommonOptions.cpp                        \
    PASEEngineTest.cpp                           \
    MockPlatformClocks.cpp                       \
    TestPersistedStorageImplementation.cpp       \
    $(NULL)

libWeaveTestGroupKeyStore_a_SOURCES            = \
    TestGroupKeyStore.cpp                        \
    $(NULL)

WEAVE_LDADD                                    = \
    $(top_builddir)/src/lib/libWeave.a           \
    $(NULL)

MOCKBLE_LDADD                                  = \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

if CONFIG_BLE_PLATFORM_BLUEZ

AM_CPPFLAGS                                   += \
    -I$(top_srcdir)/src/platform/ble/bluez       \
    $(BLUEZ_CPPFLAGS)                            \
    $(NULL)

libMockBlePlatformDelegate_a_DEPENDENCIES      = \
    $(top_builddir)/src/platform/ble/bluez/libWoBluez.a \
    $(NULL)

MOCKBLE_LDADD                                 += \
    libMockBlePlatformDelegate.a                 \
    $(top_builddir)/src/platform/ble/bluez/libWoBluez.a \
    $(BLUEZ_LDFLAGS)                                    \
    @BLUEZ_LIBS@ @GLIB_LIBS@ @DBUS_LIBS@ -ldl -lrt      \
    $(NULL)

else

libMockBleApplicationDelegate_a_SOURCES	       = \
    MockBleApplicationDelegate.cpp               \
    $(NULL)

MOCKBLE_LDADD                                 += \
    libMockBlePlatformDelegate.a                 \
    libMockBleApplicationDelegate.a              \
    $(NULL)

endif #CONFIG_BLE_PLATFORM_BLUEZ

noinst_LIBRARIES                              += \
    libMockBlePlatformDelegate.a                 \
    libMockBleApplicationDelegate.a              \
    $(NULL)

libMockBlePlatformDelegate_a_SOURCES           = \
    MockBlePlatformDelegate.cpp                  \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

COMMON_LDADD                                   = \
    $(COMMON_LDFLAGS)                            \
    $(WEAVE_LDADD)                               \
    $(MOCKBLE_LDADD)                             \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)                 \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)           \
    $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)            \
    $(NULL)

# Test applications that should be installed when the 'install' target is run.
#
# These will be part of the externally-consumable binary SDK.
#
# These need to go in libexec since they need to be "trampolined"
# through the 'weave-run' script due to runtime dependencies on
# DLLs/DSOs that the runtime linker / loader will need to resolve when
# they invoked.

libexec_PROGRAMS                               = \
    $(NULL)

if WEAVE_BUILD_DEVICE_MANAGER
libexec_PROGRAMS                              += \
    mock-device                                  \
    $(NULL)
endif

libexec_PROGRAMS                              += \
    weave-device-descriptor                      \
    weave-key-export                             \
    weave-ping                                   \
    weave-heartbeat                              \
    $(NULL)

# Test applications that should be run when the 'check' target is run.
#
# These will NOT be part of the externally-consumable binary SDK.

check_PROGRAMS                                 = \
    TestASN1                                     \
    TestAppKeys                                  \
    TestArgParser                                \
    TestCASE                                     \
    TestCodeUtils                                \
    TestCrypto                                   \
    TestDRBG                                     \
    TestDeviceDescriptor                         \
    TestECDH                                     \
    TestECDSA                                    \
    TestECMath                                   \
    TestEventLogging                             \
    TestFabricStateDelegate                      \
    TestInetAddress                              \
    TestInetBuffer                               \
    TestInetEndPoint                             \
    TestInetTimer                                \
    TestKeyExport                                \
    TestKeyIds                                   \
    TestMsgEnc                                   \
    TestNetworkInfo                              \
    TestPASE                                     \
    TestPacketBuffer                             \
    TestPasscodeEnc                              \
    TestPersistedCounter                         \
    TestPersistedStorage                         \
    TestProfileStringSupport                     \
    TestProvHash                                 \
    TestRetainedPacketBuffer                     \
    TestSerialNumUtils                           \
    TestSoftwareUpdate                           \
    TestSystemObject                             \
    TestSystemTimer                              \
    TestTAKE                                     \
    TestTLV                                      \
    TestTimeUtils                                \
    TestTimeZone                                 \
    TestWeaveCert                                \
    TestWeaveEncoding                            \
    TestWeaveFabricState                         \
    TestWeaveSignature                           \
    infratest                                    \
    TestErrorStr                                 \
    TestStatusReportStr                          \
    TestThermostatStatus                         \
    TestPairingCodeUtils                         \
    TestResourceIdentifier                       \
    $(NULL)

if HAVE_CXX11
check_PROGRAMS +=                                \
    TestTDM                                      \
    $(NULL)
endif

check_PROGRAMS +=                                \
    TestTDM                                      \
    TestPathStore                                \
    TestWdmUpdateEncoder                         \
    TestWdmUpdateResponse                        \
    $(NULL)

if WEAVE_BUILD_WARM
check_PROGRAMS                                += \
    TestWarm                                     \
    $(NULL)
endif

if WEAVE_WITH_OPENSSL
check_PROGRAMS                                += \
    TestWeaveProvBundle                          \
    $(NULL)
endif

if WEAVE_SYSTEM_CONFIG_USE_SOCKETS
check_PROGRAMS                                += \
    TestInetLayerDNS                            \
    TestWoble                                    \
    $(NULL)
endif

# Test scripts that should be run when the 'check' target is run.
#
# These will NOT be part of the externally-consumable binary SDK.

if WEAVE_RUN_HAPPY_SERVICE
check_SCRIPTS                                  = \
    $(NULL)
else
check_SCRIPTS                                  = \
    test-weave-device-descriptor-encode.sh       \
    test-Verhoeff.sh                             \
    $(NULL)
endif # WEAVE_RUN_HAPPY_SERVICE

if WEAVE_BUILD_TOOLS
check_SCRIPTS                                 += \
    TestWeaveTool.py                             \
    $(NULL)
endif # WEAVE_BUILD_TOOLS

if WEAVE_WITH_JAVA
check_SCRIPTS                                 += \
    run-security-support-test.sh                 \
    $(NULL)
endif # WEAVE_WITH_JAVA

# Test applications that should be built but not installed that
# require no network or complicated setup and should always be
# built to ensure overall "build sanity".
#
# These will NOT be part of the externally-consumable binary SDK.

local_test_programs                            = \
    GenerateEventLog                             \
    TestASN1                                     \
    TestAppKeys                                  \
    TestArgParser                                \
    TestCASE                                     \
    TestCodeUtils                                \
    TestCrypto                                   \
    TestDRBG                                     \
    TestDeviceDescriptor                         \
    TestECDH                                     \
    TestECDSA                                    \
    TestECMath                                   \
    TestFabricStateDelegate                      \
    TestInetAddress                              \
    TestInetBuffer                               \
    TestInetEndPoint                             \
    TestInetTimer                                \
    TestKeyExport                                \
    TestKeyIds                                   \
    TestMsgEnc                                   \
    TestNetworkInfo                              \
    TestPASE                                     \
    TestPacketBuffer                             \
    TestPasscodeEnc                              \
    TestProfileStringSupport                     \
    TestProvHash                                 \
    TestRetainedPacketBuffer                     \
    TestSerialNumUtils                           \
    TestSoftwareUpdate                           \
    TestSystemObject                             \
    TestSystemTimer                              \
    TestTAKE                                     \
    TestTLV                                      \
    TestTimeUtils                                \
    TestTimeZone                                 \
    TestWeaveCert                                \
    TestWeaveEncoding                            \
    TestWeaveFabricState                         \
    TestWeaveProvBundle                          \
    TestWeaveSignature                           \
    infratest                                    \
    TestErrorStr                                 \
    TestStatusReportStr                          \
    TestThermostatStatus                         \
    TestPairingCodeUtils                         \
    TestResourceIdentifier                       \
    $(NULL)

if HAVE_CXX11
local_test_programs                           += \
    TestTDM                                      \
    TestWDM                                      \
    $(NULL)
endif

if WEAVE_BUILD_WARM
local_test_programs                           += \
    TestWarm                                     \
    TestPathStore                                \
    TestWdmUpdateEncoder                         \
    TestWdmUpdateResponse                        \
    $(NULL)
endif

# Test applications that should be built but not installed that
# require network or complicated setup and should always be built to
# ensure overall "build sanity".
#
# These will NOT be part of the externally-consumable binary SDK.

network_test_programs                          = \
    TestBinding                                  \
    TestEventLogging                             \
    TestInetLayer                                \
    TestInetLayerMulticast                       \
    TestPersistedCounter                         \
    TestPersistedStorage                         \
    TestRADaemon                                 \
    TestWRMP                                     \
    TestWeaveMessageLayer                        \
    TestWeaveTunnelBR                            \
    TestWeaveTunnelServer                        \
    TestWdmNext                                  \
    TestWdmOneWayCommandSender                   \
    TestWdmOneWayCommandReceiver                 \
    TestInetLayerDNS                            \
    TestWoble                                    \
    mock-device                                  \
    mock-weave-bg                                \
    weave-bdx-client-development                 \
    weave-bdx-client-v0                          \
    weave-bdx-server-development                 \
    weave-bdx-server-v0                          \
    weave-connection-tunnel                      \
    weave-dd-client                              \
    weave-service-dir                            \
    weave-swu-client                             \
    weave-swu-server                             \
    $(NULL)

if WEAVE_BUILD_LEGACY_WDM
network_test_programs                         += \
    TestDataManagement                           \
    wdmtest                                      \
    $(NULL)
endif


# Test applications that should be built but not installed and should
# always be built to ensure overall "build sanity".
#
# These will NOT be part of the externally-consumable binary SDK.

noinst_PROGRAMS                                = \
    $(local_test_programs)                       \
    $(network_test_programs)                     \
    $(NULL)

# Test applications and scripts that should be built and run when the
# 'check' target is run.

if WEAVE_RUN_HAPPY_SERVICE
TESTS                                          = \
    $(check_SCRIPTS)                             \
    $(NULL)
else
TESTS                                          = \
    $(check_PROGRAMS)                            \
    $(check_SCRIPTS)                             \
    $(NULL)
endif # WEAVE_RUN_HAPPY_SERVICE

# The additional environment variables and their values that will be
# made available to all programs and scripts in TESTS.

TESTS_ENVIRONMENT                                                 = \
    export                                                          \
    CMP='$(CMP)'                                                    \
    RM='$(RM)'                                                      \
    abs_builddir='$(abs_builddir)'                                  \
    abs_srcdir='$(abs_srcdir)'                                      \
    abs_top_builddir='$(abs_top_builddir)'                          \
    abs_top_srcdir='$(abs_top_srcdir)'                              \
    builddir='$(builddir)'                                          \
    srcdir='$(srcdir)'                                              \
    top_builddir='$(top_builddir)'                                  \
    top_srcdir='$(top_srcdir)'                                      \
    WEAVE_SYSTEM_CONFIG_USE_LWIP='$(WEAVE_SYSTEM_CONFIG_USE_LWIP)'  \
    $(NULL)

if WEAVE_RUN_HAPPY_SERVICE

TESTS_ENVIRONMENT                             += \
    weave_service_address='$(HAPPY_SERVICE)'

endif # WEAVE_RUN_HAPPY_SERVICE

if WEAVE_RUN_HAPPY_DNS

TESTS_ENVIRONMENT                             += \
    happy_dns='$(HAPPY_DNS)'

endif # WEAVE_RUN_HAPPY_DNS

TESTS_ENVIRONMENT                             += \
    PYTHONPATH='$(HAPPY_PATH):$(PYTHONPATH)';    \
    $(NULL)

if WEAVE_RUN_HAPPY
if WEAVE_RUN_HAPPY_SERVICE

if WEAVE_RUN_HAPPY_TUNNEL
check_SCRIPTS                                 +=             \
    happy/tests/service/tunnel/test_weave_tunnel_01.py       \
    happy/tests/service/tunnel/test_weave_tunnel_02.py       \
    $(NULL)
endif # WEAVE_RUN_HAPPY_TUNNEL

if WEAVE_RUN_HAPPY_TIME
check_SCRIPTS                                 +=             \
    happy/tests/service/time/test_weave_time_01.py           \
    $(NULL)
endif #WEAVE_RUN_HAPPY_TIME

if WEAVE_RUN_HAPPY_ECHO
check_SCRIPTS                                 +=             \
    happy/tests/service/echo/test_weave_echo_01.py           \
    happy/tests/service/echo/test_weave_echo_02.py           \
    happy/tests/service/echo/test_weave_echo_03.py           \
    $(NULL)
endif # WEAVE_RUN_HAPPY_ECHO

if WEAVE_RUN_HAPPY_KEY_EXPORT
check_SCRIPTS                                 +=             \
    happy/tests/service/key_export/test_weave_key_export_01.py \
    $(NULL)
endif # WEAVE_RUN_HAPPY_KEY_EXPORT

if WEAVE_RUN_HAPPY_PAIRING
check_SCRIPTS                                 +=             \
    happy/tests/service/pairing/test_weave_pairing_01.py     \
    $(NULL)
endif # WEAVE_RUN_HAPPY_PAIRING

if WEAVE_RUN_HAPPY_PERF
check_SCRIPTS                                 +=             \
    happy/tests/service/weave_service_perf_run.sh            \
    $(NULL)
endif # WEAVE_RUN_HAPPY_PERF

if WEAVE_RUN_HAPPY_WDM
check_SCRIPTS                                 +=             \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_01.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_02.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_03.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_04.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_05.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_06.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_07.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_08.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_09.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_10.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_11.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_12.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_13.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_14.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_15.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_16.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_17.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_18.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_19.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_20.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_21.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_22.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_23.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_24.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_25.py                              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_mutual_subscribe_application_key_01.py              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_01_cond_OneLeaf.py                           \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_02_cond_BackToBack.py                        \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_03_uncond_OneLeaf.py                         \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_04_uncond_BackToBack.py                      \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_05_cond_WholeDictionary.py                   \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_06_cond_WholeLargeDictionary.py              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_07_cond_OneLeaf_multi_traits.py              \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_08_mixed_OneLeaf_multi_traits.py             \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_09_mixed_SameLevelLeafs_multi_traits.py      \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_10_cond_Root_multi_traits.py                 \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_11_uncond_WholeLargeDictionary.py            \
    happy/tests/service/wdmNext/test_weave_wdm_next_service_update_12_uncond_OneLeaf_BeforeSub.py               \
    $(NULL)
endif # WEAVE_RUN_HAPPY_WDM

else  # WEAVE_RUN_HAPPY_SERVICE

if WEAVE_RUN_LONG_TESTS

if WEAVE_RUN_HAPPY_BDX

# Parse the bdx filename and generate the script:
# e.g: test_weave_bdx_faults_download_v0_v0_1_of_2
happy/tests/standalone/bdx/test_weave_bdx_faults_%.sh :
	mkdir -p happy/tests/standalone/bdx
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/bdx/test_weave_bdx_faults_01.py --direction $${tokens[0]} --versions $${tokens[1]/v/}_$${tokens[2]/v/} --group $${tokens[3]} --numgroups $${tokens[5]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v0_1_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v0_2_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v0_3_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v0_4_of_4.sh     \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v1_1_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v1_2_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v1_3_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v0_v1_4_of_4.sh     \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v0_1_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v0_2_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v0_3_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v0_4_of_4.sh     \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v1_1_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v1_2_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v1_3_of_4.sh     \
    happy/tests/standalone/bdx/test_weave_bdx_faults_download_v1_v1_4_of_4.sh     \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v0_1_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v0_2_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v0_3_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v0_4_of_4.sh       \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v1_1_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v1_2_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v1_3_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v0_v1_4_of_4.sh       \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v0_1_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v0_2_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v0_3_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v0_4_of_4.sh       \
	$(NULL)

check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v1_1_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v1_2_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v1_3_of_4.sh       \
    happy/tests/standalone/bdx/test_weave_bdx_faults_upload_v1_v1_4_of_4.sh       \
	$(NULL)

endif # WEAVE_RUN_HAPPY_BDX

if WEAVE_RUN_HAPPY_SECMGR

# Parse the echo filename and generate the script:
# e.g: test_weave_echo_secmgr_faults_case_wrmp_1_of_2.sh
happy/tests/standalone/echo/test_weave_echo_secmgr_faults_%.sh :
	mkdir -p happy/tests/standalone/echo
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/echo/test_weave_echo_secmgr_faults.py --$${tokens[0]} --$${tokens[1]} --group $${tokens[2]} --numgroups $${tokens[4]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_1_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_2_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_3_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_4_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_5_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_6_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_7_of_8.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_wrmp_8_of_8.sh \
	$(NULL)

check_SCRIPTS                                 +=                \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_group-key_wrmp_1_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_group-key_wrmp_2_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_group-key_wrmp_3_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_group-key_wrmp_4_of_4.sh \
	$(NULL)

check_SCRIPTS                                 +=                \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_tcp_1_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_tcp_2_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_tcp_3_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_case_tcp_4_of_4.sh \
	$(NULL)

check_SCRIPTS                                 +=                \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_pase_tcp_1_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_pase_tcp_2_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_pase_tcp_3_of_4.sh \
	happy/tests/standalone/echo/test_weave_echo_secmgr_faults_pase_tcp_4_of_4.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_SECMGR

if WEAVE_RUN_HAPPY_WDM

# Parse the wdm filename and generate the script:
# e.g: test_weave_echo_secmgr_faults_case_wrmp_1_of_2.sh
happy/tests/standalone/wdmNext/test_weave_wdm_next_%.sh :
	mkdir -p happy/tests/standalone/wdmNext
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/wdmNext/test_weave_wdm_next_$${tokens[0]}_subscribe_faults.py --group $${tokens[3]} --numgroups $${tokens[5]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_1_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_2_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_3_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_4_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_5_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_6_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_7_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_8_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_9_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_10_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_11_of_12.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_12_of_12.sh \
	$(NULL)

check_SCRIPTS                                 +=                \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_1_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_2_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_3_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_4_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_5_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_6_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_7_of_8.sh \
	happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_subscribe_faults_8_of_8.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_WDM

if WEAVE_RUN_HAPPY_KEY_EXPORT

# Parse the key export filename and generate the script:
# e.g: test_weave_key_export_faults_wrmp_1_of_4.sh
happy/tests/standalone/key_export/test_weave_key_export_faults_%.sh :
	mkdir -p happy/tests/standalone/key_export
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/key_export/test_weave_key_export_faults.py --$${tokens[0]} --group $${tokens[1]} --numgroups $${tokens[3]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/key_export/test_weave_key_export_faults_tcp_1_of_2.sh \
	happy/tests/standalone/key_export/test_weave_key_export_faults_tcp_2_of_2.sh \
	happy/tests/standalone/key_export/test_weave_key_export_faults_wrmp_1_of_4.sh \
	happy/tests/standalone/key_export/test_weave_key_export_faults_wrmp_2_of_4.sh \
	happy/tests/standalone/key_export/test_weave_key_export_faults_wrmp_3_of_4.sh \
	happy/tests/standalone/key_export/test_weave_key_export_faults_wrmp_4_of_4.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_KEY_EXPORT

if WEAVE_RUN_HAPPY_SERVICEDIR

# Parse the servicedir filename and generate the script:
# e.g: t_weave_service_directory_faults_1_of_4.sh
happy/tests/standalone/servicedir/test_weave_service_directory_faults_%.sh :
	mkdir -p happy/tests/standalone/servicedir
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/servicedir/test_service_directory_01.py --fault-injection --group $${tokens[0]} --numgroups $${tokens[2]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/servicedir/test_weave_service_directory_faults_1_of_4.sh \
	happy/tests/standalone/servicedir/test_weave_service_directory_faults_2_of_4.sh \
	happy/tests/standalone/servicedir/test_weave_service_directory_faults_3_of_4.sh \
	happy/tests/standalone/servicedir/test_weave_service_directory_faults_4_of_4.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_SERVICEDIR

if WEAVE_RUN_HAPPY_TUNNEL

# Parse the tunnel filename and generate the script:
# e.g: test_weave_tunnel_faults_1_of_4.sh
happy/tests/standalone/tunnel/test_weave_tunnel_faults_%.sh :
	mkdir -p happy/tests/standalone/tunnel
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/tunnel/test_weave_tunnel_faults.py --group $${tokens[0]} --numgroups $${tokens[2]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_1_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_2_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_3_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_4_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_5_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_6_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_7_of_8.sh \
	happy/tests/standalone/tunnel/test_weave_tunnel_faults_8_of_8.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_TUNNEL

if WEAVE_RUN_HAPPY_TIME

# Parse the time filename and generate the script:
# e.g: t_weave_time_faults_1_of_8.sh
happy/tests/standalone/time/test_weave_time_faults_%.sh :
	mkdir -p happy/tests/standalone/time
	matched_part_of_the_file_name=$(*F) ; \
	IFS='_' tokens=( $${matched_part_of_the_file_name} ) ; \
	echo "#!/bin/bash" > $@ ; \
	echo "${abs_srcdir}/happy/tests/standalone/time/test_weave_time_01.py --enable-faults --syncmode $${tokens[0]} --group $${tokens[1]} --numgroups $${tokens[3]}" >> $@ ; \
	echo 'exit $$?' >>$@
	chmod u+x $@

check_SCRIPTS                                 +=                \
	happy/tests/standalone/time/test_weave_time_faults_local_1_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_local_2_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_local_3_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_local_4_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_service_1_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_service_2_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_service_3_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_service_4_of_4.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_1_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_2_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_3_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_4_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_5_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_6_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_7_of_8.sh \
	happy/tests/standalone/time/test_weave_time_faults_auto_8_of_8.sh \
	$(NULL)

endif # WEAVE_RUN_HAPPY_TIME

endif # WEAVE_RUN_LONG_TESTS

if WEAVE_RUN_HAPPY_WDM

check_SCRIPTS                                 +=                \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_01.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_02.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_03.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_04.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_05.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_06.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_07.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_08.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_09.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_10.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_11.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_12.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_13.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_14.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_15.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_one_way_subscribe_16.py    \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_01.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_02.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_03.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_04.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_05.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_06.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_07.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_08.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_09.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_10.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_11.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_12.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_13.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_14.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_15.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_16.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_17.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_18.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_19.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_20.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_21.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_22.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_23.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_24.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_25.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_26.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_27.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_28.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_29.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_30.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_31.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_32.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_33.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_34.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_35.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_36.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_37.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_38.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_39.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_40.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_41.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_42.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_43.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_44.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_45.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_46.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_47.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_48.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_49.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_50.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_51.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_52.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_53.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_54.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_55.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_56.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_57.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_58.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_59.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_60.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_61.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_62.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_63.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_64.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_65.py     \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_subscribe_faults_happy.sh \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_application_key_01.py      \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_application_key_02.py      \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_oneway_resub.py            \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_mutual_resub.py            \
    happy/tests/standalone/wdmNext/test_weave_wdm_next_subless_notify_01.py       \
    $(NULL)

endif # WEAVE_RUN_HAPPY_WDM

if WEAVE_RUN_HAPPY_BDX
check_SCRIPTS                                 +=                \
    happy/tests/standalone/bdx/test_weave_bdx_01.py                        \
    happy/tests/standalone/bdx/test_weave_bdx_02.py                        \
    happy/tests/standalone/bdx/test_weave_bdx_03.py                        \
    happy/tests/standalone/bdx/test_weave_bdx_04.py                        \
    happy/tests/standalone/bdx/test_weave_bdx_05.py                        \
    $(NULL)
endif # WEAVE_RUN_HAPPY_BDX

if WEAVE_RUN_HAPPY_ECHO
check_SCRIPTS                                 +=                \
    happy/tests/standalone/echo/test_weave_echo_01.py                      \
    happy/tests/standalone/echo/test_weave_echo_03.py                      \
    $(NULL)
endif # WEAVE_RUN_HAPPY_ECHO

if WEAVE_RUN_HAPPY_SECMGR
check_SCRIPTS                                 +=                \
    happy/tests/standalone/echo/test_weave_echo_02.py                      \
    $(NULL)
endif # WEAVE_RUN_HAPPY_SECMGR

if WEAVE_RUN_HAPPY_TUNNEL
check_SCRIPTS                                 +=                \
    happy/tests/standalone/tunnel/test_weave_tunnel_01.py                  \
    happy/tests/standalone/tunnel/test_weave_tunnel_02.py                  \
    happy/tests/standalone/tunnel/test_weave_tunnel_03.py                  \
    happy/tests/standalone/tunnel/test_weave_tunnel_04.py                  \
    happy/tests/standalone/con_tunnel/test_weave_con_tunnel_01.py          \
    $(NULL)
endif # WEAVE_RUN_HAPPY_TUNNEL

if WEAVE_RUN_HAPPY_KEY_EXPORT
check_SCRIPTS                                 +=                \
    happy/tests/standalone/key_export/test_weave_key_export_01.py          \
    $(NULL)
endif # WEAVE_RUN_HAPPY_KEY_EXPORT

if WEAVE_RUN_HAPPY_PAIRING
check_SCRIPTS                                 +=                \
    happy/tests/standalone/pairing/test_weave_pairing_01.py                \
    $(NULL)
endif # WEAVE_RUN_HAPPY_PAIRING

if WEAVE_RUN_HAPPY_PAIRING
if CONFIG_NETWORK_LAYER_BLE
if CONFIG_BLE_PLATFORM_BLUEZ
check_SCRIPTS                                 +=                \
    happy/tests/standalone/pairing/test_weave_ble_01.py                \
    $(NULL)
endif # CONFIG_BLE_PLATFORM_BLUEZ
endif # CONFIG_NETWORK_LAYER_BLE
endif # WEAVE_RUN_HAPPY_PAIRING

if WEAVE_RUN_HAPPY_SERVICEDIR
check_SCRIPTS                                 +=                \
    happy/tests/standalone/servicedir/test_service_directory_01.py         \
    $(NULL)
endif

if WEAVE_RUN_HAPPY_TIME
check_SCRIPTS                                 +=                \
    happy/tests/standalone/time/test_weave_time_local.sh                   \
    happy/tests/standalone/time/test_weave_time_service.sh                 \
    happy/tests/standalone/time/test_weave_time_auto.sh                    \
    $(NULL)
endif # WEAVE_RUN_HAPPY_TIME

if WEAVE_RUN_HAPPY_CORE
check_SCRIPTS                                                           += \
    happy/tests/standalone/wdmv0/test_weave_wdmv0_01.py                    \
    happy/tests/standalone/swu/test_weave_swu_01.py                        \
    happy/tests/standalone/swu/test_weave_swu_02.py                        \
    happy/tests/standalone/messagelayer/test_weave_messagelayer_01.py      \
    happy/tests/standalone/dd/test_weave_dd_01.py                          \
    happy/tests/standalone/heartbeat/test_weave_heartbeat_01.py            \
    happy/tests/standalone/wrmp/test_weave_wrmp_01.py                      \
    $(NULL)
endif # WEAVE_RUN_HAPPY_CORE

if WEAVE_RUN_HAPPY_INET
check_SCRIPTS                                                           += \
    happy/tests/standalone/inet/test_inet_01.py                            \
    happy/tests/standalone/inet/test_inet_dns_01.py                        \
    happy/tests/standalone/inet/test_inet_multicast_01.py                  \
    $(NULL)
endif # WEAVE_RUN_HAPPY_INET

endif # WEAVE_RUN_HAPPY_SERVICE
endif # WEAVE_RUN_HAPPY

# The bin_LINKS variable enumerates programs that need to be
# "trampolined" through the 'weave-run' script due to runtime
# dependencies on DLLs/DSOs that the runtime linker / loader will need
# to resolve when they invoked.

bin_LINKS                                      = \
    $(libexec_PROGRAMS)                          \
    $(libexec_SCRIPTS)                           \
    $(NULL)

# Source, compiler, and linker options for test programs.

GenerateEventLog_SOURCES                 = GenerateEventLog.cpp MockEvents.cpp \
                                           schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.cpp \
                                           schema/nest/test/trait/TestETrait.cpp \
                                           schema/nest/test/trait/TestCommon.cpp

GenerateEventLog_LDFLAGS                 = $(AM_CPPFLAGS)
GenerateEventLog_CPPFLAGS                = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
GenerateEventLog_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

TestASN1_SOURCES                         = TestASN1.cpp
TestASN1_LDADD                           = $(COMMON_LDADD)

TestAppKeys_SOURCES                      = TestAppKeys.cpp
TestAppKeys_LDADD                        = libWeaveTestCommon.a $(COMMON_LDADD)

TestArgParser_SOURCES                    = TestArgParser.cpp
TestArgParser_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

TestBinding_SOURCES                      = TestBinding.cpp
TestBinding_LDFLAGS                      = $(AM_CPPFLAGS)
TestBinding_LDADD                        = libWeaveTestCommon.a $(COMMON_LDADD)

TestCASE_SOURCES                         = TestCASE.cpp
TestCASE_LDFLAGS                         = $(AM_CPPFLAGS)
TestCASE_LDADD                           = libWeaveTestCommon.a $(COMMON_LDADD)

TestCodeUtils_SOURCES                    = TestCodeUtils.cpp
TestCodeUtils_LDADD                      =

TestCrypto_SOURCES                       = TestCrypto.cpp
TestCrypto_CPPFLAGS                      = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/crypto-tests
TestCrypto_LDADD                         = libWeaveCryptoTests.a $(COMMON_LDADD)

TestDRBG_SOURCES                         = TestDRBG.cpp
TestDRBG_LDADD                           = $(COMMON_LDADD)

TestDataManagement_SOURCES               = TestDataManagement.cpp \
                                           TestProfile.cpp
TestDataManagement_LDFLAGS               = $(AM_CPPFLAGS)
TestDataManagement_LDADD                 = libWeaveTestCommon.a $(COMMON_LDADD)

TestDeviceDescriptor_SOURCES             = TestDeviceDescriptor.cpp
TestDeviceDescriptor_LDADD               = $(COMMON_LDADD)

TestECDH_SOURCES                         = TestECDH.cpp
TestECDH_LDADD                           = $(COMMON_LDADD)

TestECDSA_SOURCES                        = TestECDSA.cpp
TestECDSA_LDADD                          = $(COMMON_LDADD)

TestECMath_SOURCES                       = TestECMath.cpp TestECMathParams.cpp
TestECMath_LDADD                         = $(COMMON_LDADD)

TestEventLogging_SOURCES                 = schema/nest/test/trait/TestETrait.cpp \
                                           schema/nest/test/trait/TestCommon.cpp \
                                           MockExternalEvents.cpp \
                                           TestEventLogging.cpp
TestEventLogging_CPPFLAGS	         = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestEventLogging_LDFLAGS                 = $(AM_CPPFLAGS)
TestEventLogging_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

if HAVE_CXX11
TestTDM_SOURCES                          = TestTDM.cpp \
                                           schema/nest/test/trait/TestHTrait.cpp \
                                           schema/nest/test/trait/TestCTrait.cpp \
                                           schema/nest/test/trait/TestBTrait.cpp \
                                           schema/nest/test/trait/TestMismatchedCTrait.cpp \
                                           schema/nest/test/trait/TestCommon.cpp \
                                           MockMismatchedSchemaSinkAndSource.cpp \
                                           TestPersistedStorageImplementation.cpp \
                                           MockTestBTrait.cpp

TestTDM_CPPFLAGS                         = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestTDM_LDFLAGS                          = $(AM_CPPFLAGS)
TestTDM_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)
endif

TestWDM_SOURCES                          = TestWdm.cpp
TestWDM_CPPFLAGS                         = $(AM_CPPFLAGS)
TestWDM_LDFLAGS                          = $(AM_CPPFLAGS)
TestWDM_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)

TestPathStore_SOURCES                          = TestPathStore.cpp \
                                                 schema/nest/test/trait/TestHTrait.cpp \
                                                 TestPersistedStorageImplementation.cpp \
                                                 schema/nest/test/trait/TestCommon.cpp

TestPathStore_CPPFLAGS                         = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestPathStore_LDFLAGS                          = $(AM_CPPFLAGS)
TestPathStore_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)


TestWdmUpdateEncoder_SOURCES                   = TestWdmUpdateEncoder.cpp \
												 MockSinkTraits.cpp						\
												 schema/nest/test/trait/TestATrait.cpp			\
												 schema/nest/test/trait/TestBTrait.cpp			\
												 schema/nest/test/trait/TestETrait.cpp			\
												 schema/nest/test/trait/TestCommon.cpp \
											     schema/weave/trait/locale/LocaleSettingsTrait.cpp		\
											     schema/weave/trait/locale/LocaleCapabilitiesTrait.cpp	\
											     schema/weave/trait/security/BoltLockSettingsTrait.cpp	\
											     schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.cpp	\
												 MockWdmNodeOptions.cpp                   \
                                                 TestPersistedStorageImplementation.cpp

TestWdmUpdateEncoder_CPPFLAGS                         = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestWdmUpdateEncoder_LDFLAGS                          = $(AM_CPPFLAGS)
TestWdmUpdateEncoder_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)

TestWdmUpdateResponse_SOURCES                  = TestWdmUpdateResponse.cpp \
                                                 TestPersistedStorageImplementation.cpp

TestWdmUpdateResponse_CPPFLAGS                 = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestWdmUpdateResponse_LDFLAGS                  = $(AM_CPPFLAGS)
TestWdmUpdateResponse_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

TestFabricStateDelegate_SOURCES          = TestFabricStateDelegate.cpp TestPersistedStorageImplementation.cpp
TestFabricStateDelegate_LDFLAGS          = $(AM_CPPFLAGS)
TestFabricStateDelegate_LDADD            = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetEndPoint_SOURCES                 = TestInetEndPoint.cpp
TestInetEndPoint_LDFLAGS                 = $(AM_CPPFLAGS)
TestInetEndPoint_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetBuffer_SOURCES                   = TestInetBuffer.cpp
TestInetBuffer_LDADD                     = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetTimer_SOURCES                    = TestInetTimer.cpp
TestInetTimer_LDFLAGS                    = $(AM_CPPFLAGS)
TestInetTimer_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetLayer_SOURCES                    = TestInetLayer.cpp \
                                           TestInetLayerCommon.cpp \
                                           $(NULL)
TestInetLayer_LDFLAGS                    = $(AM_CPPFLAGS)
TestInetLayer_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetLayerMulticast_SOURCES           = TestInetLayerMulticast.cpp \
                                           TestInetLayerCommon.cpp \
                                           $(NULL)
TestInetLayerMulticast_LDFLAGS           = $(AM_CPPFLAGS)
TestInetLayerMulticast_LDADD             = libWeaveTestCommon.a $(COMMON_LDADD)

TestInetAddress_SOURCES                  = TestInetAddress.cpp
TestInetAddress_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

TestKeyExport_SOURCES                    = TestKeyExport.cpp
TestKeyExport_LDFLAGS                    = $(AM_CPPFLAGS)
TestKeyExport_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

TestKeyIds_SOURCES                       = TestKeyIds.cpp
TestKeyIds_LDFLAGS                       = $(AM_CPPFLAGS)
TestKeyIds_LDADD                         = libWeaveTestCommon.a $(COMMON_LDADD)

TestMsgEnc_SOURCES                       = TestMsgEnc.cpp
TestMsgEnc_LDFLAGS                       = $(AM_CPPFLAGS)
TestMsgEnc_LDADD                         = libWeaveTestCommon.a $(COMMON_LDADD)

TestNetworkInfo_SOURCES                  = TestNetworkInfo.cpp
TestNetworkInfo_LDFLAGS                  = $(AM_CPPFLAGS)
TestNetworkInfo_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

TestPASE_SOURCES                         = TestPASE.cpp
TestPASE_LDFLAGS                         = $(AM_CPPFLAGS)
TestPASE_LDADD                           = libWeaveTestCommon.a $(COMMON_LDADD)

TestPacketBuffer_SOURCES                 = TestPacketBuffer.cpp
TestPacketBuffer_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

TestPasscodeEnc_SOURCES                  = TestPasscodeEnc.cpp
TestPasscodeEnc_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

TestPersistedCounter_SOURCES             = TestPersistedCounter.cpp TestPersistedStorageImplementation.cpp
TestPersistedCounter_LDADD               = libWeaveTestCommon.a $(COMMON_LDADD)

TestPersistedStorage_SOURCES             = TestPersistedStorage.cpp TestPersistedStorageImplementation.cpp
TestPersistedStorage_LDADD               = libWeaveTestCommon.a $(COMMON_LDADD)

TestProfileStringSupport_SOURCES         = TestProfileStringSupport.cpp
TestProfileStringSupport_LDADD           = $(COMMON_LDADD)

TestProvHash_SOURCES                     = TestProvHash.cpp
TestProvHash_LDADD                       = $(COMMON_LDADD)

TestRADaemon_SOURCES                     = TestRADaemon.cpp
TestRADaemon_CPPFLAGS                    = $(AM_CPPFLAGS) -I$(top_srcdir)/src/ra-daemon
TestRADaemon_LDADD                       = libWeaveTestCommon.a -L$(top_builddir)/src/ra-daemon -lRADaemon $(COMMON_LDADD)

TestRetainedPacketBuffer_SOURCES         = TestRetainedPacketBuffer.cpp
TestRetainedPacketBuffer_LDADD           = libWeaveTestCommon.a $(COMMON_LDADD)

TestSerialNumUtils_SOURCES               = TestSerialNumUtils.cpp
TestSerialNumUtils_LDADD                 = $(COMMON_LDADD)

TestSoftwareUpdate_SOURCES               = TestSoftwareUpdate.cpp
TestSoftwareUpdate_LDFLAGS               = $(AM_CPPFLAGS)
TestSoftwareUpdate_LDADD                 = $(COMMON_LDADD)

TestSystemObject_SOURCES                 = TestSystemObject.cpp
TestSystemObject_CPPFLAGS                = $(AM_CPPFLAGS) $(PTHREAD_CFLAGS)
TestSystemObject_LDFLAGS                 = $(PTHREAD_CFLAGS)
TestSystemObject_LDADD                   = libWeaveTestCommon.a $(PTHREAD_LIBS) $(COMMON_LDADD)

TestSystemTimer_SOURCES                  = TestSystemTimer.cpp
TestSystemTimer_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

TestTAKE_SOURCES                         = TestTAKE.cpp
TestTAKE_LDFLAGS                         = $(AM_CPPFLAGS)
TestTAKE_LDADD                           = libWeaveTestCommon.a $(COMMON_LDADD)

TestTLV_SOURCES                          = TestTLV.cpp
TestTLV_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)

TestTimeUtils_SOURCES                    = TestTimeUtils.cpp
TestTimeUtils_LDADD                      = $(COMMON_LDADD)

TestTimeZone_SOURCES                     = TestTimeZone.cpp
TestTimeZone_LDADD                       = $(COMMON_LDADD)

TestWRMP_SOURCES                         = TestWRMP.cpp
TestWRMP_LDFLAGS                         = $(AM_CPPFLAGS)
TestWRMP_LDADD                           = libWeaveTestCommon.a $(COMMON_LDADD)

if WEAVE_BUILD_WARM
TestWarm_SOURCES                         = TestWarm.cpp TestPersistedStorageImplementation.cpp
TestWarm_LDFLAGS                         = $(AM_CPPFLAGS)
TestWarm_LDADD                           = libWeaveTestGroupKeyStore.a $(COMMON_LDADD)
endif

TestWeaveCert_SOURCES                    = TestWeaveCert.cpp TestWeaveCertData.cpp
TestWeaveCert_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

TestWeaveEncoding_SOURCES                = TestWeaveEncoding.cpp
TestWeaveEncoding_LDADD                  =

TestWeaveFabricState_SOURCES             = TestWeaveFabricState.cpp TestPersistedStorageImplementation.cpp
TestWeaveFabricState_LDFLAGS             = $(AM_CPPFLAGS)
TestWeaveFabricState_LDADD               = libWeaveTestCommon.a $(COMMON_LDADD)

TestWeaveMessageLayer_SOURCES            = TestWeaveMessageLayer.cpp
TestWeaveMessageLayer_LDFLAGS            = $(AM_CPPFLAGS)
TestWeaveMessageLayer_LDADD              = libWeaveTestCommon.a $(COMMON_LDADD)

TestWeaveProvBundle_SOURCES              = TestWeaveProvBundle.cpp
TestWeaveProvBundle_LDFLAGS              = $(AM_CPPFLAGS)
TestWeaveProvBundle_LDADD                = $(COMMON_LDADD)

TestWeaveSignature_SOURCES               = TestWeaveSignature.cpp
TestWeaveSignature_LDFLAGS               = $(AM_CPPFLAGS)
TestWeaveSignature_LDADD                 = $(COMMON_LDADD)

TestWeaveTunnelBR_SOURCES                = TestWeaveTunnelBR.cpp
TestWeaveTunnelBR_LDFLAGS                = $(AM_CPPFLAGS)
TestWeaveTunnelBR_LDADD                  = libWeaveTestCommon.a $(COMMON_LDADD)

TestWeaveTunnelServer_SOURCES            = TestWeaveTunnelServer.cpp
TestWeaveTunnelServer_LDFLAGS            = $(AM_CPPFLAGS)
TestWeaveTunnelServer_LDADD              = libWeaveTestCommon.a $(COMMON_LDADD)

TestWdmNext_SOURCES                      = TestWdmNext.cpp						\
                                           MockSinkTraits.cpp						\
                                           MockSourceTraits.cpp						\
                                           MockLoggingManager.cpp					\
                                           MockEvents.cpp                                               \
                                           TestWdmSubscriptionlessNotificationSender.cpp		\
                                           TestWdmSubscriptionlessNotificationReceiver.cpp		\
                                           schema/nest/test/trait/TestATrait.cpp			\
                                           schema/nest/test/trait/TestBTrait.cpp			\
                                           schema/nest/test/trait/TestETrait.cpp			\
                                           schema/nest/test/trait/TestCommon.cpp \
                                           schema/weave/trait/locale/LocaleSettingsTrait.cpp		\
                                           schema/weave/trait/locale/LocaleCapabilitiesTrait.cpp	\
                                           schema/weave/trait/security/BoltLockSettingsTrait.cpp	\
                                           schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.cpp	\
										   MockWdmNodeOptions.cpp                   \
                                           MockWdmViewServer.cpp					\
                                           MockWdmViewClient.cpp					\
                                           MockWdmSubscriptionInitiator.cpp				\
                                           MockWdmTestVerifier.cpp	        			\
                                           MockWdmSubscriptionResponder.cpp				\
                                           WdmNextPerfUtility.cpp

TestWdmNext_CPPFLAGS                     = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestWdmNext_LDFLAGS                      = $(AM_CPPFLAGS)
TestWdmNext_LDADD                        = libWeaveTestCommon.a $(COMMON_LDADD)

TestWdmOneWayCommandSender_SOURCES       = TestWdmOneWayCommandSender.cpp

TestWdmOneWayCommandSender_CPPFLAGS      = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestWdmOneWayCommandSender_LDFLAGS       = $(AM_CPPFLAGS)
TestWdmOneWayCommandSender_LDADD         = libWeaveTestCommon.a $(COMMON_LDADD)

TestWdmOneWayCommandReceiver_SOURCES     = TestWdmOneWayCommandReceiver.cpp				\
                                           MockSourceTraits.cpp						\
                                           MockLoggingManager.cpp					\
                                           MockEvents.cpp						\
                                           schema/nest/test/trait/TestATrait.cpp			\
                                           schema/nest/test/trait/TestBTrait.cpp			\
                                           schema/nest/test/trait/TestETrait.cpp			\
                                           schema/nest/test/trait/TestCommon.cpp                        \
                                           schema/weave/trait/locale/LocaleSettingsTrait.cpp		\
                                           schema/weave/trait/locale/LocaleCapabilitiesTrait.cpp	\
                                           schema/weave/trait/security/BoltLockSettingsTrait.cpp	\
                                           schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.cpp

TestWdmOneWayCommandReceiver_CPPFLAGS    = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
TestWdmOneWayCommandReceiver_LDFLAGS     = $(AM_CPPFLAGS)
TestWdmOneWayCommandReceiver_LDADD       = libWeaveTestCommon.a $(COMMON_LDADD)


infratest_SOURCES                        = infratest.cpp
infratest_LDADD                          = $(COMMON_LDADD)

mock_device_SOURCES                      = mock-device.cpp                                          \
                                           MockDCLPServer.cpp                                       \
                                           MockDCServer.cpp                                         \
                                           MockDDServer.cpp                                         \
                                           MockFPServer.cpp                                         \
                                           MockNPServer.cpp                                         \
                                           MockOpActions.cpp                                        \
                                           MockPairingServer.cpp                                    \
                                           MockSPServer.cpp                                         \
                                           MockTimeSyncServer.cpp                                   \
                                           MockTimeSyncClient.cpp                                   \
                                           MockTimeSyncCoordinator.cpp                              \
                                           MockTimeSyncUtil.cpp                                     \
                                           MockTokenPairingServer.cpp                               \
                                           MockSinkTraits.cpp                                       \
                                           MockSourceTraits.cpp                                     \
                                           schema/nest/test/trait/TestATrait.cpp                    \
                                           schema/nest/test/trait/TestBTrait.cpp                    \
                                           schema/nest/test/trait/TestETrait.cpp                    \
                                           schema/nest/test/trait/TestCommon.cpp \
                                           schema/weave/trait/locale/LocaleSettingsTrait.cpp        \
                                           schema/weave/trait/locale/LocaleCapabilitiesTrait.cpp    \
                                           schema/weave/trait/security/BoltLockSettingsTrait.cpp    \
                                           schema/weave/trait/telemetry/NetworkWiFiTelemetryTrait.cpp \
                                           MockWdmViewServer.cpp                                    \
                                           MockWdmViewClient.cpp                                    \
                                           MockWdmSubscriptionInitiator.cpp                         \
                                           MockWdmTestVerifier.cpp	                            \
                                           MockWdmSubscriptionResponder.cpp                         \
										   MockWdmNodeOptions.cpp                                   \
                                           MockLoggingManager.cpp                                   \
                                           MockEvents.cpp

if WEAVE_BUILD_LEGACY_WDM
mock_device_SOURCES                      += \
                                           MockDMPublisher.cpp                                      \
                                           TestProfile.cpp
endif

TestErrorStr_SOURCES                     = TestErrorStr.cpp
TestErrorStr_LDADD                       = $(COMMON_LDADD)

TestStatusReportStr_SOURCES              = TestStatusReportStr.cpp
TestStatusReportStr_LDADD                = $(COMMON_LDADD)

TestThermostatStatus_SOURCES             = TestThermostatStatus.cpp
TestThermostatStatus_LDADD               = $(COMMON_LDADD)

TestWoble_SOURCES                        = TestWoble.cpp
TestWoble_LDADD                          = libWeaveTestCommon.a $(COMMON_LDADD)

TestPairingCodeUtils_SOURCES             = TestPairingCodeUtils.cpp
TestPairingCodeUtils_LDADD               = $(COMMON_LDADD)

TestResourceIdentifier_SOURCES           = TestResourceIdentifier.cpp
TestResourceIdentifier_LDADD             = $(COMMON_LDADD) $(TEST_PLATFORM_LDADD)

TestInetLayerDNS_SOURCES                = TestInetLayerDNS.cpp
TestInetLayerDNS_LDFLAGS                = $(AM_CPPFLAGS)
TestInetLayerDNS_LDADD                  = libWeaveTestCommon.a $(COMMON_LDADD)

mock_device_CPPFLAGS                     = $(AM_CPPFLAGS) -I$(top_srcdir)/src/test-apps/schema
mock_device_LDADD                        = libWeaveTestCommon.a $(COMMON_LDADD)

if CONFIG_BLE_PLATFORM_BLUEZ
mock_device_LDADD                       +=  @GLIB_LIBS@ @DBUS_LIBS@ -ldl -lrt
endif

mock_device_LDFLAGS                      = $(AM_CPPFLAGS)

mock_weave_bg_SOURCES                    = mock-weave-bg.cpp
mock_weave_bg_LDFLAGS                    = $(AM_CPPFLAGS)
mock_weave_bg_LDADD                      = libWeaveTestCommon.a $(COMMON_LDADD)

wdmtest_SOURCES                          = wdmtest.cpp
wdmtest_LDFLAGS                          = $(AM_CPPFLAGS)
wdmtest_LDADD                            = libWeaveTestCommon.a $(COMMON_LDADD)

weave_bdx_client_development_SOURCES     = weave-bdx-client-development.cpp  \
                                           weave-bdx-common-development.cpp
weave_bdx_client_development_LDFLAGS     = $(AM_CPPFLAGS)
weave_bdx_client_development_LDADD       = libWeaveTestCommon.a $(COMMON_LDADD)

weave_bdx_server_development_SOURCES     = weave-bdx-server-development.cpp  \
                                           weave-bdx-common-development.cpp
weave_bdx_server_development_LDFLAGS     = $(AM_CPPFLAGS)

weave_bdx_server_development_LDADD       = libWeaveTestCommon.a $(COMMON_LDADD)

if WEAVE_WITH_CURL
weave_bdx_client_development_LDADD      += $(CURL_LIBS)
weave_bdx_server_development_LDADD      += $(CURL_LIBS)
endif # WEAVE_WITH_CURL

weave_bdx_client_v0_SOURCES              = weave-bdx-client-v0.cpp
weave_bdx_client_v0_LDFLAGS              = ${AM_CPPFLAGS}
weave_bdx_client_v0_LDADD                = libWeaveTestCommon.a $(COMMON_LDADD)

weave_bdx_server_v0_SOURCES              = weave-bdx-server-v0.cpp              \
                                           nlweavebdxserver.cpp
weave_bdx_server_v0_LDFLAGS              = ${AM_CPPFLAGS}
weave_bdx_server_v0_LDADD                = libWeaveTestCommon.a $(COMMON_LDADD)

weave_connection_tunnel_SOURCES          = weave-connection-tunnel.cpp
weave_connection_tunnel_LDFLAGS          = ${AM_CPPFLAGS}
weave_connection_tunnel_LDADD            = libWeaveTestCommon.a $(COMMON_LDADD)

weave_dd_client_SOURCES                  = weave-dd-client.cpp
weave_dd_client_LDFLAGS                  = ${AM_CPPFLAGS}
weave_dd_client_LDADD                    = libWeaveTestCommon.a $(COMMON_LDADD)

weave_device_descriptor_SOURCES          = weave-device-descriptor.cpp
weave_device_descriptor_LDFLAGS          = ${AM_CPPFLAGS}
weave_device_descriptor_LDADD            = libWeaveTestCommon.a $(COMMON_LDADD)

weave_heartbeat_SOURCES                  = weave-heartbeat.cpp
weave_heartbeat_LDFLAGS                  = ${AM_CPPFLAGS}
weave_heartbeat_LDADD                    = libWeaveTestCommon.a  $(COMMON_LDADD)

weave_key_export_SOURCES                 = weave-key-export.cpp
weave_key_export_LDFLAGS                 = ${AM_CPPFLAGS}
weave_key_export_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

weave_ping_SOURCES                       = weave-ping.cpp
weave_ping_LDFLAGS                       = ${AM_CPPFLAGS}
weave_ping_LDADD                         = libWeaveTestCommon.a $(COMMON_LDADD)

weave_service_dir_SOURCES                = weave-service-dir.cpp \
                                           MockSDServer.cpp
weave_service_dir_LDFLAGS                = ${AM_CPPFLAGS}
weave_service_dir_LDADD                  = libWeaveTestCommon.a $(COMMON_LDADD)

weave_swu_client_SOURCES                 = weave-swu-client.cpp  \
                                           nlweaveswuclient.cpp  \
                                           MockIAServer.cpp
weave_swu_client_LDFLAGS                 = ${AM_CPPFLAGS}
weave_swu_client_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

weave_swu_server_SOURCES                 = weave-swu-server.cpp  \
                                           MockSWUServer.cpp
weave_swu_server_LDFLAGS                 = ${AM_CPPFLAGS}
weave_swu_server_LDADD                   = libWeaveTestCommon.a $(COMMON_LDADD)

if WEAVE_BUILD_COVERAGE
CLEANFILES                               = $(wildcard *.gcda *.gcno)

if WEAVE_BUILD_COVERAGE_REPORTS
# The bundle should positively be qualified with the absolute build
# path. Otherwise, VPATH will get auto-prefixed to it if there is
# already such a directory in the non-colocated source tree.

WEAVE_COVERAGE_BUNDLE                    = ${abs_builddir}/${PACKAGE}${NL_COVERAGE_BUNDLE_SUFFIX}
WEAVE_COVERAGE_INFO                      = ${WEAVE_COVERAGE_BUNDLE}/${PACKAGE}${NL_COVERAGE_INFO_SUFFIX}

WEAVE_LIBS_COVERAGE_BUNDLE               = ${abs_builddir}/${PACKAGE}-libs${NL_COVERAGE_BUNDLE_SUFFIX}
WEAVE_LIBS_COVERAGE_SOURCE               = ${abs_builddir}/${PACKAGE}-libs
WEAVE_LIBS_COVERAGE_INFO                 = ${WEAVE_LIBS_COVERAGE_BUNDLE}/${PACKAGE}-libs${NL_COVERAGE_INFO_SUFFIX}

$(WEAVE_COVERAGE_BUNDLE):
	$(call create-directory)

$(WEAVE_LIBS_COVERAGE_SOURCE):
	$(call create-directory)

$(WEAVE_LIBS_COVERAGE_BUNDLE):
	$(call create-directory)

$(WEAVE_COVERAGE_INFO): check-local | $(WEAVE_COVERAGE_BUNDLE)
	$(call generate-coverage-report,${abs_top_builddir})

# collect all the *.gcno & *gcda files of libWeave.a for Weave Library coverage analysis, rename the *h file paths to make it a clean report
$(WEAVE_LIBS_COVERAGE_INFO): $(WEAVE_COVERAGE_INFO) | $(WEAVE_LIBS_COVERAGE_BUNDLE) $(WEAVE_LIBS_COVERAGE_SOURCE)
	$(AM_V_at)find ${abs_top_builddir} -name "libWeave_a*.gc*" ! -path "${WEAVE_LIBS_COVERAGE_SOURCE}/*" | xargs -i cp {} ${WEAVE_LIBS_COVERAGE_SOURCE}
	$(call generate-coverage-report,${WEAVE_LIBS_COVERAGE_SOURCE})
	$(AM_V_at)$(SED) \
	    -e 's,${abs_top_builddir},$(abspath ${abs_top_srcdir}),g' \
	    -e 's,include/BleLayer/,ble/,g' \
	    -e 's,include/InetLayer/,inet/,g' \
	    -e 's,include/SystemLayer/,system/,g' \
	    -e 's,include/Warm/,warm/,g' \
	    -e 's,include/Weave/Core/,lib/core/,g' \
	    -e 's,include/Weave/Profiles/,lib/profiles/,g' \
	    -e 's,include/Weave/Support/,lib/support/,g' \
	    -e '/\/nlio/,/end_of_record/'d \
	    < $(@) \
	    > $(@).clean
	$(NL_V_GENHTML)$(GENHTML) $(NL_V_GENHTML_FLAGS) --config-file="$(abs_top_nlbuild_autotools_dir)/etc/lcov.config" $(WEAVE_LIBS_COVERAGE_INFO).clean --output-directory $(WEAVE_LIBS_COVERAGE_BUNDLE)
	$(AM_V_at)rm -rf $(WEAVE_LIBS_COVERAGE_SOURCE)

coverage-local: $(WEAVE_LIBS_COVERAGE_INFO)

clean-local: clean-local-coverage

.PHONY: clean-local-coverage
clean-local-coverage:
	-$(AM_V_at)rm -rf $(WEAVE_COVERAGE_BUNDLE)
	-$(AM_V_at)rm -rf $(WEAVE_LIBS_COVERAGE_BUNDLE)
endif # WEAVE_BUILD_COVERAGE_REPORTS
endif # WEAVE_BUILD_COVERAGE

# Targets and commands for installing and uninstalling the bin_LINKS
# programs as symbolic links which are trampolined through an
# execution script.

install-exec-local: install-exec-binLINKS

uninstall-local: uninstall-exec-binLINKS

install-exec-binLINKS: $(bin_LINKS)
	@$(NORMAL_INSTALL)
	@list='$(bin_LINKS)'; test -n "$(bindir)" || list=; \
	if test -n "$$list"; then \
          echo "  MKDIR    $(DESTDIR)$(bindir)"; \
	  $(MKDIR_P) "$(DESTDIR)$(bindir)" || exit 1; \
	fi; \
	for link in $${list}; do \
            echo "  LN       $${link}"; \
	    $(LN_S) -f "../`echo ${libexecdir} | sed -e 's,${exec_prefix}/,,g'`/weave-run" "$(DESTDIR)$(bindir)/$${link}"; \
        done

uninstall-exec-binLINKS: $(bin_LINKS)
	@$(NORMAL_UNINSTALL)
	@list='$(bin_LINKS)'; test -n "$(bindir)" || exit 0; \
	files=`for p in $$list; do echo "$$p"; done | \
	       sed -e 's,.*/,,;$(transform)'`; \
	dir='$(DESTDIR)$(bindir)'; $(am__uninstall_files_from_dir)
endif # WEAVE_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
