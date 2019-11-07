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

import java.math.BigInteger;
import java.util.EnumSet;
import java.util.Random;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;

public class GenericTraitUpdatableDataSink
{
    protected GenericTraitUpdatableDataSink(long traitInstancePtr, WDMClient wdmClient)
    {
        mWDMClient = wdmClient;
        mTraitInstancePtr = traitInstancePtr;
        mCompHandler = null;
        init(mTraitInstancePtr);
    }

    protected void close()
    {
        if (mWDMClient != null)
        {
            mWDMClient.removeDataSinkRef(mTraitInstancePtr);
            mWDMClient = null;
        }

        if (mTraitInstancePtr != 0)
        {
            close(mTraitInstancePtr);
            mTraitInstancePtr = 0;
        }
        mCompHandler = null;
    }

    public CompletionHandler getCompletionHandler()
    {
        return mCompHandler;
    }

    public void setCompletionHandler(CompletionHandler compHandler)
    {
        mCompHandler = compHandler;
    }

    public void setInt(String path, int value, boolean isConditional)
    {
        boolean isSigned = true;
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, value, isConditional, isSigned);
    }

    public void setInt(String path, long value, boolean isConditional)
    {
        boolean isSigned = true;
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, value, isConditional, isSigned);
    }

    public void setInt(String path, BigInteger value, boolean isConditional)
    {
        boolean isSigned = true;
        long convertedVal = value.longValue();
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, convertedVal, isConditional, isSigned);
    }

    public void setUnsigned(String path, int value, boolean isConditional)
    {
        boolean isSigned = false;
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, value, isConditional, isSigned);
    }

    public void setUnsigned(String path, long value, boolean isConditional)
    {
        boolean isSigned = false;
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, value, isConditional, isSigned);
    }

    public void setUnsigned(String path, BigInteger value, boolean isConditional)
    {
        boolean isSigned = false;
        long convertedVal = value.longValue();
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setInt(mTraitInstancePtr, path, convertedVal, isConditional, isSigned);
    }

    public void setDouble(String path, double value, boolean isConditional)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setDouble(mTraitInstancePtr, path, value, isConditional);
    }

    public void setBoolean(String path, boolean value, boolean isConditional)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setBoolean(mTraitInstancePtr, path, value, isConditional);
    }

    public void setString(String path, String value, boolean isConditional)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setString(mTraitInstancePtr, path, value, isConditional);
    }

    public void setNULL(String path, boolean isConditional)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setNULL(mTraitInstancePtr, path, isConditional);
    }

    public void setBytes(String path, byte[] value, boolean isConditional)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        setBytes(mTraitInstancePtr, path, value, isConditional);
    }


    public void setInt(String path, int value)
    {
        setInt(path, value, false);
    }

    public void setInt(String path, long value)
    {
        setInt(path, value, false);
    }

    public void setInt(String path, BigInteger value)
    {
        setInt(path, value, false);
    }

    public void setUnsigned(String path, int value)
    {
        setUnsigned(path, value, false);
    }

    public void setUnsigned(String path, long value)
    {
        setUnsigned(path, value, false);
    }

    public void setUnsigned(String path, BigInteger value)
    {
        setUnsigned(path, value, false);
    }

    public void setDouble(String path, double value)
    {
        setDouble(path, value, false);
    }

    public void setBoolean(String path, boolean value)
    {
        setBoolean(path, value, false);
    }

    public void setString(String path, String value)
    {
        setString(path, value, false);
    }

    public void setNULL(String path)
    {
        setNULL(path, false);
    }

    public void setBytes(String path, byte[] value)
    {
        setBytes(path, value, false);
    }

    public int getInt(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return 0;
        }
        return (int)getInt(mTraitInstancePtr, path);
    }

    public long getLong(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return 0;
        }
        return (long)getInt(mTraitInstancePtr, path);
    }

    public BigInteger getBigInteger(String path, int bitLen)
    {
        long value = (long)getInt(mTraitInstancePtr, path);
        String valStr = Long.toString(value);
        BigInteger bigInt = new BigInteger(valStr);
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return bigInt;
        }
        if (bitLen != 8 && bitLen != 16 && bitLen != 32 && bitLen != 64)
        {
            Log.e(TAG, "Not support bit len, return original value" + bitLen);
            return bigInt;
        }
        BigInteger twoComplement = BigInteger.ONE.shiftLeft(bitLen);

        if (bigInt.compareTo(BigInteger.ZERO) < 0)
            bigInt = bigInt.add(twoComplement);
        if (bigInt.compareTo(twoComplement) >= 0)
        {
            Log.e(TAG, "overflow range:" + bitLen);
            return twoComplement;
        }
        else if (bigInt.compareTo(BigInteger.ZERO) < 0 )
        {
            Log.e(TAG, "incorrect range:" + bitLen);
            return BigInteger.ZERO;
        }
        else
        {
            return bigInt;
        }
    }

    public double getDouble(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return 0;
        }
        return getDouble(mTraitInstancePtr, path);
    }

    public boolean getBoolean(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return false;
        }
        return getBoolean(mTraitInstancePtr, path);
    }

    public String getString(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return null;
        }
        return getString(mTraitInstancePtr, path);
    }

    public byte[] getBytes(String path)
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return null;
        }
        return getBytes(mTraitInstancePtr, path);
    }

    public long getVersion()
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return 0;
        }
        return getVersion(mTraitInstancePtr);
    }

    public void beginRefreshData()
    {
        if (mTraitInstancePtr == 0)
        {
            Log.e(TAG, "unexpected err, mTraitInstancePtr is 0");
            return;
        }
        beginRefreshData(mTraitInstancePtr);
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

    private void onRefreshDataComplete()
    {
        if (mCompHandler == null)
        {
            Log.e(TAG, "unexpected err, mCompHandler is null");
            return;
        }
        mCompHandler.onRefreshDataComplete();
    }

    public interface CompletionHandler
    {
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

    private long mTraitInstancePtr;
    private WDMClient mWDMClient;
    private final static String TAG = GenericTraitUpdatableDataSink.class.getSimpleName();

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void init(long genericTraitUpdatableDataSinkPtr);
    private native void close(long genericTraitUpdatableDataSinkPtr);
    private native void beginRefreshData(long genericTraitUpdatableDataSinkPtr);
    private native void setInt(long genericTraitUpdatableDataSinkPtr, String path, long value, boolean isConditional, boolean isSigned);
    private native void setDouble(long genericTraitUpdatableDataSinkPtr, String path, double value, boolean isConditional);
    private native void setBoolean(long genericTraitUpdatableDataSinkPtr, String path, boolean value, boolean isConditional);
    private native void setString(long genericTraitUpdatableDataSinkPtr, String path, String value, boolean isConditional);
    private native void setNULL(long genericTraitUpdatableDataSinkPtr, String path, boolean isConditional);
    private native void setBytes(long genericTraitUpdatableDataSinkPtr, String path, byte[] value, boolean isConditional);
    private native long getInt(long genericTraitUpdatableDataSinkPtr, String path);
    private native double getDouble(long genericTraitUpdatableDataSinkPtr, String path);
    private native boolean getBoolean(long genericTraitUpdatableDataSinkPtr, String path);
    private native String getString(long genericTraitUpdatableDataSinkPtr, String path);
    private native byte[] getBytes(long genericTraitUpdatableDataSinkPtr, String path);
    private native long getVersion(long genericTraitUpdatableDataSinkPtr);
};
