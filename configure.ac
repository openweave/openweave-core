#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
#    Copyright (c) 2018-2019 Google LLC
#    Copyright (c) 2014-2018 Nest Labs, Inc.
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
#      This file is the GNU autoconf input source file for the
#      Nest Labs Weave package.
#

#
# Declare autoconf version requirements
#
AC_PREREQ([2.68])

#
# Initialize autoconf for the package
#
AC_INIT([Weave],
        m4_esyscmd([third_party/nlbuild-autotools/repo/scripts/mkversion -b `cat .default-version` .]),
        [weave.support@nestlabs.com],
        [weave],
        [https://developer.nest.com/])

# Tell the rest of the build system the absolute path where the
# nlbuild-autotools repository is rooted at.

AC_SUBST(nlbuild_autotools_stem,[third_party/nlbuild-autotools/repo])
AC_SUBST(abs_top_nlbuild_autotools_dir,[\${abs_top_srcdir}/\${nlbuild_autotools_stem}])

#
# Weave interface current, revision, and age versions.
#
# NOTE: At present, the Nest Weave SDK makes NO ABI compatibility
#       commitments. Consequently, these simply serve as documentation
#       for how the interfaces have evolved.
#
# Maintainters: Please manage these fields as follows:
#
#   Interfaces removed:    CURRENT++, AGE = 0, REVISION = 0
#   Interfaces added:      CURRENT++, AGE++,   REVISION = 0
#   No interfaces changed:                     REVISION++
#
#
AC_SUBST(LIBWEAVE_VERSION_CURRENT,  [1])
AC_SUBST(LIBWEAVE_VERSION_AGE,      [0])
AC_SUBST(LIBWEAVE_VERSION_REVISION, [5])
AC_SUBST(LIBWEAVE_VERSION_INFO,     [${LIBWEAVE_VERSION_CURRENT}:${LIBWEAVE_VERSION_REVISION}:${LIBWEAVE_VERSION_AGE}])

#
# Check the sanity of the source directory by checking for the
# presence of a key watch file
#
AC_CONFIG_SRCDIR([src/lib/core/WeaveConfig.h])

#
# Tell autoconf where to find auxilliary build tools (e.g. config.guess,
# install-sh, missing, etc.)
#
AC_CONFIG_AUX_DIR([third_party/nlbuild-autotools/repo/third_party/autoconf])

#
# Tell autoconf where to find auxilliary M4 macros
# Note that nlbuild-autotools has macros in two subdirs, for local vs third-party scripts
#
AC_CONFIG_MACRO_DIRS([build/autoconf/m4 third_party/nlbuild-autotools/repo/third_party/autoconf/m4 third_party/nlbuild-autotools/repo/autoconf/m4])

#
# Tell autoconf what file the package is using to aggregate C preprocessor
# defines.
#
AC_CONFIG_HEADERS([src/include/BuildConfig.h])

#
# Guards against multiple inclusion of the BuildConfig.h
#

AH_TOP([#ifndef WEAVE_BUILD_CONFIG_H
#define WEAVE_BUILD_CONFIG_H])

AH_BOTTOM([#endif // WEAVE_BUILD_CONFIG_H])

#
# Figure out what the canonical build, host and target tuples are.
#
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

#
# Mac OS X / Darwin ends up putting some versioning cruft on the end of its
# tuple that we don't care about in this script. Create "clean" variables
# devoid of it.
#

NL_FILTERED_CANONICAL_BUILD
NL_FILTERED_CANONICAL_HOST
NL_FILTERED_CANONICAL_TARGET

#
# Configure automake with the desired options, indicating that this is not
# a native GNU package, that we want "silent" build rules, and that we want
# objects built in the same subdirectory as their source rather than collapsed
# together at the top-level directory.
#
# Disable silent build rules by either passing --disable-silent-rules to
# configure or passing V=1 to make
#
AM_INIT_AUTOMAKE([1.14 foreign silent-rules subdir-objects tar-pax])

#
# Silent build rules requires at least automake-1.11. Employ
# techniques for not breaking earlier versions of automake.
#
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AM_SILENT_RULES([yes])

#
# Enable maintainer mode to prevent the package from constantly trying
# to rebuild configure, Makefile.in, etc. Rebuilding such files rarely,
# if ever, needs to be done "in the field".
#
# Use the included 'bootstrap' script instead when necessary.
#
AM_MAINTAINER_MODE

#
# Check for the target style
#
WEAVE_TARGET_STYLE=
WEAVE_TARGET_STYLE_UNIX=0
WEAVE_TARGET_STYLE_EMBEDDED=0

AC_MSG_CHECKING([for target style])

# The user may have attempted to explicitly specify the target
# style. Sanity check it or default to 'auto'.

AC_ARG_WITH(target-style,
    [AS_HELP_STRING([--with-target-style=TARGET],
        [Specify the target style from one of: auto, embedded or unix @<:@default=auto@:>@.])],
    [
        case "${with_target_style}" in

        auto|embedded|unix)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_target_style} for --with-target-style])
            ;;

        esac
    ],
    [with_target_style=auto])

# At this point, the target style is one of the allowed
# values. If it's 'auto' we autodetect it.

if test "${with_target_style}" = "auto"; then
    case ${target_os} in

	*cygwin*|*darwin*|*linux*|*freebsd*|*netbsd*|*openbsd*)
            with_target_style=unix
	    ;;

	*freertos*)
            with_target_style=embedded
	    ;;

	*)
	    AC_MSG_RESULT([unknown])
	    AC_MSG_ERROR([Unsupported target OS ${target_os}])
	    ;;

    esac
fi

WEAVE_TARGET_STYLE=${with_target_style}

case ${with_target_style} in

    embedded)
        WEAVE_TARGET_STYLE_EMBEDDED=1
        ;;

    unix)
        WEAVE_TARGET_STYLE_UNIX=1
        ;;

esac

AC_MSG_RESULT(${WEAVE_TARGET_STYLE})

AC_SUBST(WEAVE_TARGET_STYLE)
AC_DEFINE_UNQUOTED([WEAVE_TARGET_STYLE],[${WEAVE_TARGET_STYLE}],[Weave target style])

AC_SUBST(WEAVE_TARGET_STYLE_EMBEDDED)
AM_CONDITIONAL([WEAVE_TARGET_STYLE_EMBEDDED], [test "${WEAVE_TARGET_STYLE}" = "embedded"])
AC_DEFINE_UNQUOTED([WEAVE_TARGET_STYLE_EMBEDDED],[${WEAVE_TARGET_STYLE_EMBEDDED}],[Define to 1 if you want to use Weave with an embedded (i.e. non-Unix-based) system])

AC_SUBST(WEAVE_TARGET_STYLE_UNIX)
AM_CONDITIONAL([WEAVE_TARGET_STYLE_UNIX], [test "${WEAVE_TARGET_STYLE}" = "unix"])
AC_DEFINE_UNQUOTED([WEAVE_TARGET_STYLE_UNIX],[${WEAVE_TARGET_STYLE_UNIX}],[Define to 1 if you want to use Weave with a Unix-based system])

#
# Checks for build host programs
#

# If we are cross-compiling and we are on an embedded target that
# doesn't support independent, standalone executables, then all
# compiler tests that attempt to create an executable will fail. In
# such circumstances, set AC_NO_EXECUTABLES (see http://sourceware.org/
# ml/newlib/2006/msg00353.html).

AC_MSG_CHECKING([whether to disable executable checking])
if test "$cross_compiling" = yes -a "${WEAVE_TARGET_STYLE}" = "embedded"; then
#    AC_NO_EXECUTABLES
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi

# Passing -Werror to GCC-based or -compatible compilers breaks some
# autoconf tests (see
# http://lists.gnu.org/archive/html/autoconf-patches/2008-09/msg00014.html).
#
# If -Werror has been passed transform it into -Wno-error. We'll
# transform it back later with NL_RESTORE_WERROR.

NL_SAVE_WERROR

# Check for compilers.
#
# These should be checked BEFORE we check for and, implicitly,
# initialize libtool such that libtool knows what languages it has to
# work with.

AC_PROG_CPP
AC_PROG_CPP_WERROR

AC_PROG_CC
AC_PROG_CC_C_O

AC_PROG_CXXCPP

AC_PROG_CXX
AC_PROG_CXX_C_O

AC_PROG_OBJC
AC_PROG_OBJCXX

# Check for other compiler toolchain tools.

AC_CHECK_TOOL(AR, ar)
AC_CHECK_TOOL(RANLIB, ranlib)
AC_CHECK_TOOL(OBJCOPY, objcopy)
AC_CHECK_TOOL(STRIP, strip)

# Check for other host tools.

AC_PROG_INSTALL
AC_PROG_LN_S

AC_PATH_PROG(CMP, cmp)
AC_PATH_PROG(PERL, perl)

#
# Checks for specific compiler characteristics
#

#
# Common compiler flags we would like to have.
#
#   -Wall -Wshadow -Wframe-larger-than=9472 -Wtype-limits   CC, CXX, OBJC, OBJCXX [unix, embedded]
#   -fPIC                                                   CC, CXX, OBJC, OBJCXX [unix]
#   -fno-stack-protector                                    CC, CXX, OBJC, OBJCXX [unix]
#   -fno-exceptions                                             CXX,       OBJCXX [unix]
#   -fobjc-call-cxx-cdtors                                           OBJC, OBJCXX [cocoa]
#   -fobjc-arc                                                       OBJC, OBJCXX [cocoa]
#
#

PROSPECTIVE_CFLAGS="-Wall -Wshadow -Wframe-larger-than=9472 -Wtype-limits"
PROSPECTIVE_CXXFLAGS=""
PROSPECTIVE_OBJCFLAGS="-fobjc-call-cxx-cdtors -fobjc-arc"
PROSPECTIVE_OBJCXXFLAGS=""

# Suppress clang-specific warnings about -Wnonportable-include-path.
#
# -Wnonportable-include-path warns about include paths that can be
# problematic. This creates false positives for Weave when compiling
# the Weave Addressing and Routing Module (WARM) in which source
# directories (e.g. src/warm/...) and public include directories
# (e.g. src/include/Warm/...) have identical, and consequently
# ambiguous, naming on case-insensitive file systems.

case "`basename ${CC}`" in

    *clang)
        AX_CHECK_COMPILER_OPTIONS([C],   -Wno-nonportable-include-path)
        ;;

esac

case "`basename ${CXX}`" in

    *clang++)
        AX_CHECK_COMPILER_OPTIONS([C++], -Wno-nonportable-include-path)
        ;;

esac

# For the flags that are desired for the "unix" target style, add
# those to the set to be checked. For the "embedded" target style, the
# Weave system integrator knows best as to what compiler flags should
# be added and will have passed them in.

if test "${WEAVE_TARGET_STYLE}" = "unix"; then
    PROSPECTIVE_CFLAGS="${PROSPECTIVE_CFLAGS} -fPIC -fno-stack-protector"
    PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} -fno-exceptions"
fi

# Weave intentionally uses derived classes that do not use an explicit
# destructor and a virtual one at that. Consequently, do not emit
# compiler warnings or errors about it when deletion is performed on
# such objects.

PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} -Wno-delete-non-virtual-dtor"

# Check whether the C++ compiler supports C++11 with GNU
# extensions. If it is supported, enable it.  Otherwise, proceed
# without error or warning.  While the Weave code itself does not rely
# on the GNU extensions, Android build targets crash when the standard
# is set to strict C++11

AX_CXX_COMPILE_STDCXX_11(noext, optional)

AM_CONDITIONAL([HAVE_CXX11], [test "x${HAVE_CXX11}" == "x1"])

if test "x${HAVE_CXX11}" == "x1"; then
PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} -std=gnu++11"
fi

AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS})

# Check for and initialize libtool

LT_INIT

#
# Android support
#
with_android=no

AC_MSG_CHECKING([for an Android target])

        case "${target}" in

        *android*)
            with_android=yes
            ;;

        *)
            AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[
            #if !defined(__ANDROID__) || !__ANDROID__
            #error "We are not compiling with Android."
            #endif
            ]])],
            [with_android=yes],
            [with_android=no])
            ;;

        esac

AC_MSG_RESULT(${with_android})

AM_CONDITIONAL([WEAVE_WITH_ANDROID], [test "${with_android}" = "yes"])

AC_SUBST(CROSS_SYSROOT, [$lt_sysroot])
#
# Weave package-specific build customization options
#

#
# Check for logging style
#

WEAVE_LOGGING_STYLE=
WEAVE_LOGGING_STYLE_ANDROID=0
WEAVE_LOGGING_STYLE_EXTERNAL=0
WEAVE_LOGGING_STYLE_STDIO=0
WEAVE_LOGGING_STYLE_STDIO_WEAK=0

