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

/**
 *    @file
 *      This file implements a process to effect a functional test for
 *      LwIP's Domain Name Service (DNS) interface.
 *
 */

#include <string.h>

#include "ToolCommon.h"
#include <lwip/dns.h>

#define TOOL_NAME "TestDNS"

// TODO BUG: TestDNS is never built.

static bool HandleNonOptionArgs(const char *progName, int argc, char *argv[]);
static void DriveSend();
static void HandleConnectionComplete(TCPEndPoint *ep, INET_ERROR conErr);
static void HandleConnectionReceived(TCPEndPoint *listeningEP, TCPEndPoint *conEP, IPAddress peerAddr, uint16_t peerPort);
static void HandleConnectionClosed(TCPEndPoint *ep, INET_ERROR err);
static void HandleDataSent(TCPEndPoint *ep, uint16_t len);
static void HandleDataReceived(TCPEndPoint *ep, PacketBuffer *data);
static void HandleAcceptError(TCPEndPoint *endPoint, INET_ERROR err);
static void HandleUDPMessageReceived(UDPEndPoint *endPoint, PacketBuffer *msg, IPAddress senderAddr, uint16_t senderPort);
static void HandleUDPReceiveError(UDPEndPoint *endPoint, INET_ERROR err, IPAddress senderAddr, uint16_t senderPort);
static PacketBuffer *MakeDataBuffer(int32_t len);

// Globals
ip_addr_t   ipaddrs[DNS_MAX_ADDRS_PER_NAME];
uint8_t     numipaddrs = DNS_MAX_ADDRS_PER_NAME;

bool Listen = false;
IPAddress DestAddr = IPAddress::Any;
uint64_t LastSendTime = 0;
int32_t MaxSendCount = -1;
int32_t SendInterval = 1000000;
int32_t SendLength = 3200;
int32_t MaxSendLength = -1;
int32_t MinRcvLength = 0;
int32_t MaxRcvLength = -1;
int32_t TotalSendLength = 0;
int32_t TotalRcvLength = 0;
bool UseTCP = false;
TCPEndPoint *ConnectionEP = NULL;
TCPEndPoint *ListenEP = NULL;
UDPEndPoint *UDPEP = NULL;
static const char *hostname       = NULL;
static const char *DNSServerAddr  = NULL;

static HelpOptions gHelpOptions(
    TOOL_NAME,
    "Usage: " TOOL_NAME " [<options...>] <hostname> <dns-server-address>\n",
    WEAVE_VERSION_STRING "\n" WEAVE_TOOL_COPYRIGHT
);

static OptionSet *gToolOptionSets[] =
{
    &gNetworkOptions,
    &gFaultInjectionOptions,
    &gHelpOptions,
    NULL
};

static void
found_multi(const char *name, ip_addr_t *ipaddrs, u8_t numipaddrs, void *callback_arg)
{
    int i;
    printf("\tfound_multi response\n");
    printf("\tName: %s\n", name);
    printf("\tnumipaddrs: %d (DNS_MAX_ADDRS_PER_NAME: %d)\n", numipaddrs, DNS_MAX_ADDRS_PER_NAME);
    for (i = 0; i < numipaddrs; ++i)
    {
		char addrStr[64];
        IPAddress::FromIPv4(ipaddrs[i]).ToString(addrStr, sizeof(addrStr));
        printf("\t(%d) IPv4: %s\n", i, addrStr);
    }
    Done = true;
}


