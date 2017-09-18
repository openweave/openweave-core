/**
 * @file
 * DNS - host name to IP address resolver.
 *
 */

/**

 * This file implements a DNS host name to IP address resolver.

 * Port to lwIP from uIP
 * by Jim Pettinato April 2007

 * uIP version Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * DNS.C
 *
 * The lwIP DNS resolver functions are used to lookup a host name and
 * map it to a numerical IP address. It maintains a list of resolved
 * hostnames that can be queried with the dns_lookup() function.
 * New hostnames can be resolved using the dns_enqueue() function.
 *
 * The lwIP version of the resolver also adds a non-blocking version of
 * gethostbyname() that will work with a raw API application. This function
 * checks for an IP address string first and converts it if it is valid.
 * gethostbyname() then does a dns_lookup() to see if the name is 
 * already in the table. If so, the IP is returned. If not, a query is 
 * issued and the function returns with a ERR_INPROGRESS status. The app
 * using the dns client must then go into a waiting state.
 *
 * Once a hostname has been resolved (or found to be non-existent),
 * the resolver code calls a specified callback function (which 
 * must be implemented by the module that uses the resolver).
 */

/*-----------------------------------------------------------------------------
 * RFC 1035 - Domain names - implementation and specification
 * RFC 2181 - Clarifications to the DNS Specification
 *----------------------------------------------------------------------------*/

/** @todo: define good default values (rfc compliance) */
/** @todo: improve answer parsing, more checkings... */
/** @todo: check RFC1035 - 7.3. Processing responses */

/*-----------------------------------------------------------------------------
 * Includes
 *----------------------------------------------------------------------------*/

#include "lwip/opt.h"

#if LWIP_DNS /* don't build if not configured for use in lwipopts.h */

#include "lwip/udp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/dns.h"

#include <stdint.h>
#include <string.h>

/** DNS server IP address */
#ifndef DNS_SERVER_ADDRESS
#define DNS_SERVER_ADDRESS(ipaddr)        (ip4_addr_set_u32(ipaddr, ipaddr_addr("208.67.222.222"))) /* resolver1.opendns.com */
#endif

/** DNS server port address */
#ifndef DNS_SERVER_PORT
#define DNS_SERVER_PORT           53
#endif

/** DNS maximum number of retries when asking for a name, before "timeout". */
#ifndef DNS_MAX_RETRIES
#define DNS_MAX_RETRIES           4
#endif

/** DNS max attempts to find unique random ID */
#ifndef DNS_MAX_RAND_ATTEMPT
#define DNS_MAX_RAND_ATTEMPT           4
#endif

#if DNS_MAX_RAND_ATTEMPT < DNS_TABLE_SIZE
/* need to retry at least as many times as you have */
/* table entries, or the IDs might not be unique */
#undef DNS_MAX_RAND_ATTEMPT
#define DNS_MAX_RAND_ATTEMPT DNS_TABLE_SIZE
#endif



/** DNS resource record max. TTL (one week as default) */
#ifndef DNS_MAX_TTL
#define DNS_MAX_TTL               604800
#endif

/* DNS protocol flags */
#define DNS_FLAG1_RESPONSE        0x80
#define DNS_FLAG1_OPCODE_STATUS   0x10
#define DNS_FLAG1_OPCODE_INVERSE  0x08
#define DNS_FLAG1_OPCODE_STANDARD 0x00
#define DNS_FLAG1_AUTHORATIVE     0x04
#define DNS_FLAG1_TRUNC           0x02
#define DNS_FLAG1_RD              0x01
#define DNS_FLAG2_RA              0x80
#define DNS_FLAG2_ERR_MASK        0x0f
#define DNS_FLAG2_ERR_NONE        0x00
#define DNS_FLAG2_ERR_NAME        0x03

/* DNS protocol states */
#define DNS_STATE_UNUSED          0
#define DNS_STATE_NEW             1
#define DNS_STATE_ASKING          2
#define DNS_STATE_UNANSWERED      3
#define DNS_STATE_DONE            4

