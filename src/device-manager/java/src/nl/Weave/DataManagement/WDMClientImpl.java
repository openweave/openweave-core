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
package nl.Weave.DataManagement;

import android.os.Build;
import android.util.Log;

import java.util.EnumSet;
import java.util.Random;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;

public class WDMClientImpl implements WDMClient
{
    public WDMClientImpl()
    {
        init();
        mCompHandler = null;
    }

    @Override
    public void connect(long deviceMgrPtr)
    {
        mWDMClientPtr = newWDMClient(deviceMgrPtr);
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
                    traitInstance.close();
                }
            }
            mTraitMap.clear();
        }

        if (mWDMClientPtr != 0)
        {
            deleteWDMClient(mWDMClientPtr);
            mWDMClientPtr = 0;
        }

        mCompHandler = null;
    }

    @Override
    public GenericTraitUpdatableDataSinkImpl newDataSink(ResourceIdentifier resourceIdentifier, long profileId, long instanceId, String path)
    {
        long traitInstancePtr = 0;
        boolean isExist = false;
        GenericTraitUpdatableDataSinkImpl traitInstance = null;

        if (mTraitMap == null)
        {
            mTraitMap = new HashMap<Long, GenericTraitUpdatableDataSinkImpl>();
        }

        if (mWDMClientPtr == 0)
        {
            mCompHandler.onError(new Exception("wdmClientPtr is not ready."));
            return null;
        }

        traitInstancePtr = newDataSink(mWDMClientPtr, resourceIdentifier, profileId, instanceId, path);

        if (traitInstancePtr == 0)
        {
            mCompHandler.onError(new Exception("Fail to create traitInstancePtr."));
            Log.e(TAG, "traitInstancePtr is not ready");
            return null;
        }

        isExist = mTraitMap.containsKey(traitInstancePtr);
        if (isExist == false)
        {
            traitInstance = new GenericTraitUpdatableDataSinkImpl(traitInstancePtr, this);
            mTraitMap.put(traitInstancePtr, traitInstance);
        }
        else
        {
            Log.d(TAG, "trait exists" + profileId);
        }

        Log.d(TAG, "there exists %d traits" + mTraitMap.size());

        return mTraitMap.get(traitInstancePtr);
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
        if (mWDMClientPtr == 0)
        {
            Log.e(TAG, "unexpected err, mWDMClientPtr is 0");
            return;
        }
        beginFlushUpdate(mWDMClientPtr);
    }

    @Override
    public void beginRefreshData()
    {
        if (mWDMClientPtr == 0)
        {
            Log.e(TAG, "unexpected err, mWDMClientPtr is 0");
            return;
        }
        beginRefreshData(mWDMClientPtr);
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
            Log.e(TAG, "unexpected err, mCompHandler is null");
            return;
        }
        mCompHandler.onError(err);
    }

    private void onFlushUpdateComplete()
    {
        if (mCompHandler == null)
        {
            Log.e(TAG, "unexpected err, mCompHandler is null");
            return;
        }
        mCompHandler.onFlushUpdateComplete();
    }

    private void onRefreshDataComplete()
    {
        if (mCompHandler == null)
        {
            Log.e(TAG, "unexpected err, mCompHandler is null");
            return;
        }
        mCompHandler.onRefreshDataComplete();
    }

    // ----- Protected Members -----

    protected CompletionHandler mCompHandler;

    protected void finalize() throws Throwable
    {
        super.finalize();
        close();
    }

    // ----- Private Members -----

    private long mWDMClientPtr;
    private HashMap<Long, GenericTraitUpdatableDataSinkImpl> mTraitMap;
    private final static String TAG = WDMClientImpl.class.getSimpleName();

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void init();
    private native long newWDMClient(long deviceMgrPtr);
    private native void deleteWDMClient(long wdmClientPtr);
    private native long newDataSink(long wdmClientPtr, ResourceIdentifier resourceIdentifier, long profileId, long instanceId, String path);
    private native void beginFlushUpdate(long wdmClientPtr);
    private native void beginRefreshData(long wdmClientPtr);
};
