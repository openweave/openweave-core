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
#      Automake template for building/installing the OpenWeave python
#      package and the Weave Device Manager Shell script.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

# ------------------------------------------------------------
# "openweave" Python Package
# ------------------------------------------------------------

openweave_PYTHON                                  = \
    openweave/__init__.py                           \
    openweave/WeaveDeviceMgr.py                     \
    openweave/WeaveBleBase.py                       \
    openweave/WeaveCoreBluetoothMgr.py              \
    openweave/WeaveBluezMgr.py                      \
    openweave/WeaveBleUtility.py                    \
    $(NULL)

# Location into which the "openweave" python package will be installed.
openweavedir                                      = $(pyexecdir)/openweave

# ------------------------------------------------------------
# DLL wrapper for Weave Device Manager 
# ------------------------------------------------------------

openweave_LTLIBRARIES                             = _WeaveDeviceMgr.la

_WeaveDeviceMgr_la_SOURCES                        = \
    WeaveDeviceManager-ScriptBinding.cpp            \
    $(NULL)

if CONFIG_NETWORK_LAYER_BLE

_WeaveDeviceMgr_la_SOURCES                       += \
    WeaveDeviceManager-BlePlatformDelegate.cpp      \
    WeaveDeviceManager-BleApplicationDelegate.cpp   \
    $(NULL)

endif # CONFIG_NETWORK_LAYER_BLE

_WeaveDeviceMgr_la_CPPFLAGS                       = \
    -I$(srcdir)/..                                  \
    $(SOCKETS_CPPFLAGS)                             \
    $(NULL)

_WeaveDeviceMgr_la_CXXFLAGS                       = \
    $(PTHREAD_CFLAGS)                               \
    $(NULL)

_WeaveDeviceMgr_la_LIBADD                         = \
    -L$(top_builddir)/src/lib -lWeave               \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)              \
    $(PTHREAD_LIBS)                                 \
    $(NULL)

_WeaveDeviceMgr_la_DEPENDENCIES                   = \
    $(top_builddir)/src/lib/libWeave.a              \
    $(NULL)

_WeaveDeviceMgr_la_LDFLAGS                        = \
    -avoid-version                                  \
    -module                                         \
    -export-dynamic                                 \
    $(NULL)

noinst_HEADERS                                    = \
    WeaveDeviceManager-BleApplicationDelegate.h     \
    WeaveDeviceManager-BlePlatformDelegate.h        \
    $(NULL)

# After installation, remove the extraneous .a and .la files from
# the install directory.
install-exec-hook:
	rm -f $(DESTDIR)$(openweavedir)/_WeaveDeviceMgr.la
	rm -f $(DESTDIR)$(openweavedir)/_WeaveDeviceMgr.a

# ------------------------------------------------------------
# Weave Device Manager Shell Script 
# ------------------------------------------------------------

dist_bin_SCRIPTS = weave-device-mgr

weave-device-mgr: $(srcdir)/weave-device-mgr.py
	cp $(srcdir)/weave-device-mgr.py weave-device-mgr


include $(abs_top_nlbuild_autotools_dir)/automake/post.am
