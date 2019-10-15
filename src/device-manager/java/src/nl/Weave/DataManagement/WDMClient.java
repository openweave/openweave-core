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

public class WDMClient
{
    public WDMClient(long deviceMgrPtr)
    {
        init();
        mWDMClientPtr = newWDMClient(deviceMgrPtr);
        mCompHandler = null;
    }

    public void close()
    {
        if (mHmap != null)
        {
            Iterator iter = mHmap.entrySet().iterator();
            while (iter.hasNext())
            {
                Map.Entry entry = (Map.Entry) iter.next();
                GenericTraitUpdatableDataSink traitInstance = (GenericTraitUpdatableDataSink)entry.getValue();
                if (traitInstance != null)
                {
                    traitInstance.shutdown();
                }
            }
            mHmap.clear();
        }

        if (mWDMClientPtr != 0)
        {
            deleteWDMClient(mWDMClientPtr);
            mWDMClientPtr = 0;
        }
        mCompHandler = null;
    }

    public GenericTraitUpdatableDataSink newDataSink(int resourceType, byte[] resourceId, long profileId, long instanceId, String path)
    {
        long traitInstancePtr = 0;
        boolean isExist = false;
        GenericTraitUpdatableDataSink traitInstance = null;

        if (mHmap == null)
        {
            mHmap = new HashMap<Long, GenericTraitUpdatableDataSink>();
        }
        int testsize = 0;
        if (mWDMClientPtr == 0)
        {
            mCompHandler.onError(new Exception("wdmClientPtr is not ready."));
            return null;
        }

        traitInstancePtr = newDataSink(mWDMClientPtr, resourceType, resourceId, profileId, instanceId, path);

        if (traitInstancePtr == 0)
        {
            mCompHandler.onError(new Exception("Fail to create traitInstancePtr."));
            Log.e(TAG, "traitInstancePtr is not ready");
            return null;
        }

        isExist = mHmap.containsKey(traitInstancePtr);
        if (isExist == false)
        {
            traitInstance = new GenericTraitUpdatableDataSink(traitInstancePtr, mWDMClientPtr);
            mHmap.put(traitInstancePtr, traitInstance);
        }
        else
        {
            Log.d(TAG, "trait exists" + profileId);
        }

        Log.d(TAG, "there exists %d traits" + mHmap.size());

        return mHmap.get(traitInstancePtr);
    }

    public void beginFlushUpdate()
    {
        beginFlushUpdate(mWDMClientPtr);
    }

    public void beginRefreshData()
    {
        beginRefreshData(mWDMClientPtr);
    }

    public CompletionHandler getCompletionHandler()
    {
        return mCompHandler;
    }

    public void setCompletionHandler(CompletionHandler compHandler)
    {
        mCompHandler = compHandler;
    }

    public void onError(Throwable err)
    {
        mCompHandler.onError(err);
    }

    public void onFlushUpdateComplete()
    {
        mCompHandler.onFlushUpdateComplete();
    }

    public void onRefreshDataComplete()
    {
        mCompHandler.onRefreshDataComplete();
    }

    public interface CompletionHandler
    {
        void onFlushUpdateComplete();
        void onRefreshDataComplete();
        void onError(Throwable err);
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
    private HashMap<Long, GenericTraitUpdatableDataSink> mHmap;
    private final static String TAG = WDMClient.class.getSimpleName();

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void init();
    private native long newWDMClient(long deviceMgrPtr);
    private native void deleteWDMClient(long wdmClientPtr);
    private native long newDataSink(long wdmClientPtr, int resourceType, byte[] resourceId, long profileId, long instanceId, String path);
    private native void beginFlushUpdate(long wdmClientPtr);
    private native void beginRefreshData(long wdmClientPtr);
};
