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
#      This file is a GNU automake template for the Weave Support
#      sub-group of the Weave SDK library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

#      The work done here is primarily focused on auto-generating
#      the ASN1OID.h header file.
#

EXTRA_DIST                    = \
    gen-oid-table.py            \
    ula/make-ula-global-id.py   \
    $(NULL)

BUILT_SOURCES                 = \
    ASN1OID.h                   \
    $(NULL)

ASN1OID.h: $(srcdir)/gen-oid-table.py
	$(AM_V_at)$(RM) $(@)
	$(AM_V_GEN)$(srcdir)/gen-oid-table.py > $(@)

clean-local:
	-@$(AM_V_at)$(RM) ASN1OID.h

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
