/*
 *
 *    Copyright (c) 2019 Google LLC
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
package nl.Weave.DataManagement;
import android.os.Build;
import android.util.Log;

import java.math.BigInteger;
/**
 *  Represents Resource Identifier
 */
public class ResourceIdentifier
{
    public ResourceIdentifier()
    {
        ResourceTypeEnum = nl.Weave.DataManagement.ResourceType.RESOURCE_TYPE_RESERVED;
        ResourceIdInt64 = -2;
        ResourceIdBytes = null;
    }

    private ResourceType ResourceTypeEnum;

    private long ResourceIdInt64;

    private byte[] ResourceIdBytes;

    public static ResourceIdentifier Make(ResourceType resourceType, int resourceIdInt64)
    {
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        resourceIdentifier.ResourceTypeEnum = resourceType;
        resourceIdentifier.ResourceIdInt64 = resourceIdInt64;
        return resourceIdentifier;
    }

    public static ResourceIdentifier Make(ResourceType resourceType, String resourceIdInt64Str)
    {
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        resourceIdentifier.ResourceTypeEnum = resourceType;
        resourceIdentifier.ResourceIdInt64 = Long.parseUnsignedLong(resourceIdInt64Str, 16);
        return resourceIdentifier;
    }

    /* resourceIdentifierStr's format would be something like DEVICE_18B4300000000001, RESERVED_FFFFFFFFFFFFFFFE */
    public static ResourceIdentifier Make(String resourceIdentifierStr)
    {
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        String[] splitedIdentifier = resourceIdentifierStr.split("_");
        boolean isLegalResourceType = false;
        if (splitedIdentifier.length != 2)
        {
            Log.e(TAG, "unexpected resourceIdentifierStr, expected ResourceType_ResourceId");
            return null;
        }
        if (splitedIdentifier[0] == null)
        {
            Log.e(TAG, "unexpected resourceIdentifierStr, ResourceType should not be null");
            return null;
        }

        for (ResourceType enumVal : ResourceType.values())
        {
            if (splitedIdentifier[0].compareTo(ResourceTypeAsString(enumVal)) == 0)
            {
                isLegalResourceType = true;
                resourceIdentifier.ResourceTypeEnum = enumVal;
                resourceIdentifier.ResourceIdInt64 = Long.parseUnsignedLong(splitedIdentifier[1], 16);
                return resourceIdentifier;
            }
        }
        if (!isLegalResourceType)
        {
            Log.e(TAG, "unexpected resourceIdentifierStr, ResourceType is not applicable");
        }
        return null;
    }

    public static ResourceIdentifier Make(ResourceType resourceType, byte[] ResourceIdBytes)
    {
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        resourceIdentifier.ResourceTypeEnum = resourceType;
        resourceIdentifier.ResourceIdBytes = ResourceIdBytes;
        return resourceIdentifier;
    }

    private final static String TAG = ResourceIdentifier.class.getSimpleName();

    private static String ResourceTypeAsString(ResourceType val)
    {
        String retval;
        switch (val)
        {
        case RESOURCE_TYPE_RESERVED:
            retval = "RESERVED";
            break;
        case RESOURCE_TYPE_DEVICE:
            retval = "DEVICE";
            break;
        case RESOURCE_TYPE_USER:
            retval = "USER";
            break;
        case RESOURCE_TYPE_ACCOUNT:
            retval = "ACCOUNT";
            break;
        case RESOURCE_TYPE_AREA:
            retval = "AREA";
            break;
        case RESOURCE_TYPE_FIXTURE:
            retval = "FIXTURE";
            break;
        case RESOURCE_TYPE_GROUP:
            retval = "GROUP";
            break;
        case RESOURCE_TYPE_ANNOTATION:
            retval = "ANNOTATION";
            break;
        case RESOURCE_TYPE_STRUCTURE:
            retval = "STRUCTURE";
            break;
        case RESOURCE_TYPE_GUEST:
            retval = "GUEST";
            break;
        case RESOURCE_TYPE_SERVICE:
            retval = "SERVICE";
            break;
        default:
            retval = null;
        }
        return retval;
    }

    public static byte[] IntToResByteArray(long value) {
        byte[] bytes = new byte[8];
        for (int i = 0; i < 8; i++) {
            bytes[i] = (byte)(value >>> (i * 8));
        }
        return bytes;
    }
}
