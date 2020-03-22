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
    /**
     * clear the whole trait data.
     */
    public void clear();

    /**
     * Assigns the provided value to the given path as a signed integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the int value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setSigned(String path, int value, boolean isConditional);

    /**
     * Assigns the provided value to the given path as a signed integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the long value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setSigned(String path, long value, boolean isConditional);

    /**
     * Assigns the provided value to the given path as a signed integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the BigInteger value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setSigned(String path, BigInteger value, boolean isConditional);

    /**
     * Assigns the provided value to the given path as an unsigned integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the int value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setUnsigned(String path, int value, boolean isConditional);

    /**
     * Assigns the provided value to the given path as an unsigned integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the int value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setUnsigned(String path, long value, boolean isConditional);

    /**
     * Assigns the provided value to the given path as an unsigned integer value.
     *
     * @param path the proto path to the property to modify
     * @param value the BigInteger value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setUnsigned(String path, BigInteger value, boolean isConditional);

    /**
     * Assigns the provided value to the given path.
     *
     * @param path the proto path to the property to modify
     * @param value the double value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void set(String path, double value, boolean isConditional);

    /**
     * Assigns the provided value to the given path.
     *
     * @param path the proto path to the property to modify
     * @param value the boolean value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void set(String path, boolean value, boolean isConditional);

    /**
     * Assigns the provided value to the given path.
     *
     * @param path the proto path to the property to modify
     * @param value the string value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void set(String path, String value, boolean isConditional);

    /**
     * Assigns the provided value to the given path.
     *
     * @param path the proto path to the property to modify
     * @param value the bytes value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void set(String path, byte[] value, boolean isConditional);

    /**
     * Assigns the provided value to the given path.
     *
     * @param path the proto path to the property to modify
     * @param value the string array value to assign to the property
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void set(String path, String[] value, boolean isConditional);

    /**
     * Assigns Null to the given path.
     *
     * @param path the proto path to the property to modify
     * @param isConditional whether or not to allow overwriting any conflicting changes. If true, then if a later
     *     version of the trait has modified this property and does not equal to required version from update,
     *     this update will be dropped; otherwise, this value will overwrite the newer change
     */
    public void setNull(String path, boolean isConditional);

    /**
     * Assigns the provided value to the given path as a signed integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the int value to assign to the property
     */
    public void setSigned(String path, int value);

    /**
     * Assigns the provided value to the given path as a signed integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the long value to assign to the property
     */
    public void setSigned(String path, long value);

    /**
     * Assigns the provided value to the given path as a signed integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the BigInteger value to assign to the property
     */
    public void setSigned(String path, BigInteger value);

    /**
     * Assigns the provided value to the given path as a unsigned integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the int value to assign to the property
     */
    public void setUnsigned(String path, int value);

    /**
     * Assigns the provided value to the given path as a unsigned integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the long value to assign to the property
     */
    public void setUnsigned(String path, long value);

    /**
     * Assigns the provided value to the given path as a unsigned integer value with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the BigInteger value to assign to the property
     */
    public void setUnsigned(String path, BigInteger value);

    /**
     * Assigns the provided value to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the double value to assign to the property
     */
    public void set(String path, double value);

    /**
     * Assigns the provided value to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the boolean value to assign to the property
     */
    public void set(String path, boolean value);

    /**
     * Assigns the provided value to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the String value to assign to the property
     */
    public void set(String path, String value);

    /**
     * Assigns the provided value to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the bytes value to assign to the property
     */
    public void set(String path, byte[] value);

    /**
     * Assigns the provided value to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     * @param value the string array to assign to the property
     */
    public void set(String path, String[] value);

    /**
     * Assigns Null to the given path with unconditional capability
     *
     * @param path the proto path to the property to modify
     */
    public void setNull(String path);

    /**
     * Returns the int value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public int getInt(String path);

    /**
     * Returns the long value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public long getLong(String path);

    /**
     * Returns the BigInteger value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public BigInteger getBigInteger(String path);

    /**
     * Returns the double value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public double getDouble(String path);

    /**
     * Returns the Boolean value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public boolean getBoolean(String path);

    /**
     * Returns the string value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public String getString(String path);

    /**
     * Returns the Bytes value assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public byte[] getBytes(String path);

    /**
     * Check if null property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public boolean isNull(String path);

    /**
     * Returns the string array assigned to the property at the given path within this trait.
     *
     * @throws Exception if no property exists at this path, or if the type of the property does not match
     */
    public String[] getStringArray(String path);

    /** Returns the version of the trait represented by this data sink. */
    public long getVersion();

    /** Delete the trait property data on on particular path. */
    public void deleteData(String path);

    /**
     * Begins a sync of the trait data. The result of this operation can be observed through the {@link CompletionHandler}
     * that has been assigned via {@link #setCompletionHandler}.
     */
    public void beginRefreshData();

    public CompletionHandler getCompletionHandler();
    public void setCompletionHandler(CompletionHandler compHandler);

    public interface CompletionHandler
    {
        void onRefreshDataComplete();
        void onError(Throwable err);
    }
};
