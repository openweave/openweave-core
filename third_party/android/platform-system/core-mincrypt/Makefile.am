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
#      This file is the GNU automake template for the Weave in-package,
#      MinCrypt Android crypto library.
#
#      MinCrypt lacks any build system of its own, so this actually builds
#      the library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                             = \
    include                              \
    libmincrypt/Android.mk               \
    libmincrypt/NOTICE                   \
    libmincrypt/dsa_sig.c                \
    libmincrypt/p256.c                   \
    libmincrypt/p256_ec.c                \
    libmincrypt/p256_ecdsa.c             \
    libmincrypt/rsa.c                    \
    libmincrypt/test                     \
    libmincrypt/tools                    \
    $(NULL)

lib_LIBRARIES                          = \
    libmincrypt.a                        \
    $(NULL)

libmincrypt_a_CPPFLAGS                 = \
    -I $(srcdir)/include                 \
    $(NULL)

libmincrypt_a_SOURCES                  = \
    libmincrypt/sha.c                    \
    libmincrypt/sha256.c                 \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
