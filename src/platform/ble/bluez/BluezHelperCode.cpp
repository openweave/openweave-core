/*
 *
 *    Copyright (c) 2017 Nest Labs, Inc.
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
 *      This file provides Weave over Bluez Peripheral implementation, which uses Bluez DBUS APIs.
 *
 */

#include "BluezHelperCode.h"
#include "BluezBlePlatformDelegate.h"
#include "WoBluez.h"

#if CONFIG_BLE_PLATFORM_BLUEZ

namespace nl {
namespace Ble {
namespace Platform {
namespace BlueZ {

BluezServerEndpoint * gBluezServerEndpoint           = NULL;
BluezBlePlatformDelegate * gBluezBlePlatformDelegate = NULL;
static GMainLoop * gBluezMainLoop;
static DBusConnection * gBluezDbusConn;
static Adapter * gDefaultAdapter;
static bool gAdapterFound = false;

static void WeaveRegisterSetup(DBusMessageIter * iter, void * bluezData)
{
    DBusMessageIter dict;
    const char * path = ADVERTISING_PATH;
    gboolean success  = FALSE;
    const char * msg  = NULL;

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in WeaveRegisterSetup");

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                               DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                                   DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                               &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in WeaveRegisterSetup");

    success = dbus_message_iter_close_container(iter, &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in WeaveRegisterSetup");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }
}

static void WeaveRegisterReply(DBusMessage * message, void * bluezData)
{
    DBusConnection * dbusConn = static_cast<DBusConnection *>(bluezData);
    DBusError error;
    dbus_error_init(&error);

    if (TRUE == dbus_set_error_from_message(&error, message))
    {
        WeaveLogError(Ble, "Fail to register weave advertisement in WeaveRegisterReply: %s", error.name);
        dbus_error_free(&error);

        if (FALSE == g_dbus_unregister_interface(dbusConn, ADVERTISING_PATH, ADVERTISING_INTERFACE))
        {
            WeaveLogError(Ble, "Fail to unregister weave advertisement in WeaveRegisterReply");
        }
    }
    else
    {
        WeaveLogProgress(Ble, "Weave advertisement object registered");
    }
}

static gboolean WeaveAdvertisingGetType(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    gboolean success = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &(gBluezServerEndpoint->advertisingType));
    if (FALSE == success)
    {
        WeaveLogError(Ble, "Fail to get advertising type in WeaveAdvertisingGetType");
    }

    return success;
}

static gboolean GetWeaveUUIDs(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    const char * msg = NULL;
    gboolean success = FALSE;
    DBusMessageIter dbusArray;

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "as", &dbusArray);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in GetWeaveUUIDs");

    success = dbus_message_iter_append_basic(&dbusArray, DBUS_TYPE_STRING, &(gBluezServerEndpoint->advertisingUUID));
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in GetWeaveUUIDs");

    success = dbus_message_iter_close_container(iter, &dbusArray);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in GetWeaveUUIDs");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean WeaveServiceDataCheck(const GDBusPropertyTable * property, void * bluezData)
{
    gboolean success = FALSE;

    if (NULL != gBluezServerEndpoint->weaveServiceData)
    {
        success = TRUE;
    }

    return success;
}

static gboolean AppendArrayVariant(DBusMessageIter * iter, int type, void * val, int nElements)
{
    const char * msg = NULL;
    gboolean success = FALSE;
    DBusMessageIter variant, array;
    const char *** strArray = (const char ***) val;
    int i;
    char typeSig[2]  = { (char) type, '\0' };
    char arraySig[3] = { DBUS_TYPE_ARRAY, (char) type, '\0' };

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, arraySig, &variant);
    VerifyOrExit(success == TRUE, msg = "Fail to open DBUS_TYPE_VARIANT container in AppendArrayVariant");

    success = dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, typeSig, &array);
    VerifyOrExit(success == TRUE, msg = "Fail to open DBUS_TYPE_ARRAY container in AppendArrayVariant");

    if (dbus_type_is_fixed(type) == TRUE)
    {
        success = dbus_message_iter_append_fixed_array(&array, type, val, nElements);
        VerifyOrExit(success == TRUE, msg = "Fail to append fixed array in AppendArrayVariant");
    }
    else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH)
    {
        for (i = 0; i < nElements; i++)
        {
            success = dbus_message_iter_append_basic(&array, type, &((*strArray)[i]));
            VerifyOrExit(success == TRUE, msg = "Fail to append basic in AppendArrayVariant");
        }
    }

    success = dbus_message_iter_close_container(&variant, &array);
    VerifyOrExit(success == TRUE, msg = "Fail to close DBUS_TYPE_ARRAY container in AppendArrayVariant");

