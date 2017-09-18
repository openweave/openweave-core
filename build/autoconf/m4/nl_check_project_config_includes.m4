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
#      This file defines a GNU autoconf M4-style macro for checking
#      project-specific header search paths.
#

#
# NL_CHECK_PROJECT_CONFIG_INCLUDES(option, header, description, default)
#
#   option      - The stem of the option advertised via AC_ARG_WITH.
#   header      - The project-specific configuration header to check for.
#   description - The short description of the project-specific configuration
#                 header.
#   default     - [optional] The default location to search for project-specific
#                 configuration headers.
#
# Create a configuration option that allows the user to specify the place to
# search for the named project-specific configuration header.
# ----------------------------------------------------------------------------
AC_DEFUN([NL_CHECK_PROJECT_CONFIG_INCLUDES],
[
AC_ARG_WITH($1,
    [AS_HELP_STRING([--with-$1=DIR],[Specify $3 project-specific configuration header ($2) search directory @<:@default=$4@:>@.])],
    [
        if test "x${withval}" != "x"; then
            if test -d "${withval}/include"; then
                _nl_tmp_includes="${withval}/include"
            else
                _nl_tmp_includes="${withval}"
            fi

            CPPFLAGS="-I${_nl_tmp_includes} ${CPPFLAGS}"

            AC_CHECK_HEADERS([$2],
            [
                # Accumulate a list of the project config files that have been selected.
                NL_PROJECT_CONFIG_INCLUDES="$NL_PROJECT_CONFIG_INCLUDES ${_nl_tmp_includes}/$2"
            ],
            [
                AC_MSG_ERROR([$3 project-specific configuration was requested, but "$ac_header" could not be found in "${_nl_tmp_includes}".])
            ])
        fi
    ],
    [
        # If default location specified...
        if test "x$4" != "x"; then
        
            # Add it to the include paths.
            CPPFLAGS="-I$4 ${CPPFLAGS}"

            # Check for specified header, but do not fail if not found.
            AC_CHECK_HEADERS([$2],
            [
                # Accumulate a list of the project config files that have been selected.
                NL_PROJECT_CONFIG_INCLUDES="$NL_PROJECT_CONFIG_INCLUDES $4/$2"
            ])
        fi
    ])
])

# A list of the project configuration headers that have been selected
# for inclusion.
#
AC_SUBST(NL_PROJECT_CONFIG_INCLUDES)
