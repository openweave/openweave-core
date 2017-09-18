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
#      This file is the GNU automake template for the Nest Micro-ECC
#      library.
#

EXTRA_DIST                             = \
    README.third_party                   \
    micro-ecc/LICENSE.txt                \
    micro-ecc/README.md                  \
    micro-ecc/asm_arm.inc                \
    micro-ecc/asm_arm_mult_square.inc    \
    micro-ecc/asm_avr.inc                \
    micro-ecc/asm_avr_mult_square.inc    \
    micro-ecc/curve-specific.inc         \
    micro-ecc/emk_project.py             \
    micro-ecc/emk_rules.py               \
    micro-ecc/platform-specific.inc      \
    micro-ecc/scripts                    \
    micro-ecc/test                       \
    $(NULL)

lib_LIBRARIES                          = libuECC.a

libuECC_a_CPPFLAGS                     = \
    -DuECC_WORD_SIZE=4                   \
    -DuECC_RNG_MAX_TRIES=2               \
    -DuECC_ENABLE_VLI_API=1              \
    -DuECC_SUPPORTS_secp256k1=0          \
    -DuECC_SUPPORT_COMPRESSED_POINT=0    \
    $(NULL)

libuECC_a_SOURCES                      = \
    micro-ecc/uECC.c                     \
    $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
