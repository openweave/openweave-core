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
 *      Represents GenericTraitUpdatableDataSink Interface.
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

public interface GenericTraitUpdatableDataSink
{
    public void setSigned(String path, int value, boolean isConditional);
    public void setSigned(String path, long value, boolean isConditional);
    public void setSigned(String path, BigInteger value, boolean isConditional);
    public void setUnsigned(String path, int value, boolean isConditional);
    public void setUnsigned(String path, long value, boolean isConditional);
    public void setUnsigned(String path, BigInteger value, boolean isConditional);
    public void set(String path, double value, boolean isConditional);
    public void set(String path, boolean value, boolean isConditional);
    public void set(String path, String value, boolean isConditional);
    public void set(String path, byte[] value, boolean isConditional);
    public void setNull(String path, boolean isConditional);
    public void setSigned(String path, int value);
    public void setSigned(String path, long value);
    public void setSigned(String path, BigInteger value);
    public void setUnsigned(String path, int value);
    public void setUnsigned(String path, long value);
    public void setUnsigned(String path, BigInteger value);
    public void set(String path, double value);
    public void set(String path, boolean value);
    public void set(String path, String value);
    public void set(String path, byte[] value);
    public void setNull(String path);
    public int getInt(String path);
    public long getLong(String path);
    public BigInteger getBigInteger(String path);
    public double getDouble(String path);
    public boolean getBoolean(String path);
    public String getString(String path);
    public byte[] getBytes(String path);
    public long getVersion();
    public void beginRefreshData();

    public CompletionHandler getCompletionHandler();
    public void setCompletionHandler(CompletionHandler compHandler);

    public interface CompletionHandler
    {
        void onRefreshDataComplete();
        void onError(Throwable err);
    }
};