int main(int argc, char *argv[])
{
    unsigned int i;

    SetSIGUSR1Handler();

    if (argc == 1)
    {
        gHelpOptions.PrintBriefUsage(stderr);
        exit(EXIT_FAILURE);
    }

    if (!ParseArgs(TOOL_NAME, argc, argv, gToolOptionSets, HandleNonOptionArgs))
    {
        exit(EXIT_FAILURE);
    }

    InitSystemLayer();
    InitNetwork();

    u8_t numdns = 1;
    ip_addr_t dnsserver[1];
    IPAddress DNSServerIPv4Addr;
    IPAddress::FromString(DNSServerAddr, DNSServerIPv4Addr);
    dnsserver[0] = DNSServerIPv4Addr.ToIPv4();
    dns_setserver(numdns, dnsserver);

	printf("\nStarted dns_gethostbyname_multi test...\n\n");

    //Expected request / response
    printf("Expected request / response #1\n");
    printf("hn: %s, ips: %p, nips: %d, fm: %p, arg: %p\n", hostname, ipaddrs, numipaddrs, found_multi, NULL);
    printf("ip[0]: %d, ip[1]: %d\n", ipaddrs[0], ipaddrs[1]);
    err_t res = dns_gethostbyname_multi(hostname, ipaddrs, &numipaddrs, found_multi, NULL);
    if (res == ERR_INPROGRESS)
    {
	    printf("\tdns_gethostbyname_multi: %d (ERR_INPROGRESS)\n", res);
    }
    else
    {
	    printf("\tdns_gethostbyname_multi: %d (expected -5: ERR_INPROGRESS)\n", res);
    }
/*
    //Cancel request / response
    printf("Cancel request / response #1\n");
    dns_cancel((dns_found_callbackX)found_multi, NULL);

    //Expected request / response
    printf("Expected request / response #2\n");
    res = dns_gethostbyname_multi(hostname, ipaddrs, &numipaddrs, found_multi, NULL);
    if (res == ERR_INPROGRESS)
    {
	    printf("\tdns_gethostbyname_multi: %d (ERR_INPROGRESS)\n", res);
    }
    else
    {
	    printf("\tdns_gethostbyname_multi: %d (expected -5: ERR_INPROGRESS)\n", res);
    }
*/
	while (!Done)
	{
		struct timeval sleepTime;
		sleepTime.tv_sec = 0;
		sleepTime.tv_usec = 10000;

		ServiceNetwork(sleepTime);
	}

    //Expected cached response
    printf("Expected cached response #1\n");
    numipaddrs = DNS_MAX_ADDRS_PER_NAME;
    printf("hn: %s, ips: %p, nips: %d, fm: %p, arg: %p\n", hostname, ipaddrs, numipaddrs, found_multi, NULL);
    printf("ip[0]: %d, ip[1]: %d\n", ipaddrs[0], ipaddrs[1]);
    res = dns_gethostbyname_multi(hostname, ipaddrs, &numipaddrs, found_multi, NULL);
    if (res == ERR_OK)
    {
	    printf("\tdns_gethostbyname_multi: %d (ERR_OK)\n", res);
        printf("\tlocal DNS cache response\n");
        printf("\tName: %s\n", hostname);
        printf("\tnumipaddrs: %d\n", numipaddrs);
        for (i = 0; i < numipaddrs; ++i)
        {
            char addrStr[64];
            IPAddress::FromIPv4(ipaddrs[i]).ToString(addrStr, sizeof(addrStr));
            printf("\t(%d) IPv4: %s\n", i, addrStr);
        }
    }
    else
    {
	    printf("\tdns_gethostbyname_multi: %d (expected : ERR_OK)\n", res);
    }

    //Expected cached response
    printf("Expected cached response #2\n");
    numipaddrs = DNS_MAX_ADDRS_PER_NAME-1;
    printf("hn: %s, ips: %p, nips: %d, fm: %p, arg: %p\n", hostname, ipaddrs, numipaddrs, found_multi, NULL);
    printf("ip[0]: %d, ip[1]: %d\n", ipaddrs[0], ipaddrs[1]);
    res = dns_gethostbyname_multi(hostname, ipaddrs, &numipaddrs, found_multi, NULL);
    if (res == ERR_OK)
    {
	    printf("\tdns_gethostbyname_multi: %d (ERR_OK)\n", res);
        printf("\tlocal DNS cache response\n");
        printf("\tName: %s\n", hostname);
        printf("\tnumipaddrs: %d\n", numipaddrs);
        for (i = 0; i < numipaddrs; ++i)
        {
            char addrStr[64];
            IPAddress::FromIPv4(ipaddrs[i]).ToString(addrStr, sizeof(addrStr));
            printf("\t(%d) IPv4: %s\n", i, addrStr);
        }
    }
    else
    {
	    printf("\tdns_gethostbyname_multi: %d (expected : ERR_OK)\n", res);
    }

    //Expected cached response
    printf("Expected cached response #3\n");
    numipaddrs = 0;
    printf("hn: %s, ips: %p, nips: %d, fm: %p, arg: %p\n", hostname, ipaddrs, numipaddrs, found_multi, NULL);
    printf("ip[0]: %d, ip[1]: %d\n", ipaddrs[0], ipaddrs[1]);
    res = dns_gethostbyname_multi(hostname, ipaddrs, &numipaddrs, found_multi, NULL);
    if (res == ERR_OK)
    {
	    printf("\tdns_gethostbyname_multi: %d (ERR_OK)\n", res);
        printf("\tlocal DNS cache response\n");
        printf("\tName: %s\n", hostname);
        printf("\tnumipaddrs: %d\n", numipaddrs);
        for (i = 0; i < numipaddrs; ++i)
        {
            char addrStr[64];
            IPAddress::FromIPv4(ipaddrs[i]).ToString(addrStr, sizeof(addrStr));
            printf("\t(%d) IPv4: %s\n", i, addrStr);
        }
    }
    else
    {
	    printf("\tdns_gethostbyname_multi: %d (expected : ERR_OK)\n", res);
    }

    return 0;
}

