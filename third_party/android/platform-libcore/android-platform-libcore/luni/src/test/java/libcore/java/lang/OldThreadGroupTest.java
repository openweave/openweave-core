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

package libcore.java.lang;

import java.util.Vector;
import junit.framework.TestCase;

public class OldThreadGroupTest extends TestCase implements Thread.UncaughtExceptionHandler {

    class MyThread extends Thread {
        public volatile int heartBeat = 0;

        public MyThread(ThreadGroup group, String name)
                throws SecurityException, IllegalThreadStateException {
            super(group, name);
        }

        @Override
        public void run() {
            while (true) {
                heartBeat++;
                try {
                    Thread.sleep(50);
                } catch (InterruptedException e) {
                    break;
                }
            }
        }

        public boolean isActivelyRunning() {
            long MAX_WAIT = 100;
            return isActivelyRunning(MAX_WAIT);
        }

        public boolean isActivelyRunning(long maxWait) {
            int beat = heartBeat;
            long start = System.currentTimeMillis();
            do {
                Thread.yield();
                int beat2 = heartBeat;
                if (beat != beat2) {
                    return true;
                }
            } while (System.currentTimeMillis() - start < maxWait);
            return false;
        }

    }

    private ThreadGroup initialThreadGroup = null;

    public void test_activeGroupCount() {
        ThreadGroup tg = new ThreadGroup("group count");
        assertEquals("Incorrect number of groups",
                0, tg.activeGroupCount());
        Thread t1 = new Thread(tg, new Runnable() {
            public void run() {

            }
        });
        assertEquals("Incorrect number of groups",
                0, tg.activeGroupCount());
        t1.start();
        assertEquals("Incorrect number of groups",
                0, tg.activeGroupCount());
        new ThreadGroup(tg, "test group 1");
        assertEquals("Incorrect number of groups",
                1, tg.activeGroupCount());
        new ThreadGroup(tg, "test group 2");
        assertEquals("Incorrect number of groups",
                2, tg.activeGroupCount());
    }

    @SuppressWarnings("deprecation")
    public void test_allowThreadSuspensionZ() {
        ThreadGroup tg = new ThreadGroup("thread suspension");
        assertTrue("Thread suspention can not be changed",
                tg.allowThreadSuspension(false));
        assertTrue("Thread suspention can not be changed",
                tg.allowThreadSuspension(true));
    }

    /*
     * Checks whether the current Thread is in the given list.
     */
    private boolean inListOfThreads(Thread[] threads) {
        for (int i = 0; i < threads.length; i++) {
            if (Thread.currentThread() == threads[i]) {
                return true;
            }
        }

        return false;
    }

    public void test_enumerateLThreadArray() {
        int numThreads = initialThreadGroup.activeCount();
        Thread[] listOfThreads = new Thread[numThreads];

        int countThread = initialThreadGroup.enumerate(listOfThreads);
        assertEquals(numThreads, countThread);
        assertTrue("Current thread must be in enumeration of threads",
                inListOfThreads(listOfThreads));
    }

    public void test_enumerateLThreadArrayLZtest_enumerateLThreadArrayLZ() {
        int numThreads = initialThreadGroup.activeCount();
        Thread[] listOfThreads = new Thread[numThreads];

        int countThread = initialThreadGroup.enumerate(listOfThreads, false);
        assertEquals(numThreads, countThread);

        countThread = initialThreadGroup.enumerate(listOfThreads, true);
        assertEquals(numThreads, countThread);
        assertTrue("Current thread must be in enumeration of threads",
                inListOfThreads(listOfThreads));

        ThreadGroup subGroup = new ThreadGroup(initialThreadGroup, "Test Group 1");
        int subThreadsCount = 3;
        Vector<MyThread> subThreads = populateGroupsWithThreads(subGroup,
                subThreadsCount);

        countThread = initialThreadGroup.enumerate(listOfThreads, true);
        assertEquals(numThreads, countThread);
        assertTrue("Current thread must be in enumeration of threads",
                inListOfThreads(listOfThreads));

        for(MyThread thr:subThreads) {
            thr.start();
        }
        // lets give them some time to start
        try {
            Thread.sleep(500);
        } catch (InterruptedException ie) {
            fail("Should not be interrupted");
        }

        int numThreads2 = initialThreadGroup.activeCount();
        listOfThreads = new Thread[numThreads2];

        assertEquals(numThreads + subThreadsCount, numThreads2);

        countThread = initialThreadGroup.enumerate(listOfThreads, true);
        assertEquals(numThreads2, countThread);
        assertTrue("Current thread must be in enumeration of threads",
                inListOfThreads(listOfThreads));

        for(MyThread thr:subThreads) {
            thr.interrupt();
        }
        // lets give them some time to die
        try {
            Thread.sleep(500);
        } catch (InterruptedException ie) {
            fail("Should not be interrupted");
        }

        int numThreads3 = initialThreadGroup.activeCount();
        listOfThreads = new Thread[numThreads3];

        assertEquals(numThreads, numThreads3);

        countThread = initialThreadGroup.enumerate(listOfThreads, false);
        assertEquals(numThreads3, countThread);
        assertTrue("Current thread must be in enumeration of threads",
                inListOfThreads(listOfThreads));
    }

