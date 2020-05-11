/*

    Copyright (c) 2020 Google LLC.
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

/**
*    @file
*      Represents WdmClient interface.
*/

package nl.Weave.DataManagement;
import java.math.BigInteger;

public interface WdmClient
{
    /**
     * Close WdmClient
     */
    public void close();

    /**
     * set weave node id in trait catalog in WdmClient
     *
     * @param nodeId weave node id
     *
     */
    public void setNodeId(BigInteger nodeId);

    /**
     * Create the new data newDataSink
     *
     * @param resourceIdentifier resource id is a globally-unique identifier for a Weave/phoenix resource
     * @param profileId trait profile id
     * @param instanceId trait instance id
     * @param path trait path
     *
     */
    public GenericTraitUpdatableDataSink newDataSink(ResourceIdentifier resourceIdentifier, long profileId, long instanceId, String path);

    /**
     * Begins a flush of all trait data. The result of this operation can be observed through the {@link CompletionHandler}
     * that has been assigned via {@link #setCompletionHandler}.when operation completes, onFlushUpdateComplete is called,
     * application would receive throwable exception list, if it is empty, it means success without failed path,
     * if anything inside, the array member could be WdmClientFlushUpdateException(local client error)
     * or WdmClientFlushUpdateDeviceException(remote device status), application can use the path and dataSink from the above member to clear
     * particular data or skip the error if necessary. When operation fails, it usually means the operation cannot complete at all, for example
     * communication or protocol issue, onError would be called.
     */
    public void beginFlushUpdate();

    /**
     * Begins a sync of all trait data. The result of this operation can be observed through the {@link CompletionHandler}
     * that has been assigned via {@link #setCompletionHandler}.
     */
    public void beginRefreshData();

    public CompletionHandler getCompletionHandler();
    public void setCompletionHandler(CompletionHandler compHandler);

    public GenericTraitUpdatableDataSink getDataSink(long traitInstancePtr);

    public interface CompletionHandler
    {
        void onFlushUpdateComplete(Throwable[] exceptions, WdmClient wdmClient);
        void onRefreshDataComplete();
        void onError(Throwable err);
    }
};
