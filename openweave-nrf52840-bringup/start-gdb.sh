:

GNU_INSTALL_ROOT=${GNU_INSTALL_ROOT:-${HOME}/tools/arm/gcc-arm-none-eabi-7-2018-q2-update/bin}
GDB=${GNU_INSTALL_ROOT}/arm-none-eabi-gdb

DEFAULT_APP=./build/openweave-nrf52840-bringup.out

STARTUP_CMDS=()

if [[ ! -z "$OPENTHREAD_ROOT" ]]; then
    STARTUP_CMDS+=(
        "set substitute-path /build/KNGP-TOLL1-JOB1/openthread/examples/.. ${OPENTHREAD_ROOT}"
    )
fi

STARTUP_CMDS+=(
    'target remote localhost:2331'
)

if [ $# -eq 0 ]; then
    APP=${DEFAULT_APP}
else
    APP=$1
fi

exec ${GDB} -q "${STARTUP_CMDS[@]/#/-ex=}" ${APP}
