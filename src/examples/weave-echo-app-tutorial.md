# Weave Example Application Tutorial

## Introduction
The Weave Echo example application shows you how to implement a Weave
application program using one of the supported Weave profiles, namely
the very simple Weave Echo profile. This profile allows you to send a
Weave message to a peer and expect a Weave response (similar to the
ICMP Echo Request/Echo Response messages).
For the purposes of this tutorial, we will focus on implementing Weave Echo
over an underlying TCP connection. However, Weave supports other transport
protocols, for example, regular UDP and a reliable version of UDP, called
the Weave Reliable Messaging Protocol (WRMP).

Weave Profiles are, essentially, implementations of specific protocols over
the Weave transport. An example of such a protocol is Weave Data Management
which is a data synchronization protocol using a publish/subscribe mechanism
between peers. Furthermore, when two Weave nodes are exchanging messages of a
particular Weave Profile, they do so over a construct called a Weave Exchange
which is a description of a Weave-based conversation over a Weave profile. A
Weave Exchange is characterised by the ExchangeContext object, and every Weave
node must create an ExchangeContext object before initiating a Weave
conversation.
After constructing a Weave ExchangeContext, Weave messages are sent and
received using the WeaveMessageLayer class which sends the Weave message
over a chosen transport (TCP, UDP, or WRMP).

## Setup

To run the Weave Echo example application, each Weave node (requester and
responder) must have a unique IPv6 address which it can use to exchange
messages with its peer. For simplicity, these addresses are hardcoded
within the example code as fd00:0:1:1::1 for the WeaveEchoRequester and
fd00:0:1:1::2 for the WeaveEchoResponder, respectively.
These addresses could be assigned to the local machine’s loopback interface
using the following Linux commands :

$ sudo ip addr add fd00:0:1:1::1 dev lo
$ sudo ip addr add fd00:0:1:1::2 dev lo

## Code Walk Through
As part of this example, we have a WeaveEchoRequester program that acts as
the client and sends echo requests to a WeaveEchoResponder program that
receives EchoRequests and sends back EchoResponse messages. Additionally,
we have collected a set of common functions, for example, for initialization
and shutdown of the Weave stack in `weave-app-common.cpp`.

### Weave EchoRequester
The Weave EchoRequester is implemented in the file `weave-echo-requester.cpp`.
We dissect the program here to understand the general flow.

The program includes the required header files:
* `weave-app-common.h` :
                         Defines the common functions for
                         initializing/shutting down the Weave stack and perform
                         network I/O, etc.
* `Weave/Profiles/echo/WeaveEcho.h` :
                         Header file for the Weave Echo Profile that would
                         be used by the application.
* `Weave/Support/ErrorStr.h` :
                         Utility support file for printing Weave errors in
                         string format.

The main function logic flow is organized as follows:-

1. Initialize the Weave stack.

   ```
   // Initialize the Weave stack as the client.
   InitializeWeave(false);
   ```
   * As part of initializing Weave, the several sublayers of the Weave stack are
     initialized.
   * The layers of primary interest to developers (from bottom upwards) are:
       1. SystemLayer: For handling PacketBuffers, Timers, etc.
       2. InetLayer: Abstracts transport layers, e.g., TCP, UDP, etc. over Linux
          based and LwIP network platforms.
       3. WeaveMessageLayer: Handles mapping of Weave messaging onto IP-based
          transports. It provides APIs for establishing listening sockets,
          initiating and accepting TCP connections, and sending and receiving
          encoded Weave messages.
       4. WeaveExchangeManager: The WeaveExchangeManager manages
          ExchangeContexts for all Profile communication
   * The WeaveEchoClient initializes the Weave stack as a TCP client that would
     connect to a Weave server.

2. Initialize the EchoClient in the Weave Echo profile.

   ```
   // Initialize the EchoClient application.
   err = gEchoClient.Init(&ExchangeMgr);
   ```
3. Set the application callback for the Echo responses.

   ```
   gEchoClient.OnEchoResponseReceived = HandleEchoResponseReceived;
   ```

