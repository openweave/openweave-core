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
#      This file is the GNU automake template for various Nest Weave
#      support tools.

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

EXTRA_DIST                              = \
    decode-service-dir.py                 \
    gen-qr-code.py                        \
    openssl-bn-to-c.py                    \
    weave-run.py                          \
    $(NULL)

if WEAVE_BUILD_TOOLS
bin_SCRIPTS                             = \
    $(NULL)

if WEAVE_WITH_PYTHON
bin_SCRIPTS                            += \
    gen-qr-code                           \
    $(NULL)

libexec_SCRIPTS                         = \
    weave-run                             \
    $(NULL)

CLEANFILES                              = \
    $(bin_SCRIPTS)                        \
    $(libexec_SCRIPTS)                    \
    $(NULL)

# Create a convenience link between the suffixed script and the actual
# name by which the script will be installed for non-co-located build
# and source directories.

%: %.py
	$(AM_V_LN_S)$(LN_S) $(<) $(@)

endif # WEAVE_WITH_PYTHON

endif # WEAVE_BUILD_TOOLS

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
