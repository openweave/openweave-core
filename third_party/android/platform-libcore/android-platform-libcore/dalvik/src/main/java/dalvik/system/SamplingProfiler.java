/*
 * Copyright (C) 2010 The Android Open Source Project
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

package dalvik.system;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map.Entry;
import java.util.Map;
import java.util.Set;
import java.util.Timer;
import java.util.TimerTask;

/**
 * A sampling profiler. It currently is implemented without any
 * virtual machine support, relying solely on {@code
 * Thread.getStackTrace} to collect samples. As such, the overhead is
 * higher than a native approach and it does not provide insight into
 * where time is spent within native code, but it can still provide
 * useful insight into where a program is spending time.
 *
 * <h3>Usage Example</h3>
 *
 * The following example shows how to use the {@code
 * SamplingProfiler}. It samples the current thread's stack to a depth
 * of 12 stack frame elements over two different measurement periods
 * at a rate of 10 samples a second. In then prints the results in
 * hprof format to the standard output.
 *
 * <pre> {@code
 * ThreadSet threadSet = SamplingProfiler.newArrayThreadSet(Thread.currentThread());
 * SamplingProfiler profiler = new SamplingProfiler(12, threadSet);
 * profiler.start(10);
 * // period of measurement
 * profiler.stop();
 * // period of non-measurement
 * profiler.start(10);
 * // another period of measurement
 * profiler.stop();
 * profiler.shutdown();
 * profiler.writeHprofData(System.out);
 * }</pre>
 *
 * @hide
 */
public final class SamplingProfiler {

    /*
     *  Real hprof output examples don't start the thread and trace
     *  identifiers at one but seem to start at these arbitrary
     *  constants. It certainly seems useful to have relatively unique
     *  identifers when manual searching hprof output.
     */
    private int nextThreadId = 200001;
    private int nextTraceId = 300001;
    private int nextObjectId = 1;

    /**
     * Map of currently active threads to their identifiers. When
     * threads disappear they are removed and only referenced by their
     * identifiers to prevent retaining garbage threads.
     */
    private final Map<Thread, Integer> threadIds = new HashMap<Thread, Integer>();

    /**
     * List of thread creation and death events.
     */
    private final List<ThreadEvent> threadHistory = new ArrayList<ThreadEvent>();

    /**
     * Map of stack traces to a mutable sample count.
     */
    private final Map<Trace, int[]> traces = new HashMap<Trace, int[]>();

    /**
     * Timer that is used for the lifetime of the profiler
     */
    // note that dalvik/vm/Thread.c depends on this name
    private final Timer timer = new Timer("SamplingProfiler", true);

    /**
     * A sampler is created every time profiling starts and cleared
     * everytime profiling stops because once a {@code TimerTask} is
     * canceled it cannot be reused.
     */
    private TimerTask sampler;

    /**
     * The maximum number of {@code StackTraceElements} to retain in
     * each stack.
     */
    private final int depth;

    /**
     * The {@code ThreadSet} that identifies which threads to sample.
     */
    private final ThreadSet threadSet;

    /**
     * Create a sampling profiler that collects stacks with the
     * specified depth from the threads specified by the specified
     * thread collector.
     *
     * @param depth The maximum stack depth to retain for each sample
     * similar to the hprof option of the same name. Any stack deeper
     * than this will be truncated to this depth. A good starting
     * value is 4 although it is not uncommon to need to raise this to
     * get enough context to understand program behavior. While
     * programs with extensive recursion may require a high value for
     * depth, simply passing in a value for Integer.MAX_VALUE is not
     * advised because of the significant memory need to retain such
     * stacks and runtime overhead to compare stacks.
     */
    public SamplingProfiler(int depth, ThreadSet threadSet) {
        this.depth = depth;
        this.threadSet = threadSet;
    }

    /**
     * A ThreadSet specifies the set of threads to sample.
     */
    public static interface ThreadSet {
        /**
         * Returns an array containing the threads to be sampled. The
         * array may be longer than the number of threads to be
         * sampled, in which case the extra elements must be null.
         */
        public Thread[] threads();
    }

