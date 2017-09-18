## System requirements for building Weave

Building the Nest Weave SDK has fairly modest system requirements.

### Linux

The Nest Weave SDK officially recommends and supports Ubuntu
12.04. However, the Nest Weave SDK should work on any similar
Linux-based system.

### Mac OS X

On Mac OS X, the Nest Weave SDK requires a means by which to clone,
with symbolic links, one directory subtree to another. On Linux,
this is commonly provided with 'cp -Rs' available as part of GNU
coreutils <http://www.gnu.org/software/coreutils/>. If installed,
XQuartz <http://xquartz.macosforge.org/> provides similar
functionality via 'lndir -silent'. If you don't already have GNU
coreutils or XQuartz installed, you can install one of these,
XQuartz being the easier of the two.

### Windows

On Windows, the Nest Weave SDK requires [Cygwin](https://www.cygwin.com/).

## Supported targets

### Linux

When building the Nest Weave SDK on a Linux host, the
following native and cross-compiled build targets are supported and
tested:

<table>
  <tr>
    <th rowspan="2">Tuple</th>
    <th colspan="4">Language Binding</th>
  </tr>
  <tr>
    <td>C/C++</td>
    <td>Cocoa</td>
    <td>Java</td>
    <td>Python</td>
  </tr>
  <tr>
    <td>i386-unknown-linux-gnu</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td>X</td>
  </tr>
  <tr>
    <td>x86_64-unknown-linux-gnu</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td>X</td>
  </tr>
  <tr>
    <td colspan="5"></td>
  </tr>
  <tr>
    <td>arm-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
  <tr>
    <td>armv7-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
  <tr>
    <td>i386-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
</table>


### Mac OS X

When building the Nest Weave SDK on a Mac OS X build host, the
following native and cross-compiled build targets are supported and
tested:

<table>
  <tr>
    <th rowspan="2">Tuple</th>
    <th colspan="4">Language Binding</th>
  </tr>
  <tr>
    <td>C/C++</td>
    <td>Cocoa</td>
    <td>Java</td>
    <td>Python</td>
  </tr>
  <tr>
    <td>armv7-apple-darwin-ios</td>
    <td>X</td>
    <td>X</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>armv7s-apple-darwin-ios</td>
    <td>X</td>
    <td>X</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>i386-apple-darwin-ios</td>
    <td>X</td>
    <td>X</td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td colspan="5"></td>
  </tr>
  <tr>
    <td>i386-apple-darwin-macosx</td>
    <td>X</td>
    <td></td>
    <td></td>
    <td>X</td>
  </tr>
  <tr>
    <td>x86_64-apple-darwin-macosx</td>
    <td>X</td>
    <td></td>
    <td></td>
    <td>X</td>
  </tr>
  <tr>
    <td colspan="5"></td>
  </tr>
  <tr>
    <td>arm-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
  <tr>
    <td>armv7-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
  <tr>
    <td>i386-unknown-linux-android</td>
    <td>X</td>
    <td></td>
    <td>X</td>
    <td></td>
  </tr>
</table>

### Windows

<table>
  <tr>
    <th rowspan="2">Tuple</th>
    <th colspan="4">Language Binding</th>
  </tr>
  <tr>
    <td>C/C++</td>
    <td>Cocoa</td>
    <td>Java</td>
    <td>Python</td>
  </tr>
  <tr>
    <td>i686-pc-cygwin</td>
    <td>X</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
</table>


### Embedded

<table>
  <tr>
    <th rowspan="2">Tuple</th>
    <th colspan="4">Language Binding</th>
  </tr>
  <tr>
    <td>C/C++</td>
    <td>Cocoa</td>
    <td>Java</td>
    <td>Python</td>
  </tr>
  <tr>
    <td>arm*-unknown-linux*</td>
    <td>X</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
  <tr>
    <td>arm*-unknown-freertos-lwip</td>
    <td>X</td>
    <td></td>
    <td></td>
    <td></td>
  </tr>
</table>

## Supported toolchains

arm-unknown-linux-android
armv7-unknown-linux-android
i386-unknown-linux-android

* gcc 4.6

arm*-unknown-linux*
arm*-unknown-freertos-lwip

* gcc 4.4.1
* LLVM/clang 3.1
* LLVM/clang 3.3

i386-apple-darwin-macosx
x86_64-apple-darwin-macosx

* LLVM/clang 3.5 (6.0)


i386-unknown-linux-gnu
x86_64-unknown-linux-gnu

* gcc 4.6.3


i686-pc-cygwin

* gcc 4.8.3


NOTE: Other toolchains and other versions of those toolchains may work for your
environment. However, because they have not been officially tested by Nest,
they cannot be guaranteed to work or officially supported by Nest.


## Configuring and starting the build

The Nest Weave SDK uses the GNU autotools system for its build. As a result,
there are three phases to using the Nest Weave SDK build:

* Configure
* Build
* Stage

Prerequisite:
If you're using Weave's bundled OpenSSL, make sure you've installed
Perl text::template.

If you want to jump right in, the steps you need to perform are:

1. Make sure that the Weave Happy tool is up to date.

        % [[placeholder: add command to update Happy]]

2. Unarchive the Nest Weave SDK:


        % tar -zxf weave-<version>.tar.gz


3. Configure it:


        % cd weave-<version>
        % ./configure


4. Build it:


        % make all


5. Stage it to a place your code can compile and link against:


        % mkdir weave-<version>-output


NOTE: Feel free to name this directory whatever you would like to or whatever
your project's build system dictates.

```
% make DESTDIR=`pwd`/weave-<version>-output install
```

At this point you will have in `weave-<version>-output`:

<table>
  <tr>
    <th>Location</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>bin/</td>
    <td>Architecture-independent programs and tools</td>
  </tr>
  <tr>
    <td>include/</td>
    <td>Weave SDK public headers</td>
  </tr>
  <tr>
    <td>lib/</td>
    <td>Architecture-independent libraries</td>
  </tr>
  <tr>
    <td>share/</td>
    <td>Read-only architecture-independent content</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;doc/</td>
    <td>Weave SDK documentation</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;java/</td>
    <td>Weave SDK Java language interface classes and,archives</td>
  </tr>
  <tr>
    <td>&lt;target tuple&gt;/</td>
    <td>Architecture-dependent files</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;bin/</td>
    <td>Architecture-dependent programs and tools</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;lib/</td>
    <td>Architecture-dependent libraries</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;python/</td>
    <td>Python language interface modules and libraries</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;libexec/</td>
    <td>Architecture-dependent helper programs and tools</td>
  </tr>
</table>


In general, you'll direct your project's toolchain at `include` as a header
search path for Weave headers and at `<target tuple>/lib` as a library search
path for Weave libraries.

If you are building for Android, iOS, or for a standalone system, convenience
makefiles have been written that will do all of the configure, build, and
stage steps for you. Type `make` or `make all` with the appropriate makefile
to perform all three steps automatically:

```
% make -f Makefile-Android
% make -f Makefile-iOS
% make -f Makefile-Standalone
```

Type `make help` to learn more about the elements in those convenience make
files may be overridden:

```
% make -f Makefile-Android help
% make -f Makefile-iOS help
% make -f Makefile-Standalone help
```

If you are using the Nest Weave SDK for an embedded system, you will likely
need to interact with the Nest Weave SDK build system directly.

### Configure


There are two ways to configure the Nest Weave SDK:

* Configuration Script
* Configuration Headers

#### Configuration script

The Nest Weave SDK uses GNU autotools for its build. So, the first thing you
need to do is run the `configure` script at the top level of the Nest Weave SDK,
either in the SDK directory itself or from another non-colocated build directory.

##### In-the-SDK

```
% cd weave-<version>
% ./configure [ <options> ... ]
```

##### Out-of-the-SDK

NOTE: Feel free to name this directory whatever you would like.

```
% mkdir weave-<version>-build
% cd weave-<version>-build
% ../weave-<version>-configure [ <options> ... ]
```

##### Configuration options

The Nest Weave SDK configuration script has been written to attempt to automatically
use the most appropriate or relevant default options for the target you have selected;
however, many of these options may be explicitly overridden.

If you are building for an embedded system, rather than an Android, iOS, or a standalone
desktop or server system, you may be interested in altering some of these.

At any time, use the `--help` flag to learn more about configuration options available
to you:

```
% ./configure --help
```

#### Configuration headers

In addition to the Nest Weave SDK configuration script, there are two, optional
project-specific configuration headers that you may provide to the Nest Weave SDK
to change, at compile time, Nest Weave behavior:

* InetProjectConfig.h
* WeaveProjectConfig.h

The directory location of these files, if your project is providing them, can be
specified by the configuration script options:

```
--with-weave-inet-project-includes=DIR
```

Specify Weave InetLayer project-specific configuration header (InetProjectConfig.h)
search directory [default=none].

```
--with-weave-project-includes=DIR
```

Specify Weave Core project-specific configuration header (WeaveProjectConfig.h)
search directory [default=none].

These files SHOULD NOT be placed in the Weave SDK itself but rather should be
colocated with the your project's source and header files.

For Android, iOS, and standalone systems, reasonable defaults that these headers
may override are documented and provided in:

* InetLayer/InetConfig.h
* Weave/Core/WeaveConfig.h

For more information, consult these headers or the in-the-SDK documentation.