AC_MSG_CHECKING([for logging style])

AC_ARG_WITH(logging-style,
    [AS_HELP_STRING([--with-logging-style=STYLE],
        [Specify the logging style from one of: auto, android, stdio, stdio-weak, or external @<:@default=auto@:>@.])],
    [
        case "${with_logging_style}" in

        android|auto|external|stdio|stdio-weak)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_logging_style} for --with-logging-style])
            ;;

        esac
    ],
    [with_logging_style=auto])

# At this point, the logging style is one of the allowed values. If
# it's 'auto' we autodetect it. Otherwise, sanity check the specified
# value.

if test "${with_logging_style}" = "auto"; then

    if test "${with_android}" = "yes"; then
        with_logging_style=android

    elif test "${with_target_style}" = "unix"; then
        with_logging_style=stdio

    else
        with_logging_style=external

    fi

else
    if test "${with_android}" != "yes"; then
        if test "${with_logging_style}" = "android"; then
            AC_MSG_ERROR([Android-style logging may only be specified when building for Android.])
        fi
    fi
fi

WEAVE_LOGGING_STYLE=${with_logging_style}

case ${with_logging_style} in

    android)
        WEAVE_LOGGING_STYLE_ANDROID=1
        ;;

    external)
        WEAVE_LOGGING_STYLE_EXTERNAL=1
        ;;

    stdio)
        WEAVE_LOGGING_STYLE_STDIO=1
        ;;

    stdio-weak)
        WEAVE_LOGGING_STYLE_STDIO_WEAK=1
        ;;

esac

AC_MSG_RESULT(${WEAVE_LOGGING_STYLE})

AC_SUBST(WEAVE_LOGGING_STYLE)
AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE],[${WEAVE_LOGGING_STYLE}],[Weave logging style])

AC_SUBST(WEAVE_LOGGING_STYLE_ANDROID)
AM_CONDITIONAL([WEAVE_LOGGING_STYLE_ANDROID], [test "${WEAVE_LOGGING_STYLE}" = "android"])
AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE_ANDROID],[${WEAVE_LOGGING_STYLE_ANDROID}],[Define to 1 if you want to use Weave with the Android logging system])

AC_SUBST(WEAVE_LOGGING_STYLE_EXTERNAL)
AM_CONDITIONAL([WEAVE_LOGGING_STYLE_EXTERNAL], [test "${WEAVE_LOGGING_STYLE}" = "external"])
AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE_EXTERNAL],[${WEAVE_LOGGING_STYLE_EXTERNAL}],[Define to 1 if you want to use Weave with an external (i.e. platform- and integrator-defined) logging system])

AC_SUBST(WEAVE_LOGGING_STYLE_STDIO)
AM_CONDITIONAL([WEAVE_LOGGING_STYLE_STDIO], [test "${WEAVE_LOGGING_STYLE}" = "stdio"])
AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE_STDIO],[${WEAVE_LOGGING_STYLE_STDIO}],[Define to 1 if you want to use Weave with a C Standard I/O Library-based logging system])

AC_SUBST(WEAVE_LOGGING_STYLE_STDIO_WEAK)
AM_CONDITIONAL([WEAVE_LOGGING_STYLE_STDIO_WEAK], [test "${WEAVE_LOGGING_STYLE}" = "stdio-weak"])
AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE_STDIO_WEAK],[${WEAVE_LOGGING_STYLE_STDIO_WEAK}],[Define to 1 if you want to use Weave with a C Standard I/O Library-based logging system,but override, at link-time an external (i.e. platform- and integrator-defined) logging system])

#
# If C Standard I/O logging is used, prepending timestamps before log
# lines may be enabled to support test automation and other debugging
# activities.
#

WEAVE_LOGGING_STYLE_STDIO_WITH_TIMESTAMPS=0

AC_MSG_CHECKING([whether to enable stdio logging timestamps])
AC_ARG_ENABLE(stdio-logging-timestamps,
    [AS_HELP_STRING([--enable-stdio-logging-timestamps],[Enable timestamps in stdio- or stdio-weak- style log output @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            if test "${enableval}" = "yes"; then
                if test "${WEAVE_LOGGING_STYLE}" != "stdio"; then
                    AC_MSG_ERROR([The Weave logging style must be 'stdio' to use logging timestamps. Use --with-logging-style=stdio Use --with-logging-style=stdio-weak to enable this logging style.])
                else
                    WEAVE_LOGGING_STYLE_STDIO_WITH_TIMESTAMPS=1
                fi
            fi

            enable_stdio_logging_timestamps=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-stdio-logging-timestamps])
            ;;

        esac
    ],
    [enable_stdio_logging_timestamps=no])
AC_MSG_RESULT(${enable_stdio_logging_timestamps})

AC_DEFINE_UNQUOTED([WEAVE_LOGGING_STYLE_STDIO_WITH_TIMESTAMPS],[${WEAVE_LOGGING_STYLE_STDIO_WITH_TIMESTAMPS}],[Define to 1 if you want to use Weave with a C Standard I/O Library-based logging system with timestamped log entries])

#
# Cocoa support
#
AC_MSG_CHECKING([whether to build Cocoa support])
AC_ARG_ENABLE(cocoa,
    [AS_HELP_STRING([--enable-cocoa],[Enable Cocoa support, required for the iOS device manager @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_cocoa=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-cocoa])
            ;;

        esac
    ],
    [enable_cocoa=no])
AC_MSG_RESULT(${enable_cocoa})

AM_CONDITIONAL([WEAVE_WITH_COCOA], [test "${enable_cocoa}" = "yes"])

if test "${enable_cocoa}" = "yes" ; then

    # Check if the Objective C and Objective C++ compilers support
    # some overall options we want for those sources.

    AX_CHECK_COMPILER_OPTIONS([Objective C],   ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_OBJCFLAGS})
    AX_CHECK_COMPILER_OPTIONS([Objective C++], ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS} ${PROSPECTIVE_OBJCFLAGS} ${PROSPECTIVE_OBJCXXFLAGS})

    AC_LANG_PUSH([Objective C++])

        # Check for required Cocoa headers.

        AC_CHECK_HEADERS([Foundation/Foundation.h],
        [],
        [
            AC_MSG_ERROR(The Cocoa header "$ac_header" is required but cannot be found.)
        ])

    AC_LANG_POP([Objective C++])
fi

#
# Java support
#

# Influential external variables for Java support

AC_ARG_VAR(JAVA_HOME,    [Java SDK root directory])
AC_ARG_VAR(JNI_CPPFLAGS, [Java Native Interface (JNI) C preprocessor flags])
AC_ARG_VAR(JAVAC,        [Java class compiler command])
AC_ARG_VAR(JAR,          [Java archiver command])

AC_MSG_CHECKING([whether to build Java support])
AC_ARG_ENABLE(java,
    [AS_HELP_STRING([--disable-java],[Disable Java support, otherwise required for the Android device manager and Nest service support @<:@default=auto@:>@.])],
    [
        case "${enableval}" in

        auto|no|yes)
            enable_java=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-java])
            ;;

        esac
    ],
    [enable_java=auto])

if test "${enable_java}" = "auto"; then
    if test "x${JAVA_HOME}" != "x"; then
        enable_java=yes
    else
        enable_java=no
    fi
fi

AC_MSG_RESULT(${enable_java})

# If Java support has been requested, then check for the relevant Java
# headers and tools.

if test "${enable_java}" = "yes" ; then
    # Check for Java

    # If a specific set of JNI_INCLUDE_DIRS has *not* been given, attempt
    # to locate the JNI include dirs from JAVA_HOME or the location of
    # javac.
    if test "x${JNI_INCLUDE_DIRS}" = "x" ; then
        AC_JNI_INCLUDE_DIR
    fi
    
    if test "x${JNI_INCLUDE_DIRS}" != "x" ; then
        for dir in ${JNI_INCLUDE_DIRS} ; do
            JNI_CPPFLAGS="${JNI_CPPFLAGS} -I${dir}"
        done

        AC_SUBST(JNI_CPPFLAGS)
    fi

    AC_PATH_PROG([JAVAC], [javac], [], [${JAVA_HOME:+${JAVA_HOME}/bin}${PATH_SEPARATOR}${PATH}])
    AC_PATH_PROG([JAR],   [jar],   [], [${JAVA_HOME:+${JAVA_HOME}/bin}${PATH_SEPARATOR}${PATH}])
fi

AM_CONDITIONAL([WEAVE_WITH_JAVA], [test "${enable_java}" = "yes"])

#
# Python support
#
AC_MSG_CHECKING([whether to build Python support])
AC_ARG_ENABLE(python,
    [AS_HELP_STRING([--disable-python],[Disable Python support @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            enable_python=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-python])
            ;;

        esac
    ],
    [enable_python=yes])
AC_MSG_RESULT(${enable_python})

AM_CONDITIONAL([WEAVE_WITH_PYTHON], [test "${enable_python}" = "yes"])

if test "${enable_python}" = "yes" ; then

    # In a normal "build-and-install on 'this' machine" context,
    # placing Python packages in the normal pythondir and pyexecdir
    # (typically
    # lib/pythonPYTHON_VERSION/{dist,site}-packages/PACKAGE) makes
    # sense.
    #
    # However, in our case, we're building and 'installing' for
    # standalone distrubution to any number of machines for which we
    # don't know the Python version. In those cases, override
    # pythondir and pyexec dir.

    case "${prefix}" in

        /opt|/usr|/usr/local|NONE)
            ;;

        *)
            am_cv_python_pythondir="\${prefix}/lib/python"
            am_cv_python_pyexecdir="\${exec_prefix}/lib/python"
            ;;

    esac

    # Check for Python

    AM_PATH_PYTHON
fi

#
# Debug and profile instances
#
AC_MSG_NOTICE([checking whether to build debug and profile instances])

# Debug

NL_ENABLE_DEBUG([no])

AM_CONDITIONAL([WEAVE_BUILD_DEBUG], [test "${nl_cv_build_debug}" = "yes"])

# Profile

AC_MSG_CHECKING([whether to build profile instances of programs and libraries])
AC_ARG_ENABLE(profile,
    [AS_HELP_STRING([--disable-profile],[Disable the generation of profile instances @<:@default=yes@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            build_profile=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-profile])
            ;;

        esac
    ],
    [build_profile=no])
AC_MSG_RESULT(${build_profile})
AM_CONDITIONAL([WEAVE_BUILD_PROFILE], [test "${build_profile}" = "yes"])

#
# Code coverage and compiler optimization
#

# Coverage

NL_ENABLE_COVERAGE([no])

AM_CONDITIONAL([WEAVE_BUILD_COVERAGE], [test "${nl_cv_build_coverage}" = "yes"])

NL_ENABLE_COVERAGE_REPORTS([auto])

AM_CONDITIONAL([WEAVE_BUILD_COVERAGE_REPORTS], [test "${nl_cv_build_coverage_reports}" = "yes"])

# Optimization

NL_ENABLE_OPTIMIZATION([yes])

AM_CONDITIONAL([WEAVE_BUILD_OPTIMIZED], [test "${nl_cv_build_optimized}" = "yes"])

#
# Treat Compiler Warnings as Errors
#
NL_ENABLE_WERROR([no])

#
# Tests and tools
#
AC_MSG_NOTICE([checking whether to build tools and tests])

# Tests

NL_ENABLE_TESTS([yes])

AM_CONDITIONAL([WEAVE_BUILD_TESTS], [test "${nl_cv_build_tests}" = "yes"])

NL_ENABLE_LONG_TESTS([no])

AM_CONDITIONAL([WEAVE_RUN_LONG_TESTS], [test "${nl_cv_build_long_tests}" = "yes"])

# Tools

AC_MSG_CHECKING([whether to build tools])
AC_ARG_ENABLE(tools,
    [AS_HELP_STRING([--disable-tools],[Disable building of tools @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            build_tools=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-tools])
            ;;

        esac
    ],
    [build_tools=yes])
AC_MSG_RESULT(${build_tools})
AM_CONDITIONAL([WEAVE_BUILD_TOOLS], [test "${build_tools}" = "yes"])

# Happy

AC_MSG_CHECKING([checking whether to run functional tests with Happy])
AC_ARG_WITH(happy,
    [AS_HELP_STRING([--with-happy=HAPPY],
        [Run Weave functional tests using the Nest Labs network functional test framework (Happy): yes, no, auto, or a path to the Nest Labs network functional test framework (Happy) [default=auto].])],
    [
        case "${with_happy}" in

        no|yes)
            ;;

        *)
            configure_happy_path=${with_happy}
            happy_path="${with_happy}/lib/python${am_cv_python_version}/site-packages/"
            with_happy=yes
            ;;
        esac
    ],
    [with_happy=auto])
