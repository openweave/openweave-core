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

public class TargetFabricId
{
    /** Special target fabric id specifying that only devices that are NOT a member of a fabric should respond.
         */
    public static final long NotInFabric = 0;

    /** Special target fabric id specifying that only devices that ARE a member of a fabric should respond.
         */
    public static final long AnyFabric = -256;

    /** Special target fabric id specifying that all devices should respond regardless of fabric membership.
         */
    public static final long Any = -1;
}