4. Formulate the address, Weave node identifier, and destination port for the peer.

   ```
   // Set the destination fields before initiating the Weave connection.

   IPAddress::FromString("fd00:0:1:1::2", gDestv6Addr);

   // Derive the destination node ID from the IPv6 address.
   gDestNodeId = IPv6InterfaceIdToWeaveNodeId(gDestv6Addr.InterfaceId());

   // Set the destination port to be the Weave port.
   gDestPort = WEAVE_PORT;
   ```

5. Attempt to establish a TCP connection (managed by the Weave connection object)
   to the peer.
   Attempt a number of times before giving up and exiting the program.

   ```
    // Wait until the Weave connection is established.
    while (!gClientConEstablished)
   {
       // Start the Weave connection to the weave echo responder.
       StartClientConnection();

       DriveIO();

       if (gConnectAttempts > MAX_CONNECT_ATTEMPTS)
       {
           exit(EXIT_FAILURE);
       }
   }
   ```
   * When creating the TCP connection in StartClientConnection(), we create
     a new WeaveConnection object.
     ```
     gCon = MessageLayer.NewConnection();
     ```
   * Set the callbacks for handling completion of a connection attempt and
     the closure of an established connection.
     ```
     gCon->OnConnectionComplete = HandleConnectionComplete;
     gCon->OnConnectionClosed = HandleConnectionClosed;
     ```
   * Next, attempt to connect to the peer using the WeaveConnection object.
     ```
     err = gCon->Connect(gDestNodeId, gDestv6Addr, gDestPort);
     ```

6. Once established, send the configured number of Echo requests.

   ```
   // Connection has been established. Now send the EchoRequests.
   for (int i = 0; i < MAX_ECHO_COUNT; i++)
   {
       // Send an EchoRequest message.
       SendEchoRequest();

       // Wait for response until the Echo interval.
       while (!EchoIntervalExpired())
       {
           DriveIO();

       }

       // Check if expected response was received.
       if (gWaitingForEchoResp)
       {
           printf("No response received\n");

           gWaitingForEchoResp = false;
       }
   }
   ```
   * In SendEchoRequest(), as part of formulating the EchoRequest, we
     create a new PacketBuffer and write some application data into it.
     ```
     FormulateEchoRequestBuffer(payloadBuf);
     ```
   * Inside the SendEchoRequest() API function , the WeaveEchoClient profile
     creates an ExchangeContext.
     ```
     ExchangeCtx = ExchangeMgr->NewContext(nodeId, nodeAddr, WEAVE_PORT,
                                           sendIntfId, this);
     ```
   * Sets the response handler on the created ExchangeContext.
     ```
     ExchangeCtx->OnMessageReceived = HandleResponse;
     ```
   * Sends the message over that ExchangeContext specifying the appropriate
     profile and message type.
     ```
     err = ExchangeCtx->SendMessage(kWeaveProfile_Echo,
                                    kEchoMessageType_EchoRequest, payload);
     ```
7. Close the connection.

   ```
   // Done with all EchoRequests and EchoResponses.
   CloseClientConnection();
   ```
8. Shutdown the EchoClient in the Weave Echo Profile.
   ```
   gEchoClient.Shutdown();
   ```

9. Shutdown the Weave stack.

### Weave EchoResponder
The Weave EchoResponder is implemented in the file `weave-echo-responder.cpp`.
Weave EchoResponder's logic flow is similar to the Weave EchoRequester.
1. We initialize the Weave stack as the TCP server.
   ```
   // Initialize the Weave stack.

   InitializeWeave(true);
   ```

2. Then, we initialize the Weave EchoServer profile that listens for incoming
   TCP connections.
   ```
   // Initialize the EchoServer application.

   err = gEchoServer.Init(&ExchangeMgr);
   ```
3. We wait in a loop for EchoRequest messages from the client.
   The requests are handled by the EchoServer profile in the following function.
   ```
   void WeaveEchoServer::HandleEchoRequest(ExchangeContext *ec,
                                           const IPPacketInfo *pktInfo,
                                           const WeaveMessageInfo *msgInfo,
                                           uint32_t profileId, uint8_t msgType,
                                           PacketBuffer *payload)
   {
      ...

   }
   ```
   The response is sent inside this function by calling the SendMessage(..) API
   on the received ExchangeContext specifying the appropriate profile and
   message type.
   ```
   // Send an Echo Response back to the sender.
   ec->SendMessage(kWeaveProfile_Echo, kEchoMessageType_EchoResponse, payload);
   ```
