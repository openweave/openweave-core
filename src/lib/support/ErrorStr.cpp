/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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
 *      This file implements functions to translate error codes used
 *      throughout the Weave package into human-readable strings.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdio.h>

#include <Weave/Core/WeaveCore.h>
#include <Weave/Support/ASN1.h>
#include <Weave/Support/CodeUtils.h>

namespace nl {

/**
 * Static buffer to store the formatted error string.
 */
static char sErrorStr[WEAVE_CONFIG_ERROR_STR_SIZE];

static const ErrorFormatter sASN1ErrorFormatter =
{
    nl::Weave::ASN1::FormatASN1Error,
    NULL
};

static const ErrorFormatter sWeaveErrorFormatter =
{
    nl::Weave::FormatWeaveError,
    &sASN1ErrorFormatter
};

/**
 * Linked-list of error formatter functions.
 */
static const ErrorFormatter * sErrorFormatterList = &sWeaveErrorFormatter;

/**
 * This routine returns a human-readable NULL-terminated C string
 * describing the provided error.
 *
 * @param[in] err                      The error for format and describe.
 *
 * @return A pointer to a NULL-terminated C string describing the
 *         provided error.
 */
NL_DLL_EXPORT const char * ErrorStr(int32_t err)
{
    if (err == 0)
    {
        return "No Error";
    }

    // Search the registered error formatter for one that will format the given
    // error code.
    for (const ErrorFormatter * errFormatter = sErrorFormatterList;
         errFormatter != NULL;
         errFormatter = errFormatter->Next)
    {
        if (errFormatter->FormatError(sErrorStr, sizeof(sErrorStr), err))
        {
            return sErrorStr;
        }
    }

    // Use a default formatting if no formatter found.
    FormatError(sErrorStr, sizeof(sErrorStr), NULL, err, NULL);
    return sErrorStr;
}

/**
 * Add a new error formatter function to the global list of error formatters.
 *
 * @param[in] errFormatter             An ErrorFormatter structure containing a
 *                                     pointer to the new error function.  Note
 *                                     that a pointer to the supplied ErrorFormatter
 *                                     structure will be retained by the function.
 *                                     Thus the memory for the structure must
 *                                     remain reserved.
 */
NL_DLL_EXPORT void RegisterErrorFormatter(ErrorFormatter * errFormatter)
{
    // Do nothing if a formatter with the same format function is already in the list.
    for (const ErrorFormatter * existingFormatter = sErrorFormatterList;
         existingFormatter != NULL;
         existingFormatter = existingFormatter->Next)
    {
        if (existingFormatter->FormatError == errFormatter->FormatError)
        {
            return;
        }
    }

    // Add the formatter to the global list.
    errFormatter->Next = sErrorFormatterList;
    sErrorFormatterList = errFormatter;
}


#if !WEAVE_CONFIG_CUSTOM_ERROR_FORMATTER

/**
 * Generates a human-readable NULL-terminated C string describing the provided error.
 *
 * @param[in] buf                   Buffer into which the error string will be placed.
 * @param[in] bufSize               Size of the supplied buffer in bytes.
 * @param[in] subsys                A short string describing the subsystem that originated
 *                                  the error, or NULL if the origin of the error is
 *                                  unknown/unavailable.  This string should be 10
 *                                  characters or less.
 * @param[in] err                   The error to be formatted.
 * @param[in] desc                  A string describing the cause or meaning of the error,
 *                                  or NULL if no such information is available.
 */
NL_DLL_EXPORT void FormatError(char * buf, uint16_t bufSize, const char * subsys, int32_t err, const char * desc)
{
#if WEAVE_CONFIG_SHORT_ERROR_STR

    if (subsys == NULL)
    {
        (void)snprintf(buf, bufSize, "Error " WEAVE_CONFIG_SHORT_FORM_ERROR_VALUE_FORMAT, err);
    }
    else
    {
        (void)snprintf(buf, bufSize, "Error %s:" WEAVE_CONFIG_SHORT_FORM_ERROR_VALUE_FORMAT, subsys, err);
    }

#else // WEAVE_CONFIG_SHORT_ERROR_STR

    int len = 0;

    if (subsys != NULL)
    {
        len = snprintf(buf, bufSize, "%s ", subsys);
    }

    len += snprintf(buf + len, bufSize - len, "Error %" PRId32 " (0x%08" PRIX32 ")", err, (uint32_t)err);

    if (desc != NULL)
    {
        (void)snprintf(buf + len, bufSize - len, ": %s", desc);
    }

#endif // WEAVE_CONFIG_SHORT_ERROR_STR
}

#endif // WEAVE_CONFIG_CUSTOM_ERROR_FORMATTER

} // namespace nl