bool HandleNonOptionArgs(const char *progName, int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("TestDNS: Missing %s argument\n", argc == 0 ? "<hostname>" : "<dns-server-address>");
        return false;
    }

    if (argc > 2)
    {
        printf("Unexpected argument: %s\n", argv[1]);
    }

    hostname        = argv[0];
    DNSServerAddr   = argv[1];

    return true;
}

void StartTest()
{
	INET_ERROR err;

	if (!UseTCP)
	{
		err = Inet.NewUDPEndPoint(&UDPEP);
		FAIL_ERROR(err, "InetLayer::NewUDPEndPoint failed");
		UDPEP->OnMessageReceived = HandleUDPMessageReceived;
		UDPEP->OnReceiveError = HandleUDPReceiveError;
	}

	if (Listen)
	{
		if (UseTCP)
		{
			err = Inet.NewTCPEndPoint(&ListenEP);
			FAIL_ERROR(err, "InetLayer::NewTCPEndPoint failed");

			ListenEP->OnConnectionReceived = HandleConnectionReceived;
			ListenEP->OnAcceptError = HandleAcceptError;

			err = ListenEP->Bind(kIPAddressType_IPv6, IPAddress::Any, 4242, true);
			FAIL_ERROR(err, "TCPEndPoint::Bind failed");

			err = ListenEP->Listen(1);
			FAIL_ERROR(err, "TCPEndPoint::Listen failed");
		}
		else
		{
			err = UDPEP->Bind(kIPAddressType_IPv6, IPAddress::Any, 4242);
			FAIL_ERROR(err, "UDPEndPoint::Bind failed");

			err = UDPEP->Listen();
			FAIL_ERROR(err, "UDPEndPoint::Listen failed");
		}

		printf("Listening...\n");
	}
	else
	{
		err = Inet.NewTCPEndPoint(&ConnectionEP);
		FAIL_ERROR(err, "InetLayer::NewTCPEndPoint failed");

		ConnectionEP->OnConnectComplete = HandleConnectionComplete;
		ConnectionEP->OnConnectionClosed = HandleConnectionClosed;
		ConnectionEP->OnDataSent = HandleDataSent;
		ConnectionEP->OnDataReceived = HandleDataReceived;

		err = ConnectionEP->Connect(DestAddr, 4242);
		FAIL_ERROR(err, "TCPEndPoint::Connect failed");
	}
}

void DriveSend()
{
	if (TotalSendLength == MaxSendLength)
		return;

	bool isTimeToSend = (Now() >= LastSendTime + SendInterval);
	if (!isTimeToSend)
		return;

	if (UseTCP)
	{
		if (ConnectionEP == NULL)
			return;

		if (ConnectionEP->PendingSendLength() > 0)
			return;

		int32_t sendLen = SendLength;
		if (MaxSendLength != -1)
		{
			int32_t remainingLen = MaxSendLength - TotalSendLength;
			if (sendLen > remainingLen)
				sendLen = remainingLen;
		}

		INET_ERROR err;
		PacketBuffer *buf = MakeDataBuffer(sendLen);
		if (buf == NULL)
		{
			printf("Failed to allocate PacketBuffer\n");
			LastSendTime = Now();
			return;
		}

		LastSendTime = Now();

		err = ConnectionEP->Send(buf);
		FAIL_ERROR(err, "TCPEndPoint::Send failed");

		TotalSendLength += sendLen;

		if (MaxSendLength != -1 && TotalSendLength == MaxSendLength && !Listen)
		{
			printf("Closing connection\n");
			err = ConnectionEP->Close();
			FAIL_ERROR(err, "TCPEndPoint::Close failed");

			printf("Freeing end point\n");
			ConnectionEP->Free();
			ConnectionEP = NULL;
		}
	}
	else if (DestAddr != IPAddress::Any)
	{
		int32_t sendLen = SendLength;
		if (MaxSendLength != -1)
		{
			int32_t remainingLen = MaxSendLength - TotalSendLength;
			if (sendLen > remainingLen)
				sendLen = remainingLen;
		}

		INET_ERROR err;
		PacketBuffer *buf = MakeDataBuffer(sendLen);
		if (buf == NULL)
		{
			printf("Failed to allocate PacketBuffer\n");
			LastSendTime = Now();
			return;
		}

		LastSendTime = Now();

		err = UDPEP->SendTo(DestAddr, 4242, buf);
		FAIL_ERROR(err, "UDPEndPoint::SendTo failed");

		TotalSendLength += sendLen;
	}
}

void HandleConnectionComplete(TCPEndPoint *ep, INET_ERROR conErr)
{
	INET_ERROR err;
	IPAddress peerAddr;
	uint16_t peerPort;

	if (conErr == WEAVE_NO_ERROR)
	{
		err = ep->GetPeerInfo(&peerAddr, &peerPort);
		FAIL_ERROR(err, "TCPEndPoint::GetPeerInfo failed");

		char peerAddrStr[64];
		peerAddr.ToString(peerAddrStr, sizeof(peerAddrStr));

		printf("Connection established to %s:%ld\n", peerAddrStr, (long)peerPort);
	}
	else
		printf("Connection FAILED: %s\n", ErrorStr(conErr));
}

