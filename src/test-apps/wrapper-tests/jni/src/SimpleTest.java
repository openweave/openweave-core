/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.InvocationTargetException;
import java.util.Arrays;

class SimpleTest {
    
    public int runTests(String[] testNames)
    {
        String testGroupName = this.getClass().getSimpleName();
        
        for (String testName : testNames) {
            java.lang.reflect.Method testMethod;
            
            try {
                testMethod = this.getClass().getDeclaredMethod(testName);
            }
            catch (Exception ex) {
                System.err.format("%s: Unable to find test method %s\n", testGroupName, testName);
                return -1;
            }
            
            try {
                testMethod.invoke(this);
            }
            catch (InvocationTargetException invokeEx) {
                Throwable ex = invokeEx.getTargetException();
                System.err.format("%s %s FAILED: %s", testGroupName, testName, toStringWithStackTrace(ex));
                return -1;
            } 
            catch (Exception ex) {
                System.err.format("%s: Unable to invoke test method %s\n%s\n", testGroupName, testName, ex);
                return -1;
            }
        }
        
        System.out.format("%s: ALL TESTS SUCCEEDED\n", testGroupName);
        return 0;
    }
    
    protected static byte[] toByteArray(int[] val)
    {
        byte[] res = new byte[val.length];
        for (int i = 0; i < val.length; i++) {
            res[i] = (byte)val[i];
        }
        return res;
    }
    
    protected static void assertEqual(byte[] val1, byte[] val2, String description) throws Exception
    {
        if (!Arrays.equals(val1, val2)) {
            throw new Exception(String.format("Value Mismatch: %s", description));
        }
    }
    
    protected static String toStringWithStackTrace(Throwable ex)
    {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        ex.printStackTrace(pw);
        return sw.toString();
    }
}