#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
/** DNS message header */
struct dns_hdr {
  PACK_STRUCT_FIELD(u16_t id);
  PACK_STRUCT_FIELD(u8_t flags1);
  PACK_STRUCT_FIELD(u8_t flags2);
  PACK_STRUCT_FIELD(u16_t numquestions);
  PACK_STRUCT_FIELD(u16_t numanswers);
  PACK_STRUCT_FIELD(u16_t numauthrr);
  PACK_STRUCT_FIELD(u16_t numextrarr);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif
#define SIZEOF_DNS_HDR 12

/** DNS query message structure.
    No packing needed: only used locally on the stack. */
struct dns_query {
  /* DNS query record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t cls;
};
#define SIZEOF_DNS_QUERY 4

/** DNS answer message structure.
    No packing needed: only used locally on the stack. */
struct dns_answer {
  /* DNS answer record starts with either a domain name or a pointer
     to a name already present somewhere in the packet. */
  u16_t type;
  u16_t cls;
  u32_t ttl;
  u16_t len;
};
#define SIZEOF_DNS_ANSWER 10

/** DNS table entry */
struct dns_table_entry {
  u32_t ttl;
  u16_t queryID;
  u8_t  state;
  u8_t  numdns;
  u8_t  tmr;
  u8_t  ismulti;
  dns_found_callbackX found;
  void *arg;
  u8_t  retries;
  u8_t  seqno;
  u8_t  err;
  u8_t  numipaddrs;
  ip_addr_t ipaddrs[DNS_MAX_ADDRS_PER_NAME];
  u8_t  offset;
  char name[DNS_MAX_NAME_LENGTH];
  /* pointer to callback on DNS query done */
};

volatile  u16_t newID;

/* Bool type does not seem to be defined */
typedef int bool;

enum{
  false = 0,
  true = 1
};

#if DNS_LOCAL_HOSTLIST

#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
/** Local host-list. For hostnames in this list, no
 *  external name resolution is performed */
static struct local_hostlist_entry *local_hostlist_dynamic;
#else /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

/** Defining this allows the local_hostlist_static to be placed in a different
 * linker section (e.g. FLASH) */
#ifndef DNS_LOCAL_HOSTLIST_STORAGE_PRE
#define DNS_LOCAL_HOSTLIST_STORAGE_PRE static
#endif /* DNS_LOCAL_HOSTLIST_STORAGE_PRE */
/** Defining this allows the local_hostlist_static to be placed in a different
 * linker section (e.g. FLASH) */
#ifndef DNS_LOCAL_HOSTLIST_STORAGE_POST
#define DNS_LOCAL_HOSTLIST_STORAGE_POST
#endif /* DNS_LOCAL_HOSTLIST_STORAGE_POST */
DNS_LOCAL_HOSTLIST_STORAGE_PRE struct local_hostlist_entry local_hostlist_static[]
  DNS_LOCAL_HOSTLIST_STORAGE_POST = DNS_LOCAL_HOSTLIST_INIT;

#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */

static void dns_init_local();
#endif /* DNS_LOCAL_HOSTLIST */


/* forward declarations */
static void dns_recv(void *s, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port);
static void dns_check_entries(void);

/*-----------------------------------------------------------------------------
 * Globales
 *----------------------------------------------------------------------------*/

/* DNS variables */
static struct udp_pcb        *dns_pcb;
static u8_t                   dns_seqno;
static struct dns_table_entry dns_table[DNS_TABLE_SIZE];
static ip_addr_t              dns_servers[DNS_MAX_SERVERS];
/** Contiguous buffer for processing responses */
static u8_t                   dns_payload_buffer[LWIP_MEM_ALIGN_BUFFER(DNS_MSG_SIZE)];
static u8_t*                  dns_payload;

/**
 * Initialize the resolver: set up the UDP pcb and configure the default server
 * (DNS_SERVER_ADDRESS).
 */

void
dns_init()
{
  ip_addr_t dnsserver;

  dns_payload = (u8_t *)LWIP_MEM_ALIGN(dns_payload_buffer);
  
  /* initialize default DNS server address */
  DNS_SERVER_ADDRESS(&dnsserver);

  LWIP_DEBUGF(DNS_DEBUG, ("dns_init: initializing\n"));

  /* if dns client not yet initialized... */
  if (dns_pcb == NULL) {
    dns_pcb = udp_new();

    if (dns_pcb != NULL) {
      /* initialize DNS table not needed (initialized to zero since it is a
       * global variable) */
      LWIP_ASSERT("For implicit initialization to work, DNS_STATE_UNUSED needs to be 0",
        DNS_STATE_UNUSED == 0);

      /* initialize DNS client */
      udp_bind(dns_pcb, IP_ADDR_ANY, 0);
      udp_recv(dns_pcb, dns_recv, NULL);

      /* initialize default DNS primary server */
      dns_setserver(0, &dnsserver);
    }
  }
#if DNS_LOCAL_HOSTLIST
  dns_init_local();
#endif
}

/**
 * Initialize one of the DNS servers.
 *
 * @param numdns the index of the DNS server to set must be < DNS_MAX_SERVERS
 * @param dnsserver IP address of the DNS server to set
 */
void
dns_setserver(u8_t numdns, ip_addr_t *dnsserver)
{
  if ((numdns < DNS_MAX_SERVERS) && (dns_pcb != NULL) &&
      (dnsserver != NULL) && !ip_addr_isany(dnsserver)) {
    dns_servers[numdns] = (*dnsserver);
  }
}

/**
 * Obtain one of the currently configured DNS server.
 *
 * @param numdns the index of the DNS server
 * @return IP address of the indexed DNS server or "ip_addr_any" if the DNS
 *         server has not been configured.
 */
ip_addr_t
dns_getserver(u8_t numdns)
{
  if (numdns < DNS_MAX_SERVERS) {
    return dns_servers[numdns];
  } else {
    return *IP_ADDR_ANY;
  }
}

/**
 * The DNS resolver client timer - handle retries and timeouts and should
 * be called every DNS_TMR_INTERVAL milliseconds (every second by default).
 */
void
dns_tmr(void)
{
  if (dns_pcb != NULL) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_tmr: dns_check_entries\n"));
    dns_check_entries();
  }
}

#if DNS_LOCAL_HOSTLIST
static void
dns_init_local()
{
#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC && defined(DNS_LOCAL_HOSTLIST_INIT)
  int i;
  struct local_hostlist_entry *entry;
  /* Dynamic: copy entries from DNS_LOCAL_HOSTLIST_INIT to list */
  struct local_hostlist_entry local_hostlist_init[] = DNS_LOCAL_HOSTLIST_INIT;
  size_t namelen;
  for (i = 0; i < sizeof(local_hostlist_init) / sizeof(struct local_hostlist_entry); i++) {
    struct local_hostlist_entry *init_entry = &local_hostlist_init[i];
    LWIP_ASSERT("invalid host name (NULL)", init_entry->name != NULL);
    namelen = strlen(init_entry->name);
    LWIP_ASSERT("namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN", namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN);
    entry = (struct local_hostlist_entry *)memp_malloc(MEMP_LOCALHOSTLIST);
    LWIP_ASSERT("mem-error in dns_init_local", entry != NULL);
    if (entry != NULL) {
      entry->name = (char*)entry + sizeof(struct local_hostlist_entry);
      MEMCPY((char*)entry->name, init_entry->name, namelen);
      ((char*)entry->name)[namelen] = 0;
      entry->addr = init_entry->addr;
      entry->next = local_hostlist_dynamic;
      local_hostlist_dynamic = entry;
    }
  }
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC && defined(DNS_LOCAL_HOSTLIST_INIT) */
}

/**
 * Scans the local host-list for a hostname.
 *
 * @param hostname Hostname to look for in the local host-list
 * @return The first IP address for the hostname in the local host-list or
 *         IPADDR_NONE if not found.
 */
static u32_t
dns_lookup_local(const char *hostname)
{
#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
  struct local_hostlist_entry *entry = local_hostlist_dynamic;
  while(entry != NULL) {
    if(strcmp(entry->name, hostname) == 0) {
      return ip4_addr_get_u32(&entry->addr);
    }
    entry = entry->next;
  }
#else /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  int i;
  for (i = 0; i < sizeof(local_hostlist_static) / sizeof(struct local_hostlist_entry); i++) {
    if(strcmp(local_hostlist_static[i].name, hostname) == 0) {
      return ip4_addr_get_u32(&local_hostlist_static[i].addr);
    }
  }
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC */
  return IPADDR_NONE;
}

#if DNS_LOCAL_HOSTLIST_IS_DYNAMIC
/** Remove all entries from the local host-list for a specific hostname
 * and/or IP addess
 *
 * @param hostname hostname for which entries shall be removed from the local
 *                 host-list
 * @param addr address for which entries shall be removed from the local host-list
 * @return the number of removed entries
 */
int
dns_local_removehost(const char *hostname, const ip_addr_t *addr)
{
  int removed = 0;
  struct local_hostlist_entry *entry = local_hostlist_dynamic;
  struct local_hostlist_entry *last_entry = NULL;
  while (entry != NULL) {
    if (((hostname == NULL) || !strcmp(entry->name, hostname)) &&
        ((addr == NULL) || ip_addr_cmp(&entry->addr, addr))) {
      struct local_hostlist_entry *free_entry;
      if (last_entry != NULL) {
        last_entry->next = entry->next;
      } else {
        local_hostlist_dynamic = entry->next;
      }
      free_entry = entry;
      entry = entry->next;
      memp_free(MEMP_LOCALHOSTLIST, free_entry);
      removed++;
    } else {
      last_entry = entry;
      entry = entry->next;
    }
  }
  return removed;
}

/**
 * Add a hostname/IP address pair to the local host-list.
 * Duplicates are not checked.
 *
 * @param hostname hostname of the new entry
 * @param addr IP address of the new entry
 * @return ERR_OK if succeeded or ERR_MEM on memory error
 */
err_t
dns_local_addhost(const char *hostname, const ip_addr_t *addr)
{
  struct local_hostlist_entry *entry;
  size_t namelen;
  LWIP_ASSERT("invalid host name (NULL)", hostname != NULL);
  namelen = strlen(hostname);
  LWIP_ASSERT("namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN", namelen <= DNS_LOCAL_HOSTLIST_MAX_NAMELEN);
  entry = (struct local_hostlist_entry *)memp_malloc(MEMP_LOCALHOSTLIST);
  if (entry == NULL) {
    return ERR_MEM;
  }
  entry->name = (char*)entry + sizeof(struct local_hostlist_entry);
  MEMCPY((char*)entry->name, hostname, namelen);
  ((char*)entry->name)[namelen] = 0;
  ip_addr_copy(entry->addr, *addr);
  entry->next = local_hostlist_dynamic;
  local_hostlist_dynamic = entry;
  return ERR_OK;
}
#endif /* DNS_LOCAL_HOSTLIST_IS_DYNAMIC*/
#endif /* DNS_LOCAL_HOSTLIST */

/**
 * Look up a hostname in the array of known hostnames.
 *
 * @note This function only looks in the internal array of known
 * hostnames, it does not send out a query for the hostname if none
 * was found. The function dns_enqueue() can be used to send a query
 * for a hostname.
 *
 * @param name the hostname to look up
 * @param ipaddrs the ip_addr_t array where the found IP addresses are saved (if any)
 * @param numipaddrs maximum number of IP addresses the caller is asking for
 * @return true if at least one IP address is associated with hostname (false if none are found)
 */
static u8_t
dns_lookup(const char *name, ip_addr_t *ipaddrs, u8_t *numipaddrs)
{
  u8_t i, j;
#if DNS_LOCAL_HOSTLIST || defined(DNS_LOOKUP_LOCAL_EXTERN)
  u32_t ipaddr;
#endif /* DNS_LOCAL_HOSTLIST || defined(DNS_LOOKUP_LOCAL_EXTERN) */
#if DNS_LOCAL_HOSTLIST
  if ((ipaddr = dns_lookup_local(name)) != IPADDR_NONE) {
    ip4_addr_set_u32(ipaddrs, ipaddr);
    *numipaddrs = 1;
    return 1;
  }
#endif /* DNS_LOCAL_HOSTLIST */
#ifdef DNS_LOOKUP_LOCAL_EXTERN
  if((ipaddr = DNS_LOOKUP_LOCAL_EXTERN(name)) != IPADDR_NONE) {
    ip4_addr_set_u32(ipaddrs, ipaddr);
    *numipaddrs = 1;
    return 1;
  }
#endif /* DNS_LOOKUP_LOCAL_EXTERN */

  /* Walk through name list, return true if found. If not, return false. */
  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    if ((dns_table[i].state == DNS_STATE_DONE) &&
        (strcmp(name, dns_table[i].name) == 0)) {
      LWIP_DEBUGF(DNS_DEBUG, ("dns_lookup: \"%s\": numipaddrs = ", name));
      ip_addr_debug_print(DNS_DEBUG, &(dns_table[i].numipaddrs));
      LWIP_DEBUGF(DNS_DEBUG, ("\n"));
      dns_table[i].offset++;
      *numipaddrs = LWIP_MIN(dns_table[i].numipaddrs, *numipaddrs);
      if (*numipaddrs > 0) {
        for (j = 0; j < *numipaddrs; ++j) {
          memcpy(&ipaddrs[j], &dns_table[i].ipaddrs[(dns_table[i].offset + j) % dns_table[i].numipaddrs], sizeof(ip_addr_t));
        }
      }
      return 1;
    }
  }
  return 0;
}