if test "${nl_cv_build_tests}" != "yes"; then
    with_happy=no
    happy_path="-"
else
    if test -n ${happy_path}; then
        grep "Nest Labs" ${configure_happy_path}/setup.py &> /dev/null
        if test $? = 0; then
            AC_MSG_ERROR(["You have specified a valid path, ${configure_happy_path} to a Nest Labs network functional test framework (Happy) source directory; however, it does not appear to be built and installed. Please build and install Happy from the specified path, ${configure_happy_path}, and then try again."])
        fi
        if ! test -d ${happy_path}; then
            AC_MSG_ERROR(["No Nest Labs network functional test framework (Happy) installation could be found at ${configure_happy_path}. Please provide a path to a valid Happy installation."])
        fi
    fi
fi
if test "${with_happy}" != "no"; then
    PYTHONPATH=${happy_path}:${PYTHONPATH} python -c 'import happy' 2> /dev/null
    if test $? != 0; then
        if test "${with_happy}" = "yes"; then
            if test -z ${happy_path}; then
                AC_MSG_ERROR(["The Nest Labs network functional test framework (Happy) was configured with the defaults; however, Happy could not be found in the Python default package locations. Please either install Happy in a default location or invoke 'configure' with an explicit path using '--with-happy=<path to happy>'."])
            else
                AC_MSG_ERROR(["The Nest Labs network functional test framework (Happy) was configured with the path ${happy_path}; however, Happy could neither be found in the Python default package locations and nor at ${happy_path}."])
            fi
        else
            # with_happy = auto
            with_happy=no
            happy_path="-"
        fi
    else
        with_happy=yes
    fi
fi
if test "${with_happy}" = "yes"; then
    HAPPY_MINIMUM_VERSION="1.1.36"
    happy_version=`PYTHONPATH=${happy_path}:${PYTHONPATH} python -c 'import happy; print happy.__version__ if "__version__" in dir(happy) else "0.0.0"'`
    AX_COMPARE_VERSION([${HAPPY_MINIMUM_VERSION}],[gt],[${happy_version}],[AC_MSG_ERROR(["The Nest Labs network functional test framework (Happy) is installed with version ${happy_version}; however, Weave test apps require at least version ${HAPPY_MINIMUM_VERSION}."])],[])
fi
AC_MSG_RESULT(${with_happy})
AM_CONDITIONAL([WEAVE_RUN_HAPPY], [test "${with_happy}" = "yes"])
AC_SUBST(HAPPY_PATH,[${happy_path}])

AC_MSG_CHECKING([checking whether to run functional tests with Happy against cloud services])
AC_ARG_WITH(happy-service,
    [AS_HELP_STRING([--with-happy-service=SERVICE],
        [Run Weave Happy functional tests connecting to cloud services: no or cloud services address [default=no].])],
    [
        case "${with_happy_service}" in

        no)
            ;;

        yes)
            AC_MSG_ERROR([Invalid value ${with_happy_service} for --with-happy-service])
            ;;

        *)
            use_happy_service=yes
            ;;
        esac
    ],
    [use_happy_service=no])

if test "${with_happy}" != "yes"; then
    if test "${use_happy_service}" = "yes"; then
        AC_MSG_ERROR(["Nest Labs network functional test framework must be enabled to run tests against cloud services."])
    fi
fi
AC_MSG_RESULT(${use_happy_service})
AM_CONDITIONAL([WEAVE_RUN_HAPPY_SERVICE], [test "${use_happy_service}" = "yes"])
AC_SUBST(HAPPY_SERVICE,[${with_happy_service}])

AC_MSG_CHECKING([checking whether to use specific DNS servers while running Happy tests])
AC_ARG_WITH(happy-dns,
    [AS_HELP_STRING([--with-happy-dns=DNS],
        [Run Weave Happy functional tests using specif DNS servers [default=none].])],
    [
        case "${with_happy_dns}" in

        no)
            ;;

        yes)
            AC_MSG_ERROR([Invalid value ${with_happy_dns} for --with-happy-dns])
            ;;

        *)
            use_happy_dns=yes
            ;;
        esac
    ],
    [use_happy_dns=no])

AC_MSG_RESULT(${use_happy_dns})
AM_CONDITIONAL([WEAVE_RUN_HAPPY_DNS], [test "${use_happy_dns}" = "yes"])
AC_SUBST(HAPPY_DNS,[${with_happy_dns}])

#
# Happy Service Test Suite
#
CONFIG_TARGET_SUITES=all

AC_MSG_CHECKING([checking which test suite is running while running Happy service tests])
AC_ARG_WITH(happy-test-suite,
    [AS_HELP_STRING([--with-happy-test-suite=SUITE],
        [Specify the target test suite from one or more of, separated by commas: minimal, all, wdm, echo, tunnel, time, pairing, bdx, key_export, core, inet, secmgr, servicedir, perf@<:@default=all@:>@.])],
    [
        CONFIG_TARGET_SUITES=`echo ${with_happy_test_suite} | sed -e 's/,/ /g'`
        for target_suite in ${CONFIG_TARGET_SUITES}; do
            case "${target_suite}" in

            perf)
                if test "${use_happy_service}" = "no"; then
                    AC_MSG_ERROR([Invalid value ${with_happy_test_suite} for --with-happy-test-suite; suite is available only with service tests])
                fi
                ;;

            bdx | key_export | core | secmgr)
                if test "${use_happy_service}" = "yes"; then
                    AC_MSG_ERROR([Invalid value ${with_happy_test_suite} for --with-happy-test-suite; suite is available only with standalone tests])
                fi
                ;;

            yes | no | minimal | all | inet | wdm | echo | tunnel | time | pairing | servicedir)
                ;;

            *)
                AC_MSG_ERROR([Invalid value ${with_happy_test_suite} for --with-happy-test-suite])
                ;;

            esac
        done
    ],
    [with_happy_test_suite=${CONFIG_TARGET_SUITES}])

AC_MSG_RESULT(${with_happy_test_suite})

for target_suite in ${CONFIG_TARGET_SUITES}; do
    case ${target_suite} in

    minimal)
        HAPPY_TEST_USE_ECHO=1
        HAPPY_TEST_USE_TUNNEL=1
        ;;

    all)
        HAPPY_TEST_USE_ECHO=1
        HAPPY_TEST_USE_TUNNEL=1
        HAPPY_TEST_USE_TIME=1
        HAPPY_TEST_USE_PAIRING=1
        HAPPY_TEST_USE_WDM=1
        HAPPY_TEST_USE_KEY_EXPORT=1
        HAPPY_TEST_USE_BDX=1
        HAPPY_TEST_USE_CORE=1
        HAPPY_TEST_USE_INET=1
        HAPPY_TEST_USE_SECMGR=1
        HAPPY_TEST_USE_SERVICEDIR=1
        ;;

    wdm)
        HAPPY_TEST_USE_WDM=1
        ;;

    echo)
        HAPPY_TEST_USE_ECHO=1
        ;;

    tunnel)
        HAPPY_TEST_USE_TUNNEL=1
        ;;

    time)
        HAPPY_TEST_USE_TIME=1
        ;;

    pairing)
        HAPPY_TEST_USE_PAIRING=1
        ;;

    key_export)
        HAPPY_TEST_USE_KEY_EXPORT=1
        ;;

    perf)
        HAPPY_TEST_USE_PERF=1
        ;;

    bdx)
        HAPPY_TEST_USE_BDX=1
        ;;

    core)
        HAPPY_TEST_USE_CORE=1
        ;;

    inet)
        HAPPY_TEST_USE_INET=1
        ;;

    secmgr)
        HAPPY_TEST_USE_SECMGR=1
        ;;

    servicedir)
        HAPPY_TEST_USE_SERVICEDIR=1
        ;;

    esac
done

AM_CONDITIONAL([WEAVE_RUN_HAPPY_WDM], [test "${HAPPY_TEST_USE_WDM}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_ECHO], [test "${HAPPY_TEST_USE_ECHO}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_PAIRING], [test "${HAPPY_TEST_USE_PAIRING}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_PERF], [test "${HAPPY_TEST_USE_PERF}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_TUNNEL], [test "${HAPPY_TEST_USE_TUNNEL}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_TIME], [test "${HAPPY_TEST_USE_TIME}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_KEY_EXPORT], [test "${HAPPY_TEST_USE_KEY_EXPORT}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_CORE], [test "${HAPPY_TEST_USE_CORE}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_INET], [test "${HAPPY_TEST_USE_INET}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_BDX], [test "${HAPPY_TEST_USE_BDX}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_SECMGR], [test "${HAPPY_TEST_USE_SECMGR}" = 1])
AM_CONDITIONAL([WEAVE_RUN_HAPPY_SERVICEDIR], [test "${HAPPY_TEST_USE_SERVICEDIR}" = 1])

#
# Device Manager
#

AC_MSG_CHECKING([whether to build Device Manager])
AC_ARG_ENABLE(device_manager,
    [AS_HELP_STRING([--disable-device-manager],[Disable building of Device Manager @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            build_device_manager=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-device-manager])
            ;;

        esac
    ],
    [build_device_manager=yes])
AC_MSG_RESULT(${build_device_manager})
AM_CONDITIONAL([WEAVE_BUILD_DEVICE_MANAGER], [test "${build_device_manager}" = "yes"])

# Build device manager with dummy implementations of persisted storate Read/Write APIs.
AM_CONDITIONAL([WEAVE_BUILD_DEVICE_MANAGER_WITH_DUMMY_PERSISTED_STORE], [test "${enable_java}" = "yes" || test "${enable_python}" = "yes" || test "${enable_cocoa}" = "yes"])

#
# WoBle Control Path and Throughput Test  (Woble Test)
#

AC_MSG_CHECKING([whether to build WoBle Test])
AC_ARG_ENABLE(woble-test,
    [AS_HELP_STRING([--enable-woble-test],[Enable WoBle Test @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            enable_woble_test=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-woble-test])
            ;;

        esac
    ],
    [enable_woble_test=no])
AC_MSG_RESULT(${enable_woble_test})
AM_CONDITIONAL([WEAVE_ENABLE_WOBLE_TEST], [test "${enable_woble_test}" = "yes"])
if test "${enable_woble_test}" = "yes"; then
    WEAVE_ENABLE_WOBLE_TEST=1
else
    WEAVE_ENABLE_WOBLE_TEST=0
fi
AC_DEFINE_UNQUOTED([WEAVE_ENABLE_WOBLE_TEST],[${WEAVE_ENABLE_WOBLE_TEST}],[Define to 1 if you want to enable WoBle Control Path and Throughput Test.])

#
# WARM - Weave Address and Routing Module
#

AC_MSG_CHECKING([whether to build Warm])
AC_ARG_ENABLE(warm,
    [AS_HELP_STRING([--disable-warm],[Disable building of the Weave Addressing and Routing Module (WARM) @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            build_warm=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-warm])
            ;;

        esac
    ],
    [build_warm=yes])
AC_MSG_RESULT(${build_warm})
AM_CONDITIONAL([WEAVE_BUILD_WARM], [test "${build_warm}" = "yes"])

#
# Weave Tunnel failover support
#

AC_MSG_CHECKING([whether to build support for Weave Tunnel failover])
AC_ARG_ENABLE(tunnel-failover,
    [AS_HELP_STRING([--enable-tunnel-failover],[Enable building of Weave tunnel failover @<:@default=yes@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            build_tunnel_failover=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-tunnel-failover])
            ;;

        esac
    ],
    [build_tunnel_failover=no])
AC_MSG_RESULT(${build_tunnel_failover})

if test "${build_tunnel_failover}" = yes; then
    AC_DEFINE(WEAVE_CONFIG_TUNNEL_FAILOVER_SUPPORTED, 1, [Define to 1 for enabling Weave Tunnel failover])
fi

#
# Legacy WDM profile support
#

AC_MSG_CHECKING([whether to build the legacy WDM profile])
AC_ARG_ENABLE(legacy-wdm,
    [AS_HELP_STRING([--disable-legacy-wdm],[Disable building of the legacy WDM profile @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            build_legacy_wdm=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-legacy-wdm])
            ;;

        esac
    ],
    [build_legacy_wdm=yes])
AC_MSG_RESULT(${build_legacy_wdm})
if test "${build_legacy_wdm}" = yes; then
    LEGACY_WDM=1
else
    if test "${build_device_manager}" = "yes"; then
        AC_MSG_ERROR([The Legacy Weave Data Management profile must be enabled to build the Weave Device Manager])
    fi
    LEGACY_WDM=0
fi
AM_CONDITIONAL([WEAVE_BUILD_LEGACY_WDM], [test "${LEGACY_WDM}" = 1])
AC_DEFINE_UNQUOTED([WEAVE_CONFIG_LEGACY_WDM],[${LEGACY_WDM}],[Define to 1 to use the legacy WDM profile.])