    success = dbus_message_iter_close_container(iter, &variant);
    VerifyOrExit(success == TRUE, msg = "Fail to close DBUS_TYPE_VARIANT container in AppendArrayVariant");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean DictAppendBasicArray(DBusMessageIter * dict, int keyType, const void * key, int type, void * val, int nElements)
{
    const char * msg = NULL;
    gboolean success = FALSE;
    DBusMessageIter entry;

    success = dbus_message_iter_open_container(dict, DBUS_TYPE_DICT_ENTRY, NULL, &entry);
    VerifyOrExit(success == TRUE, msg = "Fail to open DBUS_TYPE_DICT_ENTRY container in DictAppendBasicArray");

    success = dbus_message_iter_append_basic(&entry, keyType, key);
    VerifyOrExit(success == TRUE, msg = "Fail to append key in DictAppendBasicArray");

    success = AppendArrayVariant(&entry, type, val, nElements);
    VerifyOrExit(success == TRUE, msg = "Fail to append array variant in DictAppendBasicArray");

    success = dbus_message_iter_close_container(dict, &entry);
    VerifyOrExit(success == TRUE, msg = "Fail to close DBUS_TYPE_DICT_ENTRY container in DictAppendBasicArray");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean GetWeaveServiceData(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    const char * msg = NULL;
    gboolean success = FALSE;
    DBusMessageIter dict;

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "{sv}", &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to open DBUS_TYPE_ARRAY container in GetWeaveServiceData");

    success = DictAppendBasicArray(&dict, DBUS_TYPE_STRING, &gBluezServerEndpoint->advertisingUUID, DBUS_TYPE_BYTE,
                                   &gBluezServerEndpoint->weaveServiceData, sizeof(WeaveServiceData));
    VerifyOrExit(success == TRUE, msg = "Fail to append dictionary in GetWeaveServiceData");

    success = dbus_message_iter_close_container(iter, &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to close DBUS_TYPE_ARRAY container in GetWeaveServiceData");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean WeaveNameCheck(const GDBusPropertyTable * property, void * bluezData)
{
    gboolean success = FALSE;

    if (NULL != gBluezServerEndpoint->adapterName)
    {
        success = TRUE;
    }

    return success;
}

static gboolean WeaveGetName(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    gboolean success = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &(gBluezServerEndpoint->adapterName));

    if (FALSE == success)
    {
        WeaveLogError(Ble, "Fail to get Weave Local name in WeaveGetName");
    }

    return success;
}

static DBusMessage * WeaveDestroyAdvertising(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    if (FALSE == g_dbus_unregister_interface(dbusConn, ADVERTISING_PATH, ADVERTISING_INTERFACE))
    {
        WeaveLogError(Ble, "Fail to destroy advertising object in WeaveDestroyAdvertising");
    }

    return dbus_message_new_method_return(dbusMsg);
}

static const GDBusMethodTable weaveAdvertisingMethods[] = {
    { "Release", WeaveDestroyAdvertising, (GDBusMethodFlags) 0, 0, NULL, NULL }, { }
};

static const GDBusPropertyTable weaveAdvertisingProperties[] = { { "Type", "s", WeaveAdvertisingGetType },
                                                                 { "ServiceUUIDs", "as", GetWeaveUUIDs, NULL, NULL },
                                                                 { "LocalName", "s", WeaveGetName, NULL, WeaveNameCheck },
                                                                 { "ServiceData", "a{sv}", GetWeaveServiceData, NULL,
                                                                   WeaveServiceDataCheck },
                                                                 { } };

gboolean AdvertisingRegister(DBusConnection * dbusConn, GDBusProxy * proxy)
{
    gboolean success = FALSE;
    const char * msg = NULL;

    success = g_dbus_register_interface(dbusConn, ADVERTISING_PATH, ADVERTISING_INTERFACE, weaveAdvertisingMethods, NULL,
                                        weaveAdvertisingProperties, NULL, NULL);
    VerifyOrExit(success == TRUE, msg = "Failed to register advertising object in AdvertisingRegister");

    success = g_dbus_proxy_method_call(proxy, "RegisterAdvertisement", WeaveRegisterSetup, WeaveRegisterReply, dbusConn, NULL);
    VerifyOrExit(success == TRUE, msg = "Failed to call RegisterAdvertisement in AdvertisingRegister");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static DBusMessage * WeaveDestroyProfile(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    if (FALSE == g_dbus_unregister_interface(dbusConn, WEAVE_PATH, PROFILE_INTERFACE))
    {
        WeaveLogError(Ble, "Failed to destroy advertising object in WeaveDestroyProfile");
    }

    return dbus_message_new_method_return(dbusMsg);
}

static void RegisterWeaveAppSetup(DBusMessageIter * iter, void * bluezData)
{
    DBusMessageIter dict;
    const char * path = "/";
    gboolean success  = FALSE;
    const char * msg  = NULL;

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in RegisterWeaveAppSetup");

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY,
                                               DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING
                                                   DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING,
                                               &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in RegisterWeaveAppSetup");

    success = dbus_message_iter_close_container(iter, &dict);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in RegisterWeaveAppSetup");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }
}

static void RegisterWeaveAppReply(DBusMessage * message, void * bluezData)
{
    DBusError error;
    dbus_error_init(&error);

    if (TRUE == dbus_set_error_from_message(&error, message))
    {
        WeaveLogError(Ble, "Failed to setup weave application in RegisterWeaveAppReply: %s", error.name);
        dbus_error_free(&error);
    }
}

static const GDBusMethodTable weaveAppMethods[] = { { "Release", WeaveDestroyProfile, GDBusMethodFlags(0), 0, NULL, NULL }, { } };

static const GDBusPropertyTable weaveAppProperties[] = { { "UUIDs", "as", GetWeaveUUIDs }, { } };

gboolean SetupWeaveApp(DBusConnection * dbusConn, GDBusProxy * proxy)
{
    gboolean success = FALSE;
    const char * msg = NULL;
    success =
        g_dbus_register_interface(dbusConn, WEAVE_PATH, PROFILE_INTERFACE, weaveAppMethods, NULL, weaveAppProperties, NULL, NULL);
    VerifyOrExit(success == TRUE, msg = "Fail in register interface in SetupWeaveApp");

    success = g_dbus_proxy_method_call(proxy, "RegisterApplication", RegisterWeaveAppSetup, RegisterWeaveAppReply, NULL, NULL);
    if (FALSE == success)
    {
        msg = "Fail to call RegisterApplication in SetupWeaveApp";
        g_dbus_unregister_interface(dbusConn, WEAVE_PATH, PROFILE_INTERFACE);
        ExitNow();
    }

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static void WeaveCharacteristicDestroy(void * bluezData)
{
    Characteristic * WeaveCharacteristic = static_cast<Characteristic *>(bluezData);
    if (NULL != WeaveCharacteristic)
    {
        g_free(WeaveCharacteristic->path);
        g_free(WeaveCharacteristic->servicePath);
        g_free(WeaveCharacteristic->uuid);
        g_strfreev(WeaveCharacteristic->flags);
        g_free(WeaveCharacteristic->value);
        g_free(WeaveCharacteristic);
    }
}

static gboolean WeaveServiceGetUUID(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Service * weaveService = static_cast<Service *>(bluezData);
    gboolean success       = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &weaveService->uuid);
    if (FALSE == success)
    {
        WeaveLogError(Ble, "Failed to get weave service uuid property in WeaveServiceGetUUID");
    }

    return success;
}

static gboolean WeaveServiceGetPrimary(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Service * weaveService = static_cast<Service *>(bluezData);
    dbus_bool_t servicePrimary;
    gboolean success = FALSE;
    const char * msg = NULL;

    VerifyOrExit(weaveService != NULL, msg = "weaveService is NULL in WeaveServiceGetPrimary");

    servicePrimary = weaveService->isPrimary ? TRUE : FALSE;

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &servicePrimary);
    VerifyOrExit(success == TRUE, msg = "Failed to get weave service primary property in WeaveServiceGetPrimary");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static const GDBusPropertyTable serviceProperties[] = { { "UUID", "s", WeaveServiceGetUUID },
                                                        { "Primary", "b", WeaveServiceGetPrimary },
                                                        { } };

static void ServiceDestroy(void * bluezData)
{
    Service * weaveService = static_cast<Service *>(bluezData);
    if (NULL != weaveService)
    {
        g_free(weaveService->path);
        g_free(weaveService->uuid);
        g_free(weaveService);
    }
}

gboolean RegisterWeaveService(DBusConnection * dbusConn)
{
    gboolean success = FALSE;
    const char * msg = NULL;
    Service * weaveService;
    weaveService = g_new0(Service, 1);

    VerifyOrExit(weaveService != NULL, msg = "weaveService is NULL in RegisterWeaveService");

    weaveService->dbusConn  = dbusConn;
    weaveService->path      = g_strdup_printf("%s/WeaveService%p", WEAVE_PATH, weaveService);
    weaveService->isPrimary = true;
    weaveService->uuid      = g_strdup(UUID_WEAVE);

    success = g_dbus_register_interface(dbusConn, weaveService->path, SERVICE_INTERFACE, NULL, NULL, serviceProperties,
                                        weaveService, ServiceDestroy);
    if (FALSE == success)
    {
        msg = "Failed to register weave service";
        ServiceDestroy(weaveService);
        weaveService = NULL;
    }

    gBluezServerEndpoint->weaveService = weaveService;

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean CharacteristicGetUUID(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    const char * msg                = NULL;
    gboolean success                = FALSE;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicGetUUID");

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &characteristic->uuid);
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in CharacteristicGetUUID");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean CharacteristicGetService(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    const char * msg                = NULL;
    gboolean success                = FALSE;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicGetService");

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &characteristic->servicePath);
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in CharacteristicGetService");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean CharacteristicGetValue(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    DBusMessageIter array;
    const char * msg = NULL;
    gboolean success = FALSE;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicGetValue");

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "y", &array);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in CharacteristicGetValue");

    success = dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE, &characteristic->value, characteristic->valueLen);
    VerifyOrExit(success == TRUE, msg = "Fail to append array in CharacteristicGetValue");

    success = dbus_message_iter_close_container(iter, &array);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in CharacteristicGetValue");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean CharacteristicGetNotifying(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    dbus_bool_t characteristicNotify;
    const char * msg = NULL;
    gboolean success = FALSE;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicGetNotifying");

    characteristicNotify = characteristic->isNotifying ? TRUE : FALSE;

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &characteristicNotify);
    VerifyOrExit(success == TRUE, msg = "Fail to append basic in CharacteristicGetNotifying");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static gboolean CharacteristicGetFlags(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    int flagIndex                   = 0;
    DBusMessageIter array;
    const char * msg = NULL;
    gboolean success = FALSE;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicGetFlags");

    success = dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, "s", &array);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in CharacteristicGetFlags");

