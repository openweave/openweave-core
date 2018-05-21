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
#      test applications
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

LOG_DRIVER=$(abs_srcdir)/weave-parallel-test-engine

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                                 = \
    $(NULL)

if WEAVE_BUILD_TESTS

# A non-installed common library that may be used for all test
# applications in this directory.
#
# This will NOT be part of the externally-consumable binary SDK.

noinst_LIBRARIES                               = \
    libFuzzUtil.a                                \
    $(NULL)

MAYBE_FUZZ                                     = \
    $(NULL)

if WEAVE_FUZZING_ENABLED
    MAYBE_FUZZ += -g -O1 -fsanitize=fuzzer,address
endif

# C preprocessor option flags that will apply to all compiled objects in this
# makefile.
    
AM_CPPFLAGS                                    = \
    $(MAYBE_FUZZ)                                \
    -I$(top_srcdir)/src/include                  \
    -I$(top_srcdir)/src/test-apps                \
    -I$(top_srcdir)/tools/weave/                 \
    $(LWIP_CPPFLAGS)                             \
    $(SOCKETS_CPPFLAGS)                          \
    $(PTHREAD_CFLAGS)                            \
    $(NULL)

libFuzzUtil_a_SOURCES                     = \
    FuzzUtils.cpp                           \
    $(NULL)


TEST_COMMON_LDADD                              = \
    $(top_builddir)/src/test-apps/libWeaveTestCommon.a             \
    $(NULL)

WEAVE_LDADD                                    = \
    $(top_builddir)/src/lib/libWeave.a       \
    $(NULL)

if CONFIG_BLE_PLATFORM_BLUEZ

BLE_LDADD                                    = \
    $(top_builddir)/src/platform/ble/bluez/libWoBluez.a \
    $(BLUEZ_LDFLAGS)                                    \
    @BLUEZ_LIBS@ @GLIB_LIBS@ @DBUS_LIBS@ -ldl -lrt      \
    $(NULL)

else

BLE_LDADD                                  = \
    $(NULL)

endif #CONFIG_BLE_PLATFORM_BLUEZ

COMMON_LDADD                                   = \
    $(COMMON_LDFLAGS)                            \
    $(WEAVE_LDADD)                               \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)                 \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)           \
    $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)            \
    $(BLE_LDADD)                                 \
    $(NULL)
#                             

# Test applications that should be run when the 'check' target is run.
#
# These will NOT be part of the externally-consumable binary SDK.

check_PROGRAMS                                 = \
    FuzzPASEInitiatorStep1                       \
    FuzzPASEResponderStep1                       \
    FuzzPASEResponderStep2                       \
    FuzzPASEInitiatorStep2                       \
    FuzzPASEKeyConfirm                           \
    FuzzCertificateConversion                    \
    $(NULL)


# Test applications that should be built but not installed and should
# always be built to ensure overall "build sanity".
#
# These will NOT be part of the externally-consumable binary SDK.

noinst_PROGRAMS                                = \
    FuzzPASEInitiatorStep1                       \
    FuzzPASEResponderStep1                       \
    FuzzPASEResponderStep2                       \
    FuzzPASEInitiatorStep2                       \
    FuzzPASEKeyConfirm                           \
    FuzzCertificateConversion                    \
    $(NULL)

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


# Source, compiler, and linker options for test programs.

FuzzPASEInitiatorStep1_SOURCES           = FuzzPASEInitiatorStep1.cpp
FuzzPASEInitiatorStep1_LDFLAGS           = $(AM_CPPFLAGS)
FuzzPASEInitiatorStep1_LDADD             = libFuzzUtil.a $(TEST_COMMON_LDADD) $(COMMON_LDADD)

FuzzPASEResponderStep1_SOURCES           = FuzzPASEResponderStep1.cpp
FuzzPASEResponderStep1_LDFLAGS           = $(AM_CPPFLAGS)
FuzzPASEResponderStep1_LDADD             = libFuzzUtil.a $(TEST_COMMON_LDADD) $(COMMON_LDADD) 

FuzzPASEResponderStep2_SOURCES           = FuzzPASEResponderStep2.cpp
FuzzPASEResponderStep2_LDFLAGS           = $(AM_CPPFLAGS)
FuzzPASEResponderStep2_LDADD             = libFuzzUtil.a $(TEST_COMMON_LDADD) $(COMMON_LDADD) 

FuzzPASEInitiatorStep2_SOURCES           = FuzzPASEInitiatorStep2.cpp
FuzzPASEInitiatorStep2_LDFLAGS           = $(AM_CPPFLAGS)
FuzzPASEInitiatorStep2_LDADD             = libFuzzUtil.a $(TEST_COMMON_LDADD) $(COMMON_LDADD) 

FuzzPASEKeyConfirm_SOURCES               = FuzzPASEKeyConfirm.cpp
FuzzPASEKeyConfirm_LDFLAGS               = $(AM_CPPFLAGS)
FuzzPASEKeyConfirm_LDADD                 = libFuzzUtil.a $(TEST_COMMON_LDADD) $(COMMON_LDADD) 

FuzzCertificateConversion_SOURCES        = FuzzCertificateConversion.cpp
FuzzCertificateConversion_LDFLAGS        = $(AM_CPPFLAGS)
FuzzCertificateConversion_LDADD          = libFuzzUtil.a $(top_builddir)/src/tools/weave/libWeaveTool.a $(TEST_COMMON_LDADD) $(COMMON_LDADD) 

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
