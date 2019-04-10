:

JLINK_GDB_SERVER=JLinkGDBServerCLExe
DEVICE_TYPE=NRF52840_XXAA

# Launch JLink GDB Server in background; redirect output thru sed to add prefix.
${JLINK_GDB_SERVER} -device ${DEVICE_TYPE} -if SWD -speed 4000 -rtos GDBServer/RTOSPlugin_FreeRTOS $* > >(exec sed -e 's/^/JLinkGDBServer: /') 2>&1 &
GDB_SERVER_PID=$!

# Repeatedly open a telnet connection to the GDB server's RTT port until
# the user kills the GDB server with an interrupt character.
while true;
do
    # Wait for GDB server to begin listening on RTT port
    while ! (cat /proc/net/tcp | egrep -q '^\s+[[:digit:]]+:\s+0100007F:4A4D\s+00000000:0000\s+0A'); 
    do
        # Quit if the GDB server exits.
        if ! (kill -0 ${GDB_SERVER_PID} >/dev/null 2>&1); then
            echo ""
            exit
        fi

        # Wait a bit.
        sleep 0.1s

    done

    # Telnet to RTT port.
    telnet localhost 19021

done

