# What is OpenWeave?

Weave is the network application layer that provides a secure, reliable
communications backbone for Nest's products. OpenWeave is the open source
release of Weave.

This initial release makes available some of Weave’s core components. With
OpenWeave core, you can preview the technology by inspecting the code, learning
about the architecture, and understanding the security model.

At Nest, we believe the core technologies that underpin connected home products
need to be open and accessible. Alignment around common fundamentals will help
products securely and seamlessly communicate with one another. Nest's first open
source release was our implementation of Thread, OpenThread. OpenWeave can run
on OpenThread, and is another step in the direction of making our core
technologies more widely available.

# OpenWeave Features

* **Low Overhead**  
  Whereas many approaches to the Internet of Things attempt to start from
  server-based or -grown technologies, struggling to scale down, Weave starts
  with the perspective that if you can tackle the toughest Internet of Things
  problems on the constrained product front, you can successfully scale up from
  there to any product.

* **Pervasive architecture**  
  By building an application architecture on top of IP and starting with a low
  overhead architecture, we have achieved a holistic, common, and consistent
  architecture from low-power, sleepy Thread end nodes to cloud services and
  mobile devices with end-to-end addressability, reachability, and security,
  making it easier on your developers and helping instill customer confidence in
  Weave-enabled products.

* **Robust**  
  Weave leverages and takes advantages of underlying network attributes, such as
  Thread’s no single point-of-failure and mesh resiliency. In addition, Weave
  devices are auto-configuring and enjoy the ability to route among and across
  different network link technologies.

* **Secure**  
  Weave employs security as a fundamental and core design tenet with:
  * Defense in depth with end-to-end application security, independent of underlying network
  * Tiered trust domains with application-specific groupwise keys

* **Easy to Use**  
  Out-of-box experience is key to avoiding a connected product return and an
  impact on your product’s profitability. Weave’s focus on a smooth, cohesive,
  integrated provisioning and out-of-box experience ensures returns are
  minimized.  In addition to offering simple setup and administration for the
  end user, Weave also offers a straightforward but capable platform for the
  application developer.

* **Versatile**  
  OpenWeave has been designed to scale to home area networks containing low to
  mid-100s of devices. The communication support for variety of interaction
  models / patterns such as device-to-device, device-to-service,
  device-to-mobile/PC (remote and local).

# Get started with OpenWeave Core

Want to try OpenWeave? The quickest way is to build the standalone
distribution, and run through the attached examples. On Linux and OS X
machines, you can build OpenWeave distribution with

    make -f Makefile-Standalone

For more detailed information on configuring and building OpenWeave
for different platforms, please consult [BUILDING.md](./BUILDING.md).

Once the distribution is built, you can dive into a number of
examples:

* `weave-echo` will demonstrate perhaps the simplest possible Weave
  protocol. The associated example will also guide you through the
  steps needed to set up and start the OpenWeave stack.

* `weave-device-manager` and `mock-device` will guide you through the
  steps required to securely provision a new device to join the Weave
  fabric.

## Next Steps

Beyond the initial set of examples, you will find a larger set of test applications that the OpenWeave team uses to test the codebase. You will find applications that

* Test other Weave protocols — time sync, heartbeat, bulk data
  transfer, and a variety of security protocols
* Examples of different topologies found in different home area
  network scenarios
* Examples of establishing wide-area connectivity via a dedicated Weave tunnel 
* Examples of different scenarios for different Weave Data Management scenarios

# Need help?

## Wiki

