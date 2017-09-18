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
#    Description:
#      This file is the GNU automake template for the Weave in-package,
#      LwIP library.
#
#      LwIP lacks any build system of its own, so this actually builds
#      the library.
#

include $(abs_top_nlbuild_autotools_dir)/automake/pre.am

if WEAVE_WITH_LWIP_INTERNAL

lib_LIBRARIES                                                                  = liblwip.a

liblwip_API_PATHS                                                              = \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/api_lib.c                  \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/api_msg.c                  \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/err.c                      \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/netbuf.c                   \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/netdb.c                    \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/netifapi.c                 \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/sockets.c                  \
    @top_builddir@/third_party/lwip/repo/lwip/src/api/tcpip.c                    \
    $(NULL)

liblwip_CORE_PATHS                                                             = \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/def.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/dns.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/inet_chksum.c             \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/init.c                    \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/autoip.c             \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/icmp.c               \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/igmp.c               \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/ip4.c                \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/ip4_addr.c           \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/ip6_route_table.c    \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/dhcp6.c              \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/ethip6.c             \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/icmp6.c              \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/inet6.c              \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/ip6.c                \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/ip6_addr.c           \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/ip6_frag.c           \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/mld6.c               \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv6/nd6.c                \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/mem.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/memp.c                    \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/netif.c                   \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/pbuf.c                    \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/raw.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/stats.c                   \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/sys.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/tcp.c                     \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/tcp_in.c                  \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/tcp_out.c                 \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/udp.c                     \
    $(NULL)

liblwip_NETIF_PATHS                                                            = \
    $(NULL)

liblwip_SYS_PATHS                                                              = \
    sys_arch.c                                                                   \
    $(NULL)

liblwip_CONTRIB_PATHS                                                          = \
    TapInterface.c                                                               \
    $(NULL)

liblwip_V1ONLY_PATHS                                                           = \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/dhcp.c                    \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/ip_frag.c            \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/timers.c                  \
    @top_builddir@/third_party/lwip/repo/lwip/src/netif/etharp.c                 \
    $(NULL)

liblwip_V2ONLY_PATHS                                                           = \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/etharp.c             \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/ip4_frag.c           \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ipv4/dhcp.c               \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/ip.c                      \
    @top_builddir@/third_party/lwip/repo/lwip/src/core/timeouts.c                \
    @top_builddir@/third_party/lwip/repo/lwip/src/netif/ethernet.c               \
    $(NULL)

liblwip_VERSION_PATHS = $(liblwip_V1ONLY_PATHS)

liblwip_a_SOURCES                                                              = \
    $(liblwip_API_PATHS)                                                         \
    $(liblwip_CORE_PATHS)                                                        \
    $(liblwip_NETIF_PATHS)                                                       \
    $(liblwip_VERSION_PATHS)                                                     \
    $(liblwip_SYS_PATHS)                                                         \
    $(liblwip_CONTRIB_PATHS)                                                     \
    $(NULL)

liblwip_a_CPPFLAGS                                                             = \
    $(LWIP_CPPFLAGS)                                                             \
    $(NULL)

endif # WEAVE_WITH_LWIP_INTERNAL

EXTRA_DIST                                                                     = \
    arch                                                                         \
    lwipopts.h                                                                   \
    sys_arch.c                                                                   \
    TapInterface.c                                                               \
    TapInterface.h                                                               \
    $(NULL)

PRETTY_FILES := $(NULL)

include $(abs_top_nlbuild_autotools_dir)/automake/post.am