#
# Documentation
#

# Determine whether or not documentation (via Doxygen) should be built
# or not, with 'auto' as the default and establish a default support
# value for GraphViz 'dot' support.

NL_ENABLE_DOCS([auto],[NO])

AM_CONDITIONAL(WEAVE_BUILD_DOCS, [test "${nl_cv_build_docs}" = "yes"])

#
# Network Technology Layer
#
AC_MSG_CHECKING([network layer])
AC_ARG_WITH(network-layer,
    [AS_HELP_STRING([--with-network-layer=LAYER],
        [Specify the target network layers from one of: ble, inet, or all @<:@default=all@:>@.])],
    [
        case "${with_network_layer}" in

        ble|inet|all)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_network_layer} for --with-network-layer])
            ;;

        esac
    ],
    [with_network_layer=all])
AC_MSG_RESULT(${with_network_layer})

case "${with_network_layer}" in

ble)
      CONFIG_NETWORK_LAYER_BLE=1
      CONFIG_NETWORK_LAYER_INET=0
      ;;

inet)
      CONFIG_NETWORK_LAYER_BLE=0
      CONFIG_NETWORK_LAYER_INET=1
      ;;

all)
      CONFIG_NETWORK_LAYER_BLE=1
      CONFIG_NETWORK_LAYER_INET=1
      ;;

esac

# NOTE WELL: The following logic is a transitional patch (BLE layer is currently dependent on INET layer)
if test ${CONFIG_NETWORK_LAYER_INET} = 0 && test ${CONFIG_NETWORK_LAYER_BLE} = 1; then
    with_network_layer=all
    CONFIG_NETWORK_LAYER_INET=1
    AC_MSG_NOTICE([NOTE WELL: the BLE layer currently requires the INET layer.])
fi

AC_SUBST(CONFIG_NETWORK_LAYER_BLE)
AM_CONDITIONAL([CONFIG_NETWORK_LAYER_BLE],    [test "${CONFIG_NETWORK_LAYER_BLE}" = 1])
AC_DEFINE_UNQUOTED([CONFIG_NETWORK_LAYER_BLE],[${CONFIG_NETWORK_LAYER_BLE}],[Define to 1 if you want to use Weave with the Bluetooth Low Energy network stack.])

AC_SUBST(CONFIG_NETWORK_LAYER_INET)
AM_CONDITIONAL([CONFIG_NETWORK_LAYER_INET],    [test "${CONFIG_NETWORK_LAYER_INET}" = 1])
AC_DEFINE_UNQUOTED([CONFIG_NETWORK_LAYER_INET],[${CONFIG_NETWORK_LAYER_INET}],[Define to 1 if you want to use Weave with the Internet Protocol network stack.])

#
# OpenWeave Device Layer
#
AC_MSG_CHECKING([device layer])
AC_ARG_WITH(device-layer,
    [AS_HELP_STRING([--with-device-layer=LAYER],
        [Specify the target environment for the Weave Device Layer.  Choose one of: esp32, or none @<:@default=none@:>@.])],
    [
        case "${with_device_layer}" in

        esp32|none)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_device_layer} for --with-device-layer])
            ;;

        esac
    ],
    [with_device_layer=none])
AC_MSG_RESULT(${with_device_layer})

case "${with_device_layer}" in
esp32)
      CONFIG_DEVICE_LAYER=1
      WEAVE_DEVICE_LAYER_TARGET=ESP32
      WEAVE_DEVICE_LAYER_TARGET_ESP32=1
      ;;

none)
      CONFIG_DEVICE_LAYER=0
      WEAVE_DEVICE_LAYER_TARGET=NONE
      WEAVE_DEVICE_LAYER_TARGET_ESP32=0
      ;;

esac

AC_SUBST(CONFIG_DEVICE_LAYER)
AM_CONDITIONAL([CONFIG_DEVICE_LAYER],    [test "${CONFIG_DEVICE_LAYER}" = 1])
AC_DEFINE_UNQUOTED([CONFIG_DEVICE_LAYER],[${CONFIG_DEVICE_LAYER}],[Define to 1 if you want to use the OpenWeave Device Layer.])

AC_SUBST(WEAVE_DEVICE_LAYER_TARGET)
AC_DEFINE_UNQUOTED([WEAVE_DEVICE_LAYER_TARGET],[${WEAVE_DEVICE_LAYER_TARGET}],[Target platform name for OpenWeave Device Layer.])

AC_SUBST(WEAVE_DEVICE_LAYER_TARGET_ESP32)
AM_CONDITIONAL([WEAVE_DEVICE_LAYER_TARGET_ESP32],    [test "${WEAVE_DEVICE_LAYER_TARGET_ESP32}" = 1])
AC_DEFINE_UNQUOTED([WEAVE_DEVICE_LAYER_TARGET_ESP32],[${WEAVE_DEVICE_LAYER_TARGET_ESP32}],[Define to 1 if you want to build the OpenWeave Device Layer for the Espressif ESP32.])

#
# WoBle over Bluez Peripheral support
#

# Only a concern when the BLE layer is enabled.

CONFIG_BLE_PLATFORM_BLUEZ=0
enable_woble_bluez="no"

NL_WITH_OPTIONAL_INTERNAL_PACKAGE(
  [BlueZ],
  [BLUEZ],
  [bluez],
  [-lgdbus-internal -lshared-glib],
  [
  # actions if not external
  # At this time, enable this only for linux, and only if the tests are enabled
      if test "${nl_cv_build_tests}" = "yes" && test "${CONFIG_NETWORK_LAYER_BLE}" = 1 ; then
      case "${target}" in

          *linux*)
              if test "x${HAVE_CXX11}" == "0"; then
                AC_MSG_ERROR([BlueZ support requires C++11 compiler])
              fi
              CONFIG_BLE_PLATFORM_BLUEZ=1
              enable_woble_bluez="yes"
              BLUEZ_CPPFLAGS="-I\${abs_top_srcdir}/third_party/bluez/repo/"
              BLUEZ_LDFLAGS="-L${ac_pwd}/third_party/bluez/repo/gdbus/ -L${ac_pwd}/third_party/bluez/repo/src/"
              BLUEZ_LIBS="-lgdbus-internal -lshared-glib"
              ;;

          *)
              CONFIG_BLE_PLATFORM_BLUEZ=0
              ;;

          esac
      fi
  ],
  [ # actions if not internal

    CONFIG_BLE_PLATFORM_BLUEZ=1
    enable_woble_bluez="yes"
  ]
)

# Depending on whether bluez has been configured for an internal
# location, its directory stem within this package needs to be set
# accordingly. In addition, if the location is internal, then we need
# to attempt to pull it down using the bootstrap makefile.

if test "${nl_with_bluez}" = "internal" && test "${enable_woble_bluez}" = "yes"; then
    maybe_bluez_dirstem="bluez/repo"
    bluez_dirstem="third_party/${maybe_bluez_dirstem}"

    AC_MSG_NOTICE([attempting to create internal ${bluez_dirstem}])

    ${MAKE-make} --no-print-directory -C ${srcdir} -f Makefile-bootstrap ${bluez_dirstem}

    if test $? -ne 0; then
        AC_MSG_ERROR([failed to create ${bluez_dirstem}. Please check your network connection or the correctness of 'repos.conf'])
    fi

    echo "  BOOTSTRAP    ${bluez_dirstem}"

    (cd ${srcdir}/${bluez_dirstem} && ./bootstrap)

else
    maybe_bluez_dirstem=""
fi

AC_SUBST(BLUEZ_SUBDIRS, [${maybe_bluez_dirstem}])
AM_CONDITIONAL([WEAVE_WITH_BLUEZ_INTERNAL], [test "${nl_with_bluez}" = "internal"])

AM_CONDITIONAL([CONFIG_BLE_PLATFORM_BLUEZ], [test "${enable_woble_bluez}" = "yes"])
AC_DEFINE_UNQUOTED([CONFIG_BLE_PLATFORM_BLUEZ],[${CONFIG_BLE_PLATFORM_BLUEZ}],[Define to 1 if you want to enable WoBle over bluez.])

if test "${enable_woble_bluez}" = "yes"; then
    PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.28, dummy=yes,
				AC_MSG_ERROR(GLib >= 2.28 is required))
    AC_SUBST(GLIB_CFLAGS)
    AC_SUBST(GLIB_LIBS)

    PROSPECTIVE_CFLAGS="${PROSPECTIVE_CFLAGS} ${GLIB_CFLAGS}"
    PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} ${GLIB_CFLAGS}"

    AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
    AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS})

    PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.6, dummy=yes,
				AC_MSG_ERROR(D-Bus >= 1.6 is required))
    AC_SUBST(DBUS_CFLAGS)
    AC_SUBST(DBUS_LIBS)

    PROSPECTIVE_CFLAGS="${PROSPECTIVE_CFLAGS} ${DBUS_CFLAGS}"
    PROSPECTIVE_CXXFLAGS="${PROSPECTIVE_CXXFLAGS} ${DBUS_CFLAGS}"

    AX_CHECK_COMPILER_OPTIONS([C],   ${PROSPECTIVE_CFLAGS})
    AX_CHECK_COMPILER_OPTIONS([C++], ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS})
fi

#
# Weave target network stack(s)
#
CONFIG_TARGET_NETWORKS=sockets

AC_MSG_CHECKING([target network])
AC_ARG_WITH(target-network,
    [AS_HELP_STRING([--with-target-network=NETWORK],
        [Specify the target network stack from one or more of, separated by commas: sockets or lwip @<:@default=sockets@:>@.])],
    [
        CONFIG_TARGET_NETWORKS=`echo ${with_target_network} | sed -e 's/,/ /g'`

        for target_network in ${CONFIG_TARGET_NETWORKS}; do
            case "${target_network}" in

            lwip|sockets)
                ;;

            *)
                AC_MSG_ERROR([Invalid value ${with_target_network} for --with-target-network])
                ;;

            esac
        done
    ],
    [with_target_network=${CONFIG_TARGET_NETWORKS}])
AC_MSG_RESULT(${with_target_network})
AC_SUBST(CONFIG_TARGET_NETWORKS)
AC_DEFINE_UNQUOTED([CONFIG_TARGET_NETWORKS], "${CONFIG_TARGET_NETWORKS}", [Weave target network stack(s)])

WEAVE_SYSTEM_CONFIG_USE_LWIP=0
WEAVE_SYSTEM_CONFIG_USE_SOCKETS=0

for target_network in ${CONFIG_TARGET_NETWORKS}; do
    case ${target_network} in

    lwip)
        WEAVE_SYSTEM_CONFIG_USE_LWIP=1
        ;;

    sockets)
        WEAVE_SYSTEM_CONFIG_USE_SOCKETS=1
        ;;

    esac
done

AC_SUBST(WEAVE_SYSTEM_CONFIG_USE_LWIP)
AM_CONDITIONAL([WEAVE_SYSTEM_CONFIG_USE_LWIP], [test "${WEAVE_SYSTEM_CONFIG_USE_LWIP}" = 1])
AC_DEFINE_UNQUOTED([WEAVE_SYSTEM_CONFIG_USE_LWIP], [${WEAVE_SYSTEM_CONFIG_USE_LWIP}],
    [Define to 1 if you want to use LwIP with Weave System Layer.])

AC_SUBST(WEAVE_SYSTEM_CONFIG_USE_SOCKETS)
AM_CONDITIONAL([WEAVE_SYSTEM_CONFIG_USE_SOCKETS], [test "${WEAVE_SYSTEM_CONFIG_USE_SOCKETS}" = 1])
AC_DEFINE_UNQUOTED([WEAVE_SYSTEM_CONFIG_USE_SOCKETS], [${WEAVE_SYSTEM_CONFIG_USE_SOCKETS}],
    [Define to 1 if you want to use BSD sockets with Weave System Layer.])