#if DNS_DOES_NAME_CHECK
/**
 * Compare the "dotted" name "query" with the encoded name "response"
 * to make sure an answer from the DNS server matches the current dns_table
 * entry (otherwise, answers might arrive late for hostname not on the list
 * any more).
 *
 * @param query hostname (not encoded) from the dns_table
 * @param response encoded hostname in the DNS response
 * @return 0: names equal; 1: names differ
 */
static u8_t
dns_compare_name(unsigned char *query, unsigned char *response)
{
  unsigned char n;

  do {
    n = *response++;
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name */
      break;
    } else {
      /* Not compressed name */
      while (n > 0) {
        if ((*query) != (*response)) {
          return 1;
        }
        ++response;
        ++query;
        --n;
      };
      ++query;
    }
  } while (*response != 0);

  return 0;
}
#endif /* DNS_DOES_NAME_CHECK */

/**
 * Walk through a compact encoded DNS name and return the end of the name.
 *
 * @param query encoded DNS name in the DNS server response
 * @return end of the name
 */
static unsigned char *
dns_parse_name(unsigned char *query)
{
  unsigned char n;

  do {
    n = *query++;
    /** @see RFC 1035 - 4.1.4. Message compression */
    if ((n & 0xc0) == 0xc0) {
      /* Compressed name */
      break;
    } else {
      /* Not compressed name */
      while (n > 0) {
        ++query;
        --n;
      };
    }
  } while (*query != 0);

  return query + 1;
}

