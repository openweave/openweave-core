/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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

package java.security;

import java.util.ArrayList;
import java.util.List;
import java.util.WeakHashMap;
import org.apache.harmony.security.fortress.SecurityUtils;

/**
 * {@code AccessController} provides static methods to perform access control
 * checks and privileged operations.
 */
public final class AccessController {

    private AccessController() {
        throw new Error("statics only.");
    }

    /**
     * A map used to store a mapping between a given Thread and
     * AccessControllerContext-s used in successive calls of doPrivileged(). A
     * WeakHashMap is used to allow automatic wiping of the dead threads from
     * the map. The thread (normally Thread.currentThread()) is used as a key
     * for the map, and a value is ArrayList where all AccessControlContext-s
     * are stored.
     * ((ArrayList)contexts.get(Thread.currentThread())).lastElement() - is
     * reference to the latest context passed to the doPrivileged() call.
     *
     * TODO: ThreadLocal?
     */
    private static final WeakHashMap<Thread, ArrayList<AccessControlContext>> contexts = new WeakHashMap<Thread, ArrayList<AccessControlContext>>();

    /**
     * Returns the result of executing the specified privileged action. Only the
     * {@code ProtectionDomain} of the direct caller of this method and the
     * {@code ProtectionDomain}s of all subsequent classes in the call chain are
     * checked to be granted the necessary permission if access checks are
     * performed.
     * <p>
     * If an instance of {@code RuntimeException} is thrown during the execution
     * of the {@code PrivilegedAction#run()} method of the given action, it will
     * be propagated through this method.
     *
     * @param action
     *            the action to be executed with privileges
     * @return the result of executing the privileged action
     * @throws NullPointerException
     *             if the specified action is {@code null}
     */
    public static <T> T doPrivileged(PrivilegedAction<T> action) {
        if (action == null) {
            throw new NullPointerException("action == null");
        }
        return doPrivileged(action, null);
    }

    /**
     * Returns the result of executing the specified privileged action. The
     * {@code ProtectionDomain} of the direct caller of this method, the {@code
     * ProtectionDomain}s of all subsequent classes in the call chain and all
     * {@code ProtectionDomain}s of the given context are checked to be granted
     * the necessary permission if access checks are performed.
     * <p>
     * If an instance of {@code RuntimeException} is thrown during the execution
     * of the {@code PrivilegedAction#run()} method of the given action, it will
     * be propagated through this method.
     *
     * @param action
     *            the action to be executed with privileges
     * @param context
     *            the {@code AccessControlContext} whose protection domains are
     *            checked additionally
     * @return the result of executing the privileged action
     * @throws NullPointerException
     *             if the specified action is {@code null}
     */
    public static <T> T doPrivileged(PrivilegedAction<T> action,
            AccessControlContext context) {
        if (action == null) {
            throw new NullPointerException("action == null");
        }
        List<AccessControlContext> contextsStack = contextsForThread();
        contextsStack.add(context);
        try {
            return action.run();
        } finally {
            contextsStack.remove(contextsStack.size() - 1);
        }
    }

    /**
     * Returns the result of executing the specified privileged action. Only the
     * {@code ProtectionDomain} of the direct caller of this method and the
     * {@code ProtectionDomain}s of all subsequent classes in the call chain are
     * checked to be granted the necessary permission if access checks are
     * performed.
     * <p>
     * If a checked exception is thrown by the action's run method, it will be
     * wrapped and propagated through this method.
     * <p>
     * If an instance of {@code RuntimeException} is thrown during the execution
     * of the {@code PrivilegedAction#run()} method of the given action, it will
     * be propagated through this method.
     *
     * @param action
     *            the action to be executed with privileges
     * @return the result of executing the privileged action
     * @throws PrivilegedActionException
     *             if the action's run method throws any checked exception
     * @throws NullPointerException
     *             if the specified action is {@code null}
     */
    public static <T> T doPrivileged(PrivilegedExceptionAction<T> action)
            throws PrivilegedActionException {
        return doPrivileged(action, null);
    }

    /**
     * Returns the result of executing the specified privileged action. The
     * {@code ProtectionDomain} of the direct caller of this method, the {@code
     * ProtectionDomain}s of all subsequent classes in the call chain and all
     * {@code ProtectionDomain}s of the given context are checked to be granted
     * the necessary permission if access checks are performed.
     * <p>
     * If a checked exception is thrown by the action's run method, it will be
     * wrapped and propagated through this method.
     * <p>
     * If an instance of {@code RuntimeException} is thrown during the execution
     * of the {@code PrivilegedAction#run()} method of the given action, it will
     * be propagated through this method.
     *
     * @param action
     *            the action to be executed with privileges
     * @param context
     *            the {@code AccessControlContext} whose protection domains are
     *            checked additionally
     * @return the result of executing the privileged action
     * @throws PrivilegedActionException
     *             if the action's run method throws any checked exception
     * @throws NullPointerException
     *             if the specified action is {@code null}
     */
    public static <T> T doPrivileged(PrivilegedExceptionAction<T> action,
            AccessControlContext context) throws PrivilegedActionException {
        if (action == null) {
            throw new NullPointerException("action == null");
        }
        List<AccessControlContext> contextsStack = contextsForThread();
        contextsStack.add(context);
        try {
            return action.run();
        } catch (RuntimeException e) {
            throw e; // so we don't wrap RuntimeExceptions with PrivilegedActionException
        } catch (Exception e) {
            throw new PrivilegedActionException(e);
        } finally {
            contextsStack.remove(contextsStack.size() - 1);
        }
    }

