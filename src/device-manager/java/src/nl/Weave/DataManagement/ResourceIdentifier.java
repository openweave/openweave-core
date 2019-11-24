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
        ResourceType = RESOURCE_TYPE_RESERVED;
        ResourceId = SELF_NODE_ID;
    }

    private int ResourceType;

    private long ResourceId;

    public static ResourceIdentifier Make(int resourceType, BigInteger value)
    {
        long resourceId = value.longValue();
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        resourceIdentifier.ResourceType = resourceType;
        resourceIdentifier.ResourceId = resourceId;
        return resourceIdentifier;
    }

    public static ResourceIdentifier Make(int resourceType, byte[] ResourceIdBytes)
    {
        ResourceIdentifier resourceIdentifier = new ResourceIdentifier();
        resourceIdentifier.ResourceType = resourceType;

        if (ResourceIdBytes.length != 8)
        {
            Log.e(TAG, "unexpected resourceIdentifier, ResourceIdBytes should be 8 bytes");
            return null;
        }
        resourceIdentifier.ResourceId = ByteArrayToLong(ResourceIdBytes);

        return resourceIdentifier;
    }

    public static long ByteArrayToLong(byte[] value)
    {
        long result = 0;
        for (int i = 0; i < value.length; i++)
        {
           result += ((long) value[i] & 0xffL) << (8 * i);
        }
        return result;
    }

    public static byte[] LongToByteArray(long value) {
        byte[] bytes = new byte[8];
        for (int i = 0; i < 8; i++) {
            bytes[i] = (byte)(value >>> (i * 8));
        }
        return bytes;
    }

    public static final int RESOURCE_TYPE_RESERVED = 0;
    public static final long SELF_NODE_ID = -2;

    private final static String TAG = ResourceIdentifier.class.getSimpleName();
}
