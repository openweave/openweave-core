#
#    Copyright (c) 2019 Google LLC.
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
#      Device Manager Python script binding module.
#
#      NOTE: The Python module loader cannot differentiate between
#            WeaveDeviceMgr.py and WeaveDeviceMgr.so when attempting
#            to import WeaveDeviceMgr. Consequently, we follow typical
#            Python convention and name the DLL/DSO with a leading
#            underscore (_) to disambiguate them when they are sitting
#            in the same directory during development.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# set-relocation-path <target to change> <existing loader path> <new loader path>
#
# For Darwin-based systems, change the dependent shared library loader
# path name from the existing name to the new name in the specified
# target.
#
define set-relocation-path
$(NL_V_AT)if [[ "$(target_os)" == darwin* ]]; then \
    target_to_change="$(1)"; \
    old_name="`otool -L $(1) | sed -n -e 's,^[[:space:]]*\(.*$(2)\).*$$,\1,gp'`"; \
    new_name="$(call Slashify,$(3))$(2)"; \
    install_name_tool -change "$${old_name}" "$${new_name}" "$${target_to_change}"; \
fi
endef # set-relocation-path


EXTRA_DIST                                        = \
    weave-device-mgr.py                             \
    $(NULL)

pyexec_LTLIBRARIES                                = _WeaveDeviceMgr.la

noinst_HEADERS                                    = \
    $(NULL)

_WeaveDeviceMgr_la_SOURCES                        = \
    WeaveDeviceManager-ScriptBinding.cpp            \
    $(NULL)

_WeaveDeviceMgr_la_CPPFLAGS                       = \
    -I$(srcdir)/..                                  \
    $(LWIP_CPPFLAGS)                                \
    $(SOCKETS_CPPFLAGS)                             \
    $(NULL)

_WeaveDeviceMgr_la_LIBADD                         = \
    ../libWeaveDeviceManager.la                     \
    -L$(top_builddir)/src/lib -lWeave               \
    $(NULL)

_WeaveDeviceMgr_la_DEPENDENCIES                   = \
    $(top_builddir)/src/lib/libWeave.a              \
    $(NULL)

_WeaveDeviceMgr_la_LDFLAGS                        = \
    -avoid-version                                  \
    -module                                         \
    -no-undefined                                   \
    $(NULL)

weavedir                                          = $(pyexecdir)/weave

modules                                           = \
    WeaveDeviceMgr.py                               \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

noinst_HEADERS                                   += \
    WeaveDeviceManager-BleApplicationDelegate.h     \
    WeaveDeviceManager-BlePlatformDelegate.h        \
    $(NULL)

_WeaveDeviceMgr_la_SOURCES                       += \
    WeaveDeviceManager-BlePlatformDelegate.cpp      \
    WeaveDeviceManager-BleApplicationDelegate.cpp   \
    $(NULL)

modules                                          += \
    WeaveBleBase.py                                    \
    WeaveCoreBluetoothMgr.py                        \
    WeaveBluezMgr.py                                \
    WeaveBleUtility.py                              \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

srcmodules                                        = $(addprefix $(srcdir)/,$(modules))
buildmodules                                      = $(addprefix $(builddir)/,$(modules))
cleanmodules                                      = $(if $(call IsRealPath,$(buildmodules)),,$(buildmodules))

weave_SCRIPTS                                     = \
    $(modules)                                      \
    $(NULL)

weave_PYTHON                                      = \
    $(weave_SCRIPTS)                                \
    $(NULL)

# Tool scripts that should be installed when the 'install' target is run.
#
# These will be part of the externally-consumable SDK.
#
# These need to go in libexec since they need to be "trampolined"
# through the 'weave-run' script due to runtime dependencies on
# DLLs/DSOs that the runtime linker / loader will need to resolve when
# they invoked.

libexec_SCRIPTS                                   = \
    weave-device-mgr                                \
    $(NULL)

# Create a convenience link between the suffixed script and the actual
# name by which the script will be installed for non-co-located build
# and source directories.

%: %.py
	$(AM_V_LN_S)$(LN_S) $(<) $(@)

# The bin_LINKS variable enumerates programs that need to be
# "trampolined" through the 'weave-run' script due to runtime
# dependencies on DLLs/DSOs that the runtime linker / loader will need
# to resolve when they invoked.

bin_LINKS                                         = \
    $(libexec_SCRIPTS)                              \
    $(NULL)

CLEANFILES                                        = \
    $(libexec_SCRIPTS)                              \
    _WeaveDeviceMgr.so                              \
    $(cleanmodule)                                  \
    $(NULL)

# Create convenience links so that those that are doing Weave Device
# Manager development can run the script right from this directory,
# picking up both the Python module and the DLL/DSO, without having to
# do a 'make install'.
#
# Because of automake's VPATH getting in our way, we need to play a
# "trick" to get the "buildmodule" DLL/DSO link to work.

all-local: _WeaveDeviceMgr.so $(buildmodule)

_WeaveDeviceMgr.so: _WeaveDeviceMgr.la
	$(AM_V_LN_S)$(LN_S) -f .libs/$(@) $(@)
	$(AM_V_at)if test "${srcmodule}" != "${buildmodule}" && \
           ! test -L "${buildmodule}"; then \
	    echo "  LN       ${buildmodule}"; \
	    $(LN_S) "${srcmodule}" "${buildmodule}"; \
        fi
	$(call set-relocation-path,$(@),libWeaveDeviceManager.$(LIBWEAVE_VERSION_CURRENT).dylib,@loader_path/../.libs/)

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

# Fix up, post-install, the dynamic loader path for _WeaveDeviceMgr.so
# such that it can find the libWeaveDeviceManager.<version>.dylib
# regardless of whether the install is in a final, configured location
# (e.g., /usr/local/...) or in a temporary installed location (e.g.,
# tmp-install/usr/local/...).

install-exec-hook:
	$(call set-relocation-path,$(DESTDIR)$(pyexecdir)/_WeaveDeviceMgr.so,libWeaveDeviceManager.$(LIBWEAVE_VERSION_CURRENT).dylib,@loader_path/../../)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
