/*

    Copyright (c) 013-2017 Nest Labs, Inc.
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
package nl.Weave.DeviceManager;

import java.util.EnumSet;

public enum DeviceFeatures
{
    None                                (0x0000),
    Protect_HomeAlarmLinkCapable        (0x0001),
    LinePowered                         (0x0002);

    DeviceFeatures(int v)
    {
        val = v;
    }

    public final int val;

    public static DeviceFeatures fromVal(int val)
    {
        for (DeviceFeatures enumVal : DeviceFeatures.values())
            if (enumVal.val == val)
                return enumVal;
        return None;
    }

    public static EnumSet<DeviceFeatures> fromFlags(int featureFlags)
    {
        EnumSet<DeviceFeatures> featureSet = EnumSet.noneOf(DeviceFeatures.class);

        for (DeviceFeatures enumVal : DeviceFeatures.values())
            if ((enumVal.val & featureFlags) != 0)
                featureSet.add(enumVal);
        return featureSet;
    }

    public static int toFlags(EnumSet<DeviceFeatures> featureSet)
    {
        int featureFlags = 0;
        for (DeviceFeatures enumVal : featureSet)
            featureFlags |= enumVal.val;
        return featureFlags;
    }
}
