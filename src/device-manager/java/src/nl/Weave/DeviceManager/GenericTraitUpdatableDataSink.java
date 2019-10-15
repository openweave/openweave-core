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


package nl.Weave.DeviceManager;

import android.os.Build;
import android.util.Log;

import java.util.EnumSet;
import java.util.Random;
import java.util.HashMap;
import java.util.Map;
import java.util.Iterator;

public class GenericTraitUpdatableDataSink
{
    public GenericTraitUpdatableDataSink(long traitInstancePtr, long wDMClientPtr)
    {
        mWDMClientPtr = wDMClientPtr;
        mTraitInstancePtr = traitInstancePtr;
        mCompHandler = null;
    }

    public void close()
    {
        if (mTraitInstancePtr != 0)
            close(mTraitInstancePtr);

        mTraitInstancePtr = 0;
        mCompHandler = null;
    }

    public void setSigned(String path, long value, boolean isConditional)
    {
        boolean isSigned = true;
        String result = Long.toString(value);
        setData(mTraitInstancePtr, path, result, isConditional, isSigned);
    }

    public void setUnsigned(String path, byte value, boolean isConditional)
    {
        boolean isSigned = false;
        long number = Byte.toUnsignedLong(value);
        String result = Long.toString(number);
        setData(mTraitInstancePtr, path, result, isConditional, isSigned);
    }

    public void setUnsigned(String path, short value, boolean isConditional)
    {
        boolean isSigned = false;
        long number = Short.toUnsignedLong(value);
        String result = Long.toString(number);
        setData(mTraitInstancePtr, path, result, isConditional, isSigned);
    }

    public void setUnsigned(String path, int value, boolean isConditional)
    {
        boolean isSigned = false;
        long number = Integer.toUnsignedLong(value);
        String result = Long.toString(number);
        setData(mTraitInstancePtr, path, result, isConditional, isSigned);
    }

    public void setUnsigned(String path, long value, boolean isConditional)
    {
        boolean isSigned = false;
        String result = Long.toUnsignedString(value);
        setData(mTraitInstancePtr, path, result, isConditional, isSigned);
    }

    public void setDouble(String path, double value, boolean isConditional)
    {
        setDouble(mTraitInstancePtr, path, value, isConditional);
    }

    public void setBoolean(String path, boolean value, boolean isConditional)
    {
        setBoolean(mTraitInstancePtr, path, value, isConditional);
    }

    public void setLeafBytes(String path, byte[] value, boolean isConditional)
    {
        setLeafBytes(mTraitInstancePtr, path, value, isConditional);
    }

    public void setString(String path, String value, boolean isConditional)
    {
        setString(mTraitInstancePtr, path, value, isConditional);
    }

    public byte getByteData(String path)
    {
        return (byte)getData(mTraitInstancePtr, path);
    }

    public short getShortData(String path)
    {
        return (short)getData(mTraitInstancePtr, path);
    }

    public int getIntData(String path)
    {
        return (int)getData(mTraitInstancePtr, path);
    }

    public long getLongData(String path)
    {
        return (long)getData(mTraitInstancePtr, path);
    }

    public double getDouble(String path)
    {
        return getDouble(mTraitInstancePtr, path);
    }

    public boolean getBoolean(String path)
    {
        return getBoolean(mTraitInstancePtr, path);
    }

    public String getString(String path)
    {
        return getString(mTraitInstancePtr, path);
    }

    public byte[] getLeafBytes(String path)
    {
        return getLeafBytes(mTraitInstancePtr, path);
    }

    public void beginRefreshData()
    {
        beginRefreshData(mTraitInstancePtr, mWDMClientPtr);
    }

    public void onError(Throwable err)
    {
        mCompHandler.onError(err);
    }

    public interface CompletionHandler
    {
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
    private long mWDMClientPtr;
    private final static String TAG = GenericTraitUpdatableDataSink.class.getSimpleName();

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void close(long genericTraitUpdatableDataSinkPtr);
    private native void beginRefreshData(long genericTraitUpdatableDataSinkPtr, long wdmClientPtr);
    private native void setData(long genericTraitUpdatableDataSinkPtr, String path, String value, boolean isConditional, boolean isSigned);
    private native void setDouble(long genericTraitUpdatableDataSinkPtr, String path, double value, boolean isConditional);
    private native void setBoolean(long genericTraitUpdatableDataSinkPtr, String path, boolean value, boolean isConditional);
    private native void setString(long genericTraitUpdatableDataSinkPtr, String path, String value, boolean isConditional);
    private native void setLeafBytes(long genericTraitUpdatableDataSinkPtr, String path, byte[] value, boolean isConditional);
    private native long getData(long genericTraitUpdatableDataSinkPtr, String path);
    private native double getDouble(long genericTraitUpdatableDataSinkPtr, String path);
    private native boolean getBoolean(long genericTraitUpdatableDataSinkPtr, String path);
    private native String getString(long genericTraitUpdatableDataSinkPtr, String path);
    private native byte[] getLeafBytes(long genericTraitUpdatableDataSinkPtr, String path);
};
