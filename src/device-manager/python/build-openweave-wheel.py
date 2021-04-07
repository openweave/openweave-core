#
#    Copyright (c) 2019 Google LLC.
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
#      Builds a Python wheel package for OpenWeave.
#

from __future__ import absolute_import
import sys
import os
import stat
from datetime import datetime
import shutil
import getpass
from setuptools import setup
from wheel.bdist_wheel import bdist_wheel
import argparse
import platform


parser = argparse.ArgumentParser(description='build the pip package for openweave using openweave components generated during the build and python source code')
parser.add_argument('--package_name', default='openweave', help='configure the python package name')

args = parser.parse_args()

owDLLName = '_WeaveDeviceMgr.so'
deviceManagerShellName = 'weave-device-mgr.py'
deviceManagerShellInstalledName = os.path.splitext(deviceManagerShellName)[0]
packageName = args.package_name

# Record the current directory at the start of execution.
curDir = os.curdir

# Expect to find the source files for the python modules in the same directory
# as the build script. 
srcDir = os.path.dirname(os.path.abspath(__file__))

# Presume that the current directory is the build directory.
buildDir = os.path.abspath(os.curdir)

# Use a temporary directory within the build directory to assemble the components
# for the installable package.
tmpDir = os.path.join(buildDir, 'openweave-wheel-components')

try:

    #
    # Perform a series of setup steps prior to creating the openweave package...
    #

    # Create the temporary components directory.
    if os.path.isdir(tmpDir):
        shutil.rmtree(tmpDir)
    os.mkdir(tmpDir)

    # Switch to the temporary directory. (Foolishly, setuptools relies on the current directory
    # for many of its features.)
    os.chdir(tmpDir)

    # Make a copy of the openweave package in the tmp directory and ensure the copied
    # directory is writable.
    owPackageDir = os.path.join(tmpDir, packageName)
    if os.path.isdir(owPackageDir):
        shutil.rmtree(owPackageDir)
    shutil.copytree(os.path.join(srcDir, 'openweave'), owPackageDir)
    os.chmod(owPackageDir, os.stat(owPackageDir).st_mode|stat.S_IWUSR)
    
    # Copy the openweave wrapper DLL from where libtool places it (.libs) into
    # the root of the openweave package directory.  This is necessary because
    # setuptools will only add package data files that are relative to the
    # associated package source root.
    shutil.copy2(os.path.join(buildDir, '.libs', owDLLName),
                 os.path.join(owPackageDir, owDLLName))
    
    # Make a copy of the Weave Device Manager Shell script in the tmp directory,
    # but without the .py suffix. This is how we want it to appear when installed
    # by the wheel.
    shutil.copy2(os.path.join(srcDir, deviceManagerShellName),
                 os.path.join(tmpDir, deviceManagerShellInstalledName))
    
    # Search for the OpenWeave LICENSE file in the parents of the source
    # directory and make a copy of the file called LICENSE.txt in the tmp
    # directory.  
    def _AllDirsToRoot(dir):
        dir = os.path.abspath(dir)
        while True:
            yield dir
            parent = os.path.dirname(dir)
            if parent == '' or parent == dir:
                break
            dir = parent
    for dir in _AllDirsToRoot(srcDir):
        licFileName = os.path.join(dir, 'LICENSE')
        if os.path.isfile(licFileName):
            shutil.copy2(licFileName,
                         os.path.join(tmpDir, 'LICENSE.txt'))
            break
    else:
        raise FileNotFoundError('Unable to find OpenWeave LICENSE file')
    
    # Define a custom version of the bdist_wheel command that configures the
    # resultant wheel as platform-specific (i.e. not "pure"). 
    class bdist_wheel_override(bdist_wheel):
        def finalize_options(self):
            bdist_wheel.finalize_options(self)
            self.root_is_pure = False

        def get_tag(self):
            python, abi, platform = bdist_wheel.get_tag(self)
            # Openweave does not depend on Python version or ABI
            python, abi = 'py2.py3', 'none'
            return python, abi, platform
    
    # Construct the package version string.  If building under Travis use the Travis
    # build number as the package version.  Otherwise use a dummy version of '0.0'.
    # (See PEP-440 for the syntax rules for python package versions).
    if 'TRAVIS_BUILD_NUMBER' in os.environ:
        owPackageVer = os.environ['TRAVIS_BUILD_NUMBER']
    else:
        owPackageVer = os.environ.get('OPENWEAVE_PYTHON_VERSION', '0.0')
    
    # Generate a description string with information on how/when the package
    # was built. 
    if 'TRAVIS_BUILD_NUMBER' in os.environ:
        buildDescription = 'Built by Travis CI on %s\n- Build: %s/#%s\n- Build URL: %s\n- Branch: %s\n- Commit: %s\n' % (
                                datetime.now().strftime('%Y/%m/%d %H:%M:%S'),
                                os.environ['TRAVIS_REPO_SLUG'],
                                os.environ['TRAVIS_BUILD_NUMBER'],
                                os.environ['TRAVIS_BUILD_WEB_URL'],
                                os.environ['TRAVIS_BRANCH'],
                                os.environ['TRAVIS_COMMIT'])
    else:
        buildDescription = 'Build by %s on %s\n' % (
                                getpass.getuser(),
                                datetime.now().strftime('%Y/%m/%d %H:%M:%S'))
    
    # Select required packages based on the target system.
    if platform.system() == 'Linux':
        requiredPackages = [
            'dbus-python',
            'pgi'
        ]
    else:
        requiredPackages = []
    
    #
    # Build the openweave package...
    #
     
    # Invoke the setuptools 'bdist_wheel' command to generate a wheel containing
    # the OpenWeave python packages, shared libraries and scripts.
    setup(
        name=packageName,
        version=owPackageVer,
        description='Python-base APIs and tools for OpenWeave.',
        long_description=buildDescription,
        url='https://github.com/openweave/openweave-core',
        license='Apache',
        classifiers=[
            'Intended Audience :: Developers',
            'License :: OSI Approved :: Apache Software License',
            'Programming Language :: Python :: 2',
            'Programming Language :: Python :: 2.7',
            'Programming Language :: Python :: 3',
        ],
        python_requires='>=2.7',
        packages=[
            packageName                     # Arrange to install a package named "openweave"
        ],
        package_dir={
            '':tmpDir,                      # By default, look in the tmp directory for packages/modules to be included.
        },
        package_data={
            packageName:[
                owDLLName                   # Include the wrapper DLL as package data in the "openweave" package.
            ]
        },
        scripts=[                           # Install the Device Manager Shell as an executable script in the 'bin' directory.
            os.path.join(tmpDir, deviceManagerShellInstalledName)
        ],
        install_requires=requiredPackages,
        options={
            'bdist_wheel':{
                'universal':False,
                'dist_dir':buildDir         # Place the generated .whl in the build directory.
            },
            'egg_info':{
                'egg_base':tmpDir           # Place the .egg-info subdirectory in the tmp directory.
            }
        },
        cmdclass={
            'bdist_wheel':bdist_wheel_override
        },
        script_args=[ 'clean', '--all', 'bdist_wheel' ]
    )

finally:
    
    # Switch back to the initial current directory.
    os.chdir(curDir)

    # Remove the temporary directory.
    if os.path.isdir(tmpDir):
        shutil.rmtree(tmpDir)