    while (characteristic->flags[flagIndex])
    {
        success = dbus_message_iter_append_basic(&array, DBUS_TYPE_STRING, &characteristic->flags[flagIndex]);
        VerifyOrExit(success == TRUE, msg = "Fail to append array in CharacteristicGetFlags");
        flagIndex++;
    }

    success = dbus_message_iter_close_container(iter, &array);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in CharacteristicGetFlags");

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return success;
}

static DBusMessage * CharacteristicRead(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    DBusMessage * readReply         = NULL;
    DBusMessageIter iter, array;
    const char * msg = NULL;
    gboolean success = FALSE;
    readReply        = g_dbus_create_reply(dbusMsg, DBUS_TYPE_INVALID);
    dbus_message_iter_init_append(readReply, &iter);

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicRead");

    success = dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "y", &array);
    VerifyOrExit(success == TRUE, msg = "Fail to open container in CharacteristicRead");

    success = dbus_message_iter_append_fixed_array(&array, DBUS_TYPE_BYTE, &(characteristic->value), characteristic->valueLen);
    VerifyOrExit(success == TRUE, msg = "Fail to append fixed array in CharacteristicRead");

    success = dbus_message_iter_close_container(&iter, &array);
    VerifyOrExit(success == TRUE, msg = "Fail to close container in CharacteristicRead");

exit:

