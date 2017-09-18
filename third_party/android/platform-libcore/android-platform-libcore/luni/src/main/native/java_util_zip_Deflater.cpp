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

#define LOG_TAG "Deflater"

#include "JniConstants.h"
#include "ScopedPrimitiveArray.h"
#include "zip.h"

static struct {
    jfieldID inRead;
    jfieldID finished;
} gCachedFields;

static void Deflater_setDictionaryImpl(JNIEnv* env, jobject, jbyteArray dict, int off, int len, jlong handle) {
    toNativeZipStream(handle)->setDictionary(env, dict, off, len, false);
}

static jlong Deflater_getTotalInImpl(JNIEnv*, jobject, jlong handle) {
    return toNativeZipStream(handle)->stream.total_in;
}

static jlong Deflater_getTotalOutImpl(JNIEnv*, jobject, jlong handle) {
    return toNativeZipStream(handle)->stream.total_out;
}

static jint Deflater_getAdlerImpl(JNIEnv*, jobject, jlong handle) {
    return toNativeZipStream(handle)->stream.adler;
}

/* Create a new stream . This stream cannot be used until it has been properly initialized. */
static jlong Deflater_createStream(JNIEnv * env, jobject, jint level, jint strategy, jboolean noHeader) {
    UniquePtr<NativeZipStream> jstream(new NativeZipStream);
    if (jstream.get() == NULL) {
        jniThrowOutOfMemoryError(env, NULL);
        return -1;
    }

    int wbits = 12; // Was 15, made it 12 to reduce memory consumption. Use MAX for fastest.
    int mlevel = 5; // Was 9, made it 5 to reduce memory consumption. Might result
                  // in out-of-memory problems according to some web pages. The
                  // ZLIB docs are a bit vague, unfortunately. The default
                  // results in 2 x 128K being allocated per Deflater, which is
                  // not acceptable.
    /*Unable to find official doc that this is the way to avoid zlib header use. However doc in zipsup.c claims it is so */
    if (noHeader) {
        wbits = wbits / -1;
    }
    int err = deflateInit2(&jstream->stream, level, Z_DEFLATED, wbits, mlevel, strategy);
    if (err != Z_OK) {
        throwExceptionForZlibError(env, "java/lang/IllegalArgumentException", err);
        return -1;
    }
    return reinterpret_cast<uintptr_t>(jstream.release());
}

static void Deflater_setInputImpl(JNIEnv* env, jobject, jbyteArray buf, jint off, jint len, jlong handle) {
    toNativeZipStream(handle)->setInput(env, buf, off, len);
}

static jint Deflater_deflateImpl(JNIEnv* env, jobject recv, jbyteArray buf, int off, int len, jlong handle, int flushParm) {
    /* We need to get the number of bytes already read */
    jint inBytes = env->GetIntField(recv, gCachedFields.inRead);

    NativeZipStream* stream = toNativeZipStream(handle);
    stream->stream.avail_out = len;
    jint sin = stream->stream.total_in;
    jint sout = stream->stream.total_out;
    ScopedByteArrayRW out(env, buf);
    if (out.get() == NULL) {
        return -1;
    }
    stream->stream.next_out = reinterpret_cast<Bytef*>(out.get() + off);
    int err = deflate(&stream->stream, flushParm);
    if (err != Z_OK) {
        if (err == Z_MEM_ERROR) {
            jniThrowOutOfMemoryError(env, NULL);
            return 0;
        }
        if (err == Z_STREAM_END) {
            env->SetBooleanField(recv, gCachedFields.finished, JNI_TRUE);
            return stream->stream.total_out - sout;
        }
    }
    if (flushParm != Z_FINISH) {
        /* Need to update the number of input bytes read. */
        env->SetIntField(recv, gCachedFields.inRead, (jint) stream->stream.total_in - sin + inBytes);
    }
    return stream->stream.total_out - sout;
}

static void Deflater_endImpl(JNIEnv*, jobject, jlong handle) {
    NativeZipStream* stream = toNativeZipStream(handle);
    deflateEnd(&stream->stream);
    delete stream;
}

static void Deflater_resetImpl(JNIEnv* env, jobject, jlong handle) {
    NativeZipStream* stream = toNativeZipStream(handle);
    int err = deflateReset(&stream->stream);
    if (err != Z_OK) {
        throwExceptionForZlibError(env, "java/lang/IllegalArgumentException", err);
    }
}

static void Deflater_setLevelsImpl(JNIEnv* env, jobject, int level, int strategy, jlong handle) {
    if (handle == -1) {
        jniThrowException(env, "java/lang/IllegalStateException", NULL);
        return;
    }
    NativeZipStream* stream = toNativeZipStream(handle);
    jbyte b = 0;
    stream->stream.next_out = reinterpret_cast<Bytef*>(&b);
    int err = deflateParams(&stream->stream, level, strategy);
    if (err != Z_OK) {
        throwExceptionForZlibError(env, "java/lang/IllegalStateException", err);
    }
}

static JNINativeMethod gMethods[] = {
    NATIVE_METHOD(Deflater, createStream, "(IIZ)J"),
    NATIVE_METHOD(Deflater, deflateImpl, "([BIIJI)I"),
    NATIVE_METHOD(Deflater, endImpl, "(J)V"),
    NATIVE_METHOD(Deflater, getAdlerImpl, "(J)I"),
    NATIVE_METHOD(Deflater, getTotalInImpl, "(J)J"),
    NATIVE_METHOD(Deflater, getTotalOutImpl, "(J)J"),
    NATIVE_METHOD(Deflater, resetImpl, "(J)V"),
    NATIVE_METHOD(Deflater, setDictionaryImpl, "([BIIJ)V"),
    NATIVE_METHOD(Deflater, setInputImpl, "([BIIJ)V"),
    NATIVE_METHOD(Deflater, setLevelsImpl, "(IIJ)V"),
};
int register_java_util_zip_Deflater(JNIEnv* env) {
    gCachedFields.finished = env->GetFieldID(JniConstants::deflaterClass, "finished", "Z");
    gCachedFields.inRead = env->GetFieldID(JniConstants::deflaterClass, "inRead", "I");
    return jniRegisterNativeMethods(env, "java/util/zip/Deflater", gMethods, NELEM(gMethods));
}
