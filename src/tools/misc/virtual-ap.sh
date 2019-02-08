#!/bin/sh

#
#    Copyright (c) 2019 Google LLC
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
#    virtual-ap.h -- Configure the local host as a virtual WiFi access point
#

# --------------------------------------------------
# Default values for user configurable options
# --------------------------------------------------

VIRT_AP_IF=${VIRT_AP_IF-wlan0}
VIRT_AP_CHANNEL=${VIRT_AP_CHANNEL-1}


# --------------------------------------------------
# Constants
# --------------------------------------------------

IP_BIN=/sbin/ip
IPTABLES_BIN=/sbin/iptables
HOSTAPD_BIN=/usr/sbin/hostapd
DNSMASQ_BIN=/usr/sbin/dnsmasq
NMCLI_BIN=/usr/bin/nmcli
TMPDIR=${TMPDIR-/tmp}
PASSPHRASE_SEED_FILE=.virtual-ap-seed
PASSPHRASE_LEN=10
IPV4_SUBNET_BASE=192.168.243
IPV4_SUBNET_MASK=255.255.255.0
HOSTAPD_DEBUG_OPTS=
DNSMASQ_DEBUG_OPTS=
TOOL_NAME=`basename $0`


# --------------------------------------------------
# Functions
# --------------------------------------------------

select_external_interface()
{
    # If an external interface has not been specified by the user...
    [ "${VIRT_AP_EXTERNAL_IF}" = "" ] && {
    
        # Use the destination interface for the first default route as the
        # external interface for traffic from the virtual AP. 
        VIRT_AP_EXTERNAL_IF=`${IP_BIN} route show default | sed -e '1!d' -e 's/.* dev \([^ ]*\).*/\1/'`

    }
}

select_ssid()
{
    # If an SSID has not been specified by the user...
    [ "${VIRT_AP_SSID}" = "" ] && {
    
        # Generate an SSID with the form 'VIRT-AP-XXXX' where XXXX is the last 4 digits of the
        # WiFi interface MAC address.
        VIRT_AP_SSID=VIRT-AP-`sed -e 's/://g' /sys/class/net/${VIRT_AP_IF}/address | tail -c 5`
    
    }
}

select_passphrase()
{
    # If passphrase has not been specified by the user...
    [ "${VIRT_AP_PASSPHRASE}" = "" ] && {
    
        # If not already done, create a file containing a random seed for generating passphrases. 
        [ ! -f ${HOME}/${PASSPHRASE_SEED_FILE} ] && {
        
            (umask 177 && cat /dev/urandom | head -c 128 > ${HOME}/${PASSPHRASE_SEED_FILE})
            
        }
    
        # Generate a passphrase from the SSID, the MAC address of the AP interface, and
        # the random seed.
        VIRT_AP_PASSPHRASE=`(echo ${VIRT_AP_SSID}; cat /sys/class/net/${VIRT_AP_IF}/address ${HOME}/${PASSPHRASE_SEED_FILE}) | openssl sha256 -binary | base64 | head -c ${PASSPHRASE_LEN}`

    }
}

error_exit()
{
    # Stop the daemons.
    stop_dnsmasq
    stop_hostapd
    
    # Remove the firewall rules.
    remove_firewall
    
    # Shutdown the AP interface.
    ap_interface_down
    
    # Remove the control directory.
    sudo rm -rf ${CONTROL_DIR}
    
    exit 1
}

