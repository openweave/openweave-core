#!/usr/bin/python


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
#    @file
#      This file implements a trampoline script for other Nest Weave
#      SDK executables to ensure that the dynamic loader can find
#      the SDK DSOs/DLLs in the appropriate location(s).
#

import os
import sys

# Determine whether or not this is running on Darwin where the dynamic
# loader is dyld and uses DYLD_LIBRARY path; otherwise, the dynamic
# loader is ld and uses LD_LIBRARY_PATH.

if sys.platform == "darwin":
  loader = "DYLD_LIBRARY_PATH"
else:
  loader = "LD_LIBRARY_PATH"

# Shared libraries that Weave SDK executables or Python scripts are
# looking for may live in one or more of the following places:
#
#   1) SDK_ROOT/lib/...
#   2) SDK_ROOT/lib/python/PACKAGE/...
#   3) SDK_ROOT/lib/pythonPYTHON_VERSION/dist-packages/PACKAGE/...
#   4) SDK_ROOT/lib/pythonPYTHON_VERSION/site-packages/PACKAGE/...
#
# where SDK_ROOT is either:
#
#   A) WEAVE_HOME
#
# or:
#
#   B) One path component "up" from this executabe.
#
# If WEAVE_HOME is set, then form up 1A, 2A, 3A, and 4A; otherwise,
# form up 1B, 2B, 3B, and 4B and append them to help the loader find
# any Weave SDK DLLs/DSOs.

pyversion = sys.version[:3]

pkgsdklibdirs = [ "lib",
                  "lib/python/weave",
                  "lib/python" + pyversion + "/dist-packages/weave",
                  "lib/python" + pyversion + "/site-packages/weave"  ]

# Determine the root of the Weave SDK

if os.environ.has_key("WEAVE_HOME"):
  pkgsdkrootdir = os.environ["WEAVE_HOME"]
else:
  pkgsdkrootdir = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

# Build up the dynamic loader search paths as described above.

for pkgsdklibdir in pkgsdklibdirs:
  try:
    loaderpaths += os.pathsep + os.path.normpath(os.path.join(pkgsdkrootdir, pkgsdklibdir))
  except NameError:
    loaderpaths = os.path.normpath(os.path.join(pkgsdkrootdir, pkgsdklibdir))

# Append to or create the dynamic loader search paths in the OS environment.

if os.environ.has_key(loader):
  os.environ[loader] += os.pathsep + loaderpaths
else:
  os.environ[loader]  = loaderpaths

# We have been trampolined into this script from a symbolic link. The
# real executable lives in libexec/... as a sibling directory to the
# library loader path.

execpath = os.path.join(pkgsdkrootdir, "libexec", os.path.basename(sys.argv[0]))

# Re-execute the real executable with the new loader path in effect.

os.execl(execpath, os.path.basename(execpath), *sys.argv[1:])
