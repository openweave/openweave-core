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
#      This file is the GNU automake template for the Nest InetLayer
#      Router Advertisement (RA) daemon library.

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

lib_LIBRARIES                    = libRADaemon.a

libRADaemon_a_CPPFLAGS           = \
    -I$(top_srcdir)/src/include    \
    $(LWIP_CPPFLAGS)               \
    $(SOCKETS_CPPFLAGS)            \
    $(NULL)

libRADaemon_a_SOURCES            = \
    RADaemon.cpp                   \
    $(NULL)

EXTRA_DIST                       = \
    RADaemon.h                     \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
