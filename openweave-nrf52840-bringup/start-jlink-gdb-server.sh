:

JLINK_GDB_SERVER=JLinkGDBServerCLExe
DEVICE_TYPE=NRF52840_XXAA

# Launch JLink GDB Server in background; redirect output thru sed to add prefix.
${JLINK_GDB_SERVER} -device ${DEVICE_TYPE} -if SWD -speed 4000 $* > >(exec sed -e 's/^/JLinkGDBServer: /') 2>&1 &
GDB_SERVER_PID=$!

# Wait for GDB server to begin listening on RTT port
while ! (cat /proc/net/tcp | egrep -q '^\s+[[:digit:]]+:\s+0100007F:4A4D'); 
do
    # Fail if the GDB server exits unexpectedly.
    if ! (kill -0 ${GDB_SERVER_PID} >/dev/null 2>&1); then
        echo ""
        exit
    fi

    # Wait a bit.
    sleep 0.1s
done

# Telnet to RTT port; wait for user to close telnet session.
telnet localhost 19021

# Signal GDB server to shutdown
kill -INT ${GDB_SERVER_PID}

# Wait for GDB server to shutdown
while (kill -0 ${GDB_SERVER_PID} >/dev/null 2>&1); 
do
    sleep 0.1s
done