    public void test_enumerateLThreadGroupArray() {
        int numGroupThreads = initialThreadGroup.activeGroupCount();
        ThreadGroup[] listOfGroups = new ThreadGroup[numGroupThreads];

        int countGroupThread = initialThreadGroup.enumerate(listOfGroups);
        assertEquals(numGroupThreads, countGroupThread);

        ThreadGroup[] listOfGroups1 = new ThreadGroup[numGroupThreads + 1];
        countGroupThread = initialThreadGroup.enumerate(listOfGroups1);
        assertEquals(numGroupThreads, countGroupThread);
        assertNull(listOfGroups1[listOfGroups1.length - 1]);

        ThreadGroup[] listOfGroups2 = new ThreadGroup[numGroupThreads - 1];
        countGroupThread = initialThreadGroup.enumerate(listOfGroups2);
        assertEquals(numGroupThreads - 1, countGroupThread);

        ThreadGroup thrGroup1 = new ThreadGroup("Test Group 1");
        countGroupThread = thrGroup1.enumerate(listOfGroups);
        assertEquals(0, countGroupThread);
     }

    public void test_enumerateLThreadGroupArrayLZ() {
        ThreadGroup thrGroup = new ThreadGroup("Test Group 1");
        Vector<MyThread> subThreads = populateGroupsWithThreads(thrGroup, 3);
        int numGroupThreads = thrGroup.activeGroupCount();
        ThreadGroup[] listOfGroups = new ThreadGroup[numGroupThreads];

        assertEquals(0, thrGroup.enumerate(listOfGroups, true));
        assertEquals(0, thrGroup.enumerate(listOfGroups, false));

        for(MyThread thr:subThreads) {
            thr.start();
        }

        numGroupThreads = thrGroup.activeGroupCount();
        listOfGroups = new ThreadGroup[numGroupThreads];

        assertEquals(0, thrGroup.enumerate(listOfGroups, true));
        assertEquals(0, thrGroup.enumerate(listOfGroups, false));

        ThreadGroup subGroup1 = new ThreadGroup(thrGroup, "Test Group 2");
        Vector<MyThread> subThreads1 = populateGroupsWithThreads(subGroup1, 3);
        numGroupThreads = thrGroup.activeGroupCount();
        listOfGroups = new ThreadGroup[numGroupThreads];

        assertEquals(1, thrGroup.enumerate(listOfGroups, true));
        assertEquals(1, thrGroup.enumerate(listOfGroups, false));

        for(MyThread thr:subThreads1) {
            thr.start();
        }
        numGroupThreads = thrGroup.activeGroupCount();
        listOfGroups = new ThreadGroup[numGroupThreads];

        assertEquals(1, thrGroup.enumerate(listOfGroups, true));
        assertEquals(1, thrGroup.enumerate(listOfGroups, false));

        for(MyThread thr:subThreads) {
            thr.interrupt();
         }

        ThreadGroup subGroup2 = new ThreadGroup(subGroup1, "Test Group 3");
        Vector<MyThread> subThreads2 = populateGroupsWithThreads(subGroup2, 3);
        numGroupThreads = thrGroup.activeGroupCount();
        listOfGroups = new ThreadGroup[numGroupThreads];

        assertEquals(2, thrGroup.enumerate(listOfGroups, true));
        assertEquals(1, thrGroup.enumerate(listOfGroups, false));
    }

