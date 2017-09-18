/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package java.security;

import java.io.IOException;
import java.io.InvalidObjectException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.ObjectStreamField;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.Map;

/**
 * Specific {@code PermissionCollection} for storing {@code BasicPermissions} of
 * arbitrary type.
 *
 * @see BasicPermission
 * @see PermissionCollection
 */
final class BasicPermissionCollection extends PermissionCollection {

    private static final long serialVersionUID = 739301742472979399L;

    private static final ObjectStreamField[] serialPersistentFields = {
        new ObjectStreamField("all_allowed", Boolean.TYPE),
        new ObjectStreamField("permissions", Hashtable.class),
        new ObjectStreamField("permClass", Class.class), };

    //should be final, but because of writeObject() cannot be
    private transient Map<String, Permission> items = new HashMap<String, Permission>();

    // true if this Collection contains a BasicPermission with '*' as its permission name
    private transient boolean allEnabled; // = false;

    private Class<? extends Permission> permClass;

    /**
     * Adds a permission to the collection. The first added permission must be a
     * subclass of BasicPermission, next permissions must be of the same class
     * as the first one.
     *
     * @see java.security.PermissionCollection#add(java.security.Permission)
     */
    @Override
    public void add(Permission permission) {
        if (isReadOnly()) {
            throw new SecurityException("collection is read-only");
        }
        if (permission == null) {
            throw new IllegalArgumentException("permission == null");
        }

        Class<? extends Permission> inClass = permission.getClass();
        if (permClass != null) {
            if (permClass != inClass) {
                throw new IllegalArgumentException("Invalid permission: " + permission);
            }
        } else if( !(permission instanceof BasicPermission)) {
            throw new IllegalArgumentException("Invalid permission: " + permission);
        } else {
            // this is the first element provided that another thread did not add
            synchronized (this) {
                if (permClass != null && inClass != permClass) {
                    throw new IllegalArgumentException("Invalid permission: " + permission);
                }
                permClass = inClass;
            }
        }

        String name = permission.getName();
        items.put(name, permission);
        allEnabled = allEnabled || (name.length() == 1 && '*' == name.charAt(0));
    }

    /**
     * Returns enumeration of contained elements.
     */
    @Override
    public Enumeration<Permission> elements() {
        return Collections.enumeration(items.values());
    }

    /**
     * Indicates whether the argument permission is implied by the receiver.
     *
     * @return boolean {@code true} if the argument permission is implied by the
     *         receiver, and {@code false} if it is not.
     * @param permission
     *            the permission to check.
     * @see Permission
     */
    @Override
    public boolean implies(Permission permission) {
        if (permission == null || permission.getClass() != permClass) {
            return false;
        }
        if (allEnabled) {
            return true;
        }
        String checkName = permission.getName();
        //first check direct coincidence
        if (items.containsKey(checkName)) {
            return true;
        }
        //now check if there are suitable wildcards
        //suppose we have "a.b.c", let's check "a.b.*" and "a.*"
        char[] name = checkName.toCharArray();
        //I presume that "a.b.*" does not imply "a.b."
        //so the dot at end is ignored
        int pos = name.length - 2;
        for (; pos >= 0; pos--) {
            if (name[pos] == '.') {
                break;
            }
        }
        while (pos >= 0) {
            name[pos + 1] = '*';
            if (items.containsKey(new String(name, 0, pos + 2))) {
                return true;
            }
            for (--pos; pos >= 0; pos--) {
                if (name[pos] == '.') {
                    break;
                }
            }
        }
        return false;
    }

    /**
     * Expected format is the following:
     * <dl>
     * <dt>boolean all_allowed
     * <dd>This is set to true if this BasicPermissionCollection contains a
     * {@code BasicPermission} with '*' as its permission name.
     * <dt>Class&lt;T&gt; permClass
     * <dd>The class to which all {@code BasicPermission}s in this
     * BasicPermissionCollection belongs.
     * <dt>Hashtable&lt;K,V&gt; permissions
     * <dd>The {@code BasicPermission}s in this collection. All {@code
     * BasicPermission}s in the collection must belong to the same class. The
     * Hashtable is indexed by the {@code BasicPermission} name; the value of
     * the Hashtable entry is the permission.
     * </dl>
     */
    private void writeObject(java.io.ObjectOutputStream out) throws IOException {
        ObjectOutputStream.PutField fields = out.putFields();
        fields.put("all_allowed", allEnabled);
        fields.put("permissions", new Hashtable<String, Permission>(items));
        fields.put("permClass", permClass);
        out.writeFields();
    }

    /**
     * Reads the object from stream and checks its consistency: all contained
     * permissions must be of the same subclass of BasicPermission.
     */
    private void readObject(java.io.ObjectInputStream in) throws IOException,
        ClassNotFoundException {
        ObjectInputStream.GetField fields = in.readFields();

        items = new HashMap<String, Permission>();
        synchronized (this) {
            permClass = (Class<? extends Permission>)fields.get("permClass", null);
            items.putAll((Hashtable<String, Permission>) fields.get(
                    "permissions", new Hashtable<String, Permission>()));
            for (Iterator<Permission> iter = items.values().iterator(); iter.hasNext();) {
                if (iter.next().getClass() != permClass) {
                    throw new InvalidObjectException("Inconsistent types of contained permissions");
                }
            }
            allEnabled = fields.get("all_allowed", false);
            if (allEnabled && !items.containsKey("*")) {
                throw new InvalidObjectException("Invalid state of wildcard flag");
            }
        }
    }
}
