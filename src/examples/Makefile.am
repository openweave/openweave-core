#
#    Copyright (c) 2017 Nest Labs, Inc.
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
#      example applications
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#
# Local headers to build against and distribute but not to install
# since they are not part of the Weave SDK public API.
#
noinst_HEADERS                                 = \
    weave-app-common.h                           \
    $(NULL)

if WEAVE_BUILD_TESTS
# C preprocessor option flags that will apply to all compiled objects in this
# makefile.

AM_CPPFLAGS                                    = \
    -I$(top_srcdir)/src/include                  \
    $(LWIP_CPPFLAGS)                             \
    $(SOCKETS_CPPFLAGS)                          \
    $(PTHREAD_CFLAGS)                            \
    $(NULL)

WEAVE_LDADD                                    = \
    $(top_builddir)/src/lib/libWeave.a           \
    $(NULL)

COMMON_LDADD                                   = \
    $(COMMON_LDFLAGS)                            \
    $(WEAVE_LDADD)                               \
    $(LWIP_LDFLAGS) $(LWIP_LIBS)                 \
    $(SOCKETS_LDFLAGS) $(SOCKETS_LIBS)           \
    $(PTHREAD_CFLAGS) $(PTHREAD_LIBS)            \
    $(NULL)

# Example applications that should be installed when the 'install' target is run.
#
# These will be part of the externally-consumable binary SDK.
#

noinst_PROGRAMS                                = \
    weave-echo-requester                         \
    weave-echo-responder                         \
    $(NULL)

# Source, compiler, and linker options for examples.

weave_echo_requester_SOURCES             = weave-app-common.cpp                    \
                                           echo/weave-echo-requester.cpp
weave_echo_requester_LDFLAGS             = ${AM_CPPFLAGS}
weave_echo_requester_LDADD               = $(COMMON_LDADD)

weave_echo_responder_SOURCES             = weave-app-common.cpp                    \
                                           echo/weave-echo-responder.cpp
weave_echo_responder_LDFLAGS             = ${AM_CPPFLAGS}
weave_echo_responder_LDADD               = $(COMMON_LDADD)
endif # WEAVE_BUILD_TESTS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