    if ((success != TRUE) && (NULL != msg))
    {
        WeaveLogError(Ble, msg);
    }

    return readReply;
}

#if BLE_CONFIG_BLUEZ_MTU_FEATURE
static bool WritePipeIORead(struct io * io, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    int fd;
    const char * msg = NULL;
    uint8_t writerData[BUFF_SIZE];
    ssize_t writerDataLength;
    bool success = false;

    VerifyOrExit(bluezData != NULL, msg = "characteristic is NULL in WritePipeIORead");

    if (io == characteristic->writePipeIO)
    {
        fd = io_get_fd(io);
        VerifyOrExit(fd >= 0, msg = "expect file descriptor with non-negatvie value in WritePipeIORead");
    }
    else
    {
        msg = "expect writePipeIO in WritePipeIORead";
        ExitNow();
    }

    writerDataLength = read(fd, writerData, sizeof(writerData));
    VerifyOrExit(writerDataLength >= 0, msg = "writerDataLength should be larger than or equal to 0");

    g_free(characteristic->value);
    characteristic->value    = static_cast<uint8_t *>(g_memdup(writerData, writerDataLength));
    characteristic->valueLen = writerDataLength;

    if (strcmp(characteristic->uuid, UUID_WEAVE_C1) == 0)
    {
        WoBLEz_WriteReceived(gBluezServerEndpoint, characteristic->value, characteristic->valueLen);
        success = true;
    }
    else
    {
        msg = "current uuid is not UUID_WEAVE_C1";
    }

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return success;
}

static bool PipeIODestroy(struct io * io, void * bluezData)
{
    const char * msg                = NULL;
    bool success                    = false;
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);

    VerifyOrExit(bluezData != NULL, msg = "characteristic is NULL in PipeIODestroy");

    if (io == characteristic->indicatePipeIO)
    {
        io_destroy(characteristic->indicatePipeIO);
        characteristic->indicatePipeIO = NULL;
    }
    else
    {
        io_destroy(characteristic->writePipeIO);
        characteristic->writePipeIO = NULL;
    }

    success = true;

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return success;
}

static DBusMessage * CharacteristicCreatePipe(Characteristic * characteristic, DBusMessage * dbusMsg)
{
    int characteristicPipefd[2];
    int fdToClose, fdToUse, ioSelection;
    struct io * io;
    const char * msg                            = NULL;
    DBusMessage * CharacteristicCreatePipeReply = NULL;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicAcquireWrite");

    if (pipe2(characteristicPipefd, O_DIRECT | O_NONBLOCK | O_CLOEXEC) < 0)
    {
        msg                           = strerror(errno);
        CharacteristicCreatePipeReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.Failed", "%s", strerror(errno));
        ExitNow();
    }

    if (TRUE == dbus_message_has_member(dbusMsg, "AcquireWrite"))
    {
        fdToClose   = characteristicPipefd[1];
        fdToUse     = characteristicPipefd[0];
        ioSelection = 1;
    }
    else if (TRUE == dbus_message_has_member(dbusMsg, "AcquireNotify"))
    {
        fdToClose   = characteristicPipefd[0];
        fdToUse     = characteristicPipefd[1];
        ioSelection = 0;
    }
    else
    {
        msg = "dbus message expects member, AcquireWrite or AcquireNotify";
        ExitNow();
    }

    io = io_new(fdToUse);
    if (io == NULL)
    {
        close(fdToClose);
        close(fdToUse);
        msg                           = strerror(errno);
        CharacteristicCreatePipeReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.Failed", "%s", strerror(errno));
        ExitNow();
    }

    io_set_close_on_destroy(io, true);
    io_set_read_handler(io, WritePipeIORead, characteristic, NULL);
    io_set_disconnect_handler(io, PipeIODestroy, characteristic, NULL);

    CharacteristicCreatePipeReply = g_dbus_create_reply(dbusMsg, DBUS_TYPE_UNIX_FD, &fdToClose, DBUS_TYPE_UINT16,
                                                        &gBluezServerEndpoint->mtu, DBUS_TYPE_INVALID);

    close(fdToClose);

    if (ioSelection == 1)
        characteristic->writePipeIO = io;
    else
        characteristic->indicatePipeIO = io;

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    return CharacteristicCreatePipeReply;
}

static DBusMessage * CharacteristicAcquireWrite(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    const char * msg                = NULL;
    const char * key;
    DBusMessageIter iter, dict, value, entry;
    DBusMessage * acquireWriteReply = NULL;
    bool acquireMTU                 = false;
    dbus_bool_t iterCheck;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicAcquireWrite");

    if (characteristic->writePipeIO != NULL)
    {
        msg               = "there exists writePipeIO, error";
        acquireWriteReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.NotPermitted", NULL);
        ExitNow();
    }

    dbus_message_iter_init(dbusMsg, &iter);
    VerifyOrExit(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY,
                 msg = "dbus iterator is not array in CharacteristicAcquireWrite");

    dbus_message_iter_recurse(&iter, &dict);

    while (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_DICT_ENTRY)
    {
        dbus_message_iter_recurse(&dict, &entry);
        dbus_message_iter_get_basic(&entry, &key);
        iterCheck = dbus_message_iter_next(&entry);
        VerifyOrExit(iterCheck == TRUE, msg = "Reach the end of iterator");

        dbus_message_iter_recurse(&entry, &value);

        if (strcasecmp(key, "MTU") == 0)
        {
            dbus_message_iter_get_basic(&value, &gBluezServerEndpoint->mtu);
            acquireMTU = true;
            break;
        }

        dbus_message_iter_next(&dict);
    }

    if (!acquireMTU)
    {
        msg               = "AcquireWite cannot get MTU from bluez";
        acquireWriteReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.InvalidArguments", NULL);
        ExitNow();
    }

    acquireWriteReply = CharacteristicCreatePipe(characteristic, dbusMsg);

    if (characteristic->writePipeIO != NULL)
    {
        if (strcmp(characteristic->uuid, UUID_WEAVE_C1) == 0)
        {
            g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "WriteAcquired");
        }
        else
        {
            msg = "uuid expects UUID_WEAVE_C1";
            ExitNow();
        }
    }

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return acquireWriteReply;
}

