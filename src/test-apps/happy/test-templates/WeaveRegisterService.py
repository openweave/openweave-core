#!/usr/bin/env python


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

#
#    @file
#       Implements generate register service cmd which is used to service provisioning

import json
import os
import requests
import shutil
import sys
import tarfile

import grpc
from grpc.framework.interfaces.face.face import ExpirationError
from httplib import HTTPSConnection
from happy.Driver import Driver
from happy.Utils import *

options = {}
options["tier"] = None
options["username"] = None
options["password"] = None

def option():
    return options.copy()


default_schema_branch = 'develop'
apigw_fmt = 'apigw01.weave01.iad02.{tier}.nestlabs.com'
apigw_siac_fmt = '{siac_name}.unstable.nestlabs.com'

phoenix_mirror_url = 'http://device-automation.nestlabs.com/phoenix-schema/python'

python_local = os.path.expanduser('~/.python-local')
phoenix_cache = os.path.join(python_local, 'phoenix')


def get_phoenix_hash(branch):
    url = '{}/{}/__latest__'.format(phoenix_mirror_url, branch)
    response = requests.get(url)
    response.raise_for_status()
    return response.text.rstrip()

_phoenix_proto_cache = dict()

def get_phoenix_proto_dir(branch):
    if branch in _phoenix_proto_cache:
        return _phoenix_proto_cache.get(branch)

    latest_hash = get_phoenix_hash(branch)
    tarball_dir = os.path.join(phoenix_cache, branch)
    tarball_path = os.path.join(tarball_dir, '{}.tar.gz'.format(latest_hash))
    extract_dir = os.path.join(tarball_dir, latest_hash)
    phoenix_dir = os.path.join(extract_dir, 'phoenix')
    if not os.path.exists(extract_dir):
        os.makedirs(extract_dir)

    if not os.path.exists(tarball_path):
        url = '{}/{}/{}.tar.gz'.format(phoenix_mirror_url, branch, latest_hash)
        response = requests.get(url, stream=True)
        response.raise_for_status()
        with open(tarball_path, 'wb') as tarball_file:
            print type(response.raw)
            shutil.copyfileobj(response.raw, tarball_file)

    if not os.path.exists(phoenix_dir):
        with tarfile.open(tarball_path) as tar:
            for member in tar.getmembers():
                if not member.isdir():
                    tar.extract(member.path, path=extract_dir)

    _phoenix_proto_cache[branch] = phoenix_dir
    return phoenix_dir

def use_phoenix_schema(branch):
    sys.path.insert(0, get_phoenix_proto_dir(branch))

class ServiceClient(object):
    def __init__(self, tier, username, password, token,
                 schema_branch=default_schema_branch):
        self.tier = tier
        self.username = username
        self.password = password

        use_phoenix_schema(schema_branch)
        import nestlabs.gateway.v2.gateway_api_pb2 as gateway_api_pb2
        import nestlabs.gateway.v2.gateway_api_pb2_grpc as gateway_api_pb2_grpc
        self.gateway_api_pb2 = gateway_api_pb2
        self.gateway_api_pb2_grpc = gateway_api_pb2_grpc

        auth_token = 'Basic ' + token
        self._auth_metadata = [('authorization', auth_token)]

    def _create_gateway_service_stub(self):
        gateway_api_pb2_grpc = self.gateway_api_pb2_grpc

        if ".unstable" in self.tier:
            siac_name = self.tier.split('.')[0]
            apigw = apigw_siac_fmt.format(siac_name=siac_name)
        else:
            apigw = apigw_fmt.format(tier=self.tier)

        return \
            gateway_api_pb2_grpc.GatewayServiceStub(
                grpc.insecure_channel('{}:9953'.format(apigw)))

    @property
    def account_id(self):
        gateway_api_pb2 = self.gateway_api_pb2
        request = gateway_api_pb2.ObserveRequest()
        request.state_types.append(gateway_api_pb2.StateType.Value('ACCEPTED'))
        request.state_types.append(gateway_api_pb2.StateType.Value('CONFIRMED'))

        stub = self._create_gateway_service_stub()
        for response in stub.Observe(request, 999999, self._auth_metadata):
            for resource_meta in response.resource_metas:
                if 'USER' in resource_meta.resource_id:
                    return resource_meta.resource_id.encode('utf-8')
            if not response.initial_resource_metas_continue:
                break

    @property
    def structure_ids(self):
        gateway_api_pb2 = self.gateway_api_pb2
        request = gateway_api_pb2.ObserveRequest()
        request.state_types.append(gateway_api_pb2.StateType.Value('ACCEPTED'))
        request.state_types.append(gateway_api_pb2.StateType.Value('CONFIRMED'))

        stub = self._create_gateway_service_stub()

        ids = []
        try:
            for response in stub.Observe(request, 15, self._auth_metadata):
                for resource_meta in response.resource_metas:
                    if 'STRUCTURE' in resource_meta.resource_id:
                        ids.append(resource_meta.resource_id.encode('utf-8'))
                if not response.initial_resource_metas_continue:
                    break
        except ExpirationError:
            pass
        finally:
            return ids

