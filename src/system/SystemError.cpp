/*
 *
 *    Copyright (c) 2016-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file contains free functions for mapping OS and LwIP
 *      stack-specific errors into Weave System Layer-specific errors
 *      and for converting those mapped errors into descriptive
 *      error strings.
 */

// Include module header
#include <SystemLayer/SystemError.h>

// Include common private header
#include "SystemLayerPrivate.h"

// Include local headers
#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#include <lwip/err.h>
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

#if !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS
#include <string.h>
#endif // !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS

#include <Weave/Support/NLDLLUtil.h>

#if !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS

/**
 *  @def WEAVE_SYSTEM_POSIX_ERROR_MIN
 *
 *  @brief
 *      This defines the base or minimum Weave System Layer error number range, when passing through errors from an underlying
 *      POSIX layer.
 */
#define WEAVE_SYSTEM_POSIX_ERROR_MIN 2000

/**
 *  @def WEAVE_SYSTEM_POSIX_ERROR_MAX
 *
 *  @brief
 *      This defines the base or maximum Weave System Layer error number range, when passing through errors from an underlying
 *      POSIX layer.
 */
#define WEAVE_SYSTEM_POSIX_ERROR_MAX 2999

#endif // !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#if !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_LWIP_ERROR_FUNCTIONS

/**
 *  @def WEAVE_SYSTEM_LWIP_ERROR_MIN
 *
 *  @brief
 *      This defines the base or minimum Weave System Layer error number range, when passing through errors from an underlying LWIP
 *      stack.
 */
#define WEAVE_SYSTEM_LWIP_ERROR_MIN 3000

/**
 *  @def WEAVE_SYSTEM_LWIP_ERROR_MAX
 *
 *  @brief
 *      This defines the base or maximum Weave System Layer error number range, when passing through errors from an underlying LWIP
 *      layer.
 */
#define WEAVE_SYSTEM_LWIP_ERROR_MAX 3999

#endif // !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_LWIP_ERROR_FUNCTIONS
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

namespace nl {
namespace Weave {
namespace System {

#if !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS
/**
 * This implements a mapping function for Weave System Layer errors that allows mapping integers in the number space of the
 * underlying POSIX network and OS stack errors into a platform- or system-specific range. Error codes beyond those currently
 * defined by POSIX or the ISO C/C++ standards are mapped similar to the standard ones.
 *
 *  @param[in] e  The POSIX network or OS error to map.
 *
 *  @return The mapped POSIX network or OS error.
 */
NL_DLL_EXPORT Error MapErrorPOSIX(int aError)
{
    return (aError == 0 ? WEAVE_SYSTEM_NO_ERROR : WEAVE_SYSTEM_POSIX_ERROR_MIN + aError);
}

/**
 * This implements a function to return an NULL-terminated OS-specific descriptive C string, associated with the specified, mapped
 * OS error.
 *
 *  @param[in] err  The mapped OS-specific error to describe.
 *
 *  @return A NULL-terminated, OS-specific descriptive C string describing the error.
 */
NL_DLL_EXPORT const char *DescribeErrorPOSIX(Error aError)
{
    const int lError = (aError - WEAVE_SYSTEM_POSIX_ERROR_MIN);
    return strerror(lError);
}

/**
 * This implements an introspection function for Weave System Layer errors that allows the caller to determine whether the
 * specified error is an internal, underlying OS error.
 *
 *  @param[in] err  The mapped error to determine whether it is an OS error.
 *
 *  @return True if the specified error is an OS error; otherwise, false.
 */
NL_DLL_EXPORT bool IsErrorPOSIX(Error aError)
{
    return (aError >= WEAVE_SYSTEM_POSIX_ERROR_MIN && aError <= WEAVE_SYSTEM_POSIX_ERROR_MAX);
}
#endif // !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_POSIX_ERROR_FUNCTIONS

#if WEAVE_SYSTEM_CONFIG_USE_LWIP
#if !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_LWIP_ERROR_FUNCTIONS

/**
 * This implements a mapping function for Weave System Layer errors that allows mapping underlying LwIP network stack errors into a
 * platform- or system-specific range.
 *
 *  @param[in] e  The LwIP error to map.
 *
 *  @return The mapped LwIP network or OS error.
 *
 */
NL_DLL_EXPORT Error MapErrorLwIP(err_t aError)
{
    return (aError == ERR_OK ? WEAVE_SYSTEM_NO_ERROR : WEAVE_SYSTEM_LWIP_ERROR_MIN - aError);
}

/**
 * This implements a function to return an NULL-terminated LwIP-specific descriptive C string, associated with the specified,
 * mapped LwIP error.
 *
 *  @param[in] err  The mapped LwIP-specific error to describe.
 *
 *  @return A NULL-terminated, LwIP-specific descriptive C string describing the error.
 *
 */
NL_DLL_EXPORT const char* DescribeErrorLwIP(Error aError)
{
    const err_t lError = -((aError) - WEAVE_SYSTEM_LWIP_ERROR_MIN);

    // If we are not compiling with LWIP_DEBUG asserted, the unmapped
    // local value may go unused.

    (void)lError;

    return lwip_strerr(lError);
}

/**
 * This implements an introspection function for Weave System Layer errors that
 * allows the caller to determine whether the specified error is an
 * internal, underlying LwIP error.
 *
 *  @param[in] err  The mapped error to determine whether it is a LwIP error.
 *
 *  @return True if the specified error is a LwIP error; otherwise, false.
 *
 */
NL_DLL_EXPORT bool IsErrorLwIP(Error aError)
{
    return (aError >= WEAVE_SYSTEM_LWIP_ERROR_MIN && aError <= WEAVE_SYSTEM_LWIP_ERROR_MAX);
}

#endif // !WEAVE_SYSTEM_CONFIG_PLATFORM_PROVIDES_LWIP_ERROR_FUNCTIONS
#endif // WEAVE_SYSTEM_CONFIG_USE_LWIP

} // namespace System
} // namespace Weave
} // namespace nl