#
# Internet Protocol Network Endpoints
#
if test ${CONFIG_NETWORK_LAYER_INET} = 1; then
    AC_MSG_CHECKING([IPv4 enabled])
    AC_ARG_ENABLE(ipv4,
        [AS_HELP_STRING([--disable-ipv4],[Disable the inclusion of IPv4 networking @<:@default=yes@:>@.])],
        [
            case "${enableval}" in 

            no|yes)
                enable_ivp4=${enableval}
                ;;

            *)
                AC_MSG_ERROR([Invalid value ${enableval} for --enable-ivp4])
                ;;

            esac
        ],
        [enable_ivp4=yes])
    AC_MSG_RESULT(${enable_ivp4})

    INET_WANT_IPV4=1

    if test "${enable_ipv4}" = no; then
        INET_WANT_IPV4=0
    fi

    AM_CONDITIONAL([INET_WANT_IPV4], [test "${INET_WANT_IPV4}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_IPV4],[${INET_WANT_IPV4}], [Define to 1 if you want to use Weave with IPv4.])

    if test "${INET_WANT_IPV4}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_IPV4, 1, [Define to 1 for enabling IPv4])
    fi

    AC_MSG_CHECKING([inet network endpoint])
    AC_ARG_WITH(inet-endpoint,
        [AS_HELP_STRING([--with-inet-endpoint=ENDPOINT],
            [Specify the inet network endpoint from one or more of: raw, tcp, tun, udp @<:@default=all@:>@.])],
        [
            INET_ENDPOINTS=`echo ${with_inet_endpoint} | sed -e 's/,/ /g'`

            for inet_endpoint in ${INET_ENDPOINTS}; do

                 case "${inet_endpoint}" in

                 dns|raw|tcp|tun|udp|all)
                     ;;

                 *)
                     AC_MSG_ERROR([Invalid value ${with_inet_endpoint} for --with-inet-endpoint])
                     ;;

                 esac

            done
        ],
        [INET_ENDPOINTS=all])
    AC_MSG_RESULT(${with_inet_endpoint})

    AC_SUBST(INET_ENDPOINTS)
    AC_DEFINE_UNQUOTED([INET_ENDPOINTS], "${INET_ENDPOINTS}", [Weave target inet network endpoint(s)])

    INET_WANT_ENDPOINT_DNS=0
    INET_WANT_ENDPOINT_RAW=0
    INET_WANT_ENDPOINT_TCP=0
    INET_WANT_ENDPOINT_TUN=0
    INET_WANT_ENDPOINT_UDP=0

    for wanted_endpoint in ${INET_ENDPOINTS}; do

        case "${wanted_endpoint}" in

        dns)
             INET_WANT_ENDPOINT_DNS=1
             ;;

        raw)
             INET_WANT_ENDPOINT_RAW=1
             ;;

        tcp)
             INET_WANT_ENDPOINT_TCP=1
             ;;

        tun)
             INET_WANT_ENDPOINT_TUN=1
             ;;

        udp)
             INET_WANT_ENDPOINT_UDP=1
             ;;

        all)
             INET_WANT_ENDPOINT_DNS=1
             INET_WANT_ENDPOINT_RAW=1
             INET_WANT_ENDPOINT_TCP=1
             INET_WANT_ENDPOINT_TUN=1
             INET_WANT_ENDPOINT_UDP=1
             ;;
        esac

    done

    if test "${INET_WANT_ENDPOINT_TUN}" = 1; then
        AC_CHECK_HEADERS(linux/if_tun.h,[AC_SUBST(HAVE_LINUX_IF_TUN_H, [1])], [], [])
	    AC_CHECK_TYPE(struct rtentry, , AC_CHECK_HEADERS(linux/route.h, [AC_SUBST(HAVE_LINUX_ROUTE_H, [1])], [], []), [[#include <net/route.h>]])
	    AC_CHECK_TYPE(struct in6_rtmsg, , AC_CHECK_HEADERS(linux/ipv6_route.h, [AC_SUBST(HAVE_LINUX_IPV6_ROUTE_H, [1])], [], []), [[#include <net/route.h>]])
        # Check whether linux/if_tun.h defines TUNGETIFF.
        AC_MSG_CHECKING([whether linux/if_tun.h declares TUNGETIFF])
        AC_COMPILE_IFELSE([
            AC_LANG_PROGRAM(
        [[
        #if HAVE_LINUX_IF_TUN_H
        # include <linux/if_tun.h>
        #endif
        ]],
        [[
        #if !defined(TUNGETIFF)
        #error "TUNGETIFF is not defined"
        #endif
        ]])],
        [
        AC_MSG_RESULT([yes])
        AC_DEFINE(HAVE_TUNGETIFF, 1, [Define to 1 if <linux/if_tun.h> header file defines the TUNGETIFF ioctl command.])
        ],
        [
        AC_MSG_RESULT([no])
        ])
        AC_CHECK_TYPES(struct in6_rtmsg, [AC_SUBST(HAVE_IN6_RTMSG, [1])],[],[#include<net/route.h>])
        if [[[ "${WEAVE_SYSTEM_CONFIG_USE_LWIP}" -eq 0 ]]] && 
                [[[ "${HAVE_LINUX_IF_TUN_H}" -eq 0 || "${HAVE_IN6_RTMSG}" -eq 0 ]]]; then
            AC_MSG_NOTICE([Tun EndPoint cannot be enabled for current target 
                WEAVE_SYSTEM_CONFIG_USE_LWIP=$WEAVE_SYSTEM_CONFIG_USE_LWIP, HAVE_LINUX_IF_TUN_H=$HAVE_LINUX_IF_TUN_H,
                HAVE_IN6_RTMSG=$HAVE_IN6_RTMSG])
            INET_WANT_ENDPOINT_TUN=0
        fi
    fi

    AM_CONDITIONAL([INET_WANT_ENDPOINT_DNS],    [test "${INET_WANT_ENDPOINT_DNS}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_ENDPOINT_DNS],[${INET_WANT_ENDPOINT_DNS}],
                       [Define to 1 if you want to use Weave with the inet dns resolver.])

    AM_CONDITIONAL([INET_WANT_ENDPOINT_RAW],    [test "${INET_WANT_ENDPOINT_RAW}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_ENDPOINT_RAW],[${INET_WANT_ENDPOINT_RAW}],
                       [Define to 1 if you want to use Weave with the inet raw endpoint.])

    AM_CONDITIONAL([INET_WANT_ENDPOINT_TCP],    [test "${INET_WANT_ENDPOINT_TCP}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_ENDPOINT_TCP],[${INET_WANT_ENDPOINT_TCP}],
                       [Define to 1 if you want to use Weave with the inet tcp endpoint.])

    AM_CONDITIONAL([INET_WANT_ENDPOINT_TUN],    [test "${INET_WANT_ENDPOINT_TUN}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_ENDPOINT_TUN],[${INET_WANT_ENDPOINT_TUN}],
                       [Define to 1 if you want to use Weave with the inet tun endpoint.])

    AM_CONDITIONAL([INET_WANT_ENDPOINT_UDP],    [test "${INET_WANT_ENDPOINT_UDP}" = 1])
    AC_DEFINE_UNQUOTED([INET_WANT_ENDPOINT_UDP],[${INET_WANT_ENDPOINT_UDP}],
                       [Define to 1 if you want to use Weave with the inet udp endpoint.])

    if test "${INET_WANT_ENDPOINT_DNS}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_DNS_RESOLVER, 1, [Define to 1 for enabling DNS Resolver])
    fi

    if test "${INET_WANT_ENDPOINT_RAW}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_RAW_ENDPOINT, 1, [Define to 1 for enabling Raw Endpoint])
    fi

    if test "${INET_WANT_ENDPOINT_TCP}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_TCP_ENDPOINT, 1, [Define to 1 for enabling TCP Endpoint])
    fi

    if test "${INET_WANT_ENDPOINT_TUN}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_TUN_ENDPOINT, 1, [Define to 1 for enabling TUN Endpoint (only for LWIP and Linux w/ support)])
    fi

    if test "${INET_WANT_ENDPOINT_UDP}" = 1; then
        AC_DEFINE(INET_CONFIG_ENABLE_UDP_ENDPOINT, 1, [Define to 1 for enabling UDP Endpoint])
    fi
fi

# Asynchronous DNS
AC_MSG_CHECKING([whether to build with asynchronous DNS resolution support])
AC_ARG_ENABLE(adns,
    [AS_HELP_STRING([--disable-adns],[Disable building of adns @<:@default=no@:>@.])],
    [
        case "${enableval}" in

        no|yes)
            build_adns=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --disable-adns])
            ;;

        esac
    ],
    [build_adns=yes])
AC_MSG_RESULT(${build_adns})

AM_CONDITIONAL([INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS], [test "${build_adns}" = "yes"])

if test ${build_adns} = "yes" && test ${INET_WANT_ENDPOINT_DNS} = 1 && test ${WEAVE_SYSTEM_CONFIG_USE_SOCKETS} = 1; then
    AC_DEFINE(INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS, 1, [Define to 1 for enabling ASYNC DNS])
else
    AC_DEFINE(INET_CONFIG_ENABLE_ASYNC_DNS_SOCKETS, 0, [Define to 0 for disabling ASYNC DNS])
fi

#
# Verhoeff support
#
AC_MSG_CHECKING([Verhoeff support])

AC_ARG_ENABLE(verhoeff,
    [AS_HELP_STRING([--enable-verhoeff],[Enable Verhoeff support @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            enable_verhoeff=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-verhoeff])
            ;;

        esac
    ],
    [enable_verhoeff=yes])

AC_MSG_RESULT(${enable_verhoeff})

AM_CONDITIONAL([WEAVE_WITH_VERHOEFF], [test "${enable_verhoeff}" = "yes"])

AC_ARG_ENABLE(fuzzing,
    [AS_HELP_STRING([--enable-fuzzing],[Enable Fuzzing - Requires clang 6.0 or greater @<:@default=no@:>@.])],
    [
        case "${enableval}" in 

        no|yes)
            enable_fuzzing=${enableval}
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${enableval} for --enable-fuzzing])
            ;;

        esac
    ],
    [enable_fuzzing=no])


AC_MSG_RESULT(${enable_fuzzing})
AM_CONDITIONAL([WEAVE_FUZZING_ENABLED], [test "${enable_fuzzing}" = "yes"])
if test "${enable_fuzzing}" = "yes"; then
    AC_DEFINE(WEAVE_FUZZING_ENABLED, 1, [Define to 1 if support for fuzzing enabled])
fi

AC_MSG_CHECKING([enhanced printf facilities])

AC_ARG_WITH(enhanced-printf,
    [AS_HELP_STRING([--with-enhanced-printf=ENHANCEMENT],
        [Specify enhancement as one of: none (will use malloc on platforms that support it, will truncate on platforms without malloc support), vcbnsprintf for callback-backed printf, or vsnprintf_ex for vsnprintf variant that skips over the initial portion of a string. @<:@default=none@:>@.])],
    [
        case "${with_enhanced_printf}" in
        auto|none|vcbprintf|vsnprintf_ex)
            ;;

        *)
            AC_MSG_ERROR([Invalid value ${with_enhanced_printf} for --with-enhanced-printf])
            ;;

        esac
    ],
    [with_enhanced_printf=auto])

AC_MSG_CHECKING([enhanced printf facilities arg parsed])

CONFIG_HAVE_VCBPRINTF=0
CONFIG_HAVE_VSNPRINTF_EX=0

if test "${with_enhanced_printf}" = "auto"; then
AC_MSG_CHECKING([enhanced printf is auto])

    AC_CHECK_HEADERS([stdio.h])

    AC_MSG_CHECKING([Does the system provide vsnprintf_ex])

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM(
             [[
#include <stdio.h>
                ]],
                [[
const char format[] = "%d\n";
const char expected = "1\n";
const char buffer [20];
snprintf_ex(buffer, 2, 1, format, 31);
                ]])],

                [AC_MSG_RESULT([yes])
                 AC_DEFINE(HAVE_VSNPRINTF_EX, 1, [Define to 1 if your libc exports vnsprintf_ex.])
                 CONFIG_HAVE_VSNPRINTF_EX=1
                 ],

                 [
                 AC_MSG_RESULT([no])  
                 AC_DEFINE(HAVE_VSNPRINTF_EX, 0, [Define to 1 if your libc exports vnsprintf_ex.])
                 ])

    AC_MSG_CHECKING([Does the system provide vcbprintf])

    AC_LINK_IFELSE([
        AC_LANG_PROGRAM(
        [[
#include <stdarg.h>
#include <stdio.h>
static void private_callback(char c, void *appState)
{
        (void)c;
        (void)appState;
        return;
}
        ]],
        [[
const char format[] = "%d\n";
const char expected = "1\n";
const char buffer [20];
va_list ap;
vcbprintf(private_callback, NULL, format, ap);
        ]])],

        [AC_MSG_RESULT([yes])
        AC_DEFINE(HAVE_VCBPRINTF, 1, [Define to 1 if your libc exports vcbprintf.])
        CONFIG_HAVE_VCBPRINTF=1
        ],

        [
        AC_MSG_RESULT([no])
        AC_DEFINE(HAVE_VCBPRINTF, 0, [Define to 1 if your libc exports vcbprintf.])
        ])

fi

WEAVE_ENHANCED_PRINTF=${with_enhanced_printf}

case ${with_enhanced_printf} in

    vcbprintf)
        CONFIG_HAVE_VCBPRINTF=1
        ;;

    vsnprintf_ex)
        CONFIG_HAVE_VSNPRINTF_EX=1
        ;;