/**
 * Send a DNS query packet.
 *
 * @param numdns index of the DNS server in the dns_servers table
 * @param name hostname to query
 * @param id index of the hostname in dns_table, used as transaction ID in the
 *        DNS query packet
 * @return ERR_OK if packet is sent; an err_t indicating the problem otherwise
 */
static err_t
dns_send(u8_t numdns, const char* name, u16_t id)
{
  err_t err;
  struct dns_hdr *hdr;
  struct dns_query qry;
  struct pbuf *p;
  char *query, *nptr;
  const char *pHostname;
  u8_t n;

  LWIP_DEBUGF(DNS_DEBUG, ("dns_send: dns_servers[%"U16_F"] \"%s\": request\n",
              (u16_t)(numdns), name));
  LWIP_ASSERT("dns server out of array", numdns < DNS_MAX_SERVERS);
  LWIP_ASSERT("dns server has no IP address set", !ip_addr_isany(&dns_servers[numdns]));

  /* if here, we have either a new query or a retry on a previous query to process */
  p = pbuf_alloc(PBUF_TRANSPORT, SIZEOF_DNS_HDR + DNS_MAX_NAME_LENGTH +
                 SIZEOF_DNS_QUERY, PBUF_RAM);
  if (p != NULL) {
    LWIP_ASSERT("pbuf must be in one piece", p->next == NULL);
    /* fill dns header */
    hdr = (struct dns_hdr*)p->payload;
    memset(hdr, 0, SIZEOF_DNS_HDR);
    hdr->id = htons(id);
    hdr->flags1 = DNS_FLAG1_RD;
    hdr->numquestions = PP_HTONS(1);
    query = (char*)hdr + SIZEOF_DNS_HDR;
    pHostname = name;
    --pHostname;

    /* convert hostname into suitable query format. */
    do {
      ++pHostname;
      nptr = query;
      ++query;
      for(n = 0; *pHostname != '.' && *pHostname != 0; ++pHostname) {
        *query = *pHostname;
        ++query;
        ++n;
      }
      *nptr = n;
    } while(*pHostname != 0);
    *query++='\0';

    /* fill dns query */
    qry.type = PP_HTONS(DNS_RRTYPE_A);
    qry.cls = PP_HTONS(DNS_RRCLASS_IN);
    SMEMCPY(query, &qry, SIZEOF_DNS_QUERY);

    /* resize pbuf to the exact dns query */
    pbuf_realloc(p, (u16_t)((query + SIZEOF_DNS_QUERY) - ((char*)(p->payload))));

    /* connect to the server for faster receiving */
    udp_connect(dns_pcb, &dns_servers[numdns], DNS_SERVER_PORT);
    /* send dns packet */
    err = udp_sendto(dns_pcb, p, &dns_servers[numdns], DNS_SERVER_PORT);

    /* free pbuf */
    pbuf_free(p);
  } else {
    err = ERR_MEM;
  }

  return err;
}

