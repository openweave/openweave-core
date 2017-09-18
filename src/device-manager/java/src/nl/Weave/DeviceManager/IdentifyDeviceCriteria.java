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

public class IdentifyDeviceCriteria {
    public IdentifyDeviceCriteria() {
        TargetFabricId = nl.Weave.DeviceManager.TargetFabricId.Any;
        TargetModes = TargetDeviceModes.Any;
        TargetVendorId = -1;
        TargetProductId = -1;
        TargetDeviceId = -1;
    }

    /** Specifies that only devices that are members of the specified Weave fabric should respond.
     *  Value can be an actual fabric id, or one of the values defined in the TargetFabricId class.
     */
    public long TargetFabricId;

    /** Specifies that only devices that are currently in the specified modes should respond.
     *  Values are taken from the TargetDeviceModes enum.
     */
    public TargetDeviceModes TargetModes;

    /** Specifies that only devices manufactured by the given vendor should respond.
     *  A value of -1 specifies any vendor.
     */
    public int TargetVendorId;

    /** Specifies that only devices with the given product id should respond.
     *  A value of -1 specifies any product.
     *  If the TargetProductId field is specified, then the TargetVendorId must also be specified.
     */
    public int TargetProductId;

    /** Specifies that only the device with the specified Weave Node id should respond.
     *  A value of -1 specifies all devices should respond.
     */
    public long TargetDeviceId;

    /**
     * Special value to use for {@link #TargetProductId} that indicates that any generation of Nest
     * Thermostat can be considered valid to respond to an identify request.
     */
    public static final int PRODUCT_WILDCARD_ID_NEST_THERMOSTAT = 0xFFF0;

    /**
     * Special value to use for {@link #TargetProductId} that indicates that any generation of Nest
     * Protect can be considered valid to respond to an identify request.
     */
    public static final int PRODUCT_WILDCARD_ID_NEST_PROTECT = 0xFFF1;

    /**
     * Special value to use for {@link #TargetProductId} that indicates that any generation of Nest
     * Cam or Nest Cam Outdoor can be considered valid to respond to an identify request.
     */
    public static final int PRODUCT_WILDCARD_ID_NEST_CAM = 0xFFF2;
}
