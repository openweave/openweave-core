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
#    Description:
#      This file is the GNU automake template for the OpenSSL elliptic
#      curve JPAKE extensions
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_OPENSSL

EXTRA_DIST                             = \
    openssl/include                      \
    $(NULL)

lib_LIBRARIES                          = \
    libopenssl-jpake.a                   \
    $(NULL)

libopenssl_jpake_a_CPPFLAGS            = \
    $(OPENSSL_CPPFLAGS)                  \
    -I $(srcdir)/openssl/include         \
	$(VISIBILITY_CFLAGS)                 \
    $(NULL)

libopenssl_jpake_a_SOURCES             = \
    openssl/crypto/jpake/jpake.c         \
    openssl/crypto/jpake/jpake_err.c     \
    openssl/crypto/ecjpake/ecjpake.c     \
    openssl/crypto/ecjpake/ecjpake_err.c \
    $(NULL)

endif # WEAVE_WITH_OPENSSL

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