/**
 * Call appropriate single or multi version of found callback.
 *
 * @param pEntry the pointer to the DNS cache entry
 * @param error used as a boolean to indicate error
 */
static void
dns_exec_found_callback(struct dns_table_entry *pEntry, err_t err) {
  if (pEntry->found) {
    if (pEntry->ismulti) {
      if (pEntry->numipaddrs == 0 || err) {
        ((dns_found_callback_multi)(*pEntry->found))(pEntry->name, NULL, 0, pEntry->arg);
      } else {
        ((dns_found_callback_multi)(*pEntry->found))(pEntry->name, pEntry->ipaddrs, pEntry->numipaddrs, pEntry->arg);
      }
    } else {
      if (pEntry->numipaddrs == 0 || err) {
        ((dns_found_callback)(*pEntry->found))(pEntry->name, NULL, pEntry->arg);
      } else {
        ((dns_found_callback)(*pEntry->found))(pEntry->name, pEntry->ipaddrs, pEntry->arg);
      }
    }
  }
}

/**
 * Generate a new ID for the query, either sequential, or random
 * depending on options.
 *
 * @param i index of the dns_table entry needing the ID
 */

static bool check_rand(u16_t r, int i)
{
	u8_t j;
    for (j = 0; j < DNS_TABLE_SIZE; ++j) {
      if( (j != i) && (dns_table[j].state != DNS_STATE_UNUSED)
         && (dns_table[j].queryID == r) )
      {
           return false;
      }
    }
	/* no clashes, its good */
	return true;
}

/**
 *
 * A few gyrations here, without this, the nested loop
 * crashes llvm. If the function pointer is not
 * volatile it still crashes llvm. It looks like the
 * branch optimiser considers local functions as 
 * fair game. The volatile breaks the link
 *
 */

/**
 * Check that a given ID is not already in use
 *
 * @param i index of the dns_table entry this will be used for
 * @param r random number to check
 */

typedef bool (*check_fn)(u16_t r, int i);
static volatile check_fn rand_check = check_rand;

static u16_t new_query_id(int i)
{
#if LWIP_DNS_RAND_ID

#if !defined(LWIP_RAND)
#error "LWIP_RAND must be specified if LWIP_DNS_RAND_ID is on"
#endif

  u8_t j;
  u32_t seq = -1, try;

  try = dns_table[i].seqno;

  for(j = 0; j<DNS_MAX_RAND_ATTEMPT; j++)
  {
    u16_t r;
    r = LWIP_RAND();
    LWIP_DEBUGF(DNS_DEBUG, ("Random ID: 0x%x\n", r));
    /* This used to be a nested loop, but it crashed llvm */
    if((*rand_check)(r, i))
	{
	    /* we found an unused ID */
		return r;
	}

	/* This relies on there being at least as many attempts as table entries */
    if((seq == -1) && (*rand_check)(try, i))
	{
	    /* we found an unused ID */
		seq = try;
	}
	try++;
  }

  LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_SEVERE,
       ("Too many clashing random IDs, using ID %d\n", seq));

  if(seq != -1)
  {
    return seq;
  }
  return try;
  /* could not find a unique random number */
  /* either bad luck, or broken rand function */
  /* use sequence number instead */
#else

  /* If not using random numbers, the  */
  /* sequence number is better than nothing */
  return dns_table[i].seqno;
#endif
}



/**
 * dns_check_entry() - see if pEntry has not yet been queried and, if so, sends out a query.
 * Check an entry in the dns_table:
 * - send out query for new entries
 * - retry old pending entries on timeout (also with different servers)
 * - remove completed entries from the table if their TTL has expired
 *
 * @param i index of the dns_table entry to check
 */
static void
dns_check_entry(u8_t i)
{
  err_t err;
  struct dns_table_entry *pEntry = &dns_table[i];


  LWIP_ASSERT("array index out of bounds", i < DNS_TABLE_SIZE);

  switch(pEntry->state) {

    case DNS_STATE_NEW: {

      /* initialize new entry */

      pEntry->state   = DNS_STATE_ASKING;
      pEntry->numdns  = 0;
      pEntry->numipaddrs = 0;
      pEntry->tmr     = 1;
      pEntry->retries = 0;

      pEntry->queryID = new_query_id(i);
      LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: new id: 0x%x\n", pEntry->queryID));
      
      /* send DNS packet for this entry */
      err = dns_send(pEntry->numdns, pEntry->name, pEntry->queryID);
      if (err != ERR_OK) {
        LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                    ("dns_send returned error: %s\n", lwip_strerr(err)));
      }
      break;
    }

    case DNS_STATE_ASKING: {
      if (--pEntry->tmr == 0) {
        if (++pEntry->retries == DNS_MAX_RETRIES) {
          if ((pEntry->numdns+1<DNS_MAX_SERVERS) && !ip_addr_isany(&dns_servers[pEntry->numdns+1])) {
            /* change of server */
            pEntry->numdns++;
            pEntry->tmr     = 1;
            pEntry->retries = 0;
            break;
          } else {
            LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": timeout\n", pEntry->name));
            /* call specified callback function if provided */
            dns_exec_found_callback(pEntry, ERR_OK);
            /* flush this entry */
            pEntry->state   = DNS_STATE_UNANSWERED;
            pEntry->found   = NULL;
            break;
          }
        }

        /* wait longer for the next retry */
        pEntry->tmr = pEntry->retries;

        /* send DNS packet for this entry */
        err = dns_send(pEntry->numdns, pEntry->name, pEntry->queryID);
        if (err != ERR_OK) {
          LWIP_DEBUGF(DNS_DEBUG | LWIP_DBG_LEVEL_WARNING,
                      ("dns_send returned error: %s\n", lwip_strerr(err)));
        }
      }
      break;
    }

    case DNS_STATE_DONE: {
      /* if the time to live is nul */
      if ((pEntry->ttl == 0) || (--pEntry->ttl == 0)) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns_check_entry: \"%s\": flush\n", pEntry->name));
        /* flush this entry */
        pEntry->state = DNS_STATE_UNUSED;
        pEntry->found = NULL;
      }
      break;
    }

    case DNS_STATE_UNANSWERED:
    case DNS_STATE_UNUSED:
      /* nothing to do */
      break;
    default:
      LWIP_ASSERT("unknown dns_table entry state:", 0);
      break;
  }
}


