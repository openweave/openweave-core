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
#      This file is the GNU automake template for the Nest BluezBleDelegates
#      library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                       = libBluezBleDelegates.a

nl_BluezBleDelegates_sources                  = \
    @top_builddir@/src/platform/ble/bluez/BluezBleDelegates.cpp                  \
    @top_builddir@/src/platform/ble/bluez/BluezHelperCode.cpp                    \
    @top_builddir@/src/platform/ble/bluez/WoBluez.cpp                            \
    $(NULL)

libBluezBleDelegates_a_CPPFLAGS             = \
    $(NULL)

libBluezBleDelegates_a_SOURCES               = $(nl_BluezBleDelegates_sources)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