esac

AC_SUBST(CONFIG_HAVE_VCBPRINTF)
AM_CONDITIONAL([CONFIG_HAVE_VCBPRINTF], [test "${CONFIG_HAVE_VCBPRINTF}" = "1"])
AC_DEFINE_UNQUOTED([CONFIG_HAVE_VCBPRINTF],[${CONFIG_HAVE_VCBPRINTF}],[Define to 1 if you want to use Weave with a system that supports callback-based vcbprintf])

AC_SUBST(CONFIG_HAVE_VSNPRINTF_EX)
AM_CONDITIONAL([CONFIG_HAVE_VSNPRINTF_EX], [test "${CONFIG_HAVE_VSNPRINTF_EX}" = "1"])
AC_DEFINE_UNQUOTED([CONFIG_HAVE_VSNPRINTF_EX],[${CONFIG_HAVE_VSNPRINTF_EX}],[Define to 1 if you want to use Weave with a system that supports callback-based vcbprintf])

#
# The user may specify project-specific configuration for the Weave
# SystemLayer, BleLayer, InetLayer, and Core sub-components.
#

AC_LANG_PUSH([C++])

# SystemLayer

NL_CHECK_PROJECT_CONFIG_INCLUDES(weave-system-project-includes, SYSTEM_PROJECT_CONFIG_INCLUDE, SystemProjectConfig.h, Weave System Layer, ${ac_abs_confdir}/build/config/standalone)

# BleLayer

if test ${CONFIG_NETWORK_LAYER_BLE} = 1; then
    NL_CHECK_PROJECT_CONFIG_INCLUDES(weave-ble-project-includes, BLE_PROJECT_CONFIG_INCLUDE, BleProjectConfig.h, Weave BLE Layer, ${ac_abs_confdir}/build/config/standalone)
fi

# InetLayer

if test ${CONFIG_NETWORK_LAYER_INET} = 1; then
    NL_CHECK_PROJECT_CONFIG_INCLUDES(weave-inet-project-includes, INET_PROJECT_CONFIG_INCLUDE, InetProjectConfig.h, Weave Inet Layer, ${ac_abs_confdir}/build/config/standalone)
fi

# Warm

if test "${build_warm}" = "yes"; then
    NL_CHECK_PROJECT_CONFIG_INCLUDES(weave-warm-project-includes, WARM_PROJECT_CONFIG_INCLUDE, WarmProjectConfig.h, WARM, ${ac_abs_confdir}/build/config/standalone)
fi

# Core

NL_CHECK_PROJECT_CONFIG_INCLUDES(weave-project-includes, WEAVE_PROJECT_CONFIG_INCLUDE, WeaveProjectConfig.h, Weave Core, ${ac_abs_confdir}/build/config/standalone)

AC_LANG_POP([C++])

#
# Checks for libraries and packages.
#
# At minimum, the following are required:
#
#   * nlassert
#   * nlio
#
# The following are optional, depending on configuration:
#
#   * curl
#   * lwip
#   * nlunit-test
#   * openssl
#   * nlfaultinjection
#
# Most of these are supplied "in package"; however, they may be also
# supplied out of package.
#
AC_MSG_NOTICE([checking package dependencies])

# Check if the build host has pkg-config

AC_PATH_PROG([PKG_CONFIG],[pkg-config])

#
# CuRL
#

NL_WITH_OPTIONAL_EXTERNAL_PACKAGE([CURL],
    [CURL],
    [libcurl],
    [-lcurl],
    [
		# Check for required CuRL headers.

		AC_CHECK_HEADERS([curl/curl.h],
		[],
		[
			AC_MSG_NOTICE(The CuRL header "$ac_header" is required but cannot be found.)
		])

		# The header curl/easy.h cannot be included without curl/curl.h.

		AC_CHECK_HEADERS([curl/easy.h],
		[],
		[
			AC_MSG_NOTICE(The CuRL header "$ac_header" is required but cannot be found.)
		],
		[
#ifdef HAVE_CURL_CURL_H
#include <curl/curl.h>
#endif
		])

		CURL_CPPFLAGS=
		CURL_LDFLAGS=
		CURL_LIBS="-lcurl"
    ]
)

AM_CONDITIONAL([WEAVE_WITH_CURL], [test "${nl_with_libcurl}" != ""])

#
# OpenSSL
#

NL_WITH_OPTIONAL_INTERNAL_PACKAGE([OpenSSL],
    [OPENSSL],
    [openssl],
    [-lcrypto],
    [
        # OpenSSL-specific compiler flags we would like to have.
        #
        #   -fvisibility=hidden          CC, CXX, OBJC, OBJCXX [openssl=internal]
        #   -fvisibility-inlines-hidden      CXX,       OBJCXX [openssl=internal]
        #
        #   IMPORTANT: Because Weave requires, when --with-openssl=internal has
        #              been specified, a specially-compiled version of OpenSSL,
        #              -fvisibility=hidden is essential when building on linux to
        #              avoid conflicts between Weave's version of OpenSSL and the
        #              native system version.
    
        PROSPECTIVE_CFLAGS="-fvisibility=hidden"
        PROSPECTIVE_CXXFLAGS="-fvisibility-inlines-hidden"
    
        AX_CHECK_COMPILER_OPTIONS([C],             VISIBILITY_CFLAGS,      ${PROSPECTIVE_CFLAGS})
        AX_CHECK_COMPILER_OPTIONS([C++],           VISIBILITY_CXXFLAGS,    ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS})
        AX_CHECK_COMPILER_OPTIONS([Objective C],   VISIBILITY_OBJCFLAGS,   ${PROSPECTIVE_CFLAGS})
        AX_CHECK_COMPILER_OPTIONS([Objective C++], VISIBILITY_OBJCXXFLAGS, ${PROSPECTIVE_CFLAGS} ${PROSPECTIVE_CXXFLAGS})

        AC_SUBST(VISIBILITY_CFLAGS)
        AC_SUBST(VISIBILITY_CXXFLAGS)
        AC_SUBST(VISIBILITY_OBJCFLAGS)
        AC_SUBST(VISIBILITY_OBJCXXCFLAGS)

        # At this point, the internal OpenSSL package will be neither
        # configured nor built, so the normal checks we undertake for an
        # external package cannot be run here. Simply set the appropriate
        # variables and trust all will be well.
    
        openssl_buildstem=${target}
    
        AC_SUBST(openssl_buildstem)
    
        OPENSSL_CPPFLAGS="-DWEAVE_WITH_OPENSSL=1 -I${ac_pwd}/third_party/openssl/${openssl_buildstem}/include"
        OPENSSL_LDFLAGS="-L${ac_pwd}/third_party/openssl/${openssl_buildstem}"
        OPENSSL_LIBS="-lcrypto"

        # OpenSSL doesn't support non-colocated source and build
        # directories. Consequently, we'll need to "shadow" the source
        # tree. Figure out the best means to do this on the host (native
        # lndir --slient, native cp -Rs, or our package-local cp -Rs).
    
        NL_PROG_LNDIR(${ac_abs_confdir}/${nlbuild_autotools_stem}/tools/host/${nl_filtered_build}/bin/cp,
                  \${top_srcdir}/${nlbuild_autotools_stem}/tools/host/${nl_filtered_build}/bin/cp)

        # Attempt to map the target system into a reasonable OpenSSL
        # build configuration target.
        #
        # Weave uses a very restrictive, minimal 32-bit only configuration
        # of OpenSSL, so collapse the specified or detected target down.
    
        AC_MSG_CHECKING([OpenSSL target configuration])

        case "${target}" in

        ppc-apple-darwin*)
            WEAVE_OPENSSL_TARGET=BSD-generic32
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

	*apple-darwin-ios)
	    WEAVE_OPENSSL_TARGET=iphoneos-cross
            WEAVE_OPENSSL_TARGET_DEFINES=
            CROSS_TOP="${CROSS_TOP}"
            CROSS_SDK="${CROSS_SDK}"
            ;;

        *86*-apple-darwin*)
            WEAVE_OPENSSL_TARGET=BSD-generic32
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

        aarch64-apple-darwin | arm*-apple-darwin*)
            WEAVE_OPENSSL_TARGET=iphoneos-cross
            WEAVE_OPENSSL_TARGET_DEFINES=
            CROSS_TOP="${CROSS_TOP}"
            CROSS_SDK="${CROSS_SDK}"
            ;;

        # Android should come before the generic Linux catch-all
        # case.

        arm-*-android*)
            WEAVE_OPENSSL_TARGET=android-armeabi
            WEAVE_OPENSSL_TARGET_DEFINES="-D__ARM_MAX_ARCH__=8"
            ;;

        armv7-*-android*)
            WEAVE_OPENSSL_TARGET=android-armeabi
            WEAVE_OPENSSL_TARGET_DEFINES="-march=armv7-a -D__ARM_MAX_ARCH__=8"
            ;;

        arm64-*-android*)
            WEAVE_OPENSSL_TARGET=android64-aarch64
            WEAVE_OPENSSL_TARGET_DEFINES="-D__ARM_MAX_ARCH__=8"
            ;;

        i386-*-android*)
            WEAVE_OPENSSL_TARGET=android-x86
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

        x86_64-*-android*)
            WEAVE_OPENSSL_TARGET=android64
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

        *-*-linux*)
            WEAVE_OPENSSL_TARGET=linux-generic32
            WEAVE_OPENSSL_TARGET_DEFINES="-DL_ENDIAN"
            ;;

        arm*-none*)
            WEAVE_OPENSSL_TARGET=embedded-armv7
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

        *cygwin*)
            WEAVE_OPENSSL_TARGET=Cygwin
            WEAVE_OPENSSL_TARGET_DEFINES=
            ;;

        *)
            AC_MSG_ERROR([Unrecognized or unsupported target system ${target}. You may perhaps try an externally-built OpenSSL configuration using either the --with-openssl option or the --with-openssl-headers and --with-openssl-libs options.])

        esac

        AC_MSG_RESULT([${WEAVE_OPENSSL_TARGET}])
    
        AC_SUBST(CROSS_TOP)
        AC_SUBST(CROSS_SDK)
        AC_SUBST(WEAVE_OPENSSL_TARGET)
        AC_SUBST(WEAVE_OPENSSL_TARGET_DEFINES)
    ],
    [
        # Check for required OpenSSL headers.
    
        AC_CHECK_HEADERS([openssl/aes.h] [openssl/bn.h] [openssl/ec.h] [openssl/err.h] [openssl/evp.h] [openssl/sha.h],
            [],
            [
                AC_MSG_ERROR(The OpenSSL header "$ac_header" is required but cannot be found.)
            ])
        
    ])

AM_CONDITIONAL([WEAVE_WITH_OPENSSL], [test "${nl_with_openssl}" != "no"])
AM_CONDITIONAL([WEAVE_WITH_OPENSSL_INTERNAL], [test "${nl_with_openssl}" = "internal"])

if test "${nl_with_openssl}" = "no"; then
    AC_DEFINE([WEAVE_WITH_OPENSSL], [0], [Define to 1 to build Weave with OpenSSL features])
else
    AC_DEFINE([WEAVE_WITH_OPENSSL], [1], [Define to 1 to build Weave with OpenSSL features])
fi

#
# LwIP
#

NL_WITH_LWIP(${WEAVE_SYSTEM_CONFIG_USE_LWIP})

#
# Nlio
#

NL_WITH_PACKAGE(
    [Nlio],
    [NLIO],
    [nlio],
    [],
    [
        # At this point, the internal Nlio package will be neither
        # configured nor built, so the normal checks we undertake for an
        # external package cannot be run here. Simply set the appropriate
        # variables and trust all will be well.

        NLIO_CPPFLAGS="-I\${abs_top_srcdir}/third_party/nlio/repo/include"
        NLIO_LDFLAGS=
        NLIO_LIBS=
    ],
    [
        # Check for required Nlio headers.

        AC_CHECK_HEADERS([nlbyteorder.h],
        [],
        [
            AC_MSG_ERROR(The nlio header "$ac_header" is required but cannot be found.)
        ])
    ]
)

# Depending on whether nlio has been configured for an internal
# location, its directory stem within this package needs to be set
# accordingly. In addition, if the location is internal, then we need
# to attempt to pull it down using the bootstrap makefile.

if test "${nl_with_nlio}" = "internal"; then
    maybe_nlio_dirstem="nlio/repo"
    nlio_dirstem="third_party/${maybe_nlio_dirstem}"

    AC_MSG_NOTICE([attempting to create internal ${nlio_dirstem}])

    ${MAKE-make} --no-print-directory -C ${srcdir} -f Makefile-bootstrap ${nlio_dirstem}

    if test $? -ne 0; then
        AC_MSG_ERROR([failed to create ${nlio_dirstem}. Please check your network connection or the correctness of 'repos.conf'])
    fi
