/*
 *
 *    Copyright (c) 013-2017 Nest Labs, Inc.
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
package nl.Weave.DeviceManager;

/** Represents an error that occurred on a Weave device during a device manager operation.
 */
public class WeaveDeviceException extends Exception {
    public WeaveDeviceException () {}

    public WeaveDeviceException (int statusCode, int profileId, int sysErrorCode, String description) {
        super(description);
        StatusCode = statusCode;
        ProfileId = profileId;
        SystemErrorCode = sysErrorCode;
    }

    /**
     * A code describing the error.
     */
    public int StatusCode;

    /**
     * The Weave profile that defines the returned status code.
     */
    public int ProfileId;

    /**
     * An optional system-specific error code describing the failure, or 0 if not available.
     */
    public int SystemErrorCode;
}