/**
 * Call dns_check_entry for each entry in dns_table - check all entries.
 */
static void
dns_check_entries(void)
{
  u8_t i;

  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    dns_check_entry(i);
  }
}

#if LWIP_TEST_CODE
u8_t
dns_expire_asking_entries(void)
{
  int j;
  struct dns_table_entry *pEntry;
  u8_t num_matched_entries = 0;
  for (j = 0; j < DNS_TABLE_SIZE; ++j) {
      pEntry = &dns_table[j];
      if (pEntry->state == DNS_STATE_ASKING) {
          num_matched_entries++;
          pEntry->tmr = 1;
          pEntry->retries = (DNS_MAX_RETRIES -1);
          pEntry->numdns = DNS_MAX_SERVERS;
      }
  }
  return num_matched_entries;
}

u8_t
dns_flush_cache(void)
{
  int j;
  struct dns_table_entry *pEntry;
  u8_t num_matched_entries = 0;
  for (j = 0; j < DNS_TABLE_SIZE; ++j) {
      pEntry = &dns_table[j];
      if (pEntry->state == DNS_STATE_DONE) {
          memset(pEntry, 0, sizeof(struct dns_table_entry));
          num_matched_entries++;
      }
  }
  return num_matched_entries;
}
#endif

/**
 * Receive input function for DNS response packets arriving for the dns UDP pcb.
 *
 * @params see udp.h
 */
static void
dns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *addr, u16_t port)
{
  bool was_unanswered = false, found = false;
  u16_t i, id;
  char *pHostname;
  struct dns_hdr *hdr;
  struct dns_answer ans;
  struct dns_table_entry *pEntry;
  u16_t nquestions, nanswers;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  /* is the dns message too big ? */
  if (p->tot_len > DNS_MSG_SIZE) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too big\n"));
    /* free pbuf and return */
    goto out;
  }

  /* is the dns message big enough ? */
  if (p->tot_len < (SIZEOF_DNS_HDR + SIZEOF_DNS_QUERY + SIZEOF_DNS_ANSWER)) {
    LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: pbuf too small\n"));
    /* free pbuf and return */
    goto out;
  }

  /* copy dns payload inside static buffer for processing */ 
  if (pbuf_copy_partial(p, dns_payload, p->tot_len, 0) == p->tot_len) {
    /* The ID in the DNS header should be our entry into the name table. */
    hdr = (struct dns_hdr*)dns_payload;
    id = htons(hdr->id);
    for(i = 0; i < DNS_TABLE_SIZE; i++) {
      pEntry = &dns_table[i];
      if(pEntry->queryID != id)
        continue;
      
      found = true;
      LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: found ID: 0x%x @ index: %d\n", id, i));
      if(pEntry->state == DNS_STATE_UNANSWERED) {
        LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: late answer, putting in cache\n"));
        pEntry->state = DNS_STATE_ASKING;
        was_unanswered = true;
      }
      if(pEntry->state == DNS_STATE_ASKING) {
        /* This entry is now completed. */
        pEntry->state = DNS_STATE_DONE;
        pEntry->err   = hdr->flags2 & DNS_FLAG2_ERR_MASK;

        /* We only care about the question(s) and the answers. The authrr
           and the extrarr are simply discarded. */
        nquestions = htons(hdr->numquestions);
        nanswers   = htons(hdr->numanswers);

        /* Check for error. If so, call callback to inform. */
        if (((hdr->flags1 & DNS_FLAG1_RESPONSE) == 0) || (pEntry->err != 0) || (nquestions != 1)) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": error in flags\n", pEntry->name));
          /* call callback to indicate error, clean up memory and return */
          if(!was_unanswered)
            dns_exec_found_callback(pEntry, pEntry->err ? pEntry->err : ERR_VAL);
          goto out;
        }

#if DNS_DOES_NAME_CHECK
        /* Check if the name in the "question" part match with the name in the entry. */
        if (dns_compare_name((unsigned char *)(pEntry->name), (unsigned char *)dns_payload + SIZEOF_DNS_HDR) != 0) {
          LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response not match to query\n", pEntry->name));
          /* call callback to indicate error, clean up memory and return */
          if(!was_unanswered)
            dns_exec_found_callback(pEntry, ERR_VAL);
          goto out;
        }
