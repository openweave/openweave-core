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

set -x

ID=$1
NAME=$2
WEAVE_CMD=${WEAVE_HOME}/output/x86_64-unknown-linux-gnu/bin/weave

mkdir ${NAME}

cd ${NAME}

openssl ecparam -name secp224r1 -genkey -out nest-weave-mock-${NAME}-endpoint-key.pem

${WEAVE_CMD} gen-service-endpoint-cert \
--id ${ID} \
--lifetime 3654 \
--sha256 \
--key nest-weave-mock-${NAME}-endpoint-key.pem \
--ca-cert ~/projects/weave/certs/mock/service-endpoint-ca/nest-weave-mock-service-endpoint-ca-cert.pem \
--ca-key ~/projects/weave/certs/mock/service-endpoint-ca/nest-weave-mock-service-endpoint-ca-key.pem \
--out nest-weave-mock-${NAME}-endpoint-cert.pem

${WEAVE_CMD} convert-cert --weave nest-weave-mock-${NAME}-endpoint-cert.pem nest-weave-mock-${NAME}-endpoint-cert.weave

${WEAVE_CMD} convert-key --weave nest-weave-mock-${NAME}-endpoint-key.pem nest-weave-mock-${NAME}-endpoint-key.weave