    /**
     * Returns a ThreadSet for a fixed set of threads that will not
     * vary at runtime. This has less overhead than a dynamically
     * calculated set, such as {@link newThreadGroupTheadSet}, which has
     * to enumerate the threads each time profiler wants to collect
     * samples.
     */
    public static ThreadSet newArrayThreadSet(Thread... threads) {
        return new ArrayThreadSet(threads);
    }

    /**
     * An ArrayThreadSet samples a fixed set of threads that does not
     * vary over the life of the profiler.
     */
    private static class ArrayThreadSet implements ThreadSet {
        private final Thread[] threads;
        public ArrayThreadSet(Thread... threads) {
            if (threads == null) {
                throw new NullPointerException("threads == null");
            }
            this.threads = threads;
        }
        public Thread[] threads() {
            return threads;
        }
    }

    /**
     * Returns a ThreadSet that is dynamically computed based on the
     * threads found in the specified ThreadGroup and that
     * ThreadGroup's children.
     */
    public static ThreadSet newThreadGroupTheadSet(ThreadGroup threadGroup) {
        return new ThreadGroupThreadSet(threadGroup);
    }

    /**
     * An ThreadGroupThreadSet sample the threads from the specified
     * ThreadGroup and the ThreadGroup's children
     */
    private static class ThreadGroupThreadSet implements ThreadSet {
        private final ThreadGroup threadGroup;
        private Thread[] threads;
        private int lastThread;

        public ThreadGroupThreadSet(ThreadGroup threadGroup) {
            if (threadGroup == null) {
                throw new NullPointerException("threadGroup == null");
            }
            this.threadGroup = threadGroup;
            resize();
        }

        private void resize() {
            int count = threadGroup.activeCount();
            // we can only tell if we had enough room for all active
            // threads if we actually are larger than the the number of
            // active threads. making it larger also leaves us room to
            // tolerate additional threads without resizing.
            threads = new Thread[count*2];
            lastThread = 0;
        }

        public Thread[] threads() {
            int threadCount;
            while (true) {
                threadCount = threadGroup.enumerate(threads);
                if (threadCount == threads.length) {
                    resize();
                } else {
                    break;
                }
            }
            if (threadCount < lastThread) {
                // avoid retaining pointers to threads that have ended
                Arrays.fill(threads, threadCount, lastThread, null);
            }
            lastThread = threadCount;
            return threads;
        }
    }

    /**
     * Starts profiler sampling at the specified rate.
     *
     * @param samplesPerSecond The number of samples to take a second
     */
    public void start(int samplesPerSecond) {
        if (samplesPerSecond < 1) {
            throw new IllegalArgumentException("samplesPerSecond < 1");
        }
        if (sampler != null) {
            throw new IllegalStateException("profiling already started");
        }
        sampler = new Sampler();
        timer.scheduleAtFixedRate(sampler, 0, 1000/samplesPerSecond);
    }

    /**
     * Stops profiler sampling. It can be restarted with {@link
     * #start(int)} to continue sampling.
     */
    public void stop() {
        if (sampler == null) {
            return;
        }
        sampler.cancel();
        sampler = null;
    }

    /**
     * Shuts down profiling after which it can not be restarted. It is
     * important to shut down profiling when done to free resources
     * used by the profiler. Shutting down the profiler also stops the
     * profiling if that has not already been done.
     */
    public void shutdown() {
        stop();
        timer.cancel();
    }

    /**
     * The Sampler does the real work of the profiler.
     *
     * At every sample time, it asks the thread set for the set
     * of threads to sample. It maintains a history of thread creation
     * and death events based on changes observed to the threads
     * returned by the {@code ThreadSet}.
     *
     * For each thread to be sampled, a stack is collected and used to
     * update the set of collected samples. Stacks are truncated to a
     * maximum depth. There is no way to tell if a stack has been truncated.
     */
    private class Sampler extends TimerTask {