Explore the [OpenWeave Wiki on
GitHub](https://github.com/openweave/openweave-core/wiki) for more
in-depth documentation on building, testing, automation and tools.

## Interact

There are numerous avenues for OpenWeave support:
* Bugs and feature requests — [submit to the Issue Tracker](https://github.com/openweave/openweave-core/issues)
* Stack Overflow — [post questions using the openweave tag](http://stackoverflow.com/questions/tagged/openweave)
* Google Groups — discussion and announcements at [openweave-users](https://groups.google.com/forum/#!forum/openweave-users)

The openweave-users Google Group is the recommended place for users to
discuss OpenWeave and interact directly with the OpenWeave team.

# Directory Structure

The OpenWeave repository is structured as follows:

| File / Folder | Contents |
|----|----|
| `aclocal.m4` | |
| `bootstrap` | GNU autotools bootstrap script for the OpenWeave build system. |
| `bootstrap-configure` | Convenience script that will bootstrap the OpenWeave build system, via `bootstrap`, and invoke `configure`.|
| `build/` | OpenWeave-specific build system support content.|
| `BUILDING.md` | More detailed information on configuring and building OpenWeave for different targets. |
| `certs/` | OpenWeave-related security certificate material.|
| `CHANGELOG` | Description of changes to OpenWeave from release-to-release.|
| `configure` | GNU autotools configuration script for OpenWeave. |
| `configure.ac` | GNU autotools configuration script source file for OpenWeave. |
| `CONTRIBUTING.md` | Guidelines for contributing to OpenWeave |
| `.default-version` | Default OpenWeave version if none is available via source code control tags, `.dist-version`, or `.local-version`.|
| `doc/` | Documentation and Doxygen build file. |
| `LICENSE` | OpenWeave license file (Apache 2.0) |
| `Makefile.am` | Top-level GNU automake makefile source.|
| `Makefile-Android` | Convenience makefile for building OpenWeave against Android.|
| `Makefile.in` | Top-level GNU autoconf makefile source. |
| `Makefile-iOS` | Convenience makefile for building OpenWeave against iOS.|
| `Makefile-Standalone` | Convenience makefile for building OpenWeave as a standalone package on desktop and server systems. |
| `README.md` | This file.|
| `src/` | Implementation of OpenWeave, including unit- and functional-tests, tools, and utilities. |
| `src/ble/` | Definition and implementation of OpenWeave over BLE |
| `src/device-manager/` | Library for interacting with a remote OpenWeave device, implementing pairing flows. Bindings are provided for Python, iOS, Java and Android. |
| `src/examples/` | Example code |
| `src/include/` | Build support for external headers |
| `src/inet/` | Abstraction of the IP stack. Currently supports socket- and LWIP-based implementation. |
| `src/lib/` | Core OpenWeave implementation. |
| `src/lwip` | LwIP configuration for the standalone OpenWeave build. |
| `src/platform` | Additional platform support code required to bind the BLE support to BlueZ libraries. |
| `src/ra-daemon/` | Route advertisement daemon implementation |
| `src/system/` | Abstraction of the required system support, such as timers, network buffers, and object pools. | 
| `src/test-apps` | Unit tests and test apps exercising Weave functionality |
| `src/tools` | Tools for generating Weave security material, and for device support |
| `src/warm/` | OpenWeave address and routing module |
| `src/wrappers` | Java wrappers for OpenWeave library |
| `third_party/` | Third-party code used by OpenWeave. |

# Want to contribute?

We would love for you to contribute to OpenWeave and help make it even
better than it is today! See the [CONTRIBUTING.md](./CONTRIBUTING.md)
file for more information.

# Versioning

OpenWeave follows the [Semantic Versioning guidelines](http://semver.org/) for release cycle transparency and to maintain backwards compatibility. 

# License

**License and the Weave and OpenWeave Brands**

The OpenWeave software is released under the [Apache 2.0
license](./LICENSE), which does not extend a license for the use of
the Weave and OpenWeave name and marks beyond what is required for
reasonable and customary use in describing the origin of the software
and reproducing the content of the NOTICE file.
 
The Weave and OpenWeave name and word (and other trademarks, including
logos and logotypes) are property of Nest Labs, Inc. Please refrain
from making commercial use of the Weave and OpenWeave names and word
marks and only use the names and word marks to accurately reference
this software distribution. Do not use the names and word marks in any
way that suggests you are endorsed by or otherwise affiliated with
Nest without first requesting and receiving a written license from us
to do so. See our [Trademarks and General
Principles](https://nest.com/legal/ip-and-other-notices/tm-list/) page
for additional details. Use of the Weave and OpenWeave logos and
logotypes is strictly prohibited without a written license from us to
do so.