void HandleConnectionReceived(TCPEndPoint *listeningEP, TCPEndPoint *conEP, IPAddress peerAddr, uint16_t peerPort)
{
	char peerAddrStr[64];
	peerAddr.ToString(peerAddrStr, sizeof(peerAddrStr));

	if (ConnectionEP == NULL)
	{
		printf("Accepted connection from %s, port %ld\n", peerAddrStr, (long)peerPort);

		conEP->OnConnectComplete = HandleConnectionComplete;
		conEP->OnConnectionClosed = HandleConnectionClosed;
		conEP->OnDataSent = HandleDataSent;
		conEP->OnDataReceived = HandleDataReceived;

		ConnectionEP = conEP;
	}
	else
		printf("Rejected connection from %s, port %ld\n", peerAddrStr, (long)peerPort);
}

void HandleConnectionClosed(TCPEndPoint *ep, INET_ERROR err)
{
	if (err == WEAVE_NO_ERROR)
		printf("Connection closed\n");
	else
		printf("Connection closed with error: %s\n", ErrorStr(err));

	printf("Freeing end point\n");
	ep->Free();
	if (ep == ConnectionEP)
		ConnectionEP = NULL;
}

void HandleDataSent(TCPEndPoint *ep, uint16_t len)
{
	printf("Data sent: %ld\n", (long)len);

	if (ep->State == TCPEndPoint::kState_Closed)
	{
		printf("Freeing end point\n");
		ep->Free();
	}
	else
		DriveSend();
}

void HandleDataReceived(TCPEndPoint *ep, PacketBuffer *data)
{
	INET_ERROR err;

	if (data->TotalLength() < MinRcvLength && ep->State == TCPEndPoint::kState_Connected)
	{
		err = ep->PutBackReceivedData(data);
		FAIL_ERROR(err, "TCPEndPoint::PutBackReceivedData failed");
		return;
	}

	for (PacketBuffer *buf = data; buf; buf = buf->Next())
	{
		printf("Data received (%ld bytes)\n", (long)buf->DataLength());
		DumpMemory(buf->Start(), buf->DataLength(), "  ", 16);

		uint8_t *p = buf->Start();
		for (uint16_t i = 0; i < buf->DataLength(); i++, p++, TotalRcvLength++)
			if (*p != (uint8_t)TotalRcvLength)
			{
				printf("Bad data value, offset %d\n", (int)i);
				exit(-1);
			}
	}

	ep->AckReceive(data->TotalLength());

	PacketBuffer::Free(data);

	if (MaxRcvLength != -1 && TotalRcvLength >= MaxRcvLength)
	{
		printf("Closing connection\n");
		err = ep->Close();
		FAIL_ERROR(err, "TCPEndPoint::Close failed");

		printf("Freeing end point\n");
		ep->Free();
		if (ep == ConnectionEP)
			ConnectionEP = NULL;
	}
}

void HandleAcceptError(TCPEndPoint *endPoint, INET_ERROR err)
{
	printf("Accept error: %s\n", ErrorStr(err));
}

void HandleUDPMessageReceived(UDPEndPoint *endPoint, PacketBuffer *msg, IPAddress senderAddr, uint16_t senderPort)
{
	char senderAddrStr[64];
	senderAddr.ToString(senderAddrStr, sizeof(senderAddrStr));

	printf("UDP message received from %s, port %ld (%ld bytes)\n", senderAddrStr, (long)senderPort, (long)msg->DataLength());
	DumpMemory(msg->Start(), msg->DataLength(), "  ", 16);

	PacketBuffer::Free(msg);
}

void HandleUDPReceiveError(UDPEndPoint *endPoint, INET_ERROR err, IPAddress senderAddr, uint16_t senderPort)
{
	char senderAddrStr[64];
	senderAddr.ToString(senderAddrStr, sizeof(senderAddrStr));

	printf("UDP receive error (%s, port %ld): %s\n", senderAddrStr, (long)senderPort, ErrorStr(err));
}

PacketBuffer *MakeDataBuffer(int32_t desiredLen)
{
	PacketBuffer *buf;

	buf = PacketBuffer::New();
	if (buf == NULL)
		return NULL;

	if (desiredLen > buf->MaxDataLength())
		desiredLen = buf->MaxDataLength();

	char *p = (char *)buf->Start();

	for (uint16_t i = 0; i < desiredLen; i++, p++)
		*p = (uint8_t)(TotalSendLength + i);

	buf->SetDataLength(desiredLen);

	return buf;
}