        private Thread timerThread;
        private Thread[] currentThreads = new Thread[0];
        private final Trace mutableTrace = new Trace();

        public void run() {
            if (timerThread == null) {
                timerThread = Thread.currentThread();
            }

            // process thread creation and death first so that we
            // assign thread ids to any new threads before allocating
            // new stacks for them
            Thread[] newThreads = threadSet.threads();
            if (!Arrays.equals(currentThreads, newThreads)) {
                updateThreadHistory(currentThreads, newThreads);
                currentThreads = newThreads.clone();
            }

            for (Thread thread : currentThreads) {
                if (thread == null) {
                    break;
                }
                if (thread == timerThread) {
                    continue;
                }

                // TODO replace with a VMStack.getThreadStackTrace
                // variant to avoid allocating unneeded elements
                StackTraceElement[] stack = thread.getStackTrace();
                if (stack.length > depth) {
                    stack = Arrays.copyOfRange(stack, 0, depth);
                }

                mutableTrace.threadId = threadIds.get(thread);
                mutableTrace.stack = stack;

                int[] count = traces.get(mutableTrace);
                if (count == null) {
                    Trace trace = new Trace(nextTraceId++, mutableTrace);
                    count = new int[1];
                    traces.put(trace, count);
                }
                count[0]++;
            }
        }

        private void updateThreadHistory(Thread[] oldThreads, Thread[] newThreads) {
            // thread start/stop shouldn't happen too often and
            // these aren't too big, so hopefully this approach
            // won't be too slow...
            Set<Thread> n = new HashSet<Thread>(Arrays.asList(newThreads));
            Set<Thread> o = new HashSet<Thread>(Arrays.asList(oldThreads));

            // added = new-old
            Set<Thread> added = new HashSet<Thread>(n);
            added.removeAll(o);

            // removed = old-new
            Set<Thread> removed = new HashSet<Thread>(o);
            removed.removeAll(n);

            for (Thread thread : added) {
                if (thread == null) {
                    continue;
                }
                if (thread == timerThread) {
                    continue;
                }
                int threadId = nextThreadId++;
                threadIds.put(thread, threadId);
                ThreadEvent event = ThreadEvent.start(nextObjectId++,threadId, thread);
                threadHistory.add(event);
            }
            for (Thread thread : removed) {
                if (thread == null) {
                    continue;
                }
                if (thread == timerThread) {
                    continue;
                }
                int threadId = threadIds.remove(thread);
                ThreadEvent event = ThreadEvent.stop(threadId);
                threadHistory.add(event);
            }
        }
    }

    private enum ThreadEventType { START, STOP };

    /**
     * ThreadEvent represents thread creation and death events for
     * reporting. It provides a record of the thread and thread group
     * names for tying samples back to their source thread.
     */
    private static class ThreadEvent {

        private final ThreadEventType type;
        private final int objectId;
        private final int threadId;
        private final String threadName;
        private final String groupName;

        private static ThreadEvent start(int objectId, int threadId, Thread thread) {
            return new ThreadEvent(ThreadEventType.START, objectId, threadId, thread);
        }

        private static ThreadEvent stop(int threadId) {
            return new ThreadEvent(ThreadEventType.STOP, threadId);
        }

        private ThreadEvent(ThreadEventType type, int objectId, int threadId, Thread thread) {
            this.type = ThreadEventType.START;
            this.objectId = objectId;
            this.threadId = threadId;
            this.threadName = thread.getName();
            this.groupName = thread.getThreadGroup().getName();
        }

        private ThreadEvent(ThreadEventType type, int threadId) {
            this.type = ThreadEventType.STOP;
            this.objectId = -1;
            this.threadId = threadId;
            this.threadName = null;
            this.groupName = null;
        }

