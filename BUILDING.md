## System requirements for building Weave

Building OpenWeave Core has fairly modest system requirements.

### Linux

OpenWeave Core officially recommends and supports Ubuntu
12.04. However, OpenWeave Core should work on any similar
Linux-based system.

### Mac OS X

On Mac OS X, OpenWeave Core requires a means by which to clone,
with symbolic links, one directory subtree to another. On Linux,
this is commonly provided with 'cp -Rs' available as part of GNU
coreutils <http://www.gnu.org/software/coreutils/>. If installed,
XQuartz <http://xquartz.macosforge.org/> provides similar
functionality via 'lndir -silent'. If you don't already have GNU
coreutils or XQuartz installed, you can install one of these,
XQuartz being the easier of the two.

### Windows

On Windows, OpenWeave Core requires [Cygwin](https://www.cygwin.com/).


## Get started with OpenWeave Core (quick start)

NOTE: These instructions have been tested on Ubuntu Linux.

If you want to jump right in, the steps you need to perform are:

1. Install Python setup tools.

        % sudo apt-get install python-setuptools 

1. Make sure that the Weave Happy tool is installed and up to date.

        % git clone https://github.com/openweave/happy.git
        % cd happy
        % make

1. Install OpenWeave Core.

        % cd ..
        % git clone https://github.com/openweave/openweave-core.git
        % cd openweave-core
        % make -f Makefile-Standalone


For more detailed information on configuring and building OpenWeave
for different platforms, see [Configuring and starting the build
(detailed instructions)](#detailed).

```
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


## Configuring and starting the build (detailed instructions)<a name="detailed"></a>

OpenWeave Core uses the GNU autotools system for its build. As a result,
there are three phases to using the OpenWeave Core build:

* Configure
* Build
* Stage

Prerequisites:

* If you're using Weave's bundled OpenSSL, make sure you've installed
  Perl text::template.

  Text::Template is available from http://www.plover.com/~mjd/perl/Template/
  or from CPAN (http://search.cpan.org/dist/Text-Template/).

        % wget "http://search.cpan.org/CPAN/authors/id/M/MS/MSCHOUT/Text-Template-1.47.tar.gz" 
        % tar -xvzf Text-Template-1.47.tar.gz
        % cd Text-Template-1.47/
        % perl Makefile.PL
        % make test
        % sudo make install

* Install `libdbus-1-dev`.

        % sudo apt-get install libdbus-1-dev


Procedure:

1. Make sure that the Weave Happy tool is installed and up to date.

        % git clone https://github.com/openweave/happy.git
        % cd happy
        % make

1. Install OpenWeave Core.

        % cd ..
        % git clone https://github.com/openweave/openweave-core.git


3. Configure it:

        % cd openweave-core
        % ./configure


4. Build it:

        % make all


5. Stage it to a place your code can compile and link against:

        % mkdir openweave-core-output


NOTE: Feel free to name this directory whatever you would like to or whatever
your project's build system dictates.

```
% make DESTDIR=`pwd`/openweave-core-output install
```

At this point you will have in `openweave-core-output`:

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
    <td>OpenWeave Core public headers</td>
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
    <td>OpenWeave Core documentation</td>
  </tr>
  <tr>
    <td>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;java/</td>
    <td>OpenWeave Core Java language interface classes and,archives</td>
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

If you are using OpenWeave Core for an embedded system, you will likely
need to interact with the OpenWeave Core build system directly.

### Configure


There are two ways to configure OpenWeave Core:

* Configuration Script
* Configuration Headers

#### Configuration script

OpenWeave Core uses GNU autotools for its build. So, the first thing you
need to do is run the `configure` script at the top level of OpenWeave Core,
either in the SDK directory itself or from another non-colocated build directory.

##### In-the-SDK

```
% cd openweave-core
% ./configure [ <options> ... ]
```

##### Out-of-the-SDK

NOTE: Feel free to name this directory whatever you would like.

```
% mkdir openweave-core-build
% cd openweave-core-build
% ../openweave-core-configure [ <options> ... ]


##### Configuration options

The OpenWeave Core configuration script has been written to attempt to automatically
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

In addition to the OpenWeave Core configuration script, there are two, optional
project-specific configuration headers that you may provide to OpenWeave Core
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

These files SHOULD NOT be placed in OpenWeave Core itself but rather should be
colocated with the your project's source and header files.

For Android, iOS, and standalone systems, reasonable defaults that these headers
may override are documented and provided in:

* InetLayer/InetConfig.h
* Weave/Core/WeaveConfig.h

For more information, consult these headers or the in-the-SDK documentation.
