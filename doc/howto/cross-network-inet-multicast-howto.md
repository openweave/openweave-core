# OpenWeave + Happy Cross Network Multicast Inet Layer HOWTO

You have decided that you would like to experiment with cross-network multicast using OpenWeave. This how-to guide illustrates using the Happy network simulation tool to set up a virtual topology that demonstrates using OpenWeave's Inet layer across two distinct networks with a multicast proxy.

## Step 1: Download and Build mcproxy

The IPv6 multicast proxy daemon, _mcproxy_, serves to proxy / forward / route IPv6 multicast traffic across two disjoint IPv6 network links.

While it is not particularly important where you clone and build the _mcproxy_ daemon executable, please take note of the location where you create it as a relative or absolute path to it will be necessary in Step 6 below.

```
% git clone https://github.com/mcproxy/mcproxy.git mcproxy
% sudo apt-get install qt5-qmake qt5-default
% cd mcproxy/mcproxy
% qmake
% make
```

## Step 2: Download, Build, and Install happy

```
% git clone https://github.com/openweave/happy.git happy
% sudo apt-get install bridge-utils python-lockfile python-psutil python-setuptools
% cd happy
% sudo make install
```

## Step 3: Download and Build openweave-core

```
% git clone https://github.com/openweave/openweave-core.git openweave-core
% cd openweave-core
% ./configure
% make
```

## Step 4: Establish the Happy Topology

This creates a topology very similar to the [Happy Codelab](https://codelabs.developers.google.com/codelabs/happy-weave-getting-started/#0).

```
% happy-network-add ThreadNetwork thread
% happy-network-address ThreadNetwork fd00:0000:0000:0006::
% happy-network-add WiFiNetwork wifi
% happy-network-address WiFiNetwork fd00:0000:0000:0001::
% happy-network-address WiFiNetwork 192.168.1.0
% happy-node-add ThreadNode
% happy-node-add WiFiNode
% happy-node-add BorderRouter
% happy-node-join ThreadNode ThreadNetwork
% happy-node-join WiFiNode WiFiNetwork
% happy-node-join BorderRouter ThreadNetwork
% happy-node-join BorderRouter WiFiNetwork
% happy-network-route --prefix fd00:0000:0000:0006:: ThreadNetwork BorderRouter
% happy-network-route --prefix fd00:0000:0000:0001:: WiFiNetwork BorderRouter
% happy-network-route --prefix 192.168.1.0 WiFiNetwork BorderRouter
```

## Step 5: Create the mcproxy Configuration for the Happy Topology

This creates an IPv6 multicast proxy configuration between the simulated "Thread" network interface on "wpan0" and the simulated "WiFi" network interface on "wlan0" in the Happy topology we created above in Step 4.

While it is not particularly important where you create the _mcproxy.conf_ file, please take note of the location where you create it as a relative or absolute path to it will be necessary in Step 6 below.

```
% cat > mcproxy.conf << EOF
protocol MLDv2;
pinstance myProxy: wpan0 ==> wlan0;
EOF
```

## Step 6: Run the Demonstration

This will run the IPv6 multicast proxy, _mcproxy_, on the "BorderRouter" node and then will launch the Inet layer multicast functional test sender and receiver on the simulated "Thread" and "WiFi" nodes, respectively.

If you'd like, you can transpose the sender and the receiver nodes and the example will work equally as well.

Each of the following sets of commands should be run from parallel, independent shells.

### Border Router

```
% happy-shell BorderRouter
$ <absolute or relative path to mcproxy daemon executable from Step 1>/mcproxy -f <absolute or relative path to mcproxy configuration file from Step 5>/mcproxy.conf
```

### Receiver

```
% happy-shell WiFiNode
$ openweave-core/src/test-apps/TestInetLayerMulticast -6 --udp -I wlan0 -g 5 --group-expected-rx-packets 5 --group-expected-tx-packets 0 -l
```

### Sender

```
% happy-shell ThreadNode
$ openweave-core/src/test-apps/TestInetLayerMulticast -6 --udp -I wpan0 -g 5 --group-expected-rx-packets 0 --group-expected-tx-packets 5 -L
```

### Output[^1]

#### Sender

```
$ openweave-core/src/test-apps/TestInetLayerMulticast -6 --udp -I wlan0 -g 5 ... -L
Weave Node ready to service events; PID: 50845; PPID: 46482
Using UDP/IPv6, device interface: wpan0 (w/o LwIP)
Will join multicast group ff15::5
1/5 transmitted for multicast group 5
2/5 transmitted for multicast group 5
3/5 transmitted for multicast group 5
4/5 transmitted for multicast group 5
5/5 transmitted for multicast group 5
Will leave multicast group ff15::5
WEAVE:IN: Async DNS worker thread woke up.
WEAVE:IN: Async DNS worker thread exiting.
WEAVE:IN: Async DNS worker thread woke up.
WEAVE:IN: Async DNS worker thread exiting.
```

#### Receiver

```
$ openweave-core/src/test-apps/TestInetLayerMulticast -6 --udp -I wlan0 -g 5 ... -l
Weave Node ready to service events; PID: 50826; PPID: 46499
Using UDP/IPv6, device interface: wlan0 (w/o LwIP)
Will join multicast group ff15::5
Listening...
UDP packet received from fd00::6:8693:b7ff:fe5a:1dc1:4242 to ff15::5:4242 (59 bytes)
1/5 received for multicast group 5
UDP packet received from fd00::6:8693:b7ff:fe5a:1dc1:4242 to ff15::5:4242 (59 bytes)
2/5 received for multicast group 5
UDP packet received from fd00::6:8693:b7ff:fe5a:1dc1:4242 to ff15::5:4242 (59 bytes)
3/5 received for multicast group 5
UDP packet received from fd00::6:8693:b7ff:fe5a:1dc1:4242 to ff15::5:4242 (59 bytes)
4/5 received for multicast group 5
UDP packet received from fd00::6:8693:b7ff:fe5a:1dc1:4242 to ff15::5:4242 (59 bytes)
5/5 received for multicast group 5
Will leave multicast group ff15::5
WEAVE:IN: Async DNS worker thread woke up.
WEAVE:IN: Async DNS worker thread exiting.
WEAVE:IN: Async DNS worker thread woke up.
WEAVE:IN: Async DNS worker thread exiting.
```

[^1]: Your output may vary slightly around the process IDs and source addresses displayed by the receiver.