        @Override
        public String toString() {
            switch (type) {
                case START:
                    return String.format(
                            "THREAD START (obj=%d, id = %d, name=\"%s\", group=\"%s\")",
                            objectId, threadId, threadName, groupName);
                case STOP:
                    return String.format("THREAD END (id = %d)", threadId);
            }
            throw new IllegalStateException(type.toString());
        }
    }

    /**
     * A Trace represents a unique stack trace for a specific thread.
     */
    private static class Trace {

        private final int traceId;
        private int threadId;
        private StackTraceElement[] stack;

        private Trace() {
            this.traceId = -1;
        }

        private Trace(int traceId, Trace mutableTrace) {
            this.traceId = traceId;
            this.threadId = mutableTrace.threadId;
            this.stack = mutableTrace.stack;
        }

        protected int getThreadId() {
            return threadId;
        }

        protected StackTraceElement[] getStack() {
            return stack;
        }

        @Override
        public final int hashCode() {
            int result = 17;
            result = 31 * result + getThreadId();
            result = 31 * result + Arrays.hashCode(getStack());
            return result;
        }

        @Override
        public final boolean equals(Object o) {
            Trace s = (Trace) o;
            return getThreadId() == s.getThreadId() && Arrays.equals(getStack(), s.getStack());
        }
    }

    /**
     * Prints the profiler's collected data in hprof format to the
     * specified {@code File}. See the {@link #writeHprofData(PrintStream)
     * PrintStream} variant for more information.
     */
    public void writeHprofData(File file) throws IOException {
        PrintStream out = null;
        try {
            out = new PrintStream(new BufferedOutputStream(new FileOutputStream(file)));
            writeHprofData(out);
        } finally {
            if (out != null) {
                out.close();
            }
        }
    }

    private static final class SampleComparator implements Comparator<Entry<Trace, int[]>> {
        private static final SampleComparator INSTANCE = new SampleComparator();
        public int compare(Entry<Trace, int[]> e1, Entry<Trace, int[]> e2) {
            return e2.getValue()[0] - e1.getValue()[0];
        }
    }

    /**
     * Prints the profiler's collected data in hprof format to the
     * specified {@code PrintStream}. The profiler needs to be
     * stopped, but not necessarily shut down, in order to produce a
     * report.
     */
    public void writeHprofData(PrintStream out) {
        if (sampler != null) {
            throw new IllegalStateException("cannot print hprof data while sampling");
        }

        for (ThreadEvent e : threadHistory) {
            out.println(e);
        }

        List<Entry<Trace, int[]>> samples = new ArrayList<Entry<Trace, int[]>>(traces.entrySet());
        Collections.sort(samples, SampleComparator.INSTANCE);
        int total = 0;
        for (Entry<Trace, int[]> sample : samples) {
            Trace trace = sample.getKey();
            int count = sample.getValue()[0];
            total += count;
            out.printf("TRACE %d: (thread=%d)\n", trace.traceId, trace.threadId);
            for (StackTraceElement e : trace.stack) {
                out.printf("\t%s\n", e);
            }
        }
        Date now = new Date();
        // "CPU SAMPLES BEGIN (total = 826) Wed Jul 21 12:03:46 2010"
        out.printf("CPU SAMPLES BEGIN (total = %d) %ta %tb %td %tT %tY\n",
                   total, now, now, now, now, now);
        out.printf("rank   self  accum   count trace method\n");
        int rank = 0;
        double accum = 0;
        for (Entry<Trace, int[]> sample : samples) {
            rank++;
            Trace trace = sample.getKey();
            int count = sample.getValue()[0];
            double self = (double)count/(double)total;
            accum += self;

            // "   1 65.62% 65.62%     542 300302 java.lang.Long.parseLong"
            out.printf("% 4d% 6.2f%%% 6.2f%% % 7d % 5d %s.%s\n",
                       rank, self*100, accum*100, count, trace.traceId,
                       trace.stack[0].getClassName(),
                       trace.stack[0].getMethodName());
        }
        out.printf("CPU SAMPLES END\n");
    }
}