static DBusMessage * CharacteristicAcquireNotify(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic  = static_cast<Characteristic *>(bluezData);
    const char * msg                 = NULL;
    DBusMessage * acquireNotifyReply = NULL;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicAcquireNotify");

    if (characteristic->isNotifying)
    {
        msg                = "Notifying has been enabled in CharacteristicAcquireNotify";
        acquireNotifyReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.NotPermitted", NULL);
        ExitNow();
    }

    if (characteristic->indicatePipeIO != NULL)
    {
        msg                = "there exists indicatePipeIO, error";
        acquireNotifyReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.NotPermitted", NULL);
        ExitNow();
    }

    acquireNotifyReply = CharacteristicCreatePipe(characteristic, dbusMsg);

    if (characteristic->indicatePipeIO != NULL)
    {
        characteristic->isNotifying = true;

        WeaveLogProgress(Ble, "Characteristic path %s notification enabled", characteristic->path);

        if (strcmp(characteristic->uuid, UUID_WEAVE_C2) == 0)
        {
            WoBLEz_SubscriptionChange(gBluezServerEndpoint);
        }
        else
        {
            msg = "uuid expects UUID_WEAVE_C2";
            ExitNow();
        }

        g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "Notifying");
        g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "NotifyAcquired");
    }

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    return acquireNotifyReply;
}
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE

static DBusMessage * CharacteristicWrite(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    DBusMessageIter iter;
    dbus_message_iter_init(dbusMsg, &iter);
    const char * msg = NULL;
    DBusMessageIter array;
    DBusMessage * writeReply = NULL;
    uint8_t * writerData     = NULL;
    int writerDataLength;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicWrite");

    if (dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
    {
        msg        = "Fail to get arg type in CharacteristicWrite";
        writeReply = g_dbus_create_error(dbusMsg, "org.bluez.Error.InvalidArguments", NULL);
        ExitNow();
    }

    dbus_message_iter_recurse(&iter, &array);
    dbus_message_iter_get_fixed_array(&array, &(writerData), &(writerDataLength));
    characteristic->value    = static_cast<uint8_t *>(g_memdup(writerData, writerDataLength));
    characteristic->valueLen = writerDataLength;

    g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "Value");

    if (strcmp(characteristic->uuid, UUID_WEAVE_C1) == 0)
    {
        WoBLEz_WriteReceived(gBluezServerEndpoint, characteristic->value, characteristic->valueLen);
    }

    writeReply = g_dbus_create_reply(dbusMsg, DBUS_TYPE_INVALID);

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    return writeReply;
}

static DBusMessage * CharacteristicStartNotify(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    const char * msg                = NULL;
    DBusMessage * notifyReply       = NULL;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicStartNotify");

    VerifyOrExit(!characteristic->isNotifying, msg = "Notifying has been enabled in CharacteristicStartNotify");

    characteristic->isNotifying = true;
    g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "Notifying");
    WeaveLogDetail(Ble, "Characteristic path %s notification enabled", characteristic->path);

    if (strcmp(characteristic->uuid, UUID_WEAVE_C2) == 0)
    {
        WoBLEz_SubscriptionChange(gBluezServerEndpoint);
    }

    notifyReply = g_dbus_create_reply(dbusMsg, DBUS_TYPE_INVALID);

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    return notifyReply;
}

static DBusMessage * CharacteristicStopNotify(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    const char * msg                = NULL;
    DBusMessage * notifyReply       = NULL;

    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicStopNotify");

    VerifyOrExit(characteristic->isNotifying, msg = "Notifying has been disabled in CharacteristicStopNotify");

    characteristic->isNotifying = false;
    g_dbus_emit_property_changed(dbusConn, characteristic->path, CHARACTERISTIC_INTERFACE, "Notifying");
    WeaveLogProgress(Ble, "Characteristic path %s notification disabled", characteristic->path);

    if (strcmp(characteristic->uuid, UUID_WEAVE_C2) == 0)
    {
        WoBLEz_SubscriptionChange(gBluezServerEndpoint);
    }

    notifyReply = g_dbus_create_reply(dbusMsg, DBUS_TYPE_INVALID);

exit:

    if (NULL != msg)
    {
        WeaveLogError(Ble, msg);
    }

    return notifyReply;
}

static DBusMessage * CharacteristicIndicationConf(DBusConnection * dbusConn, DBusMessage * dbusMsg, void * bluezData)
{
    const char * msg                = NULL;
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicIndicationConf");

    WeaveLogDetail(Ble, "Indication confirmation received at %s", characteristic->path);
    WoBLEz_IndicationConfirmation(gBluezServerEndpoint);

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return g_dbus_create_reply(dbusMsg, DBUS_TYPE_INVALID);
}

