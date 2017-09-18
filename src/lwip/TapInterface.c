/*
 *
 *    Copyright (c) 2014-2017 Nest Labs, Inc.
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

/*-----------------------------------------------------------------------------------*/
/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#if defined(linux)
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#define DEVTAP "/dev/net/tun"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d"

#elif defined(openbsd)
#define DEVTAP "/dev/tun0"
#define IFCONFIG_ARGS "tun0 inet %d.%d.%d.%d link0"

#else /* freebsd, cygwin? */
#include <net/if.h>
#define DEVTAP "/dev/tap0"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d"
#endif

#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <lwip/mem.h>
#include <netif/etharp.h>
#include <lwip/ethip6.h>

#include "TapInterface.h"

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *buf)
{
    err_t retval = ERR_OK;
    TapInterface *tapif = netif->state;
    struct pbuf *outBuf;
    int written;

    if (buf->tot_len > buf->len)
    {
        // Allocate a buffer from the buffer pool. Fail if none available.
        outBuf = pbuf_alloc(PBUF_RAW, buf->tot_len + SUB_ETHERNET_HEADER_SPACE, PBUF_POOL);
        if (outBuf == NULL) {
            fprintf(stderr, "TapInterface: Failed to allocate buffer\n");
            retval = ERR_MEM;
            goto done;
        }

        // Fail if the buffer is not big enough to hold the output data.
        if (outBuf->tot_len != outBuf->len) {
            fprintf(stderr, "TapInterface: Output data bigger than single PBUF\n");
            retval = ERR_BUF;
            goto done;
        }

        // Reserve the space needed by WICED for its buffer management.
        pbuf_header(outBuf, -SUB_ETHERNET_HEADER_SPACE);

        // Copy output data to the new buffer.
        retval = pbuf_copy(outBuf, buf);
        if (retval != ERR_OK)
            goto done;
    }

    // Otherwise send using the supplied buffer.
    else
        outBuf = buf;

    written = write(tapif->fd, outBuf->payload, outBuf->tot_len);
    if (written == -1) {
        snmp_inc_ifoutdiscards(netif);
        perror("TapInterface: write failed");
    }
    else {
        snmp_add_ifoutoctets(netif, written);
    }

done:
    if (outBuf != NULL && outBuf != buf)
        pbuf_free(outBuf);
    return retval;
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(TapInterface *tapif, struct netif *netif)
{
    struct pbuf *p, *q;
    u16_t len;
    char buf[2048];
    char *bufptr;

    /* Obtain the size of the packet and put it into the "len"
     variable. */
    len = read(tapif->fd, buf, sizeof(buf));
    snmp_add_ifinoctets(netif,len);

    /*  if (((double)rand()/(double)RAND_MAX) < 0.1) {
    printf("drop\n");
    return NULL;
    }*/

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_LINK, len, PBUF_POOL);

    if (p != NULL) {
        /* We iterate over the pbuf chain until we have read the entire
       packet into the pbuf. */
        bufptr = &buf[0];
        for(q = p; q != NULL; q = q->next) {
            /* Read enough bytes to fill this pbuf in the chain. The
         available data in the pbuf is given by the q->len
         variable. */
            /* read data into(q->payload, q->len); */
            memcpy(q->payload, bufptr, q->len);
            bufptr += q->len;
        }
        /* acknowledge that packet has been read(); */
    } else {
        /* drop packet(); */
        snmp_inc_ifindiscards(netif);
        printf("Could not allocate pbufs\n");
    }

    return p;
}


/*-----------------------------------------------------------------------------------*/
/*
 * TapInterface_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
TapInterface_SetupNetif(struct netif *netif)
{
    TapInterface *tapif = netif->state;

    netif->name[0] = 'e';
    netif->name[1] = 't';
    netif->output = etharp_output;
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = low_level_output;
    netif->mtu = 1500;
    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    netif->hwaddr_len = 6;
    memcpy(netif->hwaddr, tapif->macAddr, 6);

    return ERR_OK;
}

err_t
TapInterface_Init(TapInterface *tapif, const char *interfaceName, u8_t *macAddr)
{
#if defined(linux)
    struct ifreq ifr;
#endif

    memset(tapif, 0, sizeof(*tapif));
    tapif->fd = -1;

    tapif->interfaceName = interfaceName;

    if (macAddr != NULL)
        memcpy(tapif->macAddr, macAddr, 6);
    else {
        u32_t pid = htonl((u32_t)getpid());
        memset(tapif->macAddr, 0, 6);
        memcpy(tapif->macAddr + 2, &pid, sizeof(pid));
    }

    tapif->fd = open(DEVTAP, O_RDWR);
    if (tapif->fd == -1) {
        perror("TapInterface: unable to open " DEVTAP);
        return ERR_IF;
    }

#if defined(linux)
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
    if (strlen(tapif->interfaceName) >= sizeof(ifr.ifr_name)) {
        perror("TapInterface: invalid device name");
        return ERR_VAL;
    }
    strcpy(ifr.ifr_name, tapif->interfaceName);
    if (ioctl(tapif->fd, TUNSETIFF, (void *) &ifr) < 0) {
        perror("TapInterface: ioctl(TUNSETIFF) failed");
        return ERR_IF;
    }
#else
#warning "The LwIP TAP/TUN interface may not be fully-supported on your platform."
#endif /* defined(linux) */

    return ERR_OK;
}

int
TapInterface_Select(TapInterface *tapif, struct netif *netif, struct timeval sleepTime)
{
    fd_set fdset;
    int ret;

    FD_ZERO(&fdset);
    FD_SET(tapif->fd, &fdset);

    ret = select(tapif->fd + 1, &fdset, NULL, NULL, &sleepTime);
    if (ret > 0) {
        struct pbuf *p = low_level_input(tapif, netif);
        if (p != NULL) {
#if LINK_STATS
            lwip_stats.link.recv++;
#endif /* LINK_STATS */

            netif->input(p, netif);
        }
    }

    return ret;
}
