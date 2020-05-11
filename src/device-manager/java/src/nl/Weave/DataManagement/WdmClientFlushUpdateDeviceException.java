/*
 *
 *    Copyright (c) 2020 Google, LLC.
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
import nl.Weave.DeviceManager.WeaveDeviceException;

/** Represents a exception that occurred on a Weave device during the WdmClient FlushUpdate operation.
 */
public class WdmClientFlushUpdateDeviceException extends WeaveDeviceException {
    public WdmClientFlushUpdateDeviceException (int statusCode, int profileId, int sysErrorCode, String description, String pathStr, long dataSink) {
        super(statusCode, profileId, sysErrorCode, description != null ? description : String.format("Error Code %d, Profile Id %d, StatusCode %d, path %s", sysErrorCode, profileId, statusCode, pathStr));
        path = pathStr;
        dataSinkPtr = dataSink;
    }

    /**
     * Get data sink
     *
     * @param wdmClient weave data management client
     *
     */
    public GenericTraitUpdatableDataSink getDataSink(WdmClient wdmClient)
    {
        return wdmClient.getDataSink(dataSinkPtr);
    }

    /**
     * A flushed path
     */
    public final String path;

    /**
     * A data sink reference related to this exception
     */
    private final long dataSinkPtr;
}