#if BLE_CONFIG_BLUEZ_MTU_FEATURE
static gboolean CharacteristicPipeAcquired(const GDBusPropertyTable * property, DBusMessageIter * iter, void * bluezData)
{
    gboolean success                = FALSE;
    dbus_bool_t value               = FALSE;
    const char * msg                = NULL;
    Characteristic * characteristic = static_cast<Characteristic *>(bluezData);
    VerifyOrExit(characteristic != NULL, msg = "characteristic is NULL in CharacteristicPipeAcquired");

    if (strcmp(characteristic->uuid, UUID_WEAVE_C1) == 0)
    {
        value = (characteristic->writePipeIO != NULL) ? TRUE : FALSE;
    }
    else if (strcmp(characteristic->uuid, UUID_WEAVE_C2) == 0)
    {
        value = (characteristic->indicatePipeIO != NULL) ? TRUE : FALSE;
    }
    else
    {
        VerifyOrExit(value == TRUE, msg = "writePipeIO or indicatePipeIO is not set in C1 and C2");
    }

    success = dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &value);

exit:

    if (NULL != msg)
    {
        WeaveLogDetail(Ble, msg);
    }

    return success;
}
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE

static const GDBusPropertyTable WeaveCharacteristicProperties[] = {
    { "UUID", "s", CharacteristicGetUUID, NULL, NULL },
    { "Service", "o", CharacteristicGetService, NULL, NULL },
    { "Value", "ay", CharacteristicGetValue, NULL, NULL },
    { "Notifying", "b", CharacteristicGetNotifying, NULL, NULL },
    { "Flags", "as", CharacteristicGetFlags, NULL, NULL },
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    { "WriteAcquired", "b", CharacteristicPipeAcquired, NULL, NULL },
    { "NotifyAcquired", "b", CharacteristicPipeAcquired, NULL, NULL },
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE
    { }
};

static const GDBusMethodTable weaveCharacteristicMethods[] = {
    { "ReadValue", CharacteristicRead, G_DBUS_METHOD_FLAG_ASYNC, 0, GDBUS_ARGS( { "options", "a{sv}" }),
      GDBUS_ARGS( { "value", "ay" }) },
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    { "AcquireWrite", CharacteristicAcquireWrite, G_DBUS_METHOD_FLAG_ASYNC, 0, GDBUS_ARGS( { "options", "a{sv}" }), NULL },
    { "AcquireNotify", CharacteristicAcquireNotify, G_DBUS_METHOD_FLAG_ASYNC, 0, GDBUS_ARGS( { "options", "a{sv}" }), NULL },
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE
    { "WriteValue", CharacteristicWrite, G_DBUS_METHOD_FLAG_ASYNC, 0, GDBUS_ARGS( { "value", "ay" }, { "options", "a{sv}" }),
      NULL },
    { "StartNotify", CharacteristicStartNotify, G_DBUS_METHOD_FLAG_ASYNC, 0, NULL, NULL },
    { "StopNotify", CharacteristicStopNotify, G_DBUS_METHOD_FLAG_ASYNC, 0, NULL, NULL },
    { "Confirm", CharacteristicIndicationConf, G_DBUS_METHOD_FLAG_ASYNC, 0, NULL, NULL },
    { }
};

Characteristic * RegisterWeaveCharacteristic(DBusConnection * dbusConn, const char * uuid, const char * flags)
{
    Characteristic * weaveCharacteristic = NULL;
    gboolean success                     = FALSE;
    const char * msg                     = NULL;

    weaveCharacteristic = g_new0(Characteristic, 1);

    VerifyOrExit(weaveCharacteristic != NULL, msg = "no memory allocated for characteristic in RegisterWeaveCharacteristic");

    weaveCharacteristic->dbusConn = dbusConn;
    weaveCharacteristic->uuid     = g_strdup(uuid);
    weaveCharacteristic->value    = NULL;
    weaveCharacteristic->path =
        g_strdup_printf("%s/weaveCharacteristic%p", gBluezServerEndpoint->weaveService->path, weaveCharacteristic);
    weaveCharacteristic->servicePath = g_strdup_printf("%s", gBluezServerEndpoint->weaveService->path);
    weaveCharacteristic->flags       = g_strsplit(flags, ",", -1);
#if BLE_CONFIG_BLUEZ_MTU_FEATURE
    weaveCharacteristic->writePipeIO    = NULL;
    weaveCharacteristic->indicatePipeIO = NULL;
#endif // BLE_CONFIG_BLUEZ_MTU_FEATURE
    success = g_dbus_register_interface(dbusConn, weaveCharacteristic->path, CHARACTERISTIC_INTERFACE, weaveCharacteristicMethods,
                                        NULL, WeaveCharacteristicProperties, weaveCharacteristic, WeaveCharacteristicDestroy);

    if (FALSE == success)
    {
        msg = "Failed to register weaveCharacteristic object in RegisterWeaveCharacteristic";
        WeaveCharacteristicDestroy(weaveCharacteristic);
        weaveCharacteristic = NULL;
    }

exit:

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    return weaveCharacteristic;
}

static void WeaveConnectHandler(DBusConnection * connection, void * bluezData)
{
    if (gBluezServerEndpoint != NULL)
    {
        WoBLEz_NewConnection(gBluezServerEndpoint);
    }
}

static void WeaveDisconnectHandler(DBusConnection * connection, void * bluezData)
{
    if (gBluezServerEndpoint != NULL)
    {
        WoBLEz_ConnectionClosed(gBluezServerEndpoint);
    }
}

static void WeaveAdapterAdded(GDBusProxy * proxy)
{
    Adapter * adapter;
    DBusMessageIter iter;
    const char * str;
    gAdapterFound = false;

    if (g_dbus_proxy_get_property(proxy, "Address", &iter) == FALSE)
        return;

    dbus_message_iter_get_basic(&iter, &str);

    if (!strcasecmp(str, gBluezServerEndpoint->adapterAddr))
    {
        adapter               = static_cast<Adapter *>(g_new0(Adapter, 1));
        adapter->adapterProxy = proxy;
        gDefaultAdapter       = adapter;
        gAdapterFound         = true;
    }
}

