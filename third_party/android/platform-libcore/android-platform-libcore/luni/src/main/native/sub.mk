# This file is included by the top-level libcore Android.mk.
# It's not a normal makefile, so we don't include CLEAR_VARS
# or BUILD_*_LIBRARY.

LOCAL_SRC_FILES := \
	AsynchronousSocketCloseMonitor.cpp \
	ErrorCode.cpp \
	ICU.cpp \
	JniConstants.cpp \
	JniException.cpp \
	NativeBN.cpp \
	NativeBidi.cpp \
	NativeBreakIterator.cpp \
	NativeCollation.cpp \
	NativeConverter.cpp \
	NativeCrypto.cpp \
	NativeDecimalFormat.cpp \
	NativeIDN.cpp \
	NativeNormalizer.cpp \
	NativePluralRules.cpp \
	NetworkUtilities.cpp \
	Register.cpp \
	TimeZones.cpp \
	cbigint.cpp \
	java_io_Console.cpp \
	java_io_File.cpp \
	java_io_FileDescriptor.cpp \
	java_io_ObjectInputStream.cpp \
	java_io_ObjectOutputStream.cpp \
	java_io_ObjectStreamClass.cpp \
	java_lang_Character.cpp \
	java_lang_Double.cpp \
	java_lang_Float.cpp \
	java_lang_Math.cpp \
	java_lang_ProcessManager.cpp \
	java_lang_RealToString.cpp \
	java_lang_StrictMath.cpp \
	java_lang_System.cpp \
	java_net_InetAddress.cpp \
	java_net_NetworkInterface.cpp \
	java_nio_ByteOrder.cpp \
	java_nio_charset_Charsets.cpp \
	java_util_regex_Matcher.cpp \
	java_util_regex_Pattern.cpp \
	java_util_zip_Adler32.cpp \
	java_util_zip_CRC32.cpp \
	java_util_zip_Deflater.cpp \
	java_util_zip_Inflater.cpp \
	libcore_io_IoUtils.cpp \
	org_apache_harmony_luni_platform_OSFileSystem.cpp \
	org_apache_harmony_luni_platform_OSMemory.cpp \
	org_apache_harmony_luni_platform_OSNetworkSystem.cpp \
	org_apache_harmony_luni_util_FloatingPointParser.cpp \
	org_apache_harmony_xml_ExpatParser.cpp \
	valueOf.cpp

LOCAL_C_INCLUDES += \
	external/expat/lib \
	external/icu4c/common \
	external/icu4c/i18n \
	external/openssl/include \
	external/zlib

# Any shared/static libs that are listed here must also
# be listed in libs/nativehelper/Android.mk.
# TODO: fix this requirement

LOCAL_SHARED_LIBRARIES += \
	libcrypto \
	libcutils \
	libexpat \
	libicuuc \
	libicui18n \
	libssl \
	libutils \
	libz

LOCAL_STATIC_LIBRARIES += \
	libfdlibm