    /**
     * @tests java.lang.ThreadGroup#interrupt()
     */
    private static boolean interrupted = false;
    public void test_interrupt() {

        Thread.setDefaultUncaughtExceptionHandler(this);
        ThreadGroup tg = new ThreadGroup("interrupt");
        Thread t1 = new Thread(tg, new Runnable() {
            public void run() {
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException e) {
                    fail("ok");
                }
            }
        });
        assertFalse("Incorrect state of thread", interrupted);
        t1.start();
        assertFalse("Incorrect state of thread", interrupted);
        t1.interrupt();
        try {
            t1.join();
        } catch (InterruptedException e) {
        }
        assertTrue("Incorrect state of thread", interrupted);
        tg.destroy();
    }

    public void test_isDestroyed() {
        final ThreadGroup originalCurrent = getInitialThreadGroup();
        final ThreadGroup testRoot = new ThreadGroup(originalCurrent,
                "Test group");
        assertFalse("Test group is not destroyed yet",
                testRoot.isDestroyed());
        testRoot.destroy();
        assertTrue("Test group already destroyed",
                testRoot.isDestroyed());
    }

    @SuppressWarnings("deprecation")
    public void test_resume() {
        ThreadGroup group = new ThreadGroup("Foo");

        Thread thread = launchFiveSecondDummyThread(group);

        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            // Ignore
        }

        // No-op in Android. Must neither have an effect nor throw an exception.
        Thread.State state = thread.getState();
        group.resume();
        assertEquals(state, thread.getState());
    }

    private Thread launchFiveSecondDummyThread(ThreadGroup group) {
        Thread thread = new Thread(group, "Bar") {
            public void run() {
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException e) {
                    // Ignore
                }
            }
        };

        thread.start();

        return thread;
    }

    /*
     * @see java.lang.Thread.UncaughtExceptionHandler#uncaughtException(java.lang.Thread, java.lang.Throwable)
     */
    public void uncaughtException(Thread t, Throwable e) {
        interrupted = true;
        Thread.setDefaultUncaughtExceptionHandler(null);
    }

    @Override
    protected void setUp() {
        initialThreadGroup = Thread.currentThread().getThreadGroup();
        ThreadGroup rootThreadGroup = initialThreadGroup;
        while (rootThreadGroup.getParent() != null) {
            rootThreadGroup = rootThreadGroup.getParent();
        }
    }

    @Override
    protected void tearDown() {
        try {
            // Give the threads a chance to die.
            Thread.sleep(50);
        } catch (InterruptedException e) {
        }
    }

    private ThreadGroup getInitialThreadGroup() {
        return initialThreadGroup;
    }

    private ThreadGroup[] groups(ThreadGroup parent) {
        // No API to get the count of immediate children only ?
        int count = parent.activeGroupCount();
        ThreadGroup[] all = new ThreadGroup[count];
        parent.enumerate(all, false);
        // Now we may have nulls in the array, we must find the actual size
        int actualSize = 0;
        for (; actualSize < all.length; actualSize++) {
            if (all[actualSize] == null) {
                break;
            }
        }
        ThreadGroup[] result;
        if (actualSize == all.length) {
            result = all;
        } else {
            result = new ThreadGroup[actualSize];
            System.arraycopy(all, 0, result, 0, actualSize);
        }

        return result;

    }

    private Vector<MyThread> populateGroupsWithThreads(final ThreadGroup aGroup,
            final int threadCount) {
        Vector<MyThread> result = new Vector<MyThread>();
        populateGroupsWithThreads(aGroup, threadCount, result);
        return result;

    }

    private void populateGroupsWithThreads(final ThreadGroup aGroup,
            final int threadCount, final Vector<MyThread> allCreated) {
        for (int i = 0; i < threadCount; i++) {
            final int iClone = i;
            final String name = "(MyThread)N =" + iClone + "/" + threadCount
                    + " ,Vector size at creation: " + allCreated.size();

            MyThread t = new MyThread(aGroup, name);
            allCreated.addElement(t);
        }

        // Recursively for subgroups (if any)
        ThreadGroup[] children = groups(aGroup);
        for (ThreadGroup element : children) {
            populateGroupsWithThreads(element, threadCount, allCreated);
        }

    }
}