static void WeaveProfileAdded(GDBusProxy * proxy)
{
    if (!gAdapterFound)
        return;

    gDefaultAdapter->profileProxy = proxy;
}

static void WeaveAdvertisingAdded(GDBusProxy * proxy)
{
    if (!gAdapterFound)
        return;

    gDefaultAdapter->advertisingProxy = proxy;
}

static void WeaveProxyAdded(GDBusProxy * proxy, void * bluezData)
{
    const char * interface;

    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, ADAPTER_INTERFACE))
    {
        WeaveAdapterAdded(proxy);
    }
    else if (!strcmp(interface, PROFILE_INTERFACE))
    {
        WeaveProfileAdded(proxy);
    }
    else if (!strcmp(interface, ADVERTISING_MANAGER_INTERFACE))
    {
        WeaveAdvertisingAdded(proxy);
    }
}

static void WeaveProxyDeleted(GDBusProxy * proxy, void * bluezData)
{
    const char * interface;
    interface = g_dbus_proxy_get_interface(proxy);

    if (!strcmp(interface, ADAPTER_INTERFACE))
    {
        if (gDefaultAdapter && gDefaultAdapter->adapterProxy == proxy)
        {
            g_free(gDefaultAdapter);
            gDefaultAdapter = NULL;
        }
    }
}

static void PowerCb(const DBusError * error, void * bluezData)
{
    WEAVE_ERROR err  = WEAVE_NO_ERROR;
    gboolean success = FALSE;
    VerifyOrExit(!dbus_error_is_set(error), err = WEAVE_ERROR_INCORRECT_STATE);

    success = RegisterWeaveService(gBluezDbusConn);
    VerifyOrExit(success == TRUE, err = WEAVE_ERROR_INCORRECT_STATE);

    gBluezServerEndpoint->weaveC1 = RegisterWeaveCharacteristic(gBluezDbusConn, UUID_WEAVE_C1, FLAGS_WEAVE_C1);
    VerifyOrExit(gBluezServerEndpoint->weaveC1 != NULL, err = WEAVE_ERROR_NO_MEMORY);

    WeaveLogDetail(Ble, "weave C1 uuid: %s, path: %s", gBluezServerEndpoint->weaveC1->uuid, gBluezServerEndpoint->weaveC1->path);

    gBluezServerEndpoint->weaveC2 = RegisterWeaveCharacteristic(gBluezDbusConn, UUID_WEAVE_C2, FLAGS_WEAVE_C2);
    VerifyOrExit(gBluezServerEndpoint->weaveC2 != NULL, err = WEAVE_ERROR_NO_MEMORY);

    WeaveLogDetail(Ble, "weave C2 uuid: %s, path: %s", gBluezServerEndpoint->weaveC2->uuid, gBluezServerEndpoint->weaveC2->path);

    success = SetupWeaveApp(gBluezDbusConn, gDefaultAdapter->profileProxy);
    VerifyOrExit(success == TRUE, err = WEAVE_ERROR_INCORRECT_STATE);

    success = AdvertisingRegister(gBluezDbusConn, gDefaultAdapter->advertisingProxy);
    VerifyOrExit(success == TRUE, err = WEAVE_ERROR_INCORRECT_STATE);

exit:

    if (err != WEAVE_NO_ERROR)
    {
        WeaveLogError(Ble, "PowerCb failed: %d", err);
    }
}

static void BluezClientReady(GDBusClient * weaveClient, void * bluezData)
{
    dbus_bool_t powered = TRUE;
    gboolean err =
        g_dbus_proxy_set_property_basic(gDefaultAdapter->adapterProxy, "Powered", DBUS_TYPE_BOOLEAN, &powered, PowerCb, NULL, NULL);
    if (FALSE == err)
    {
        WeaveLogError(Ble, "Fail to set Power property in BluezClientReady");
    }

    return;
}

uint16_t GetMTUWeaveCb(BLE_CONNECTION_OBJECT connObj)
{
    uint16_t mtu = gBluezServerEndpoint->mtu;
    WeaveLogDetail(Ble, "GetMTU: %d", mtu);
    return mtu;
}

void ClearWoBluezStatus()
{
    gBluezServerEndpoint->weaveC2->isNotifying = false;
    g_dbus_emit_property_changed(gBluezServerEndpoint->weaveC2->dbusConn, gBluezServerEndpoint->weaveC2->path,
                                 CHARACTERISTIC_INTERFACE, "Notifying");
}

void ExitBluezIOThread(void)
{
    g_main_loop_quit(gBluezMainLoop);
}