else
    maybe_nlio_dirstem=""
fi

AC_SUBST(NLIO_SUBDIRS, [${maybe_nlio_dirstem}])
AM_CONDITIONAL([WEAVE_WITH_NLIO_INTERNAL], [test "${nl_with_nlio}" = "internal"])

#
# Nlassert
#

NL_WITH_PACKAGE(
    [Nlassert],
    [NLASSERT],
    [nlassert],
    [],
    [
        # At this point, the internal Nlassert package will be neither
        # configured nor built, so the normal checks we undertake for an
        # external package cannot be run here. Simply set the appropriate
        # variables and trust all will be well.

        NLASSERT_CPPFLAGS="-I\${abs_top_srcdir}/third_party/nlassert/repo/include"
        NLASSERT_LDFLAGS=
        NLASSERT_LIBS=
    ],
    [
        # Check for required Nlassert headers.

        AC_CHECK_HEADERS([nlassert.h],
        [],
        [
            AC_MSG_ERROR(The nlassert header "$ac_header" is required but cannot be found.)
        ])
    ]
)

# Depending on whether nlassert has been configured for an internal
# location, its directory stem within this package needs to be set
# accordingly. In addition, if the location is internal, then we need
# to attempt to pull it down using the bootstrap makefile.

if test "${nl_with_nlassert}" = "internal"; then
    maybe_nlassert_dirstem="nlassert/repo"
    nlassert_dirstem="third_party/${maybe_nlassert_dirstem}"

    AC_MSG_NOTICE([attempting to create internal ${nlassert_dirstem}])

    ${MAKE-make} --no-print-directory -C ${srcdir} -f Makefile-bootstrap ${nlassert_dirstem}

    if test $? -ne 0; then
        AC_MSG_ERROR([failed to create ${nlassert_dirstem}. Please check your network connection or the correctness of 'repos.conf'])
    fi
else
    maybe_nlassert_dirstem=""
fi

AC_SUBST(NLASSERT_SUBDIRS, [${maybe_nlassert_dirstem}])
AM_CONDITIONAL([WEAVE_WITH_NLASSERT_INTERNAL], [test "${nl_with_nlassert}" = "internal"])

#
# Nlfaultinjection
#

NL_WITH_OPTIONAL_INTERNAL_PACKAGE(
    [Nlfaultinjection],
    [NLFAULTINJECTION],
    [nlfaultinjection],
    [-lnlfaultinjection],
    [
        # At this point, the internal nlfaultinjection package will be neither
        # configured nor built, so the normal checks we undertake for an
        # external package cannot be run here. Simply set the appropriate
        # variables and trust all will be well.

        NLFAULTINJECTION_CPPFLAGS="-I\${abs_top_srcdir}/third_party/nlfaultinjection/repo/include"
        NLFAULTINJECTION_LDFLAGS="-L${ac_pwd}/third_party/nlfaultinjection/repo/src"
        NLFAULTINJECTION_LIBS="-lnlfaultinjection"
    ],
    [
        # Check for required nlfaultinjection headers.

        AC_LANG_PUSH([C++])
        AC_CHECK_HEADERS([nlfaultinjection.hpp],
        [],
        [
            AC_MSG_ERROR(The nlfaultinjection header "$ac_header" is required but cannot be found.)
        ])
        AC_LANG_POP([C++])
    ]
)

# Depending on whether nlfaultinjection has been configured for an internal
# location, its directory stem within this package needs to be set
# accordingly. In addition, if the location is internal, then we need
# to attempt to pull it down using the bootstrap makefile.

if test "${nl_with_nlfaultinjection}" = "internal"; then
    maybe_nlfaultinjection_dirstem="nlfaultinjection/repo"
    nlfaultinjection_dirstem="third_party/${maybe_nlfaultinjection_dirstem}"

    AC_MSG_NOTICE([attempting to create internal ${nlfaultinjection_dirstem}])

    ${MAKE-make} --no-print-directory -C ${srcdir} -f Makefile-bootstrap ${nlfaultinjection_dirstem}

    if test $? -ne 0; then
        AC_MSG_ERROR([failed to create ${nlfaultinjection_dirstem}. Please check your network connection or the correctness of 'repos.conf'])
    fi
else
    maybe_nlfaultinjection_dirstem=""
fi

AC_SUBST(NLFAULTINJECTION_SUBDIRS, [${maybe_nlfaultinjection_dirstem}])
AM_CONDITIONAL([WEAVE_WITH_NLFAULTINJECTION], [test "${nl_with_nlfaultinjection}" != "no"])
AM_CONDITIONAL([WEAVE_WITH_NLFAULTINJECTION_INTERNAL], [test "${nl_with_nlfaultinjection}" = "internal"])

if test "${nl_with_nlfaultinjection}" = "no"; then
    AC_DEFINE([WEAVE_WITH_NLFAULTINJECTION], [0], [Define to 1 to build Weave with nlFaultInjection features])
else
    AC_DEFINE([WEAVE_WITH_NLFAULTINJECTION], [1], [Define to 1 to build Weave with nlFaultInjection features])
fi

if test "${nl_cv_build_tests}" = "yes"; then
    if test "${nl_with_nlfaultinjection}" = "no"; then
        AC_MSG_ERROR(Weave tests require nlfaultinjection.)
    fi
fi


#
# Nlunit-test
#

if test "${nl_cv_build_tests}" = "yes"; then
    NL_WITH_PACKAGE(
        [Nlunit-test],
        [NLUNIT_TEST],
        [nlunit_test],
        [-lnlunit-test],
        [
	    # At this point, the internal Nlunit-test package will be neither
	    # configured nor built, so the normal checks we undertake for an
	    # external package cannot be run here. Simply set the appropriate
	    # variables and trust all will be well.

            NLUNIT_TEST_CPPFLAGS="-I\${abs_top_srcdir}/third_party/nlunit-test/repo/src"
            NLUNIT_TEST_LDFLAGS="-L${ac_pwd}/third_party/nlunit-test/repo/src"
            NLUNIT_TEST_LIBS="-lnlunit-test"
        ],
        [
            # Check for required nlunit-test headers.

            AC_CHECK_HEADERS([nlunit-test.h],
            [],
            [
                AC_MSG_ERROR(The nlunit-test header "$ac_header" is required but cannot be found.)
            ])
        ])
fi

# Depending on whether nlunit-test has been configured for an internal
# location, its directory stem within this package needs to be set
# accordingly. In addition, if the location is internal, then we need
# to attempt to pull it down using the bootstrap makefile.

if test "${nl_with_nlunit_test}" = "internal"; then
    maybe_nlunit_test_dirstem="nlunit-test/repo"
    nlunit_test_dirstem="third_party/${maybe_nlunit_test_dirstem}"

    AC_MSG_NOTICE([attempting to create internal ${nlunit_test_dirstem}])

    ${MAKE-make} --no-print-directory -C ${srcdir} -f Makefile-bootstrap ${nlunit_test_dirstem}

    if test $? -ne 0; then
        AC_MSG_ERROR([failed to create ${nlunit_test_dirstem}. Please check your network connection or the correctness of 'repos.conf'])
    fi
else
    maybe_nlunit_test_dirstem=""
fi

AC_SUBST(NLUNIT_TEST_SUBDIRS, [${maybe_nlunit_test_dirstem}])
AM_CONDITIONAL([WEAVE_WITH_NLUNIT_TEST_INTERNAL], [test "${nl_with_nlunit_test}" = "internal"])

#
# Sockets
#

# Influential external variables for BSD Sockets support

AC_ARG_VAR(SOCKETS_CPPFLAGS, [BSD Sockets C preprocessor flags])
AC_ARG_VAR(SOCKETS_LDFLAGS,  [BSD Sockets linker flags])
AC_ARG_VAR(SOCKETS_LIBS,     [BSD Sockets linker libraries])


#
# Check for headers
#
AC_HEADER_STDBOOL
AC_HEADER_STDC

AC_CHECK_HEADERS([ctype.h])
AC_CHECK_HEADERS([ifaddrs.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])

# Check for netinet/icmp6.h for ICMP6_FILTER support. Both Android and
# Mac OS X fail to properly include both netinet/in.h and sys/types.h
# in netinet/icmp6.h, making the check fail unless it exists and these
# are both also included.

AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([sys/types.h])
AC_CHECK_HEADERS([netinet/icmp6.h], [], [],
[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
])

# But, wait, this still isn't enough. Even with all of that, Android
# may still not have the ICMP6_FILTER socket option defined. Check for
# that too.

AC_MSG_CHECKING([whether netinet/icmp6.h declares ICMP6_FILTER])
AC_COMPILE_IFELSE([
          AC_LANG_PROGRAM(
[[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#if HAVE_NETINET_ICMP6_H
# include <netinet/icmp6.h>
#endif
]],
[[
#if !defined(ICMP6_FILTER)
# error "ICMP6_FILTER is not defined"
#endif
]])],
[
AC_MSG_RESULT([yes])
AC_DEFINE(HAVE_ICMP6_FILTER, 1, [Define to 1 if your <netinet/icmp6.h> header file defines the ICMP6_FILTER socket option.])
],
[
AC_MSG_RESULT([no])
])

# Check for sys/socket.h for SO_BINDTODEVICE support.

AC_CHECK_HEADERS([sys/socket.h])

AC_MSG_CHECKING([whether sys/socket.h declares SO_BINDTODEVICE])
AC_COMPILE_IFELSE([
          AC_LANG_PROGRAM(
[[
#if HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
]],
[[
#if !defined(SO_BINDTODEVICE)
# error "SO_BINDTODEVICE is not defined"
#endif
]])],
[
AC_MSG_RESULT([yes])
AC_DEFINE(HAVE_SO_BINDTODEVICE, 1, [Define to 1 if your <sys/socket.h> header file defines the SO_BINDTODEVICE socket option.])
],
[
AC_MSG_RESULT([no])
])

#
# Check for types and structures
#
AC_TYPE_INT8_T
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

#
# Checks for library functions
#

CONFIG_HAVE_HEAP=0

