#!/bin/sh


#
#    Copyright (c) 2015-2017 Nest Labs, Inc.
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

# Set this value to 1 if you configured weave-bdx-common.cpp to use curl for downloading
# the requested URI when handling a ReceiveInit
use_curl_to_download_requested_file=0

if [ -z ${srcdir}]; then
    srcdir=`pwd`
fi
if [ -z ${builddir}]; then
    builddir="$srcdir"
fi

client_program="${builddir}/weave-bdx-client"
server_program="${builddir}/weave-bdx-server"

test_file_name="test-file-development.txt"
test_file="${srcdir}/${test_file_name}"
rcvd_dir="/tmp"
sent_file="${rcvd_dir}/${test_file_name}"
rcvd_file="${rcvd_dir}/${test_file_name}.2"

# Proactively remove the received files and set up the directory
mkdir -p /tmp
rm $sent_file 2> /dev/null
rm $rcvd_file 2> /dev/null

# Start up the server in the background, suppressing its output for readability
server_cmd="${server_program} 127.0.0.1"
echo $server_cmd
${server_cmd} & >/dev/null
sleep 1 # give server a chance to start

# send the file from the client
client_send_cmd="${client_program} 1@127.0.0.1 -o ${test_file_name} --upload"
echo $client_send_cmd
${client_send_cmd} 
echo "Client finished running"
echo ""

# check if the file was sent correctly from client
diff $test_file $sent_file > /dev/null
result=${?}
if [ ${result} -eq 0 ]; then
    echo "Client sent file successfully!"
else
    echo "Client failed file send, error code=${result}"
    # kill server
    kill -9 $!
    exit ${result}
fi
echo ""
sleep 1

# If the test was successful, run the client in the other direction and verify
if [ $use_curl_to_download_requested_file -eq 1 ]; then
client_recv_cmd="${client_program} 1@127.0.0.1 -o file://${sent_file}"
else
client_recv_cmd="${client_program} 1@127.0.0.1 -o ${sent_file}"
fi

echo $client_recv_cmd
${client_recv_cmd} 
echo "Client finished running"
echo ""

# check if the file was sent correctly from client
diff $test_file $rcvd_file > /dev/null
result=${?}
if [ ${result} -eq 0 ]; then
    echo "Client received file successfully!"
else
    echo "Client failed file receive, error code=${result}"
fi

# kill server
kill -9 $!
exit ${result}

