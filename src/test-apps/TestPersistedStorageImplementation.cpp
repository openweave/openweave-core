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
 *      Implementation of a simple std::map based persisted store.
 *
 */

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "ToolCommon.h"
#include <Weave/Support/Base64.h>
#include <Weave/Support/CodeUtils.h>
#include <Weave/Support/platform/PersistedStorage.h>

#include "TestPersistedStorageImplementation.h"

std::map<std::string, std::string> sPersistentStore;

FILE *sPersistentStoreFile = NULL;

namespace nl {
namespace Weave {
namespace Platform {
namespace PersistedStorage {

static void RemoveEndOfLineSymbol(char *str)
{
    size_t len = strlen(str) - 1;
    if (str[len] == '\n')
        str[len] = '\0';
}

static WEAVE_ERROR GetCounterValueFromFile(const char *aKey, uint32_t &aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    char key[WEAVE_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH];
    char value[WEAVE_CONFIG_PERSISTED_STORAGE_MAX_VALUE_LENGTH];

    rewind(sPersistentStoreFile);

    while (fgets(key, sizeof(key), sPersistentStoreFile) != NULL)
    {
        RemoveEndOfLineSymbol(key);

        if (strcmp(key, aKey) == 0)
        {
            if (fgets(value, sizeof(value), sPersistentStoreFile) == NULL)
            {
                err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL;
            }
            else
            {
                RemoveEndOfLineSymbol(value);

                if (!ParseInt(value, aValue, 0))
                    err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL;
            }

            ExitNow();
        }
    }

    err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND;

exit:
    return err;
}

static WEAVE_ERROR SaveCounterValueToFile(const char *aKey, uint32_t aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    int res;
    char key[WEAVE_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH];
    char value[WEAVE_CONFIG_PERSISTED_STORAGE_MAX_VALUE_LENGTH];

    snprintf(value, sizeof(value), "0x%08X\n", aValue);

    rewind(sPersistentStoreFile);

    // Find the stored counter value location in the file.
    while (fgets(key, sizeof(key), sPersistentStoreFile) != NULL)
    {
        RemoveEndOfLineSymbol(key);

        // If value is found in the file then override it.
        if (strcmp(key, aKey) == 0)
        {
            res = fputs(value, sPersistentStoreFile);
            VerifyOrExit(res != EOF, err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

            ExitNow();
        }
    }

    // If value not found in the file then write the counter key and
    // the counter value to the end of the file.
    res = fputs(aKey, sPersistentStoreFile);
    VerifyOrExit(res != EOF, err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

    res = fputs("\n", sPersistentStoreFile);
    VerifyOrExit(res != EOF, err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

    res = fputs(value, sPersistentStoreFile);
    VerifyOrExit(res != EOF, err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);

exit:
    fflush(sPersistentStoreFile);
    return err;
}

WEAVE_ERROR Read(const char *aKey, uint32_t &aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    std::map<std::string, std::string>::iterator it;

    VerifyOrExit(aKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(strlen(aKey) <= WEAVE_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH,
                 err = WEAVE_ERROR_INVALID_STRING_LENGTH);

    if (sPersistentStoreFile)
    {
        err = GetCounterValueFromFile(aKey, aValue);
    }
    else
    {
        it = sPersistentStore.find(aKey);
        VerifyOrExit(it != sPersistentStore.end(), err = WEAVE_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND);

        size_t aValueLength = Base64Decode(it->second.c_str(), strlen(it->second.c_str()), (uint8_t *)&aValue);
        VerifyOrExit(aValueLength == sizeof(uint32_t), err = WEAVE_ERROR_PERSISTED_STORAGE_FAIL);
    }

exit:
    return err;
}

WEAVE_ERROR Write(const char *aKey, uint32_t aValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    VerifyOrExit(aKey != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    VerifyOrExit(strlen(aKey) <= WEAVE_CONFIG_PERSISTED_STORAGE_MAX_KEY_LENGTH,
                 err = WEAVE_ERROR_INVALID_STRING_LENGTH);

    if (sPersistentStoreFile)
    {
        err = SaveCounterValueToFile(aKey, aValue);
    }
    else
    {
        char encodedValue[2 * sizeof(uint32_t)];

        memset(encodedValue, 0, sizeof(encodedValue));
        Base64Encode((uint8_t *)&aValue, sizeof(aValue), encodedValue);

        sPersistentStore[aKey] = encodedValue;
    }

exit:
    return err;
}

} // PersistentStorage
} // Platform
} // Weave
} // nl
