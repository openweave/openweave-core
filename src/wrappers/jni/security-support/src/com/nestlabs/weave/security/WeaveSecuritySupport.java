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

package com.nestlabs.weave.security;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URL;

/**
 * Weave Security Support 
 */
public final class WeaveSecuritySupport {
    
    public static final String sNativeLibBaseName = "WeaveSecuritySupport";
    public static final int sSupportedNativeLibVersion = 1;
    
    public static native int getLibVersion();
    
    public static void forceLoad() {
        // do nothing.
    }
    
    static {
        loadNativeLibrary(sNativeLibBaseName, WeaveSecuritySupport.class.getClassLoader());
        if (getLibVersion() != sSupportedNativeLibVersion) {
            throw new UnsatisfiedLinkError("incompatible native library: " + sNativeLibBaseName);
        }
    }
    
    private static void loadNativeLibrary(String libBaseName, ClassLoader classLoader) {
        String osName = System.getProperty("os.name").toLowerCase().trim();
        String arch = System.getProperty("os.arch").toLowerCase().trim();
        
        if (osName.equals("linux")) {
            if (arch.equals("amd64")) {
                arch = "x86_64";
            }
            else if (arch.equals("x86") || arch.equals("i386")) {
                arch = "i686";
            }
        }
        else if (osName.equals("mac os x")) {
            osName = "darwin";
            arch = "x86_64";
        }
        
        String mappedLibName = System.mapLibraryName(libBaseName);
        int suffixIndex = mappedLibName.lastIndexOf('.');
        String mappedLibBaseName = mappedLibName.substring(0, suffixIndex);
        String libSuffix = mappedLibName.substring(suffixIndex, mappedLibName.length());
        
        String libResourceName = String.format("native/%s/%s/%s", osName, arch, mappedLibName);
        URL libResourceURL = classLoader.getResource(libResourceName);
        if (libResourceURL == null && osName.equals("darwin")) {
            libResourceName = String.format("native/%s/%s/%s%s", osName, arch, mappedLibBaseName,
                                            libSuffix.equals(".jnilib") ? ".dynlib" : ".jnilib");
            libResourceURL = classLoader.getResource(libResourceName);
        }

        if (libResourceURL != null) {
            File tmpLibFile = null;
            try {
                tmpLibFile = File.createTempFile(mappedLibBaseName, libSuffix);
                
                copyResourceToFile(libResourceURL, tmpLibFile);
                
                System.load(tmpLibFile.getAbsolutePath());
            }
            catch (Exception ex) {
                UnsatisfiedLinkError ule = new UnsatisfiedLinkError("unable to load a native library: " + libBaseName);
                ule.initCause(ex);
                throw ule;
            }
            finally {
                if (tmpLibFile != null) {
                    if (Boolean.getBoolean("nl.dontDeleteNativeLib") ||
                        Boolean.getBoolean("nl.dontDeleteNativeLib." + libBaseName) ||
                        !tmpLibFile.delete()) {
                        tmpLibFile.deleteOnExit();
                    }
                }
            }
        }

        else {
            System.loadLibrary(libBaseName);
        }
    }
    
    private static void copyResourceToFile(URL resourceURL, File destFile) throws IOException {
        InputStream is = null;
        OutputStream os = null;
        byte[] buf = new byte[8192];
        int readLen;
        
        try {
            is = resourceURL.openStream();
            os = new FileOutputStream(destFile);

            while ((readLen = is.read(buf)) > 0) {
                os.write(buf, 0, readLen);
            }
        }
        finally {
            if (is != null) {
                is.close();
            }
            if (os != null) {
                os.close();
            }
        }
    }
    
}