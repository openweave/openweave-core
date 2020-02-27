#!/usr/bin/env python

#    Copyright (c) 2020 Google, LLC.
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
#    Note: "openweave-core/src/test-apps/happy/test-templates/generated"
#    needs to be added to PYTHONPATH


from __future__ import print_function

import future

import json
import grpc
from grpc.framework.interfaces.face.face import ExpirationError
import time
import nestlabs.gateway.v2.gateway_api_pb2 as gateway_api_pb2
import nestlabs.gateway.v2.gateway_api_pb2_grpc as gateway_api_pb2_grpc

from http.client import HTTPSConnection
from future.moves.urllib.parse import unquote

options = {}
options["tier"] = None
options["username"] = None
options["password"] = None


def option():
    return options.copy()

apigw_fmt = 'apigw.{tier}.nestlabs.com'
apigw_siac_fmt = '{siac_name}.unstable.nestlabs.com'


class ServiceClient(object):
    """
        Gets basic account information from Service
        Args:
            tier (str): tier of the service
            username (str): Account email/username
            password (str): Account password
            token (str): Account access token
    """

    def __init__(self, tier, username, password, token):
        self.tier = tier
        self.username = username
        self.password = password

        self.gateway_api_pb2 = gateway_api_pb2
        self.gateway_api_pb2_grpc = gateway_api_pb2_grpc

        auth_token = 'Basic ' + token
        self._auth_metadata = [('authorization', auth_token)]

    def _create_gateway_service_stub(self):
        gateway_api_pb2_grpc = self.gateway_api_pb2_grpc

        if ".unstable" in self.tier:
            siac_name = self.tier.split('.')[0]
            apigw = apigw_siac_fmt.format(siac_name=siac_name)
            channel = grpc.insecure_channel('{}:9953'.format(apigw))
        else:
            apigw = apigw_fmt.format(tier=self.tier)
            port = 443
            channel_credentials = grpc.ssl_channel_credentials(None, None, None)
            channel = grpc.secure_channel('{}:{}'.format(apigw,port), channel_credentials)
        return \
            gateway_api_pb2_grpc.GatewayServiceStub(channel)

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


class ServiceAccountManager(object):
    """
    weave-register-service [-h --help] [-q --quiet] [-t --tier <NAME>] [-u --username <NAME>] [-p --password <password>]

    --tier option is the service tier

    command to generate options of register-service cmd:
        $ weave-register-serivce -t integration -u username@nestlabs.com -p yourpassword

    return:
        options of the options of register service command

    """

    def __init__(self, logger_obj, opts=options):
        """
            Initializes the ServiceAccountManager class.

            Args:
            logger_obj (logging.Logger): logger object to be used for logging.
            opts (dict): Dictionary which contains tier, username and password.
        """
        self.tier = opts["tier"]
        self.username = opts["username"]
        self.password = opts["password"]
        self.headers = {'Content-Type': 'application/json'}
        self.logger = logger_obj

    def __pre_check(self):
        if not self.tier:
            self.tier = "integration"
            emsg = "ServiceAccountManager: Using default weave_service_tier %s." % (self.tier)
            self.logger.debug(emsg)

        self.host = 'home.%s.nestlabs.com' % self.tier

        # Siac tiers contain 'unstable' and don't expose a 'home.*' hostname.
        if 'unstable' in self.tier:
            self.host = self.host.replace('home.', '')

        if not self.username:
            self.username = "test-it+pairing1@nestlabs.com"
            emsg = "ServiceAccountManager: using default weave_service_username %s." % (
                self.username)
            self.logger.debug(emsg)

        if not self.password:
            # Check if service password is set
            self.password = "nest-egg123"
            emsg = "ServiceAccountManager: using default weave_service_password %s." % (
                self.password)
            self.logger.debug(emsg)

        self.params = json.dumps(
            {'email': self.username, 'username': self.username, 'password': self.password})

        self.access_token, self.user_id = self.get_cz_token_userid()
        self.sessionJSON = self.__get_session_json()
        client = ServiceClient(
            tier=self.tier,
            username=self.username,
            password=self.password,
            token=self.access_token)
        self.structureids = client.structure_ids
        self.accountid = client.account_id
        self.initial_data = self.__get_initial_data_json()

    def get_cz_token_userid(self):
        conn = HTTPSConnection(self.host)
        path = '/api/0.2/create_user_login'
        conn.request('POST', path, self.params, headers=self.headers)
        login_response = conn.getresponse()
        login_response_data = json.load(login_response)
        # delay execution
        time.sleep(1)
        if login_response.status == 201:
            # delay execution
            time.sleep(1)
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
            self.logger.info("ServiceAccountManager: Authentication successful")
        elif auth_response.status == 400:
            emsg = "ServiceAccountManager: Unauthorized request for user authentication: status=%s error=%s" % (
                auth_response.status, auth_response.reason)
            self.logger.info(emsg)
            raise ValueError(emsg)
        else:
            # Not a 200 or 4xx auth error, server error.
            emsg = "ServiceAccountManager: Service Error on user authentication: HTTPS %s: %s. " % (
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
            emsg = 'ServiceAccountManager: Failed with status %d: %s.  Password and login correct?' % (
                response.status, response.reason)
            self.logger.info(emsg)
            raise ValueError(emsg)
        return json.loads(data)

    def __get_initial_data_json(self):
        where_id = '00000000-0000-0000-0000-000100000010'
        spoken_where_id = '00000000-0000-0000-0000-000100000010'
        initialDataJSON = {
            'structure_id': self.structureids[0].decode("UTF-8"),
            'where_id': where_id,
            'spoken_where_id': spoken_where_id}
        return initialDataJSON

    def run(self):
        self.logger.debug("[localhost] ServiceAccountManager: Run.")

        self.__pre_check()

        self.cmd = ' --account-id %s --pairing-token %s --service-config %s --init-data \'%s\'' % (self.accountid, self.sessionJSON[
                                                                                                   'weave']['pairing_token'], self.sessionJSON['weave']['service_config'], json.dumps(self.initial_data).encode("UTF-8"))

        print("Weave Access Token:")

        print(self.sessionJSON['weave']['access_token'])

        print("weave-register-service generated the service registration command:")

        print("register-service %s\n" % self.cmd)

        self.logger.debug("[localhost] ServiceAccountManager: Done.")

        return self.cmd
