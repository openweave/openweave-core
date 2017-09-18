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

public enum GetNetworkFlags
{
    None                                (0x00),
    IncludeCredentials                  (0x01);

    GetNetworkFlags(int v)
    {
        val = v;
    }

    public final int val;

    public static GetNetworkFlags fromVal(int val)
    {
        for (GetNetworkFlags enumVal : GetNetworkFlags.values())
            if (enumVal.val == val)
                return enumVal;
        return None;
    }

    public static EnumSet<GetNetworkFlags> fromFlags(int flags)
    {
        EnumSet<GetNetworkFlags> getNetworkFlags = EnumSet.noneOf(GetNetworkFlags.class);

        for (GetNetworkFlags enumVal : GetNetworkFlags.values())
            if ((enumVal.val & flags) != 0)
                getNetworkFlags.add(enumVal);
        return getNetworkFlags;
    }

    public static int toFlags(EnumSet<GetNetworkFlags> modeSet)
    {
        int modeFlags = 0;
        for (GetNetworkFlags enumVal : modeSet)
            modeFlags |= enumVal.val;
        return modeFlags;
    }
}