config_ap_interface()
{
    echo -n "Configuring ${VIRT_AP_IF} interface ... "
    
    # If Network Manager is in use...
    [ -x ${NMCLI_BIN} ] && {
    
        # Determine the current management status of the AP interface.
        MGMT_STATUS=`${NMCLI_BIN} device status | sed -ne 's/^${VIRT_AP_IF} *[[:alnum:]]* *\([[:alnum:]]*\).*/\1/p'`
        
        # If necessary, set the interface to unmanaged.
        [ "${MGMT_STATUS}" != "unmanaged" ] && {
            ${NMCLI_BIN} device set ${VIRT_AP_IF} managed no
        }
    }

    # Bring up the AP interface.
    sudo ${IP_BIN} link set ${VIRT_AP_IF} up || {
        echo "ERROR: ip link set up failed"
        error_exit
    }
    
    # Assign the host IP address to the AP interface.
    sudo ${IP_BIN} addr replace ${HOST_IPV4}/${IPV4_SUBNET_MASK} dev ${VIRT_AP_IF} || {
        echo "ERROR: ip addr replace failed"
        error_exit
    }
    
    echo 'done'
}

ap_interface_down()
{
    echo -n "Stopping ${VIRT_AP_IF} interface ... "

    sudo ${IP_BIN} link set ${VIRT_AP_IF} down >/dev/null 2>&1
    
    echo 'done'
}

config_firewall()
{
    echo -n "Configuring firewall rules for ${VIRT_AP_IF} ... "
    
    # Remove any lingering firewall rules from a previous run of the script.
    remove_firewall silent

    # --------------------------------------------------
    # Create and populate the input rules chain. 
    # --------------------------------------------------
    
    (
        # Create the input chain.
        sudo ${IPTABLES_BIN} -N ${INPUT_CHAIN} &&
        
        # Permit inbound established connections/conversations to the local host.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -m state --state RELATED,ESTABLISHED -j ACCEPT &&
        
        # Permit inbound ICMP traffic.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p icmp -j ACCEPT &&
        
        # Permit new inbound DHCP traffic on virtual AP interface.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p udp -s 0.0.0.0/32 -d 255.255.255.255/32 --dport 67 -j ACCEPT &&
        
        # Permit new inbound DNS traffic on virtual AP interface.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p udp -s ${AP_IPV4_SUBNET} -d ${HOST_IPV4} --dport 53 -j ACCEPT &&
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p tcp -m state --state NEW -m tcp -s ${AP_IPV4_SUBNET} -d ${HOST_IPV4} --dport 53 -j ACCEPT &&
        
        # Permit new inbound Weave traffic on virtual AP interface.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p udp -s ${AP_IPV4_SUBNET} -d 192.168.243.1 --dport 11095 -j ACCEPT &&
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -p tcp -m state --state NEW -m tcp -s ${AP_IPV4_SUBNET} -d ${HOST_IPV4} --dport 11095 -j ACCEPT &&
        
        # Silently drop all other inbound traffic on virtual AP interface.
        sudo ${IPTABLES_BIN} -A ${INPUT_CHAIN} -i ${VIRT_AP_IF} -j DROP

    ) || {
        echo "ERROR: iptables failed"
        error_exit
    }

    # --------------------------------------------------
    # Create and populate the forward rules chain. 
    # --------------------------------------------------
    
    (
        # Create the forward chain.
        sudo ${IPTABLES_BIN} -N ${FORWARD_CHAIN} &&
        
        # Permit established connections/conversations forwarded through the AP host.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -m state --state RELATED,ESTABLISHED -j ACCEPT &&
        
        # Permit all forwarded ICMP traffic from AP interface.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -p icmp -j ACCEPT &&
        
        # Drop any forwarded traffic from AP interface destined to a non-public address.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -d 10.0.0.0/8 -j DROP &&
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -d 172.16.0.0/12 -j DROP &&
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -d 192.168.0.0/16 -j DROP &&
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -d 224.0.0.0/8 -j DROP &&
        
        # Permit all traffic forwarded from AP interface to the external interface.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -o ${VIRT_AP_EXTERNAL_IF} -j ACCEPT &&
        
        # Drop all other forwarded traffic from AP interface.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -i ${VIRT_AP_IF} -j DROP &&
        
        # Drop all other forwarded traffic to AP interface.
        sudo ${IPTABLES_BIN} -A ${FORWARD_CHAIN} -o ${VIRT_AP_IF} -j DROP

    ) || {
        echo "ERROR: iptables failed"
        error_exit
    }
    
    # --------------------------------------------------
    # Create and populate the NAT postrouting rules chain. 
    # --------------------------------------------------
    
    (
        # Create the NAT postrouting chain.
        sudo ${IPTABLES_BIN} -t nat -N ${POSTROUTING_CHAIN} &&
        
        # Perform source NAT on traffic from AP network exiting via the external interface
        sudo ${IPTABLES_BIN} -t nat -A ${POSTROUTING_CHAIN} -s ${AP_IPV4_SUBNET} -o ${VIRT_AP_EXTERNAL_IF} -j MASQUERADE
        
    ) || {
        echo "ERROR: iptables failed"
        error_exit
    }

    # --------------------------------------------------
    # Add jump rules to the main chains 
    # --------------------------------------------------
    
    (
        # Direct all NAT post-routing traffic on virtual AP interface through the virtual AP NAT postrouting chain.
        sudo ${IPTABLES_BIN} -t nat -I POSTROUTING 1 -j ${POSTROUTING_CHAIN} &&

        # Direct all inbound traffic on virtual AP interface through the virtual AP input chain.
        sudo ${IPTABLES_BIN} -I INPUT 1 -i ${VIRT_AP_IF} -j ${INPUT_CHAIN} &&

        # Direct all forwarded traffic on virtual AP interface through the virtual AP forward chain.
        sudo ${IPTABLES_BIN} -I FORWARD 1 -j ${FORWARD_CHAIN}

    ) || {
        echo "ERROR: iptables failed"
        error_exit
    }

    echo "done"    
}