if test "${ac_no_link}" != "yes"; then
    # Check for stdlib memory management functions.
    AC_CHECK_FUNCS([malloc free realloc strdup], [CONFIG_HAVE_HEAP=1], [CONFIG_HAVE_HEAP=0])
    AC_CHECK_FUNCS([memchr memcmp memmove memset])
    AC_CHECK_FUNCS([strchr strcmp strlen])
    AC_CHECK_FUNCS([snprintf])

    # Some versions of Android are missing getifaddrs and
    # freeifaddrs. Check to see if the target system is such a version.

    AC_CHECK_FUNCS([getifaddrs freeifaddrs])

    # Check for clock_gettime, gettimeofday, settimeofday and localtime.
    # In some target environments, clock_gettime exists in librt.

    AC_SEARCH_LIBS([clock_gettime], [rt])
    AC_CHECK_FUNCS([clock_gettime gettimeofday localtime])
    AC_CHECK_HEADERS([time.h sys/time.h])
    AC_CHECK_TYPES([clockid_t], [], [], [[#include <time.h>]])

    # Check for clock_settime.  This function exists on iOS, however the compiler prevents
    # reference to it via the use of an "unavailable" attribute.  Unfortunately, this goes
    # unnoticed by autoconf's AC_CHECK_FUNCS macro, hence the need for custom detection code.

    AC_SEARCH_LIBS([clock_settime], [rt])
    AC_MSG_CHECKING([clock_settime])
    AC_LINK_IFELSE(
        [AC_LANG_SOURCE([[
            #include <stdlib.h>
            #include <time.h>
            int main(int argc, char *argv[])
            {
                clock_settime(CLOCK_REALTIME, NULL);
                return 0;
            }
        ]])],
        [AC_DEFINE([HAVE_CLOCK_SETTIME], [1], [Define to 1 if you have the `clock_settime' function.]) AC_MSG_RESULT([yes])],
        [AC_MSG_RESULT([no])]
    ) 

    # Note that different platforms have different APIs for monotonic timer with sleep time compensation.
    # CLOCK_BOOTTIME is available in later versions of linux and Android

    AC_CHECK_DECLS([CLOCK_MONOTONIC, CLOCK_BOOTTIME], [], [], [[#include <time.h>]])
fi

AC_SUBST(CONFIG_HAVE_HEAP)
AM_CONDITIONAL([CONFIG_HAVE_HEAP], [test "${CONFIG_HAVE_HEAP}" = "1"])

#
# Check for POSIX thread (pthread) support
#
AX_PTHREAD([], [])

AC_CHECK_DECL([PTHREAD_NULL], [], [AC_DEFINE([PTHREAD_NULL],[0], [Approximation of PTHREAD_NULL since pthread.h does not define one])],
              [[#include <pthread.h>]])

#
# Check for <new>
#
AC_LANG_PUSH([C++])

AC_CHECK_HEADERS([new])

AC_LANG_POP([C++])


# Add test enabled macro
if test "${nl_cv_build_tests}" = "yes"; then
    CPPFLAGS="${CPPFLAGS} -DNL_WEAVE_WITH_TESTS"
    CPPFLAGS="${CPPFLAGS} -DWEAVE_CONFIG_TEST=1"
    CPPFLAGS="${CPPFLAGS} -DWEAVE_SYSTEM_CONFIG_TEST=1"
    CPPFLAGS="${CPPFLAGS} -DWEAVE_SYSTEM_CONFIG_PROVIDE_STATISTICS=1"
    if test ${CONFIG_NETWORK_LAYER_INET} = 1; then
        CPPFLAGS="${CPPFLAGS} -DINET_CONFIG_TEST=1"
    fi
fi

# Add any OpenSSL CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${OPENSSL_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${OPENSSL_LDFLAGS}"
LIBS="${LIBS} ${OPENSSL_LIBS}"

# Add any nlunit-test CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLUNIT_TEST_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLUNIT_TEST_LDFLAGS}"
LIBS="${LIBS} ${NLUNIT_TEST_LIBS}"

# Add any nlio CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLIO_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLIO_LDFLAGS}"
LIBS="${LIBS} ${NLIO_LIBS}"

# Add any nlassert CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLASSERT_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLASSERT_LDFLAGS}"
LIBS="${LIBS} ${NLASSERT_LIBS}"

# Add any nlfaultinjection CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NLFAULTINJECTION_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NLFAULTINJECTION_LDFLAGS}"
LIBS="${LIBS} ${NLFAULTINJECTION_LIBS}"

# Add any code coverage CPPFLAGS, LDFLAGS, and LIBS

CPPFLAGS="${CPPFLAGS} ${NL_COVERAGE_CPPFLAGS}"
LDFLAGS="${LDFLAGS} ${NL_COVERAGE_LDFLAGS}"
LIBS="${LIBS} ${NL_COVERAGE_LIBS}"

# Fold in the warning-as-errors flag, unless it was already in effect.

if test "${nl_had_CPPFLAGS_werror}" = "no"; then
    CPPFLAGS="${CPPFLAGS} ${NL_WERROR_CPPFLAGS}"
fi

# At this point, we can restore the compiler flags to whatever the
# user passed in, now that we're clear of an -Werror issues by
# transforming -Wno-error back to -Werror.

NL_RESTORE_WERROR

# Coding Style
# 
# Establish what tool to use and what arguments to pass for the
# pretty-check target.

AC_SUBST(PRETTY_CHECK, ["\${PERL} \${abs_top_srcdir}/third_party/cstyle/repo/cstyle.pl"])
AC_SUBST(PRETTY_CHECK_ARGS, ["\$(shell cat \${abs_top_srcdir}/build/cstyle/cstyle.options)"])

# Configure any autotools-based subdirectories
if test "${nl_with_nlunit_test}" = "internal"; then
AC_CONFIG_SUBDIRS([third_party/nlunit-test/repo])
fi

if test "${nl_with_nlio}" = "internal"; then
AC_CONFIG_SUBDIRS([third_party/nlio/repo])
fi

if test "${nl_with_nlassert}" = "internal"; then
AC_CONFIG_SUBDIRS([third_party/nlassert/repo])
fi

if test "${nl_with_nlfaultinjection}" = "internal"; then
AC_CONFIG_SUBDIRS([third_party/nlfaultinjection/repo])
fi

if test "${nl_with_bluez}" = "internal" && test "${enable_woble_bluez}" = "yes"; then
AC_CONFIG_SUBDIRS([third_party/bluez/repo])
fi

#
# Identify the various makefiles and auto-generated files for the package
#
AC_CONFIG_FILES([
Makefile
third_party/Makefile
third_party/lwip/Makefile
third_party/android/platform-system/core-mincrypt/Makefile
third_party/micro-ecc/Makefile
third_party/openssl/Makefile
third_party/openssl-jpake/Makefile
src/Makefile
src/include/Makefile
src/ble/Makefile
src/lwip/Makefile
src/platform/ble/bluez/Makefile
src/system/Makefile
src/inet/Makefile
src/lib/Makefile
src/lib/support/Makefile
src/lib/support/verhoeff/Makefile
src/wrappers/jni/Makefile
src/wrappers/jni/jni-utils/Makefile
src/wrappers/jni/security-support/Makefile
src/device-manager/Makefile
src/device-manager/cocoa/Makefile
src/device-manager/java/Makefile
src/device-manager/python/Makefile
src/warm/Makefile
src/ra-daemon/Makefile
src/test-apps/Makefile
src/test-apps/wrapper-tests/jni/Makefile
src/test-apps/fuzz/Makefile
src/tools/weave/Makefile
src/tools/misc/Makefile
src/examples/Makefile
src/adaptations/device-layer/Makefile
doc/Makefile
])

#
# Generate the auto-generated files for the package
#
AC_OUTPUT

#
# Summarize the package configuration
#

AC_MSG_NOTICE([

  Configuration Summary
  ---------------------
  Package                                          : ${PACKAGE_NAME}
  Version                                          : ${PACKAGE_VERSION}
  Interface                                        : ${LIBWEAVE_VERSION_INFO//:/.}
  Build system                                     : ${build}
  Host system                                      : ${host}
  Target system                                    : ${target}
  Target architecture                              : ${target_cpu}
  Target OS                                        : ${target_os}
  Target style                                     : ${WEAVE_TARGET_STYLE}
  Target network layer                             : ${with_network_layer}
  Target network system(s)                         : ${CONFIG_TARGET_NETWORKS}
  IPv4 enabled                                     : ${enable_ipv4}
  Internet endpoint(s)                             : ${INET_ENDPOINTS}
  Printf enhancements                              : ${WEAVE_ENHANCED_PRINTF}
  Android support                                  : ${with_android}
  Logging style                                    : ${WEAVE_LOGGING_STYLE}
  Cross compiling                                  : ${cross_compiling}
  Build shared libraries                           : ${enable_shared}
  Build static libraries                           : ${enable_static}
  Build debug libraries                            : ${nl_cv_build_debug}
  Build profile libraries                          : ${build_profile}
  Build optimized libraries                        : ${nl_cv_build_optimized}
  Build coverage libraries                         : ${nl_cv_build_coverage}
  Build coverage reports                           : ${nl_cv_build_coverage_reports}
  Lcov                                             : ${LCOV:--}
  Genhtml                                          : ${GENHTML:--}
  Happy                                            : ${with_happy}
  Happy Path                                       : ${happy_path}
  Happy Services                                   : ${with_happy_service}
  Happy Services Test Suites                       : ${with_happy_test_suite}
  Happy DNS Servers                                : ${with_happy_dns}
  Treat warnings as errors                         : ${nl_cv_warnings_as_errors}
  Build tests                                      : ${nl_cv_build_tests}
  Build long running tests                         : ${nl_cv_build_long_tests}
  Build tools                                      : ${build_tools}
  Build Device Manager                             : ${build_device_manager}
  Build WARM                                       : ${build_warm}
  Tunnel Failover support                          : ${build_tunnel_failover}
  Build legacy WDM profile                         : ${build_legacy_wdm}
  Prefix                                           : ${prefix}
  Shadow directory program                         : ${LNDIR}
  Cocoa support                                    : ${enable_cocoa}
  Java support                                     : ${enable_java}
  Java home                                        : ${JAVA_HOME:--}
  Java Compiler                                    : ${JAVAC:--}
  Java Native Interface (JNI) compile flags        : ${JNI_CPPFLAGS:--}
  Java Archiver                                    : ${JAR:--}
  Python support                                   : ${enable_python}
  Python script directory                          : ${pythondir:--}
  Python extension directory                       : ${pyexecdir:--}
  Verhoeff support                                 : ${enable_verhoeff}
  Pairing code decoding support                    : ${enable_pairing_code_decoding}
  Documentation support                            : ${nl_cv_build_docs}
  Doxygen                                          : ${DOXYGEN:--}
  GraphViz dot                                     : ${DOT:--}
  PERL                                             : ${PERL:--}
  BlueZ peripheral support                         : ${enable_woble_bluez:--}
  BlueZ source                                     : ${nl_with_bluez:--}
  BlueZ compile flags                              : ${BLUEZ_CPPFLAGS:--}
  Bluez link flags                                 : ${BLUEZ_LDFLAGS:--}
  Bluez link libraries                             : ${BLUEZ_LIBS:--}
  CURL source                                      : ${nl_with_libcurl:--}
  CURL compile flags                               : ${CURL_CPPFLAGS:--}
  CURL link flags                                  : ${CURL_LDFLAGS:--}
  CURL link libraries                              : ${CURL_LIBS:--}
  LwIP source                                      : ${nl_with_lwip:--}
  LwIP compile flags                               : ${LWIP_CPPFLAGS:--}
  LwIP link flags                                  : ${LWIP_LDFLAGS:--}
  LwIP link libraries                              : ${LWIP_LIBS:--}
  OpenSSL source                                   : ${nl_with_openssl}
  OpenSSL target                                   : ${WEAVE_OPENSSL_TARGET:--}
  OpenSSL compile flags                            : ${OPENSSL_CPPFLAGS:--}
  OpenSSL link flags                               : ${OPENSSL_LDFLAGS:--}
  OpenSSL link libraries                           : ${OPENSSL_LIBS:--}
  Nlunit-test source                               : ${nl_with_nlunit_test:--}
  Nlunit-test compile flags                        : ${NLUNIT_TEST_CPPFLAGS:--}
  Nlunit-test link flags                           : ${NLUNIT_TEST_LDFLAGS:--}
  Nlunit-test link libraries                       : ${NLUNIT_TEST_LIBS:--}
  Nlio source                                      : ${nl_with_nlio:--}
  Nlio compile flags                               : ${NLIO_CPPFLAGS:--}
  Nlio link flags                                  : ${NLIO_LDFLAGS:--}
  Nlio link libraries                              : ${NLIO_LIBS:--}
  Nlassert source                                  : ${nl_with_nlassert:--}
  Nlassert compile flags                           : ${NLASSERT_CPPFLAGS:--}
  Nlassert link flags                              : ${NLASSERT_LDFLAGS:--}
  Nlassert link libraries                          : ${NLASSERT_LIBS:--}
  Nlfaultinjection source                          : ${nl_with_nlfaultinjection:--}
  Nlfaultinjection compile flags                   : ${NLFAULTINJECTION_CPPFLAGS:--}
  Nlfaultinjection link flags                      : ${NLFAULTINJECTION_LDFLAGS:--}
  Nlfaultinjection link libraries                  : ${NLFAULTINJECTION_LIBS:--}
  Sockets compile flags                            : ${SOCKETS_CPPFLAGS:--}
  Sockets link flags                               : ${SOCKETS_LDFLAGS:--}
  Sockets link libraries                           : ${SOCKETS_LIBS:--}
  PThreads compile flags                           : ${PTHREAD_CFLAGS:--}
  PThreads link libraries                          : ${PTHREAD_LIBS:--}
  C Preprocessor                                   : ${CPP}
  C Compiler                                       : ${CC}
  C++ Preprocessor                                 : ${CXXCPP}
  C++ Compiler                                     : ${CXX}
  Objective C Compiler                             : ${OBJC:--}
  Objective C++ Compiler                           : ${OBJCXX:--}
  Archiver                                         : ${AR}
  Archive Indexer                                  : ${RANLIB}
  Symbol Stripper                                  : ${STRIP}
  Object Copier                                    : ${OBJCOPY}
  C Preprocessor flags                             : ${CPPFLAGS:--}
  C Compile flags                                  : ${CFLAGS:--}
  C++ Compile flags                                : ${CXXFLAGS:--}
  Objective C Compile flags                        : ${OBJCFLAGS:--}
  Objective C++ Compile flags                      : ${OBJCXXFLAGS:--}
  Symbol Visibility C Compile flags                : ${VISIBILITY_CFLAGS:--}
  Symbol Visibility C++ Compile flags              : ${VISIBILITY_CXXFLAGS:--}
  Symbol Visibility Objective C Compile flags      : ${VISIBILITY_OBJCFLAGS:--}
  Symbol Visibility Objective C++ Compile flags    : ${VISIBILITY_OBJCXXFLAGS:--}
  Link flags                                       : ${LDFLAGS:--}
  Link libraries                                   : ${LIBS}
  Fuzzing Enabled                                  : ${enable_fuzzing}
])
