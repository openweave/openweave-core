/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package java.lang;

import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.EnumSet;
import java.util.HashSet;
import java.util.List;
import org.apache.harmony.kernel.vm.LangAccess;
import org.apache.harmony.kernel.vm.ReflectionAccess;

/**
 * Cache of per-class data, meant to help the performance of reflection
 * methods.
 *
 * <p><b>Note:</b> None of the methods perform access checks. It is up
 * to the (package internal) clients of this code to perform such
 * checks as necessary.</p>
 *
 * <p><b>Also Note:</b> None of the returned array values are
 * protected in any way. It is up to the (again, package internal)
 * clients of this code to protect the arrays if they should ever
 * escape the package.</p>
 */
/*package*/ class ClassCache<T> {
    // TODO: Add caching for constructors and fields.

    /** non-null; comparator used for enumerated values */
    private static final EnumComparator ENUM_COMPARATOR =
        new EnumComparator();

    /** non-null; reflection access bridge */
    /*package*/ static final ReflectionAccess REFLECT = getReflectionAccess();

    /** non-null; class that this instance represents */
    private final Class<T> clazz;

    /** null-ok; list of all public methods, both direct and inherited */
    private volatile Method[] methods;

    /** null-ok; list of all declared methods */
    private volatile Method[] declaredMethods;

    /** null-ok; list of all public declared methods */
    private volatile Method[] declaredPublicMethods;

    /** null-ok; list of all declared fields */
    private volatile Field[] declaredFields;

    /** null-ok; list of all public declared fields */
    private volatile Field[] declaredPublicFields;

    /** null-ok; list of all fields, both direct and inherited */
    private volatile Field[] allFields;

    /** null-ok; list of all public fields, both direct and inherited */
    private volatile Field[] allPublicFields;

    /**
     * null-ok; array of enumerated values in their original order, if this
     * instance's class is an enumeration
     */
    private volatile T[] enumValuesInOrder;

    /**
     * null-ok; array of enumerated values sorted by name, if this
     * instance's class is an enumeration
     */
    private volatile T[] enumValuesByName;

    static {
        /*
         * Provide access to this package from java.util as part of
         * bootstrap. TODO: See if this can be removed in favor of the
         * simpler mechanism below. (That is, see if EnumSet will be
         * happy calling LangAccess.getInstance().)
         */
        Field field;

        try {
            field = EnumSet.class.getDeclaredField("LANG_BOOTSTRAP");
            REFLECT.setAccessibleNoCheck(field, true);
        } catch (NoSuchFieldException ex) {
            // This shouldn't happen because the field is in fact defined.
            throw new AssertionError(ex);
        }

        try {
            field.set(null, LangAccessImpl.THE_ONE);
        } catch (IllegalAccessException ex) {
            // This shouldn't happen because we made the field accessible.
            throw new AssertionError(ex);
        }

        // Also set up the bootstrap-classpath-wide access mechanism.
        LangAccess.setInstance(LangAccessImpl.THE_ONE);
    }

    /**
     * Constructs an instance.
     *
     * @param clazz non-null; class that this instance represents
     */
    /*package*/ ClassCache(Class<T> clazz) {
        if (clazz == null) {
            throw new NullPointerException("clazz == null");
        }

        this.clazz = clazz;
    }

    /**
     * Gets the list of all declared methods.
     *
     * @return non-null; the list of all declared methods
     */
    public Method[] getDeclaredMethods() {
        if (declaredMethods == null) {
            declaredMethods = Class.getDeclaredMethods(clazz, false);
        }

        return declaredMethods;
    }

    /**
     * Gets the list of all declared public methods.
     *
     * @return non-null; the list of all declared public methods
     */
    private Method[] getDeclaredPublicMethods() {
        if (declaredPublicMethods == null) {
            declaredPublicMethods = Class.getDeclaredMethods(clazz, true);
        }

        return declaredPublicMethods;
    }

    /**
     * Returns public methods defined by {@code clazz}, its superclasses and all
     * implemented interfaces, not including overridden methods. This method
     * performs no security checks.
     */
    public Method[] getMethods() {
        Method[] cachedResult = methods;
        if (cachedResult == null) {
            methods = findMethods();
        }

        return methods;
    }

    private Method[] findMethods() {
        List<Method> allMethods = new ArrayList<Method>();
        getMethodsRecursive(clazz, allMethods);

        /*
         * Remove methods defined by multiple types, preferring to keep methods
         * declared by derived types.
         *
         * Classes may define multiple methods with the same name and parameter
         * types due to covariant return types. In this case both are returned,
         * with the non-synthetic method first because it is preferred by
         * getMethod(String,Class[]).
         */
        Collections.sort(allMethods, Method.ORDER_BY_SIGNATURE);
        List<Method> natural = new ArrayList<Method>(allMethods.size());
        List<Method> synthetic = new ArrayList<Method>(allMethods.size());
        Method previous = null;
        for (Method method : allMethods) {
            if (previous != null
                    && Method.ORDER_BY_SIGNATURE.compare(method, previous) == 0
                    && method.getDeclaringClass() != previous.getDeclaringClass()) {
                continue;
            }
            if (method.isSynthetic()) {
                synthetic.add(method);
            } else {
                natural.add(method);
            }
            previous = method;
        }
        List<Method> result = new ArrayList<Method>(allMethods.size());
        result.addAll(natural);
        result.addAll(synthetic);
        return result.toArray(new Method[result.size()]);
    }

    /**
     * Populates {@code sink} with public methods defined by {@code clazz}, its
     * superclasses, and all implemented interfaces, including overridden methods.
     * This method performs no security checks.
     */
    private static void getMethodsRecursive(Class<?> clazz, List<Method> result) {
        for (Class<?> c = clazz; c != null; c = c.getSuperclass()) {
            for (Method method : c.getClassCache().getDeclaredPublicMethods()) {
                result.add(method);
            }
        }

        for (Class<?> ifc : clazz.getInterfaces()) {
            getMethodsRecursive(ifc, result);
        }
    }

    /**
     * Finds and returns a method with a given name and signature. Use
     * this with one of the method lists returned by instances of this class.
     *
     * @param list non-null; the list of methods to search through
     * @param parameterTypes non-null; the formal parameter list
     * @return non-null; the matching method
     * @throws NoSuchMethodException thrown if the method does not exist
     */
    public static Method findMethodByName(Method[] list, String name,
            Class<?>[] parameterTypes) throws NoSuchMethodException {
        if (name == null) {
            throw new NullPointerException("Method name must not be null.");
        }
        for (Method method : list) {
            if (method.getName().equals(name)
                    && compareClassLists(method.getParameterTypes(), parameterTypes)) {
                return method;
            }
        }

        throw new NoSuchMethodException(name);
    }

    /**
     * Compares two class lists for equality. Empty and
     * <code>null</code> lists are considered equal. This is useful
     * for matching methods and constructors.
     *
     * <p>TODO: Take into account assignment compatibility?</p>
     *
     * @param a null-ok; the first list of types
     * @param b null-ok; the second list of types
     * @return true if and only if the lists are equal
     */
    public static boolean compareClassLists(Class<?>[] a, Class<?>[] b) {
        if (a == null) {
            return (b == null) || (b.length == 0);
        }

        int length = a.length;

        if (b == null) {
            return (length == 0);
        }

        if (length != b.length) {
            return false;
        }

        for (int i = length - 1; i >= 0; i--) {
            if (a[i] != b[i]) {
                return false;
            }
        }

        return true;
    }

    /**
     * Makes a deep copy of the given array of methods. This is useful
     * when handing out arrays from the public API.
     *
     * <p><b>Note:</b> In such cases, it is insufficient to just make
     * a shallow copy of the array, since method objects aren't
     * immutable due to the existence of {@link
     * AccessibleObject#setAccessible}.</p>
     *
     * @param orig non-null; array to copy
     * @return non-null; a deep copy of the given array
     */
    public static Method[] deepCopy(Method[] orig) {
        int length = orig.length;
        Method[] result = new Method[length];

        for (int i = length - 1; i >= 0; i--) {
            result[i] = REFLECT.clone(orig[i]);
        }

        return result;
    }

    /**
     * Gets the list of all declared fields.
     *
     * @return non-null; the list of all declared fields
     */
    public Field[] getDeclaredFields() {
        if (declaredFields == null) {
            declaredFields = Class.getDeclaredFields(clazz, false);
        }

        return declaredFields;
    }

    /**
     * Gets the list of all declared public fields.
     *
     * @return non-null; the list of all declared public fields
     */
    public Field[] getDeclaredPublicFields() {
        if (declaredPublicFields == null) {
            declaredPublicFields = Class.getDeclaredFields(clazz, true);
        }

        return declaredPublicFields;
    }

    /**
     * Gets either the list of declared fields or the list of declared
     * public fields.
     *
     * @param publicOnly whether to only return public fields
     */
    public Field[] getDeclaredFields(boolean publicOnly) {
        return publicOnly ? getDeclaredPublicFields() : getDeclaredFields();
    }

    /**
     * Gets the list of all fields, both directly
     * declared and inherited.
     *
     * @return non-null; the list of all fields
     */
    public Field[] getAllFields() {
        if (allFields == null) {
            allFields = getFullListOfFields(false);
        }

        return allFields;
    }

    /**
     * Gets the list of all public fields, both directly
     * declared and inherited.
     *
     * @return non-null; the list of all public fields
     */
    public Field[] getAllPublicFields() {
        if (allPublicFields == null) {
            allPublicFields = getFullListOfFields(true);
        }

        return allPublicFields;
    }

    /*
     * Returns the list of fields without performing any security checks
     * first. This includes the fields inherited from superclasses. If no
     * fields exist at all, an empty array is returned.
     *
     * @param publicOnly reflects whether we want only public fields
     * or all of them
     * @return the list of fields
     */
    private Field[] getFullListOfFields(boolean publicOnly) {
        ArrayList<Field> fields = new ArrayList<Field>();
        HashSet<String> seen = new HashSet<String>();

        findAllfields(clazz, fields, seen, publicOnly);

        return fields.toArray(new Field[fields.size()]);
    }

    /**
     * Collects the list of fields without performing any security checks
     * first. This includes the fields inherited from superclasses and from
     * all implemented interfaces. The latter may also implement multiple
     * interfaces, so we (potentially) recursively walk through a whole tree of
     * classes. If no fields exist at all, an empty array is returned.
     *
     * @param clazz non-null; class to inspect
     * @param fields non-null; the target list to add the results to
     * @param seen non-null; a set of signatures we've already seen
     * @param publicOnly reflects whether we want only public fields
     * or all of them
     */
    private static void findAllfields(Class<?> clazz,
            ArrayList<Field> fields, HashSet<String> seen,
            boolean publicOnly) {

        // Traverse class and superclasses, get rid of dupes by signature
        while (clazz != null) {
            for (Field field : clazz.getClassCache().getDeclaredFields(publicOnly)) {
                String signature = field.toString();
                if (!seen.contains(signature)) {
                    fields.add(field);
                    seen.add(signature);
                }
            }

            // Traverse all interfaces, and do the same recursively.
            Class<?>[] interfaces = clazz.getInterfaces();
            for (Class<?> intf : interfaces) {
                findAllfields(intf, fields, seen, publicOnly);
            }

            clazz = clazz.getSuperclass();
        }
    }

    /**
     * Finds and returns a field with a given name and signature. Use
     * this with one of the field lists returned by instances of this class.
     *
     * @param list non-null; the list of fields to search through
     * @return non-null; the matching field
     * @throws NoSuchFieldException thrown if the field does not exist
     */
    public static Field findFieldByName(Field[] list, String name)
            throws NoSuchFieldException {
        if (name == null) {
            throw new NullPointerException("Field name must not be null.");
        }
        for (Field field : list) {
            if (field.getName().equals(name)) {
                return field;
            }
        }

        throw new NoSuchFieldException(name);
    }

    /**
     * Makes a deep copy of the given array of fields. This is useful
     * when handing out arrays from the public API.
     *
     * <p><b>Note:</b> In such cases, it is insufficient to just make
     * a shallow copy of the array, since field objects aren't
     * immutable due to the existence of {@link
     * AccessibleObject#setAccessible}.</p>
     *
     * @param orig non-null; array to copy
     * @return non-null; a deep copy of the given array
     */
    public static Field[] deepCopy(Field[] orig) {
        int length = orig.length;
        Field[] result = new Field[length];

        for (int i = length - 1; i >= 0; i--) {
            result[i] = REFLECT.clone(orig[i]);
        }

        return result;
    }

    /**
     * Gets the enumerated value with a given name.
     *
     * @param name non-null; name of the value
     * @return null-ok; the named enumerated value or <code>null</code>
     * if this instance's class doesn't have such a value (including
     * if this instance isn't in fact an enumeration)
     */
    @SuppressWarnings("unchecked")
    public T getEnumValue(String name) {
        Enum[] values = (Enum[]) getEnumValuesByName();

        if (values == null) {
            return null;
        }

        // Binary search.

        int min = 0;
        int max = values.length - 1;

        while (min <= max) {
            /*
             * The guessIdx calculation is equivalent to ((min + max)
             * / 2) but won't go wonky when min and max are close to
             * Integer.MAX_VALUE.
             */
            int guessIdx = min + ((max - min) >> 1);
            Enum guess = values[guessIdx];
            int cmp = name.compareTo(guess.name());

            if (cmp < 0) {
                max = guessIdx - 1;
            } else if (cmp > 0) {
                min = guessIdx + 1;
            } else {
                return (T) guess;
            }
        }

        return null;
    }

    /**
     * Gets the array of enumerated values, sorted by name.
     *
     * @return null-ok; the value array, or <code>null</code> if this
     * instance's class isn't in fact an enumeration
     */
    public T[] getEnumValuesByName() {
        if (enumValuesByName == null) {
            T[] values = getEnumValuesInOrder();

            if (values != null) {
                values = (T[]) values.clone();
                Arrays.sort((Enum<?>[]) values, ENUM_COMPARATOR);

                /*
                 * Note: It's only safe (concurrency-wise) to set the
                 * instance variable after the array is properly sorted.
                 */
                enumValuesByName = values;
            }
        }

        return enumValuesByName;
    }

    /**
     * Gets the array of enumerated values, in their original declared
     * order.
     *
     * @return null-ok; the value array, or <code>null</code> if this
     * instance's class isn't in fact an enumeration
     */
    public T[] getEnumValuesInOrder() {
        if ((enumValuesInOrder == null) && clazz.isEnum()) {
            enumValuesInOrder = callEnumValues();
        }

        return enumValuesInOrder;
    }

    /**
     * Calls the static method <code>values()</code> on this
     * instance's class, which is presumed to be a properly-formed
     * enumeration class, using proper privilege hygiene.
     *
     * @return non-null; the array of values as reported by
     * <code>value()</code>
     */
    @SuppressWarnings("unchecked")
    private T[] callEnumValues() {
        Method method;

        try {
            Method[] methods = getDeclaredPublicMethods();
            method = findMethodByName(methods, "values", (Class[]) null);
            method = REFLECT.accessibleClone(method);
        } catch (NoSuchMethodException ex) {
            // This shouldn't happen if the class is a well-formed enum.
            throw new UnsupportedOperationException(ex);
        }

        try {
            return (T[]) method.invoke((Object[]) null);
        } catch (IllegalAccessException ex) {
            // This shouldn't happen because the method is "accessible."
            throw new Error(ex);
        } catch (InvocationTargetException ex) {
            Throwable te = ex.getTargetException();
            if (te instanceof RuntimeException) {
                throw (RuntimeException) te;
            } else if (te instanceof Error) {
                throw (Error) te;
            } else {
                throw new Error(te);
            }
        }
    }

    /**
     * Gets the reflection access object. This uses reflection to do
     * so. My head is spinning.
     *
     * @return non-null; the reflection access object
     */
    private static ReflectionAccess getReflectionAccess() {
        /*
         * Note: We can't do AccessibleObject.class.getCache() to
         * get the cache, since that would cause a circularity in
         * initialization. So instead, we do a direct call into the
         * native side.
         */
        Method[] methods =
            Class.getDeclaredMethods(AccessibleObject.class, false);

        try {
            Method method = findMethodByName(methods, "getReflectionAccess",
                    (Class[]) null);
            Class.setAccessibleNoCheck(method, true);
            return (ReflectionAccess) method.invoke((Object[]) null);
        } catch (NoSuchMethodException ex) {
            /*
             * This shouldn't happen because the method
             * AccessibleObject.getReflectionAccess() really is defined
             * in this module.
             */
            throw new Error(ex);
        } catch (IllegalAccessException ex) {
            // This shouldn't happen because the method is "accessible."
            throw new Error(ex);
        } catch (InvocationTargetException ex) {
            throw new Error(ex);
        }
    }

    /**
     * Comparator class for enumerated values. It compares strictly
     * by name.
     */
    private static class EnumComparator implements Comparator<Enum<?>> {
        public int compare(Enum<?> e1, Enum<?> e2) {
            return e1.name().compareTo(e2.name());
        }
    }
}