bool RunBluezIOThread(BluezPeripheralArgs * arg)
{
    GDBusClient * weaveClient    = NULL;
    const char * msg             = NULL;
    gboolean success             = FALSE;
    WEAVE_ERROR err              = WEAVE_NO_ERROR;
    const char * advertisingType = "peripheral";

    VerifyOrExit(arg != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);
    gBluezBlePlatformDelegate = arg->bluezBlePlatformDelegate;
    VerifyOrExit(gBluezBlePlatformDelegate != NULL, err = WEAVE_ERROR_NO_MEMORY);

    gBluezBlePlatformDelegate->SetSendIndicationCallback(WoBLEz_SendIndication);
    gBluezBlePlatformDelegate->SetGetMTUCallback(GetMTUWeaveCb);
    gBluezServerEndpoint = (BluezServerEndpoint *) g_new0(BluezServerEndpoint, 1);
    VerifyOrExit(gBluezServerEndpoint != NULL, err = WEAVE_ERROR_NO_MEMORY);

    gBluezServerEndpoint->adapterName      = g_strdup(arg->bleName);
    gBluezServerEndpoint->adapterAddr      = g_strdup(arg->bleAddress);
    gBluezServerEndpoint->advertisingUUID  = g_strdup(UUID_WEAVE_SHORT);
    gBluezServerEndpoint->advertisingType  = g_strdup(advertisingType);

    gBluezServerEndpoint->weaveServiceData = g_new0(WeaveServiceData, 1);
    VerifyOrExit(gBluezServerEndpoint->weaveServiceData != NULL, err = WEAVE_ERROR_NO_MEMORY);
    /**
     * Data arranged in "Length Type Value" pairs inside Weave service data.
     * Length should include size of value + size of Type field, which is 1 byte
     */
    gBluezServerEndpoint->weaveServiceData->dataBlock0Len = sizeof(WeaveIdInfo) + 1;
    gBluezServerEndpoint->weaveServiceData->dataBlock0Type = WEAVE_SRV_DATA_BLOCK_TYPE_WEAVE_ID_INFO;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.major = WEAVE_ID_INFO_MAJ_VER;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.minor = WEAVE_ID_INFO_MIN_VER;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.vendorId = arg->vendorId;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.productId = arg->productId;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.deviceId = arg->deviceId;
    gBluezServerEndpoint->weaveServiceData->weaveIdInfo.pairingStatus = arg->pairingStatus;

    gBluezServerEndpoint->mtu = HCI_MAX_MTU;
    gBluezMainLoop            = g_main_loop_new(NULL, FALSE);
    gBluezDbusConn            = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
    VerifyOrExit(gBluezDbusConn != NULL, err = WEAVE_ERROR_NO_MEMORY);

    success = g_dbus_attach_object_manager(gBluezDbusConn);
    VerifyOrExit(success == TRUE, msg = "Fail to attach object manager in RunBluezIOThread");

    weaveClient = g_dbus_client_new(gBluezDbusConn, BLUEZ_INTERFACE, BLUEZ_PATH);
    VerifyOrExit(weaveClient != NULL, err = WEAVE_ERROR_NO_MEMORY);

    success = g_dbus_client_set_proxy_handlers(weaveClient, WeaveProxyAdded, WeaveProxyDeleted, NULL, NULL);
    VerifyOrExit(success == TRUE, msg = "Fail to set weave proxy handler in RunBluezIOThread");

    success = g_dbus_client_set_ready_watch(weaveClient, BluezClientReady, NULL);
    VerifyOrExit(success == TRUE, msg = "Fail to set weave ready watch in RunBluezIOThread");

    success = g_dbus_client_set_connect_watch(weaveClient, WeaveConnectHandler, NULL);
    VerifyOrExit(success == TRUE, msg = "Fail to set weave connect watch in RunBluezIOThread");

    success = g_dbus_client_set_disconnect_watch(weaveClient, WeaveDisconnectHandler, NULL);
    VerifyOrExit(success == TRUE, msg = "Fail to set weave disconnect watch in RunBluezIOThread");

    g_main_loop_run(gBluezMainLoop);
    WeaveLogProgress(Ble, "Exited from Bluez main loop");
exit:

    if (err != WEAVE_NO_ERROR)
    {
        success = FALSE;
        WeaveLogError(Ble, "RunBluezIOThread failed: %d", err);
    }

    if ((success != TRUE) && (msg != NULL))
    {
        WeaveLogError(Ble, msg);
    }

    if (NULL != gBluezServerEndpoint)
    {
        if (NULL != gBluezServerEndpoint->weaveService)
        {
            ServiceDestroy(gBluezServerEndpoint->weaveService);
            gBluezServerEndpoint->weaveService = NULL;
        }

        if (NULL != gBluezServerEndpoint->weaveC1)
        {
            WeaveCharacteristicDestroy(gBluezServerEndpoint->weaveC1);
            gBluezServerEndpoint->weaveC1 = NULL;
        }

        if (NULL != gBluezServerEndpoint->weaveC2)
        {
            WeaveCharacteristicDestroy(gBluezServerEndpoint->weaveC2);
            gBluezServerEndpoint->weaveC2 = NULL;
        }

        g_free(gBluezServerEndpoint->adapterName);
        gBluezServerEndpoint->adapterName = NULL;
        g_free(gBluezServerEndpoint->adapterAddr);
        gBluezServerEndpoint->adapterAddr = NULL;
        g_free(gBluezServerEndpoint->advertisingUUID);
        gBluezServerEndpoint->advertisingUUID = NULL;
        g_free(gBluezServerEndpoint->advertisingType);
        gBluezServerEndpoint->advertisingType = NULL;
        g_free(gBluezServerEndpoint->weaveServiceData);
        gBluezServerEndpoint->weaveServiceData = NULL;
        g_free(gBluezServerEndpoint);
        gBluezServerEndpoint = NULL;
    }

    if (NULL != weaveClient)
    {
        g_dbus_client_unref(weaveClient);
    }

    if (NULL != gBluezDbusConn)
    {
        dbus_connection_unref(gBluezDbusConn);
    }

    if (NULL != gBluezMainLoop)
    {
        g_main_loop_unref(gBluezMainLoop);
    }

    gBluezBlePlatformDelegate = NULL;
    return success;
}

} // namespace BlueZ
} /* namespace Platform */
} /* namespace Ble */
} /* namespace nl */

#endif /* CONFIG_BLE_PLATFORM_BLUEZ */
