#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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
#      This file defines a GNU autoconf M4-style macro for the presence
#      of the LwIP package.
#

#
# NL_WITH_LWIP
#
# ----------------------------------------------------------------------------
AC_DEFUN([NL_WITH_LWIP],
[
    # Influential external variables for the package support

    AC_ARG_VAR(LWIP_CPPFLAGS, [LwIP C preprocessor flags])
    AC_ARG_VAR(LWIP_LDFLAGS,  [LwIP linker flags])
    AC_ARG_VAR(LWIP_LIBS,     [LwIP linker libraries])

    # Allow the user to specify both external headers and libraries
    # together (or internal).

    AC_ARG_WITH(lwip,
	AS_HELP_STRING([--with-lwip=DIR],
		   [Specify location of the LwIP headers and libraries @<:@default=internal@:>@.]),
	[
	    if test "${withval}" = "no"; then
		AC_MSG_ERROR([${PACKAGE_NAME} requires the LwIP package])
	    elif test "${withval}" = "internal"; then
		lwip_dir=${withval}
		nl_with_lwip=${withval}
	    else
		lwip_dir=${withval}
		nl_with_lwip=external
	    fi
	],
	[lwip_dir=; nl_with_lwip=internal])

    # Allow users to specify external headers and libraries independently.

    AC_ARG_WITH(lwip-includes,
	AS_HELP_STRING([--with-lwip-includes=DIR],
		   [Specify location of LwIP headers @<:@default=internal@:>@.]),
	[
	    if test "${withval}" = "no"; then
		AC_MSG_ERROR([${PACKAGE_NAME} requires the LwIP package])
	    fi

	    if test "x${lwip_dir}" != "x"; then
		AC_MSG_WARN([overriding --with-lwip=${lwip_dir}])
	    fi

	    if test "${withval}" = "internal"; then
		lwip_header_dir=${withval}
		nl_with_lwip=${withval}
	    else
		lwip_header_dir=${withval}
		nl_with_lwip=external
	    fi
	],
	[
	    lwip_header_dir=;
	    if test "x${nl_with_lwip}" = "x"; then
		nl_with_lwip=internal
	    fi
	])

    AC_ARG_WITH(lwip-libs,
	AS_HELP_STRING([--with-lwip-libs=DIR],
		   [Specify location of LwIP libraries @<:@default=internal@:>@.]),
	[
	    if test "${withval}" = "no"; then
		AC_MSG_ERROR([${PACKAGE_NAME} requires the LwIP package])
	    fi

	    if test "x${lwip_dir}" != "x"; then
		AC_MSG_WARN([overriding --with-lwip=${lwip_dir}])
	    fi

	    if test "${withval}" = "internal"; then
		lwip_library_dir=${withval}
		nl_with_lwip=${withval}
	    else
		lwip_library_dir=${withval}
		nl_with_lwip=external
	    fi
	],
	[
	    lwip_library_dir=;
	    if test "x${nl_with_lwip}" = "x"; then
		nl_with_lwip=internal
	    fi
	])

    AC_MSG_CHECKING([source of the LwIP package])
    AC_MSG_RESULT([${nl_with_lwip}])

    # If the user has selected or has defaulted into the internal LwIP
    # package, set the values appropriately. Otherwise, run through the
    # usual routine.

    if test "${nl_with_lwip}" = "internal"; then
	# LwIP also requires arch/{cc,perf,sys_arch}.h. For an
	# external LwIP, the user is expected to pass in CPP/CFLAGS
	# that point to these. However, for the internal LwIP package,
	# we need to explicitly point at our definition for these.

	LWIP_CPPFLAGS="-I${ac_abs_confdir}/third_party/lwip/repo/lwip/src/include -I${ac_abs_confdir}/third_party/lwip/repo/lwip/src/include/ipv4 -I${ac_abs_confdir}/third_party/lwip/repo/lwip/src/include/ipv6 -I${ac_abs_confdir}/src/lwip"
	LWIP_LDFLAGS="-L${ac_pwd}/src/lwip"
	LWIP_LIBS="-llwip -lpthread"

    # On Darwin, "common" symbols are not added to the table of contents
    # by ranlib and so can cause link time errors. This option 
    # offers a workaround.
    case "${target_os}" in
        *darwin*)
            LWIP_CPPFLAGS="${LWIP_CPPFLAGS} -fno-common"
            ;;
    esac
    
    else
	# We always prefer checking the values of the various '--with-lwip-...' 
	# options first to using pkg-config because the former might be used
	# in a cross-compilation environment on a system that also contains
	# pkg-config. In such a case, the user wants what he/she specified
	# rather than what pkg-config indicates.

	if test "x${lwip_dir}" != "x" -o "x${lwip_header_dir}" != "x" -o "x${lwip_library_dir}" != "x"; then
	    if test "x${lwip_dir}" != "x"; then
		if test -d "${lwip_dir}"; then
		    if test -d "${lwip_dir}/include"; then
			LWIP_CPPFLAGS="-I${lwip_dir}/include -I${lwip_dir}/include/ipv4 -I${lwip_dir}/include/ipv6"
		    else
			LWIP_CPPFLAGS="-I${lwip_dir} -I${lwip_dir}/ipv4 -I${lwip_dir}/ipv6"
		    fi

		    if test -d "${lwip_dir}/lib"; then
			LWIP_LDFLAGS="-L${lwip_dir}/lib"
		    else
			LWIP_LDFLAGS="-L${lwip_dir}"
		    fi
		else
		    AC_MSG_ERROR([No such directory ${lwip_dir}])
		fi
	    fi

	    if test "x${lwip_header_dir}" != "x"; then
		if test -d "${lwip_header_dir}"; then
		    LWIP_CPPFLAGS="-I${lwip_header_dir} -I${lwip_header_dir}/ipv4 -I${lwip_header_dir}/ipv6"
		else
		    AC_MSG_ERROR([No such directory ${lwip_header_dir}])
		fi
	    fi

	    if test "x${lwip_library_dir}" != "x"; then
		if test -d "${lwip_library_dir}"; then
		    LWIP_LDFLAGS="-L${lwip_library_dir}"
		else
		    AC_MSG_ERROR([No such directory ${lwip_library_dir}])
		fi
	    fi

	    LWIP_LIBS="${LWIP_LDFLAGS} -llwip"

	elif test "x${PKG_CONFIG}" != "x" -a "${PKG_CONFIG} --exists lwip"; then
	    LWIP_CPPFLAGS="`${PKG_CONFIG} --cflags lwip`"
	    LWIP_LDFLAGS="`${PKG_CONFIG} --libs-only-L lwip`"
	    LWIP_LIBS="`${PKG_CONFIG} --libs-only-l lwip`"

	else
	    AC_MSG_ERROR([Cannot find the LwIP package.])
	fi
    fi

    AC_SUBST(LWIP_CPPFLAGS)
    AC_SUBST(LWIP_LDFLAGS)
    AC_SUBST(LWIP_LIBS)

    nl_saved_CPPFLAGS="${CPPFLAGS}"
    nl_saved_LDFLAGS="${LDFLAGS}"
    nl_saved_LIBS="${LIBS}"

    CPPFLAGS="${CPPFLAGS} ${LWIP_CPPFLAGS}"
    LDFLAGS="${LDFLAGS} ${LWIP_LDFLAGS}"
    LIBS="${LIBS} ${LWIP_LIBS}"

    # LwIP does a miserable job of making its headers work
    # standalone. Check the two below, standalone, then check the
    # rest.

    AC_CHECK_HEADERS([lwip/debug.h] [lwip/err.h],
    [],
    [
        AC_MSG_ERROR(The LwIP header "$ac_header" is required but cannot be found.)
    ])

    AC_CHECK_HEADERS([lwip/dns.h] [lwip/ethip6.h] [lwip/init.h] [lwip/ip_addr.h] [lwip/ip.h] [lwip/mem.h] [lwip/netif.h] [lwip/opt.h] [lwip/pbuf.h] [lwip/raw.h] [lwip/snmp.h] [lwip/stats.h] [lwip/sys.h] [lwip/tcp.h] [lwip/tcpip.h] [lwip/udp.h],
    [],
    [
        AC_MSG_ERROR(The LwIP header "$ac_header" is required but cannot be found.)
    ],
    [#include <lwip/err.h>
     #include <lwip/ip4_addr.h>])

    CPPFLAGS="${nl_saved_CPPFLAGS}"
    LDFLAGS="${nl_saved_LDFLAGS}"
    LIBS="${nl_saved_LIBS}"
])
