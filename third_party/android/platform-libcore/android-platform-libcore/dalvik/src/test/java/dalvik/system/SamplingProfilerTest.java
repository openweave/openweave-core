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

import dalvik.system.SamplingProfiler.ThreadSet;
import java.math.BigInteger;
import java.security.KeyPairGenerator;
import java.security.SecureRandom;
import javax.crypto.spec.DHParameterSpec;
import junit.framework.TestCase;

public class SamplingProfilerTest extends TestCase {

    public void test_SamplingProfiler_basic() throws Exception {
        ThreadSet threadSet = SamplingProfiler.newArrayThreadSet(Thread.currentThread());
        SamplingProfiler profiler = new SamplingProfiler(12, threadSet);
        profiler.start(10);
        toBeMeasured();
        profiler.stop();
        profiler.shutdown();
        profiler.writeHprofData(System.out);
    }

    private static final String P_STR =
        "9494fec095f3b85ee286542b3836fc81a5dd0a0349b4c239dd38744d488cf8e3"
            + "1db8bcb7d33b41abb9e5a33cca9144b1cef332c94bf0573bf047a3aca98cdf3b";
    private static final String G_STR =
            "98ab7c5c431479d8645e33aa09758e0907c78747798d0968576f9877421a9089"
            + "756f7876e76590b76765645c987976d764dd4564698a87585e64554984bb4445"
            + "76e5764786f875b4456c";

    private static final byte[] P = new BigInteger(P_STR,16).toByteArray();
    private static final byte[] G = new BigInteger(G_STR,16).toByteArray();

    private static void toBeMeasured () throws Exception {
        long start = System.currentTimeMillis();
        for (int i = 0; i < 10000; i++) {
            BigInteger p = new BigInteger(P);
            BigInteger g = new BigInteger(G);
            KeyPairGenerator gen = KeyPairGenerator.getInstance("DH");
            gen.initialize(new DHParameterSpec(p, g), new SecureRandom());
        }
        long end = System.currentTimeMillis();
    }

}
