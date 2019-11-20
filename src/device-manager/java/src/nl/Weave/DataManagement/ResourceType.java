/*

    Copyright (c) 2019 Google, LLC.
    All rights reserved.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */
package nl.Weave.DataManagement;
import android.os.Build;
import android.util.Log;

public enum ResourceType
{
    RESOURCE_TYPE_RESERVED(0),
    RESOURCE_TYPE_DEVICE(1),
    RESOURCE_TYPE_USER(2),
    RESOURCE_TYPE_ACCOUNT(3),
    RESOURCE_TYPE_AREA(4),
    RESOURCE_TYPE_FIXTURE(5),
    RESOURCE_TYPE_GROUP(6),
    RESOURCE_TYPE_ANNOTATION(7),
    RESOURCE_TYPE_STRUCTURE(8),
    RESOURCE_TYPE_GUEST(9),
    RESOURCE_TYPE_SERVICE(10);

    ResourceType(int v)
    {
        val = v;
    }

    public final int val;

    public static ResourceType fromVal(int val)
    {
        for (ResourceType enumVal : ResourceType.values())
            if (enumVal.val == val)
                return enumVal;
        return RESOURCE_TYPE_RESERVED;
    }
}
