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

#    Description:
#      This file is the GNU automake template for the Nest Weave
#      command line utility.

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_BUILD_TOOLS
# Tool applications that should be installed when the 'install' target is run.
#
# These will be part of the externally-consumable SDK.
#
# These need to go in libexec since they need to be "trampolined"
# through the 'weave-run' script due to runtime dependencies on
# DLLs/DSOs that the runtime linker / loader will need to resolve when
# they invoked.

libexec_PROGRAMS			= \
    weave                                 \
    $(NULL)

noinst_HEADERS                          = \
    weave-tool.h                          \
    $(NULL)

noinst_LIBRARIES                        = \
    libWeaveTool.a                        \
    $(NULL)

libWeaveTool_a_SOURCES                  = \
    CertUtils.cpp                         \
    GeneralUtils.cpp                      \
    KeyUtils.cpp                          \
    $(NULL)

libWeaveTool_a_CPPFLAGS				    = \
    $(AM_CPPFLAGS)                        \
    -I$(top_srcdir)/src/include           \
    $(LWIP_CPPFLAGS)                      \
    $(SOCKETS_CPPFLAGS)                   \
    $(NULL)

libWeaveTool_a_CXXFLAGS                 = \
    $(PTHREAD_CFLAGS)                     \
    $(NULL)



weave_SOURCES				            = \
    Cmd_ConvertCert.cpp                   \
    Cmd_ConvertProvisioningData.cpp       \
    Cmd_ConvertKey.cpp                    \
    Cmd_GenCACert.cpp                     \
    Cmd_GenCodeSigningCert.cpp            \
    Cmd_GenDeviceCert.cpp                 \
    Cmd_GenGeneralCert.cpp                \
    Cmd_GenProvisioningData.cpp           \
    Cmd_GenServiceEndpointCert.cpp        \
    Cmd_MakeAccessToken.cpp               \
    Cmd_MakeServiceConfig.cpp             \
    Cmd_PrintCert.cpp                     \
    Cmd_PrintSig.cpp                      \
    Cmd_PrintTLV.cpp                      \
    Cmd_ValidateCert.cpp                  \
    Cmd_ResignCert.cpp                    \
    weave-tool.cpp                        \
    $(NULL)

weave_CPPFLAGS				= \
    $(AM_CPPFLAGS)                        \
    -I$(top_srcdir)/src/include           \
    $(LWIP_CPPFLAGS)                      \
    $(SOCKETS_CPPFLAGS)                   \
    $(NULL)

weave_CXXFLAGS                          = \
    $(PTHREAD_CFLAGS)                     \
    $(NULL)

weave_LDADD				                = \
    libWeaveTool.a                        \
    -L$(top_builddir)/src/lib -lWeave     \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)          \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)    \
    $(PTHREAD_LIBS)                       \
    $(NULL)

weave_DEPENDENCIES                      = \
    libWeaveTool.a                        \
    $(top_builddir)/src/lib/libWeave.a    \
    $(NULL)

# The bin_LINKS variable enumerates programs that need to be
# "trampolined" through the 'weave-run' script due to runtime
# dependencies on DLLs/DSOs that the runtime linker / loader will need
# to resolve when they invoked.

bin_LINKS                               = \
    $(libexec_PROGRAMS)                   \
    $(NULL)


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

endif # WEAVE_BUILD_TOOLS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