class WeaveRegisterService(Driver):
    """
    weave-register-service [-h --help] [-q --quiet] [-t --tier <NAME>] [-u --username <NAME>] [-p --password <password>]

    --tier option is the service tier

    command to generate options of register-service cmd:
        $ weave-register-serivce -t integration -u username@nestlabs.com -p yourpassword

    return:
        options of the options of register service command

    """
    def __init__(self, opts = options):
        Driver.__init__(self)
        self.tier = opts["tier"]
        self.username = opts["username"]
        self.password = opts["password"]
        self.headers = {'Content-Type' : 'application/json'}

    def __pre_check(self):
        if not self.tier:
            self.tier = "integration"
            emsg = "WeaveRegisterService: Using default weave_service_tier %s." % (self.tier)
            self.logger.debug(emsg)

        self.host = 'home.%s.nestlabs.com' % self.tier

        # Siac tiers contain 'unstable' and don't expose a 'home.*' hostname.
        if 'unstable' in self.tier:
            self.host = self.host.replace('home.','')

        if not self.username:
            self.username = "test-it+pairing1@nestlabs.com"
            emsg = "WeaveRegisterService: using default weave_service_username %s." % (self.username)
            self.logger.debug(emsg)

        if not self.password:
            # Check if service password is set
            self.password = "nest-egg"
            emsg = "WeaveRegisterService: using default weave_service_password %s." % (self.password)
            self.logger.debug(emsg)

        self.params = json.dumps({'email': self.username, 'username': self.username, 'password':self.password})

        self.access_token, self.user_id = self.get_cz_token_userid()
        self.sessionJSON = self.__get_session_json()
        client = ServiceClient(tier=self.tier, username=self.username, password=self.password, token=self.access_token)
        self.structureids = client.structure_ids
        self.accountid = client.account_id
        self.initial_data = self.__get_initial_data_json()

    def get_cz_token_userid(self):
        conn = HTTPSConnection(self.host)
        path = '/api/0.2/create_user_login'
        conn.request('POST', path, self.params, headers=self.headers)
        login_response = conn.getresponse()
        login_response_data = json.load(login_response)
        if login_response.status == 201:
            delayExecution(1)
            self.logger.info("create account for user %s" % self.username)
            token = login_response_data['access_token']
            user_id = login_response_data['user']
        else:
            self.logger.info("get auth info for user %s" % self.username)
            token, user_id = self.__get_account_auth()
        return token, user_id


    def __get_account_auth(self):
        conn = HTTPSConnection(self.host)
        path = '/api/0.1/authenticate_user'
        conn.request('POST', path, self.params, headers=self.headers)
        auth_response = conn.getresponse()
        auth_response_data = json.load(auth_response)
        if auth_response.status == 200:
            self.logger.info("WeaveRegisterService: Authentication successful")
        elif auth_response.status== 400:
            emsg = "WeaveRegisterService: Unauthorized request for user authentication: status=%s error=%s" % (
                auth_response.status, auth_response.reason)
            self.logger.info(emsg)
            raise ValueError(emsg)
        else:
            # Not a 200 or 4xx auth error, server error.
            emsg = "WeaveRegisterService: Service Error on user authentication: HTTPS %s: %s. " % (
            auth_response.status, auth_response.reason)
            self.logger.info(emsg)
            raise ValueError(emsg)
        token = auth_response_data['access_token']
        user_id = auth_response_data['user']
        return token, user_id

    def __get_session_json(self):
        conn = HTTPSConnection(self.host)
        path = '/session'
        conn.request('POST', path, self.params, headers=self.headers)
        response = conn.getresponse()
        data = response.read()
        if response.status != 200 and response.status != 201:
            emsg = 'WeaveRegisterService: Failed with status %d: %s.  Password and login correct?' % (response.status, response.reason)
            self.logger.info(emsg)
            raise ValueError(emsg)
        return json.loads(data)

    def __get_initial_data_json(self):
        where_id = '00000000-0000-0000-0000-000100000010'
        spoken_where_id = '00000000-0000-0000-0000-000100000010'
        initialDataJSON = {'structure_id': self.structureids[0], 'where_id': where_id, 'spoken_where_id': spoken_where_id}
        return initialDataJSON

    def run(self):
        self.logger.debug("[localhost] WeaveRegisterService: Run.")

        self.__pre_check()

        self.cmd = ' --account-id %s --pairing-token %s --service-config %s --init-data \'%s\'' % (self.accountid, self.sessionJSON['weave']['pairing_token'], self.sessionJSON['weave']['service_config'], json.dumps(self.initial_data).encode("UTF-8"))

        print "Weave Access Token:"

        print hgreen(self.sessionJSON['weave']['access_token'])

        print "weave-register-service generated the service registration command:"

        print hgreen("register-service %s\n" % self.cmd)

        self.logger.debug("[localhost] WeaveRegisterService: Done.")

        return self.cmd
