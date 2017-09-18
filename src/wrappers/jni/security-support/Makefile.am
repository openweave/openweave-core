#
#    Copyright (c) 2014-2017 Nest Labs, Inc.
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is the GNU automake template for the Nest Weave
#      Java Device Manager library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_JAVA

JARS                                               = \
    WeaveSecuritySupport.jar                         \
    $(NULL)

WeaveSecuritySupport_jar_JAVA_SRCS                 =               \
    com/nestlabs/weave/security/WeaveSecuritySupport.java          \
    com/nestlabs/weave/security/WeaveSecuritySupportException.java \
    com/nestlabs/weave/security/WeaveCertificateSupport.java       \
    com/nestlabs/weave/security/WeaveKeyExportClient.java          \
    com/nestlabs/weave/security/WeaveKeyExportSupport.java         \
    com/nestlabs/weave/security/PasscodeEncryptionSupport.java     \
    com/nestlabs/weave/security/ApplicationKeySupport.java         \
    com/nestlabs/weave/security/WeaveKeyId.java                    \
    com/nestlabs/weave/security/ApplicationGroupGlobalId.java      \
    com/nestlabs/weave/security/HKDF.java                          \
    $(NULL)

WeaveSecuritySupport_jar_JFLAGS                    = -source 6 -target 6

WeaveSecuritySupport_jar_JAVA_NATIVE_LIB           = libWeaveSecuritySupport.la 

WeaveSecuritySupport_jar_JAVA_STRIP_NATIVE_LIB     = true

WeaveSecuritySupport_jar_MAVEN_PROJECT_FILE        = WeaveSecuritySupport.pom

EXTRA_DIST                                         = \
    $(srcdir)/src                                    \
    $(srcdir)/native                                 \
    $(NULL)

lib_LTLIBRARIES                                    = libWeaveSecuritySupport.la

libWeaveSecuritySupport_la_SOURCES                 = \
    native/WeaveSecuritySupport.cpp                  \
    native/WeaveCertificateSupport.cpp               \
    native/PasscodeEncryptionSupport.cpp             \
    native/WeaveKeyExportClientNative.cpp            \
    native/WeaveKeyExportSupportNative.cpp           \
    $(NULL)

libWeaveSecuritySupport_la_DEPENDENCIES            = \
    $(top_builddir)/src/lib/libWeave.a               \
    $(top_builddir)/src/wrappers/jni/jni-utils/libJNIUtils.a \
    $(NULL)

libWeaveSecuritySupport_la_CPPFLAGS                = \
    -I$(top_srcdir)/src/wrappers/jni/jni-utils       \
	$(JNI_CPPFLAGS)                                  \
	$(NULL)

libWeaveSecuritySupport_la_LIBADD                  = \
    -L$(top_builddir)/src/wrappers/jni/jni-utils -lJNIUtils \
    -L$(top_builddir)/src/lib -lWeave                \
	$(NULL)

if WEAVE_WITH_ANDROID
if WEAVE_LOGGING_STYLE_ANDROID

libWeaveSecuritySupport_la_LIBADD                 += \
    -llog                                            \
    $(NULL)

endif # WEAVE_WITH_ANDROID
endif # WEAVE_LOGGING_STYLE_ANDROID

libWeaveSecuritySupport_la_LDFLAGS                 = \
    $(AM_LDFLAGS)                                    \
    -no-undefined                                    \
    -version-info $(LIBWEAVE_VERSION_INFO)           \
    $(NULL)

include $(abs_top_srcdir)/build/make/java.mk

all-local: all-JARS
install-data-local: install-data-JARS
uninstall-local: uninstall-JARS
mostlyclean-local: mostlyclean-local-JARS

endif # WEAVE_WITH_JAVA

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
