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

public enum TargetDeviceModes
{
    /** Locate all devices regardless of mode.
         */
    Any                                 (0x00000000),

    /** Locate all devices in 'user-selected' mode -- i.e. where the device has has been
         * directly identified by a user, e.g. by pressing a button.
         */
    UserSelectedMode                    (0x00000001);

    TargetDeviceModes(int v)
    {
        val = v;
    }

    public final int val;

    public static TargetDeviceModes fromVal(int val)
    {
        for (TargetDeviceModes enumVal : TargetDeviceModes.values())
            if (enumVal.val == val)
                return enumVal;
        return Any;
    }

    public static EnumSet<TargetDeviceModes> fromFlags(int flags)
    {
        EnumSet<TargetDeviceModes> targetDeviceModes = EnumSet.noneOf(TargetDeviceModes.class);

        for (TargetDeviceModes enumVal : TargetDeviceModes.values())
            if ((enumVal.val & flags) != 0)
                targetDeviceModes.add(enumVal);
        return targetDeviceModes;
    }

    public static int toFlags(EnumSet<TargetDeviceModes> modeSet)
    {
        int modeFlags = 0;
        for (TargetDeviceModes enumVal : modeSet)
            modeFlags |= enumVal.val;
        return modeFlags;
    }
}