remove_firewall()
{
    [ "$1" = "silent" ] || echo -n "Removing firewall rules for ${VIRT_AP_IF} ... " 

    # Remove the interface-specific jump rules.
    sudo ${IPTABLES_BIN} -D INPUT -i ${VIRT_AP_IF} -j ${INPUT_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -D FORWARD -j ${FORWARD_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -t nat -D POSTROUTING -j ${POSTROUTING_CHAIN} > /dev/null 2>&1
    
    # Remove the interface-specific chains.
    sudo ${IPTABLES_BIN} -F ${INPUT_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -X ${INPUT_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -F ${FORWARD_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -X ${FORWARD_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -t nat -F ${POSTROUTING_CHAIN} > /dev/null 2>&1
    sudo ${IPTABLES_BIN} -t nat -X ${POSTROUTING_CHAIN} > /dev/null 2>&1

    [ "$1" = "silent" ] || echo "done" 
}

start_hostapd()
{
    echo -n "Starting hostapd for ${VIRT_AP_IF} ... "

    # Construct hostapd config file
    (umask 177 && cat > ${CONTROL_DIR}/hostapd.conf) <<END

interface=${VIRT_AP_IF}
ssid=${VIRT_AP_SSID}
hw_mode=g
wpa=2
wpa_passphrase=${VIRT_AP_PASSPHRASE}
wpa_key_mgmt=WPA-PSK WPA-PSK-SHA256
country_code=US
channel=${VIRT_AP_CHANNEL}

END

    # Create the hostapd control directory with restricted permissions.
    (umask 077 && mkdir ${CONTROL_DIR}/hostapd.control)
    
    # Start hostapd daemon.
    sudo ${HOSTAPD_BIN} -B -P ${CONTROL_DIR}/hostapd.pid -f ${CONTROL_DIR}/hostapd.log -g ${CONTROL_DIR}/hostapd.control/${VIRT_AP_IF} ${HOSTAPD_DEBUG_OPTS} ${CONTROL_DIR}/hostapd.conf || {
        echo 'ERROR: hostapd failed.'
        [ -f ${CONTROL_DIR}/hostapd.log ] && cat ${CONTROL_DIR}/hostapd.log
        error_exit
        exit 1
    }
    
    # Give the user access to the hostapd control and log files
    sudo chown $USER ${CONTROL_DIR}/hostapd.pid ${CONTROL_DIR}/hostapd.log ${CONTROL_DIR}/hostapd.control/${VIRT_AP_IF}
    
    echo 'done'
}

stop_hostapd()
{
    echo -n "Stopping hostapd for ${VIRT_AP_IF} ... "

    [ -f ${CONTROL_DIR}/hostapd.pid ] && sudo kill -TERM `cat ${CONTROL_DIR}/hostapd.pid` >/dev/null 2>&1
    
    echo 'done'
}

start_dnsmasq()
{
    echo -n "Starting dnsmasq for ${VIRT_AP_IF} ... "

    # Construct dnsmasq config file
    (umask 177 && cat > ${CONTROL_DIR}/dnsmasq.conf) <<END

# Bind only to the host IP address on the AP interface
listen-address=${HOST_IPV4}
bind-interfaces

# Don't read upstream nameservers from /etc/resolv.conf.
no-resolv

# Don't read hosts from /etc/hosts
no-hosts

# Enable DHCP on AP network 
dhcp-range=${DHCP_RANGE_START},${DHCP_RANGE_END},${IPV4_SUBNET_MASK},24h

# Answer queries using Google DNS servers by default.
server=8.8.8.8
server=8.8.4.4

END

    # Start dnsmasq daemon.
    sudo ${DNSMASQ_BIN} -C ${CONTROL_DIR}/dnsmasq.conf -x ${CONTROL_DIR}/dnsmasq.pid -l ${CONTROL_DIR}/dnsmasq.leases -8 ${CONTROL_DIR}/dnsmasq.log ${DNSMASQ_DEBUG_OPTS} || {
        echo 'ERROR: dnsmasq failed.'
        [ -f ${CONTROL_DIR}/dnsmasq.log ] && cat ${CONTROL_DIR}/dnsmasq.log
        error_exit
        exit 1
    }

    # Give the user access to the dnsmasq control and log files.
    sudo chown $USER ${CONTROL_DIR}/dnsmasq.pid ${CONTROL_DIR}/dnsmasq.leases ${CONTROL_DIR}/dnsmasq.log

    echo 'done'
}

stop_dnsmasq()
{
    echo -n "Stopping dnsmasq for ${VIRT_AP_IF} ... "

    [ -f ${CONTROL_DIR}/dnsmasq.pid ] && sudo kill -TERM `cat ${CONTROL_DIR}/dnsmasq.pid` >/dev/null 2>&1
    
    echo 'done'
}

enable_ip_forwarding()
{
    [ `cat /proc/sys/net/ipv4/ip_forward` -eq 1 ] || {
        echo -n "Enabling IPv4 forwarding ... "
        echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward >/dev/null 
        echo 'done'
    }
}

virtual_ap_up()
{
    # Prompt for sudo password
    sudo true || exit 1
    
    [ -d /sys/class/net/${VIRT_AP_IF} ] || {
        echo "ERROR: WiFi device ${VIRT_AP_IF} not found"
        exit 1 
    }
    
    # Check if already running.
    [ -d ${CONTROL_DIR} ] && {
        echo "Virtual AP already running on ${VIRT_AP_IF}"
        echo "Use '$0 down' to stop"
        exit 1 
    }

    # Select an SSID if not specified by the user.
    select_ssid
    
    # Select a passphrase if not specified by the user
    select_passphrase
    
    # Select the interface through which outbound traffic from the virtual AP
    # will be directed.
    select_external_interface

    # Make control directory.
    (umask 077 && mkdir ${CONTROL_DIR}) || {
        echo "ERROR: Failed to create control directory at ${CONTROL_DIR}"
        exit 1
    }
    
    # Configure the AP interface.
    config_ap_interface
    
    # Configure firewall rules.
    config_firewall
    
    # Start hostapd
    start_hostapd
    
    # Start dnsmasq
    start_dnsmasq
    
    # Enable IP forwarding
    enable_ip_forwarding
    
    echo ""
    echo "Virtual AP enabled on ${VIRT_AP_IF}"
    echo "    SSID: ${VIRT_AP_SSID}"
    echo "    Passphrase: ${VIRT_AP_PASSPHRASE}"
    echo "    WiFi Channel: ${VIRT_AP_CHANNEL}"
}

virtual_ap_down()
{
    # Prompt for sudo password
    sudo true || exit 1

    # Stop the daemons.
    stop_dnsmasq
    stop_hostapd
    
    # Remove the firewall rules.
    remove_firewall
    
    # Shutdown the AP interface.
    ap_interface_down
    
    # Remove the control directory.
    sudo rm -rf ${CONTROL_DIR}
}

display_help()
{
    select_external_interface

    cat <<END
${TOOL_NAME}: Configure the local host as a virtual WiFi access point

Usage:
    ${TOOL_NAME} [options] <command>
    
Commands:
    up                  Bring up a virtual WiFi access point
    down                Shutdown the virtual WiFi access point
    help                Show this help message.

Options:

    --ssid <ssid>       The SSID for the virtual access point.  If not
                        specified, an SSID of 'VIRT-AP-XXXX' will be used,
                        where XXXX is derived from the MAC address of the
                        WiFi interface.
    
    --channel <chan>    The WiFi channel for the virtual access point;
                        defaults to ${VIRT_AP_CHANNEL}.

    --passphrase <pass> The passphrase for the virtual access point. If not
                        specified, a random passphrase will be generated.

    --interface <if>    The WiFi interface to use for the virtual access
                        point; defaults to ${VIRT_AP_IF}.

    --external <if>     The network interface through which outbound traffic
                        from the virtual access point will be directed;
                        defaults to ${VIRT_AP_EXTERNAL_IF}.

END
}

no_op()
{
    cat <<END
${TOOL_NAME}: Configure the local host as a virtual WiFi access point
Type '${TOOL_NAME} help' for usage.
END
}


# --------------------------------------------------
# Main Code
# --------------------------------------------------

# Default operation
OP=no_op

# Parse command line arguments
while [ $# -gt 0 ]; do

    OPT=$1
    shift
    
    case ${OPT} in
    
        up)
            OP=virtual_ap_up
            ;;
            
        down)
            OP=virtual_ap_down
            ;;
            
        --ssid)
            VIRT_AP_SSID=$1
            shift
            ;;
            
        --channel)
            VIRT_AP_CHANNEL=$1
            shift
            ;;
            
        --interface)
            VIRT_AP_IF=$1
            shift
            ;;
            
        --passphrase)
            VIRT_AP_PASSPHRASE=$1
            shift
            ;;

        --external)
            VIRT_AP_EXTERNAL_IF=$1
            shift
            ;;
            
        -h|--help|help)
            OP=display_help
            ;;
            
        *)
            echo "$0: Unknown option: ${OPT}"
            exit 1
            ;;    
    esac

done

# Setup derived variables
CONTROL_DIR=${TMPDIR}/virtual-ap-${VIRT_AP_IF}
AP_IPV4_SUBNET=${IPV4_SUBNET_BASE}.0/${IPV4_SUBNET_MASK}
HOST_IPV4=${IPV4_SUBNET_BASE}.1
INPUT_CHAIN=vap-in-${VIRT_AP_IF}
FORWARD_CHAIN=vap-fw-${VIRT_AP_IF}
POSTROUTING_CHAIN=vap-pr-${VIRT_AP_IF}
DHCP_RANGE_START=${IPV4_SUBNET_BASE}.100
DHCP_RANGE_END=${IPV4_SUBNET_BASE}.199

# Invoked the requested operation
${OP}
