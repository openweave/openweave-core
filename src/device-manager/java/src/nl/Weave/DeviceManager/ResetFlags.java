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

public enum ResetFlags
{
    None                                (0x0000),
    All                                 (0x00FF),
    NetworkConfig                       (0x0001),
    FabricConfig                        (0x0002),
    ServiceConfig                       (0x0004);

    ResetFlags(int v)
    {
        val = v;
    }

    public final int val;

    public static ResetFlags fromVal(int val)
    {
        for (ResetFlags enumVal : ResetFlags.values())
            if (enumVal.val == val)
                return enumVal;
        return None;
    }

    public static EnumSet<ResetFlags> fromFlags(int flags)
    {
        EnumSet<ResetFlags> rendezvousModes = EnumSet.noneOf(ResetFlags.class);

        for (ResetFlags enumVal : ResetFlags.values())
            if ((enumVal.val & flags) != 0)
                rendezvousModes.add(enumVal);
        return rendezvousModes;
    }

    public static int toFlags(EnumSet<ResetFlags> modeSet)
    {
        int modeFlags = 0;
        for (ResetFlags enumVal : modeSet)
            modeFlags |= enumVal.val;
        return modeFlags;
    }
}
