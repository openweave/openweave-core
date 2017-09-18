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
#      This file is the GNU automake template for the Nest Weave SDK.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

# To ensure that the .local-version file and subsequent rechecks work
# correctly, don't allow this top-level makefile to run in parallel.

.NOTPARALLEL:

AM_MAKEFLAGS                             = --no-print-directory

SUBDIRS                                  = \
    third_party                            \
    src                                    \
    doc                                    \
    $(NULL)

PRETTY_SUBDIRS                           = \
    $(filter-out third_party,$(SUBDIRS))   \
    $(NULL)

EXTRA_DIST                               = \
    Makefile-Android                       \
    Makefile-iOS                           \
    Makefile-Standalone                    \
    .default-version                       \
    bootstrap                              \
    bootstrap-configure                    \
    $(srcdir)/build/autoconf               \
    $(srcdir)/build/scripts                \
    $(srcdir)/certs                        \
    $(NULL)

BUILT_SOURCES                            = \
    .local-version                         \
    $(NULL)

dist_doc_DATA                            = \
    CHANGELOG                              \
    LICENSE                                \
    $(NULL)

DISTCLEANFILES                           = \
    .local-version                         \
    $(NULL)

# There are no source files to lint or prettify in this subdirectory.

PRETTY_FILES                            := $(NULL)

#
# Package version files:
#
# .default-version - The default package version. This file is ALWAYS checked
#                    in and should always represent the current baseline
#                    version of the package.
#
# .dist-version    - The distributed package version. This file is NEVER
#                    checked in within the upstream repository, is auto-
#                    generated, and is only found in the package distribution.
#
# .local-version   - The current source code controlled package version. This
#                    file is NEVER checked in within the upstream repository,
#                    is auto-generated, and can always be found in both the
#                    build tree and distribution.
#
# When present, the .local-version file is preferred first, the
# .dist-version second, and the .default-version last.
#

VERSION_FILE                      := $(if $(wildcard $(builddir)/.local-version),$(builddir)/.local-version,$(if $(wildcard $(srcdir)/.dist-version),$(srcdir)/.dist-version,$(srcdir)/.default-version))

#
# Override autotool's default notion of the package version variables.
# This ensures that when we create a source distribution that the
# version is always the current version, not the version when the
# package was bootstrapped.
#

WEAVE_VERSION                     ?= $(shell cat $(VERSION_FILE) 2> /dev/null)

PACKAGE_VERSION                    = $(WEAVE_VERSION)
VERSION                            = $(PACKAGE_VERSION)

#
# check-file-.local-version
#
# Speculatively regenerate .local-version and check to see if it needs
# to be updated.
#
# If WEAVE_VERSION has been supplied anywhere other than in this file
# (which is implicitly the contents of .local-version), then use that;
# otherwise, attempt to generate it from the SCM system.
#
# This is called from $(call check-file,.local-version).
#
define check-file-.local-version
if [ "$(origin WEAVE_VERSION)" != "file" ]; then       \
    echo "$(WEAVE_VERSION)" > "$(2)";                  \
else                                                   \
    $(abs_top_nlbuild_autotools_dir)/scripts/mkversion \
        -b "$(WEAVE_VERSION)" "$(top_srcdir)"          \
        > "$(2)";                                      \
fi
endef

#
# check-file-.dist-version
#
# Speculatively regenerate .dist-version and check to see if it needs
# to be updated.
#
# This is called from $(call check-file,.dist-version).
#
define check-file-.dist-version
cat "$(1)" > "$(2)"
endef

#
# A convenience target to allow package users to easily rerun the
# package configuration according to the current configuration.
#
.PHONY: reconfigure
reconfigure: $(builddir)/config.status
	$(AM_V_at)$(<) --recheck

#
# Version file regeneration rules.
#
.PHONY: force

$(builddir)/.local-version: $(srcdir)/.default-version force

$(distdir)/.dist-version: $(builddir)/.local-version force

$(distdir)/.dist-version $(builddir)/.local-version:
	$(call check-file,$(@F))

dist distcheck install-headers: $(BUILT_SOURCES)

dist-hook: $(distdir)/.dist-version

#
# Top-level convenience target for making a documentation-only
# distribution whose results appear at the top level of the build tree
# in the same fashion that the distribution would be for 'make dist'.
#

.PHONY: docdist
docdist: $(BUILT_SOURCES)
	$(MAKE) -C doc docdistdir=$(abs_builddir) $(@)

#
# Top-level convenience target for making a tools-only distribution
# whose resuls appear at the top level of the build tree in the same
# fashion that the distribution would be for 'make dist'.
#

tooldistdir                                     ?= $(abs_builddir)

weave_tooldist_alias                             = \
    $(PACKAGE_TARNAME)-tools

weave_tooldist_name                              = \
    $(weave_tooldist_alias)-$(shell echo $(target) | $(SED) -e 's/[[:digit:].]*$$//g')-$(WEAVE_VERSION)

weave_tooldist_archive                           = \
    $(tooldistdir)/$(weave_tooldist_name).tar.gz

CLEANFILES                                       = \
    $(weave_tooldist_archive)                      \
    $(NULL)

$(weave_tooldist_name):
	$(AM_V_at)rm -f -r "$(@)"
	$(call create-directory)
	$(MAKE) DESTDIR=$(tooldistdir)/$(weave_tooldist_name) install
	$(AM_V_at)rm -f -r         \
	    "$(@)/include"         \
	    "$(@)/lib/engines"     \
	    "$(@)/lib/libcrypto.a" \
	    "$(@)/lib/libssl.a"    \
	    "$(@)/lib/pkgconfig"   \
	    "$(@)/openssl"         \
	    "$(@)/share/java"

$(weave_tooldist_archive): $(weave_tooldist_name)
	$(AM_V_at)echo "  TAR      $(@)"
	$(AM_V_at)tardir="$(<)" && $(am__tar) | GZIP=$(GZIP_ENV) gzip -c > "$(@)" && rm -rf "$(<)"

.PHONY: tooldist
tooldist $(weave_tooldist_alias): $(weave_tooldist_archive)

#
# By virtue of the way we allow for the creation of both libWeave and
# libInetLayer, using the same makefile fragment included in the
# Makefile.am of each of those libraries, running 'make distclean'
# will fail in src/lib after running in src/inet because the
# dependencies for the latter have already been cleaned for the
# former.
#
# In general, we don't really care about this specifically or if a
# particular step of distclean fails generally, so propagate the
# --ignore-errors and --keep-going flags for make on recursive
# distclean to avoid this.
#

distclean-recursive: AM_MAKEFLAGS += -ik

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