    public static <T> T doPrivilegedWithCombiner(PrivilegedAction<T> action) {
        return doPrivileged(action, newContextSameDomainCombiner());
    }

    public static <T> T doPrivilegedWithCombiner(PrivilegedExceptionAction<T> action)
            throws PrivilegedActionException {
        return doPrivileged(action, newContextSameDomainCombiner());
    }

    private static AccessControlContext newContextSameDomainCombiner() {
        List<AccessControlContext> contextsStack = contextsForThread();
        DomainCombiner domainCombiner = contextsStack.isEmpty()
                ? null
                : contextsStack.get(contextsStack.size() - 1).getDomainCombiner();
        return new AccessControlContext(new ProtectionDomain[0], domainCombiner);
    }

    private static List<AccessControlContext> contextsForThread() {
        Thread currThread = Thread.currentThread();

        /*
         * Thread.currentThread() is null when Thread.class is being initialized, and contexts is
         * null when AccessController.class is still being initialized. In either case, return an
         * empty list so callers need not worry.
         */
        if (currThread == null || contexts == null) {
            return new ArrayList<AccessControlContext>();
        }

        synchronized (contexts) {
            ArrayList<AccessControlContext> result = contexts.get(currThread);
            if (result == null) {
                result = new ArrayList<AccessControlContext>();
                contexts.put(currThread, result);
            }
            return result;
        }
    }

    /**
     * Checks the specified permission against the VM's current security policy.
     * The check is performed in the context of the current thread. This method
     * returns silently if the permission is granted, otherwise an {@code
     * AccessControlException} is thrown.
     * <p>
     * A permission is considered granted if every {@link ProtectionDomain} in
     * the current execution context has been granted the specified permission.
     * If privileged operations are on the execution context, only the {@code
     * ProtectionDomain}s from the last privileged operation are taken into
     * account.
     * <p>
     * This method delegates the permission check to
     * {@link AccessControlContext#checkPermission(Permission)} on the current
     * callers' context obtained by {@link #getContext()}.
     *
     * @param permission
     *            the permission to check against the policy
     * @throws AccessControlException
     *             if the specified permission is not granted
     * @throws NullPointerException
     *             if the specified permission is {@code null}
     * @see AccessControlContext#checkPermission(Permission)
     *
     */
    public static void checkPermission(Permission permission)
            throws AccessControlException {
        if (permission == null) {
            throw new NullPointerException("permission == null");
        }

        getContext().checkPermission(permission);
    }

    /**
     * Returns array of ProtectionDomains from the classes residing on the stack
     * of the current thread, up to and including the caller of the nearest
     * privileged frame. Reflection frames are skipped. The returned array is
     * never null and never contains null elements, meaning that bootstrap
     * classes are effectively ignored.
     */
    private static native ProtectionDomain[] getStackDomains();

    /**
     * Returns the {@code AccessControlContext} for the current {@code Thread}
     * including the inherited access control context of the thread that spawned
     * the current thread (recursively).
     * <p>
     * The returned context may be used to perform access checks at a later
     * point in time, possibly by another thread.
     *
     * @return the {@code AccessControlContext} for the current {@code Thread}
     * @see Thread#currentThread
     */
    public static AccessControlContext getContext() {

        // duplicates (if any) will be removed in ACC constructor
        ProtectionDomain[] stack = getStackDomains();

        Thread currentThread = Thread.currentThread();
        if (currentThread == null || AccessController.contexts == null) {
            // Big boo time. No need to check anything ?
            return new AccessControlContext(stack);
        }

        List<AccessControlContext> threadContexts = contextsForThread();

        // if we're in a doPrivileged method, use its context.
        AccessControlContext that = threadContexts.isEmpty()
                ? SecurityUtils.getContext(currentThread)
                : threadContexts.get(threadContexts.size() - 1);

        if (that != null && that.combiner != null) {
            ProtectionDomain[] assigned = null;
            if (that.context != null && that.context.length != 0) {
                assigned = new ProtectionDomain[that.context.length];
                System.arraycopy(that.context, 0, assigned, 0, assigned.length);
            }
            ProtectionDomain[] protectionDomains = that.combiner.combine(stack, assigned);
            if (protectionDomains == null) {
                protectionDomains = new ProtectionDomain[0];
            }
            return new AccessControlContext(protectionDomains, that.combiner);
        }

        return new AccessControlContext(stack, that);
    }
}
