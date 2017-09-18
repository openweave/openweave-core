/* Licensed to the Apache Software Foundation (ASF) under one or more
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

package libcore.java.util;

import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetClass;
import dalvik.annotation.TestTargetNew;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;
import java.util.PriorityQueue;
import junit.framework.TestCase;
import tests.util.SerializationTester;

@TestTargetClass(PriorityQueue.class)
public class OldPriorityQueueTest extends TestCase {

    private static final String SERIALIZATION_FILE_NAME = "/serialization/tests/api/java/util/PriorityQueue.golden.ser";

    /**
     * @tests java.util.PriorityQueue#PriorityQueue(int)
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "",
        method = "PriorityQueue",
        args = {int.class}
    )
    public void test_ConstructorI() {
        PriorityQueue<Object> queue = new PriorityQueue<Object>(100);
        assertNotNull(queue);
        assertEquals(0, queue.size());
        assertNull(queue.comparator());

        try {
            new PriorityQueue(0);
            fail("IllegalArgumentException expected");
        } catch (IllegalArgumentException e) {
            //expected
        }
    }

    /**
     * @tests java.util.PriorityQueue#remove(Object)
     *
     */
    @TestTargetNew(
        level = TestLevel.PARTIAL_COMPLETE,
        notes = "",
        method = "remove",
        args = {java.lang.Object.class}
    )
    public void test_remove_Ljava_lang_Object_using_comparator() {
        PriorityQueue<String> queue = new PriorityQueue<String>(10,
                new MockComparatorStringByLength());
        String[] array = { "AAAAA", "AA", "AAAA", "AAAAAAAA" };
        for (int i = 0; i < array.length; i++) {
            queue.offer(array[i]);
        }
        assertFalse(queue.contains("BB"));
        // Even though "BB" is equivalent to "AA" using the string length comparator, remove()
        // uses equals(), so only "AA" succeeds in removing element "AA".
        assertFalse(queue.remove("BB"));
        assertTrue(queue.remove("AA"));
    }

    /**
     * @tests java.util.PriorityQueue#remove(Object)
     *
     */
    @TestTargetNew(
        level = TestLevel.PARTIAL_COMPLETE,
        notes = "Verifies ClassCastException.",
        method = "remove",
        args = {java.lang.Object.class}
    )
    public void test_remove_Ljava_lang_Object_not_exists() {
        Integer[] array = { 2, 45, 7, -12, 9, 23, 17, 1118, 10, 16, 39 };
        List<Integer> list = Arrays.asList(array);
        PriorityQueue<Integer> integerQueue = new PriorityQueue<Integer>(list);
        assertFalse(integerQueue.remove(111));
        assertFalse(integerQueue.remove(null));
        assertFalse(integerQueue.remove(""));
    }

    /**
     * @tests serialization/deserialization.
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "Verifies serialization/deserialization.",
        method = "!SerializationSelf",
        args = {}
    )
    public void test_Serialization() throws Exception {
        Integer[] array = { 2, 45, 7, -12, 9, 23, 17, 1118, 10, 16, 39 };
        List<Integer> list = Arrays.asList(array);
        PriorityQueue<Integer> srcIntegerQueue = new PriorityQueue<Integer>(
                list);
        PriorityQueue<Integer> destIntegerQueue = (PriorityQueue<Integer>) SerializationTester
                .getDeserilizedObject(srcIntegerQueue);
        Arrays.sort(array);
        for (int i = 0; i < array.length; i++) {
            assertEquals(array[i], destIntegerQueue.poll());
        }
        assertEquals(0, destIntegerQueue.size());
    }

    /**
     * @tests serialization/deserialization.
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "Verifies serialization/deserialization.",
        method = "!SerializationSelf",
        args = {}
    )
    public void test_Serialization_casting() throws Exception {
        Integer[] array = { 2, 45, 7, -12, 9, 23, 17, 1118, 10, 16, 39 };
        List<Integer> list = Arrays.asList(array);
        PriorityQueue<Integer> srcIntegerQueue = new PriorityQueue<Integer>(
                list);
        PriorityQueue<String> destStringQueue = (PriorityQueue<String>) SerializationTester
                .getDeserilizedObject(srcIntegerQueue);
        // will not incur class cast exception.
        Object o = destStringQueue.peek();
        Arrays.sort(array);
        Integer I = (Integer) o;
        assertEquals(array[0], I);
    }

    /**
     * @tests serialization/deserialization compatibility with RI.
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "Verifies serialization/deserialization compatibility.",
        method = "!SerializationGolden",
        args = {}
    )
    public void test_SerializationCompatibility_cast() throws Exception {
        Integer[] array = { 2, 45, 7, -12, 9, 23, 17, 1118, 10, 16, 39 };
        List<Integer> list = Arrays.asList(array);
        PriorityQueue<Integer> srcIntegerQueue = new PriorityQueue<Integer>(
                list);
        PriorityQueue<String> destStringQueue = (PriorityQueue<String>) SerializationTester
                .readObject(srcIntegerQueue, SERIALIZATION_FILE_NAME);

        // will not incur class cast exception.
        Object o = destStringQueue.peek();
        Arrays.sort(array);
        Integer I = (Integer) o;
        assertEquals(array[0], I);
    }

    private static class MockComparatorStringByLength implements
            Comparator<String> {

        public int compare(String object1, String object2) {
            int length1 = object1.length();
            int length2 = object2.length();
            if (length1 > length2) {
                return 1;
            } else if (length1 == length2) {
                return 0;
            } else {
                return -1;
            }
        }

    }

    private static class MockComparatorCast<E> implements Comparator<E> {

        public int compare(E object1, E object2) {
            return 0;
        }
    }

}
