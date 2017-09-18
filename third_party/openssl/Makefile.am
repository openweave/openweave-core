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
#      tuned OpenSSL library.
#
#      This serves as "glue" or a proxy from the Weave build system to
#      the OpenSSL build system.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_OPENSSL_INTERNAL
#
# The OpenSSL build system relies on the built-in make variables to
# ensure that ARFLAGS is set such that archive libraries can be
# successfully created.
#
# Consequently, if make is invoked with -R or --no-builtin-variables
# or if MAKEFLAGS contains -R or --no-builtin-variables, then ARFLAGS
# will be unset and OpenSSL will cryptically fail to build.
#
# Attempt to detect this condition and provide the user with a
# corrective action when detected.
#

$(info Checking ARFLAGS...)

IsVariable        = $(findstring $(2),$(origin $(1)))
IsVariableDefault = $(call IsVariable,$(1),default)

$(if $(call IsVariableDefault,ARFLAGS),$(info ARFLAGS is '$(ARFLAGS)', OpenSSL should build correctly),$(error MAKEFLAGS is '$(MAKEFLAGS)' and ARFLAGS is '$(ARFLAGS)'. Your environment may have MAKEFLAGS set with '-R' or '--no-builtin-variables' set from another build system. This prevents the OpenSSL build system from picking up ARFLAGS from make default variables. Please try unsetting MAKEFLAGS from your environment before invoking make.))
endif # WEAVE_WITH_OPENSSL_INTERNAL

EXTRA_DIST                           = \
    README.third_party                 \
    openssl                            \
    openssl.url                        \
    openssl.version                    \
    patches                            \
    $(NULL)

if WEAVE_WITH_OPENSSL_INTERNAL

# OpenSSL preprocessor definitions applicable for all target for the
# in-package OpenSSL.
#
# Pick up any target-specific preprocessor definitions that might be
# required.

WEAVE_OPENSSL_DEFINES                = $(WEAVE_OPENSSL_TARGET_DEFINES)

# Tell the OpenSSL configuration script what features we do and do not
# want enabled.

WEAVE_OPENSSL_DISABLE_OPTIONS	     = no-asm no-engine no-dso no-shared no-apps no-test no-ssl no-tools

WEAVE_OPENSSL_ENABLE_OPTIONS	     = 

WEAVE_OPENSSL_OPTIONS		     = \
    $(WEAVE_OPENSSL_DISABLE_OPTIONS)   \
    $(WEAVE_OPENSSL_ENABLE_OPTIONS)

# OpenSSL generally does a poor job of maintaining the sanctity of its
# source files across configuration and build, overwriting the package
# defaults with the user's selection.
#
# This causes unnecessary changes to appear if OpenSSL has been
# imported under active source code control.

openssl_builddir                     = $(builddir)/$(openssl_buildstem)
abs_openssl_builddir                 = $(abs_builddir)/$(openssl_buildstem)

# Sentinel files used for shadowing and configuring the package.


OPENSSL_SHADOW_SENTINEL		     = $(openssl_builddir)/.shadow-stamp
OPENSSL_CONFIG_SENTINEL		     = $(abs_openssl_builddir)/.configure-stamp

#
# GNU autoconf/automake targets proxied to the OpenSSL makefile.
#

# For the local 'all' target, build the OpenSSL source.

all-local: build-local

# OpenSSL doesn't intrinsically support non-colocated source and build
# directories, so shadow the source directory to the build directory.
#
# Use a dedicated sentinel stamp file to ensure the job is
# done. Attempts to use an actual shadowed file (e.g. Configure) will
# fail with non-colocated source and build directories because VPATH
# will believe it to already be satisfied.

$(dir $(OPENSSL_SHADOW_SENTINEL)):
	$(create-directory)

$(OPENSSL_SHADOW_SENTINEL): | $(dir $(OPENSSL_SHADOW_SENTINEL))
	@echo "  LNDIR    $(srcdir)"
	$(AM_V_at)$(LNDIR) $(abs_srcdir)/openssl/. $(@D)
	$(AM_V_at)echo "sentinel stamp for openssl shadow" > $(@)

.PHONY: shadow-local
shadow-local: $(OPENSSL_SHADOW_SENTINEL)

# OpenSSL requires configuration prior to building. Configure it
# appropriately for the target system, shadowing the source first
# if necessary.
#
# Note that, particularly for double-dash C compiler flags such as
# --sysroot, OpenSSL Configure will error out if compiler flags are
# placed anywhere other than at the end as os[/compiler][:flags] as
# shown below.
#
# There are no sentinel files created anew after configuration
# different from those prior to configuration. Consequently, create
# out own sentinel file.

$(OPENSSL_CONFIG_SENTINEL): $(OPENSSL_SHADOW_SENTINEL)
	@echo "  CONFIG   $(WEAVE_OPENSSL_TARGET)"
	$(AM_V_CONFIG)cd $(abs_openssl_builddir) && \
                INSTALL="$(INSTALL)" \
		CFLAGS="$(CFLAGS)" \
		./Configure \
		--prefix=/ \
		--openssldir=/openssl \
		$(VISIBILITY_CFLAGS) \
		$(WEAVE_OPENSSL_OPTIONS) \
		$(WEAVE_OPENSSL_DEFINES) \
		$(WEAVE_OPENSSL_TARGET)
	$(AM_V_at)echo "sentinel stamp for openssl configuration" > $(@)

.PHONY: configure-local
configure-local: $(OPENSSL_CONFIG_SENTINEL)

# Build the configured OpenSSL source.

.PHONY: build-local
build-local: configure-local
	@echo "  SUBDIR   $(openssl_builddir)"
	@echo "  MAKE     all"
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(abs_openssl_builddir) \
		CC="$(CC) $(CFLAGS)" AR="$(AR) $(ARFLAGS)" RANLIB=$(RANLIB) \
		CROSS_TOP="$(CROSS_TOP)" \
		CROSS_SDK="$(CROSS_SDK)" \
                CROSS_SYSROOT="$(CROSS_SYSROOT)" \
	        INSTALL_PREFIX=$(DESTDIR)$(prefix) \
		all

# Install (stage) the built OpenSSL source.

stage-local: build-local
	@echo "  SUBDIR   $(openssl_builddir)"
	@echo "  MAKE     install"
	$(AM_V_at)$(MAKE) $(AM_MAKEFLAGS) -C $(abs_openssl_builddir) \
	        CC="$(CC) $(CFLAGS)" AR="$(AR) $(ARFLAGS)" RANLIB=$(RANLIB) \
		CROSS_TOP="$(CROSS_TOP)" \
		CROSS_SDK="$(CROSS_SDK)" \
                CROSS_SYSROOT="$(CROSS_SYSROOT)" \
	        INSTALL="$(INSTALL)" \
	        INSTALL_PREFIX=$(DESTDIR)$(prefix) \
	        LIBDIR=$(libdir) \
	        install_sw

install-data-local: stage-local

install-headers: stage-local

# Clean things up to the requested level.

clean-local:
	$(AM_V_at)if test -d "$(abs_openssl_builddir)"; then \
	    echo "  SUBDIR   $(openssl_builddir)"; \
	    echo "  MAKE     clean";               \
	    $(MAKE) $(AM_MAKEFLAGS) -C $(abs_openssl_builddir) clean; \
	fi

# Remove the entirety of the OpenSSL build directory for the
# 'distclean' target.

distclean-local:
	@echo "  RM       $(openssl_builddir)"
	$(AM_V_at)rm -r -f $(openssl_builddir)

endif # WEAVE_WITH_OPENSSL_INTERNAL

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
