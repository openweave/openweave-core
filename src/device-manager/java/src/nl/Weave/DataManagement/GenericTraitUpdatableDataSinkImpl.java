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
 *      Represents GenericTraitUpdatableDataSink Implementation using native GenericTraitUpdatableDataSink APIs.
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
import java.util.Objects;

public class GenericTraitUpdatableDataSinkImpl implements GenericTraitUpdatableDataSink
{
    @Override
    public void clear()
    {
        if (mTraitInstancePtr != 0)
        {
            clear(mTraitInstancePtr);
        }
    }

    @Override
    public void setSigned(String path, int value, boolean isConditional)
    {
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, value, isConditional, /* isSigned= */ true);
    }

    @Override
    public void setSigned(String path, long value, boolean isConditional)
    {
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, value, isConditional, /* isSigned= */ true);
    }

    @Override
    public void setSigned(String path, BigInteger value, boolean isConditional)
    {
        long convertedVal = value.longValue();
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, convertedVal, isConditional, /* isSigned= */ true);
    }

    @Override
    public void setUnsigned(String path, int value, boolean isConditional)
    {
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, value, isConditional, /* isSigned= */ false);
    }

    @Override
    public void setUnsigned(String path, long value, boolean isConditional)
    {
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, value, isConditional, /* isSigned= */ false);
    }

    @Override
    public void setUnsigned(String path, BigInteger value, boolean isConditional)
    {
        long convertedVal = value.longValue();
        ensureNotClosed();
        setInt(mTraitInstancePtr, path, convertedVal, isConditional, /* isSigned= */ false);
    }

    @Override
    public void set(String path, double value, boolean isConditional)
    {
        ensureNotClosed();
        setDouble(mTraitInstancePtr, path, value, isConditional);
    }

    @Override
    public void set(String path, boolean value, boolean isConditional)
    {
        ensureNotClosed();
        setBoolean(mTraitInstancePtr, path, value, isConditional);
    }

    @Override
    public void set(String path, String value, boolean isConditional)
    {
        ensureNotClosed();
        setString(mTraitInstancePtr, path, value, isConditional);
    }

    @Override
    public void set(String path, byte[] value, boolean isConditional)
    {
        ensureNotClosed();
        setBytes(mTraitInstancePtr, path, value, isConditional);
    }

    @Override
    public void setNull(String path, boolean isConditional)
    {
        ensureNotClosed();
        setNull(mTraitInstancePtr, path, isConditional);
    }

    @Override
    public void set(String path, String[] value, boolean isConditional)
    {
        ensureNotClosed();
        setStringArray(mTraitInstancePtr, path, value, isConditional);
    }

    @Override
    public void setSigned(String path, int value)
    {
        setSigned(path, value, false);
    }

    @Override
    public void setSigned(String path, long value)
    {
        setSigned(path, value, false);
    }

    @Override
    public void setSigned(String path, BigInteger value)
    {
        setSigned(path, value, false);
    }

    @Override
    public void setUnsigned(String path, int value)
    {
        setUnsigned(path, value, false);
    }

    @Override
    public void setUnsigned(String path, long value)
    {
        setUnsigned(path, value, false);
    }

    @Override
    public void setUnsigned(String path, BigInteger value)
    {
        setUnsigned(path, value, false);
    }

    @Override
    public void set(String path, double value)
    {
        set(path, value, false);
    }

    @Override
    public void set(String path, boolean value)
    {
        set(path, value, false);
    }

    @Override
    public void set(String path, String value)
    {
        set(path, value, false);
    }

    @Override
    public void set(String path, byte[] value)
    {
        set(path, value, false);
    }

    @Override
    public void setNull(String path)
    {
        setNull(path, false);
    }

    @Override
    public void set(String path, String[] value)
    {
        ensureNotClosed();
        setStringArray(mTraitInstancePtr, path, value, false);
    }

    @Override
    public int getInt(String path)
    {
        ensureNotClosed();
        return (int)getLong(mTraitInstancePtr, path);
    }

    @Override
    public long getLong(String path)
    {
        ensureNotClosed();
        return getLong(mTraitInstancePtr, path);
    }

    @Override
    public BigInteger getBigInteger(String path)
    {
        ensureNotClosed();

        long value = getLong(mTraitInstancePtr, path);
        BigInteger bigInt = BigInteger.valueOf(value);
        return bigInt;
    }

    @Override
    public double getDouble(String path)
    {
        ensureNotClosed();
        return getDouble(mTraitInstancePtr, path);
    }

    @Override
    public boolean getBoolean(String path)
    {
        ensureNotClosed();
        return getBoolean(mTraitInstancePtr, path);
    }

    @Override
    public String getString(String path)
    {
        ensureNotClosed();
        return getString(mTraitInstancePtr, path);
    }

    @Override
    public byte[] getBytes(String path)
    {
        ensureNotClosed();
        return getBytes(mTraitInstancePtr, path);
    }

    @Override
    public boolean isNull(String path)
    {
        ensureNotClosed();
        return isNull(mTraitInstancePtr, path);
    }

    @Override
    public String[] getStringArray(String path)
    {
        ensureNotClosed();
        return getStringArray(mTraitInstancePtr, path);
    }

    @Override
    public long getVersion()
    {
        ensureNotClosed();
        return getVersion(mTraitInstancePtr);
    }

    @Override
    public void deleteData(String path)
    {
        ensureNotClosed();
        deleteData(mTraitInstancePtr, path);
    }

    @Override
    public void beginRefreshData()
    {
        ensureNotClosed();
        beginRefreshData(mTraitInstancePtr);
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

    // ----- Protected Members -----
    protected GenericTraitUpdatableDataSinkImpl(long traitInstancePtr, WdmClientImpl wdmClient)
    {
        mWdmClient = wdmClient;
        mTraitInstancePtr = traitInstancePtr;
        init(mTraitInstancePtr);
    }

    protected void shutdown()
    {
        if (mTraitInstancePtr != 0)
        {
            shutdown(mTraitInstancePtr);
            mTraitInstancePtr = 0;
        }
        mCompHandler = null;
    }

    protected void finalize() throws Throwable
    {
        super.finalize();
        clear();
    }

    // ----- Private Members -----

    private void onError(Throwable err)
    {
        requireCompletionHandler().onError(err);
    }

    private void onRefreshDataComplete()
    {
        requireCompletionHandler().onRefreshDataComplete();
    }

    private void ensureNotClosed() {
      if (mTraitInstancePtr == 0) {
        throw new IllegalStateException("This data sink has already been closed.");
      }
    }

    private CompletionHandler requireCompletionHandler() {
      return Objects.requireNonNull(
          mCompHandler, "No CompletionHandler set. Did you forget to call setCompletionHandler()?");
    }

    private long mTraitInstancePtr;
    private WdmClientImpl mWdmClient;
    private final static String TAG = GenericTraitUpdatableDataSink.class.getSimpleName();
    private CompletionHandler mCompHandler;

    static {
        System.loadLibrary("WeaveDeviceManager");
    }

    private native void init(long genericTraitUpdatableDataSinkPtr);
    private native void shutdown(long genericTraitUpdatableDataSinkPtr);
    private native void clear(long genericTraitUpdatableDataSinkPtr);
    private native void beginRefreshData(long genericTraitUpdatableDataSinkPtr);
    private native void setInt(long genericTraitUpdatableDataSinkPtr, String path, long value, boolean isConditional, boolean isSigned);
    private native void setDouble(long genericTraitUpdatableDataSinkPtr, String path, double value, boolean isConditional);
    private native void setBoolean(long genericTraitUpdatableDataSinkPtr, String path, boolean value, boolean isConditional);
    private native void setString(long genericTraitUpdatableDataSinkPtr, String path, String value, boolean isConditional);
    private native void setNull(long genericTraitUpdatableDataSinkPtr, String path, boolean isConditional);
    private native void setBytes(long genericTraitUpdatableDataSinkPtr, String path, byte[] value, boolean isConditional);
    private native void setStringArray(long genericTraitUpdatableDataSinkPtr, String path, String[] value, boolean isConditional);
    private native long getLong(long genericTraitUpdatableDataSinkPtr, String path);
    private native double getDouble(long genericTraitUpdatableDataSinkPtr, String path);
    private native boolean getBoolean(long genericTraitUpdatableDataSinkPtr, String path);
    private native String getString(long genericTraitUpdatableDataSinkPtr, String path);
    private native byte[] getBytes(long genericTraitUpdatableDataSinkPtr, String path);
    private native boolean isNull(long genericTraitUpdatableDataSinkPtr, String path);
    private native String[] getStringArray(long genericTraitUpdatableDataSinkPtr, String path);
    private native long getVersion(long genericTraitUpdatableDataSinkPtr);
    private native void deleteData(long genericTraitUpdatableDataSinkPtr, String path);
};
