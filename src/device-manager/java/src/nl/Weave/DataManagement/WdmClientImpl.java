/*

    Copyright (c) 2019 Google, LLC.
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
*      Represents WdmClient Implementation using native WdmClient APIs.
*/

package nl.Weave.DataManagement;

import android.os.Build;
import android.util.Log;

import java.util.EnumSet;
import java.util.Random;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;

public class WdmClientImpl implements WdmClient
{
    public WdmClientImpl()
    {
        init();
    }

    public void connect(long deviceMgrPtr)
    {
        mWdmClientPtr = newWdmClient(deviceMgrPtr);
    }

    @Override
    public void close()
    {
        if (mTraitMap != null)
        {
            Iterator iter = mTraitMap.entrySet().iterator();
            while (iter.hasNext())
            {
                Map.Entry entry = (Map.Entry) iter.next();
                GenericTraitUpdatableDataSinkImpl traitInstance = (GenericTraitUpdatableDataSinkImpl)entry.getValue();
                if (traitInstance != null)
                {
                    traitInstance.shutdown();
                }
            }
            mTraitMap.clear();
        }

        if (mWdmClientPtr != 0)
        {
            deleteWdmClient(mWdmClientPtr);
            mWdmClientPtr = 0;
        }

        mCompHandler = null;
    }

    @Override
    public GenericTraitUpdatableDataSink newDataSink(ResourceIdentifier resourceIdentifier, long profileId, long instanceId, String path)
    {
        long traitInstancePtr = 0;
        boolean isExist = false;

        ensureNotClosed();

        if (mTraitMap == null)
        {
            mTraitMap = new HashMap<Long, GenericTraitUpdatableDataSink>();
        }

        traitInstancePtr = newDataSink(mWdmClientPtr, resourceIdentifier, profileId, instanceId, path);

        if (traitInstancePtr == 0)
        {
            mCompHandler.onError(new Exception("Fail to create traitInstancePtr."));
            Log.e(TAG, "traitInstancePtr is not ready");
            return null;
        }

        GenericTraitUpdatableDataSink trait = mTraitMap.get(traitInstancePtr);
        if (trait == null)
        {
            trait = new GenericTraitUpdatableDataSinkImpl(traitInstancePtr, this);
            mTraitMap.put(traitInstancePtr, trait);
        }
        else
        {
            Log.d(TAG, "trait exists" + profileId);
        }

        return trait;
    }

    protected void removeDataSinkRef(long traitInstancePtr)
    {
        boolean isExist = false;
        isExist = mTraitMap.containsKey(traitInstancePtr);
        if (isExist == true)
        {
            mTraitMap.put(traitInstancePtr, null);
        }
    }

    @Override
    public void beginFlushUpdate()
    {
        ensureNotClosed();
        beginFlushUpdate(mWdmClientPtr);
    }

    @Override
    public void beginRefreshData()
    {
        ensureNotClosed();
        beginRefreshData(mWdmClientPtr);
    }

    @Override
    public CompletionHandler getCompletionHandler()
    {
        return mCompHandler;
    }

    @Override
    public void setCompletionHandler(CompletionHandler compHandler)
    {
        mCompHandler = compHandler;
    }

    private void onError(Throwable err)
    {
        if (mCompHandler == null)
        {
            throw new IllegalStateException("unexpected err, mCompHandler is null");
        }
        mCompHandler.onError(err);
    }

    private void onFlushUpdateComplete()
    {
        if (mCompHandler == null)
        {
            throw new IllegalStateException("unexpected err, mCompHandler is null");
        }
        mCompHandler.onFlushUpdateComplete();
    }

    private void onRefreshDataComplete()
    {
        if (mCompHandler == null)
        {
            throw new IllegalStateException("unexpected err, mCompHandler is null");
        }
        mCompHandler.onRefreshDataComplete();
    }

    private void ensureNotClosed() {
      if (mWdmClientPtr == 0) {
        throw new IllegalStateException("This WdmClient has already been closed.");
      }
    }

    // ----- Protected Members -----

    private CompletionHandler mCompHandler;

    protected void finalize() throws Throwable
    {
        super.finalize();
        close();
    }

    // ----- Private Members -----

    private long mWdmClientPtr;
    private Map<Long, GenericTraitUpdatableDataSink> mTraitMap;
    private final static String TAG = WdmClientImpl.class.getSimpleName();

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void init();
    private native long newWdmClient(long deviceMgrPtr);
    private native void deleteWdmClient(long wdmClientPtr);
    private native long newDataSink(long wdmClientPtr, ResourceIdentifier resourceIdentifier, long profileId, long instanceId, String path);
    private native void beginFlushUpdate(long wdmClientPtr);
    private native void beginRefreshData(long wdmClientPtr);
};