#endif /* DNS_DOES_NAME_CHECK */

        /* Skip the name in the "question" part */
        pHostname = (char *) dns_parse_name((unsigned char *)dns_payload + SIZEOF_DNS_HDR) + SIZEOF_DNS_QUERY;

        pEntry->ttl = DNS_MAX_TTL;
        pEntry->numipaddrs = 0;
        while (nanswers > 0 && pEntry->numipaddrs < DNS_MAX_ADDRS_PER_NAME) {
          /* skip answer resource record's host name */
          pHostname = (char *) dns_parse_name((unsigned char *)pHostname);

          /* Check for IP address type and Internet class. Others are discarded. */
          SMEMCPY(&ans, pHostname, SIZEOF_DNS_ANSWER);
          if((ans.type == PP_HTONS(DNS_RRTYPE_A)) && (ans.cls == PP_HTONS(DNS_RRCLASS_IN)) &&
             (ans.len == PP_HTONS(sizeof(ip_addr_t))) ) {
            /* read the answer resource record's TTL, and choose the smallest */
            if (pEntry->ttl > ntohl(ans.ttl)) {
                pEntry->ttl = ntohl(ans.ttl);
            }
            /* read the IP address after answer resource record's header */
            SMEMCPY(&(pEntry->ipaddrs[pEntry->numipaddrs]), (pHostname+SIZEOF_DNS_ANSWER), sizeof(ip_addr_t));
            LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: \"%s\": response = ", pEntry->name));
            ip_addr_debug_print(DNS_DEBUG, (&(pEntry->ipaddrs[pEntry->numipaddrs])));
            LWIP_DEBUGF(DNS_DEBUG, ("\n"));
            pEntry->numipaddrs++;
          }
          pHostname = pHostname + SIZEOF_DNS_ANSWER + htons(ans.len);
          --nanswers;
        }
        pEntry->offset = 0;
        /* call specified callback function if provided */
        if(!was_unanswered)
          dns_exec_found_callback(pEntry, ERR_OK);
        /* invalidate entry if the minimal TTL is zero */
        if (pEntry->ttl == 0)
        {
          pEntry->state = DNS_STATE_UNUSED;
          pEntry->found = NULL;
        }
      }
    }  // for
    if(!found)
      LWIP_DEBUGF(DNS_DEBUG, ("dns_recv: response with unknown id: 0x%x\n", id));
  } // if

out:
  /* free pbuf */
  pbuf_free(p);
  return;
}

/**
 * Determines whether a DNS entry can be recycled
 *
 * @param entry An entry object to check
 * @return a boolean representing whether the entry can be recycled.
 *        (0 == DNS entry CANNOT be recycled, 1/nonzero == DNS entry CAN be recycled)
 */
static u8_t
dns_can_recycle_entry(const struct dns_table_entry *pEntry)
{
  u8_t result;

  switch(pEntry->state) {
    case DNS_STATE_DONE:
    case DNS_STATE_UNANSWERED:
      result = true;
      break;
      
    default:
      result = false;
      break;
  }
  
  return result;
}

/**
 * Queues a new hostname to resolve and sends out a DNS query for that hostname
 *
 * @param name the hostname that is to be queried
 * @param found a callback function to be called on success, failure or timeout
 * @param callback_arg argument to pass to the callback function
 * @param ismulti boolean indicating what version of the found callback is passed
 *        (0 == dns_found_callback, 1 == dns_fond_callback_multi)
 * @return a err_t return code.
 */
static err_t
dns_enqueue(const char *name, dns_found_callbackX found, void *callback_arg, u8_t ismulti)
{
  u8_t i;
  u8_t lseq, lseqi;
  u8_t age;

  struct dns_table_entry *pEntry = NULL;
  size_t namelen;

  /* search an unused entry, or the oldest one */
  lseq = lseqi = 0;
  for (i = 0; i < DNS_TABLE_SIZE; ++i) {
    pEntry = &dns_table[i];
    /* is it an unused entry ? */
    if (pEntry->state == DNS_STATE_UNUSED)
      break;

    /* check if this is the oldest completed entry */
    if (dns_can_recycle_entry(pEntry)) {
      age = dns_seqno >= pEntry->seqno ?
              dns_seqno - pEntry->seqno :
              dns_seqno + (UINT8_MAX - pEntry->seqno);
      if (age > lseq) {
        lseq = age;
        lseqi = i;
      }
    }
  }

  /* if we have not found an unused entry, use the oldest completed one */
  if (i == DNS_TABLE_SIZE) {
    if ((lseqi >= DNS_TABLE_SIZE) || !dns_can_recycle_entry(&dns_table[lseqi])) {
      /* no entry can't be used now, table is full */
      LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": DNS entries table is full\n", name));
      return ERR_MEM;
    } else {
      /* use the oldest completed one */
      i = lseqi;
      pEntry = &dns_table[i];
    }
  }

  /* use this entry */
  LWIP_DEBUGF(DNS_DEBUG, ("dns_enqueue: \"%s\": use DNS entry %"U16_F"\n", name, (u16_t)(i)));


  /* fill the entry */
  pEntry->state = DNS_STATE_NEW;
  pEntry->seqno = dns_seqno++;
  pEntry->ismulti = ismulti;
  pEntry->found = found;
  pEntry->arg   = callback_arg;

  namelen = LWIP_MIN(strlen(name), DNS_MAX_NAME_LENGTH-1);
  MEMCPY(pEntry->name, name, namelen);
  pEntry->name[namelen] = 0;

  /* force to send query without waiting timer */
  dns_check_entry(i);

  /* dns query is enqueued */
  return ERR_INPROGRESS;
}

