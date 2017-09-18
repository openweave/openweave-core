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

public enum RendezvousMode
{
    None                                (0x0000),
    EnableWiFiRendezvousNetwork         (0x0001),
    EnableThreadRendezvous              (0x0002);

    RendezvousMode(int v)
    {
        val = v;
    }

    public final int val;

    public static RendezvousMode fromVal(int val)
    {
        for (RendezvousMode enumVal : RendezvousMode.values())
            if (enumVal.val == val)
                return enumVal;
        return None;
    }

    public static EnumSet<RendezvousMode> fromFlags(int flags)
    {
        EnumSet<RendezvousMode> rendezvousModes = EnumSet.noneOf(RendezvousMode.class);

        for (RendezvousMode enumVal : RendezvousMode.values())
            if ((enumVal.val & flags) != 0)
                rendezvousModes.add(enumVal);
        return rendezvousModes;
    }

    public static int toFlags(EnumSet<RendezvousMode> modeSet)
    {
        int modeFlags = 0;
        for (RendezvousMode enumVal : modeSet)
            modeFlags |= enumVal.val;
        return modeFlags;
    }
}
