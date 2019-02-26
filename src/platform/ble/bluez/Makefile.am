#
#    Copyright (c) 2014-2018 Nest Labs, Inc.
#    Copyright (c) 2018 Google LLC
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
#      This file is the GNU automake template for the Nest WoBluez
#      library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                       = libWoBluez.a

libWoBluez_a_CPPFLAGS               = \
    $(BLUEZ_CPPFLAGS)                 \
    $(LWIP_CPPFLAGS)                  \
    $(SOCKET_CPPFLAGS)                \
    $(NULL)

libWoBluez_a_SOURCES                = \
    BluezBleApplicationDelegate.cpp   \
    BluezBlePlatformDelegate.cpp      \
    BluezHelperCode.cpp               \
    WoBluez.cpp                       \
    $(NULL)$(nl_WoBluez_sources)

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                      = \
    BluezHelperCode.h                 \
    WoBluez.h                         \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