/**
 * Help resolve a hostname (string) into one or more IP addresses.
 *
 * Returns immediately with one of err_t return codes:
 * - ERR_OK if hostname is a valid IP address string or the host
 *   name is already in the local names table.
 * - ERR_INPROGRESS enqueue a request to be sent to the DNS server
 *   for resolution if no errors are present.
 * - ERR_ARG: dns client not initialized or invalid hostname
 *
 * @param hostname the hostname that is to be queried.
 * @param ipaddrs pointer to the ip_addr_t array where to store the addresses if they are already
 *             cached in the dns_table (only valid if ERR_OK is returned!)
 * @param numipaddrs size of the ipaddrs array.
 * @note This is both an input and an output parameter.
 * @param found a callback function to be called on success, failure or timeout (only if
 *              ERR_INPROGRESS is returned!)
 * @param callback_arg argument to pass to the callback function
 * @return a err_t return code.
 */
static err_t
dns_gethostbyname_internal(const char *hostname, ip_addr_t *ipaddrs, u8_t *numipaddrs,
                     dns_found_callbackX found, void *callback_arg, u8_t ismulti)
{
  u32_t tmpipaddr;

  /* not initialized or no valid server yet, or invalid ipaddrs pointer
   * or invalid hostname or invalid hostname length */
  if ((dns_pcb == NULL) || (ipaddrs == NULL) || (*numipaddrs == 0) ||
      (!hostname) || (!hostname[0]) ||
      (strlen(hostname) >= DNS_MAX_NAME_LENGTH)) {
    return ERR_ARG;
  }

#if LWIP_HAVE_LOOPIF
  if (strcmp(hostname, "localhost")==0) {
    ip_addr_set_loopback(ipaddrs);
    *numipaddrs = 1;
    return ERR_OK;
  }
#endif /* LWIP_HAVE_LOOPIF */

  /* host name already in octet notation? set ip addr and return ERR_OK */
  tmpipaddr = ipaddr_addr(hostname);
  if (tmpipaddr != IPADDR_NONE) {
    ip4_addr_set_u32(ipaddrs, tmpipaddr);
    *numipaddrs = 1;
    return ERR_OK;
  }
  /* already have this address cached? */
  if (dns_lookup(hostname, ipaddrs, numipaddrs)) {
    return ERR_OK;
  }

  /* queue query with specified callback */
  return dns_enqueue(hostname, found, callback_arg, ismulti);
}

/**
 * Resolve a hostname (string) into a single IP address.
 * NON-BLOCKING callback version for use with raw API!!!
 *
 * Returns immediately with one of err_t return codes:
 * - ERR_OK if hostname is a valid IP address string or the host
 *   name is already in the local names table.
 * - ERR_INPROGRESS enqueue a request to be sent to the DNS server
 *   for resolution if no errors are present.
 * - ERR_ARG: dns client not initialized or invalid hostname
 *
 * @param hostname the hostname that is to be queried
 * @param ipaddr pointer to the ip_addr_t where to store the addresses if it is already
 *             cached in the dns_table (only valid if ERR_OK is returned!)
 * @param found a callback function to be called on success, failure or timeout (only if
 *              ERR_INPROGRESS is returned!)
 * @param callback_arg argument to pass to the callback function
 * @return a err_t return code.
 */
err_t
dns_gethostbyname(const char *hostname, ip_addr_t *ipaddr,
                  dns_found_callback found, void *callback_arg)
{
    u8_t singleaddr = 1;
    return dns_gethostbyname_internal(hostname, ipaddr, &singleaddr,
                (dns_found_callbackX)found, callback_arg, 0);
}

/**
 * Resolve a hostname (string) into a set of IP addresses.
 * NON-BLOCKING callback version for use with raw API!!!
 *
 * Returns immediately with one of err_t return codes:
 * - ERR_OK if hostname is a valid IP address string or the host
 *   name is already in the local names table.
 * - ERR_INPROGRESS enqueue a request to be sent to the DNS server
 *   for resolution if no errors are present.
 * - ERR_ARG: dns client not initialized or invalid hostname
 *
 * @param hostname the hostname that is to be queried
 * @param ipaddrs pointer to the ip_addr_t array where to store the addresses if they are already
 *             cached in the dns_table (only valid if ERR_OK is returned!)
 * @param numipaddrs size of the ipaddrs array
 * @param found a callback function to be called on success, failure or timeout (only if
 *              ERR_INPROGRESS is returned!)
 * @param callback_arg argument to pass to the callback function
 * @return a err_t return code.
 */
err_t
dns_gethostbyname_multi(const char *hostname, ip_addr_t *ipaddrs, u8_t *numipaddrs,
                  dns_found_callback_multi found, void *callback_arg)
{
    return dns_gethostbyname_internal(hostname, ipaddrs, numipaddrs,
                (dns_found_callbackX)found, callback_arg, 1);
}

/**
 * Cancel one or more entries from the DNS table.
 *
 * All entries in the DNS table that match a certain callback and argument are reset.
 * This means that after a request to resolve a host name has been sent to a DNS server
 * but before the response has been received, the caller can cancel
 * the request and avoid being prompted when the response is finally received.
 * Of course this function can be used to simply move entries to DNS_STATE_UNUSED.
 *
 * @param found the callback executed in case of a cache hit.
 * @param arg argument passed to the callback.
 * @return number of entries that have been canceled.
 */
u8_t
dns_cancel(dns_found_callbackX found, void *arg)
{
  int j;
  u8_t num_matched_entries = 0;
  for (j = 0; j < DNS_TABLE_SIZE; ++j) {
    if ((dns_table[j].found == found) &&
        (dns_table[j].arg   == arg)) {
      num_matched_entries++;
      memset(&dns_table[j], 0, sizeof(struct dns_table_entry));
//      dns_table[j].found = NULL;
//      dns_table[j].arg = NULL;
    }
  }
  return num_matched_entries;
}



#endif /* LWIP_DNS */
