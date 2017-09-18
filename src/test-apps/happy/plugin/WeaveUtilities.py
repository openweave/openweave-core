#!/usr/bin/env python


#
#    Copyright (c) 2016-2017 Nest Labs, Inc.
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
#       Implements utilities to parse standard content output by the
#       Weave standalone test clients and servers (resource stats, leak
#       detection, fault-injection, etc)
#

import sys
import re
import pprint
import traceback
import unittest

import happy.HappyStateDelete
import plugins.plaid.Plaid as Plaid


from happy.Utils import *

def scan_for_leaks_and_parser_errors(test_output):
    parser_error = False
    leak_detected = False

    leak_parser_result = scan_for_resource_leak(test_output)

    if (leak_parser_result == None):
        parser_error = True
    else:
        leak_detected = leak_parser_result[0]

    for line in test_output.split("\n"):
        if "Invalid string specified for fault injection" in line:
            parser_error = True

    return parser_error, leak_detected


def parse_counter(line):
    match = re.match(r'(.*):[ \t]*(-*\d*)', line.strip())
    return [match.group(1), match.group(2)]

def parse_counters(test_output, header_string, footer_string):
    counters = {}

    lines = test_output.splitlines()

    parsing = False
    leak_detected = False

    for line in lines:
        if (header_string in line):
            parsing = True
            continue

        if (footer_string in line):
            break

        if parsing:
            name, value = parse_counter(line)
            counters[name] = int(value)

    return counters


def scan_for_resource_leak(test_output):

    leak_detected = False
    resource_leak_line_found = False
    for line in test_output.splitlines():
        match = re.match(r'Resource leak .*detected', line.strip())
        if (match != None):
            resource_leak_line_found = True
            leak_detected = True
            if ('not detected' in line):
                leak_detected = False
            break

    if resource_leak_line_found == False:
        return None

    header_string = 'Delta resources in use:'
    footer_string = 'End of delta resources in use'
    counters = parse_counters(test_output, header_string, footer_string)

    return [leak_detected, counters]


def parse_fault_injection_counters(test_output):
    header_string = 'FaultInjection counters:'
    footer_string = 'End of FaultInjection counters'
    return parse_counters(test_output, header_string, footer_string)


def validate_counters(keys, counters, node_name):
    """
    Check if the counters dictionary has positive values for
    the keys listed
    """
    missing = []
    for key in keys:
        if counters[key] < 1:
            missing.append(key)
    if missing:
        msg = "The happy sequence did not cover " + str(missing) + " on " + node_name
        print hred(msg)
        raise ValueError(msg)
    return True


def test_parse_fault_injection_counters():

    print "test_parse_fault_injection_counters"

    test_output = '''
        WDMClient_NumCancelInUse:       0
        WDMClient_NumBindingsInUse:     0
        WDMClient_NumTransactions:      0

        FaultInjection counters:
        Weave_SendAlarm: 0
        Weave_HandleAlarm: 0
        Weave_WRMDoubleTx: 0
        Weave_BDXBadBlockCounter: 0
        Weave_SMConnectRequestNew: 0
        Inet_DNSResolverNew: 0
        Inet_Send: 7
        Inet_ConnectRequestNew: 0
        Inet_ConnectToServDir: 0
        Inet_ConnectToServEP: 0
        Inet_InetBufferNew: 0
        WeaveSys_PacketBufferNew: 8
        WeaveSys_TimeoutImmediate: 0
        End of FaultInjection counters
        WEAVE:SM: CancelSessionTimer
        WEAVE:ML: Closing endpoints
    '''

    counters = parse_fault_injection_counters(test_output)

    result =  counters["Inet_Send"] == str(7) and \
              counters["WeaveSys_PacketBufferNew"] == str(8) and \
              counters["WeaveSys_TimeoutImmediate"] == str(0)

    if result:
        print "passed"
    else:
        print "failed"


def test_scan_for_resource_leak():

    print "test_scan_for_resource_leak"

    test_output = '''
    some log line
    Resource leak detected and some more words 
    Delta resources in use:
    SystemLayer_NumPacketBufs:      1
    SystemLayer_NumTimersInUse:     1
    InetLayer_NumRawEpsInUse:       0
    InetLayer_NumTCPEpsInUse:       0
    InetLayer_NumUDPEpsInUse:       0
    InetLayer_NumTunEpsInUse:       0
    InetLayer_NumDNSResolversInUse:     0
    ExchangeMgr_NumContextsInUse:       1
    MessageLayer_NumConnectionsInUse:       0
    ServiceMgr_NumRequestsInUse:        0
    WDMClient_NumViewInUse:     0
    WDMClient_NumSubscribeInUse:        0
    WDMClient_NumUpdateInUse:       0
    WDMClient_NumCancelInUse:       0
    WDMClient_NumBindingsInUse:     0
    End of delta resources in use
    some other log line
    '''

    leak_detected, counters = scan_for_resource_leak(test_output)

    result = "failed"
    if leak_detected:
        result = "passed"
    print "Test 1: " + result

    test_output = '''
    some log line
    some other log line
    '''

    scan_result = scan_for_resource_leak(test_output)
    result = "failed"
    if scan_result == None:
        result = "passed"

    print "Test 2: " + result

    test_output = '''
    some log line
    Resource leak not detected
    some other log line
    '''

    leak_detected, counters = scan_for_resource_leak(test_output)
    result = "failed"
    if leak_detected == False:
        result = "passed"

    print "Test 3: " + result

def test_scan_for_leaks_and_parser_errors():
    print "test_scan_for_leaks_and_parser_errors"

    test_output = '''
    some log line
    Resource leak detected and some more words 
    Delta resources in use:
    SystemLayer_NumPacketBufs:      1
    SystemLayer_NumTimersInUse:     1
    InetLayer_NumRawEpsInUse:       0
    InetLayer_NumTCPEpsInUse:       0
    InetLayer_NumUDPEpsInUse:       0
    InetLayer_NumTunEpsInUse:       0
    InetLayer_NumDNSResolversInUse:     0
    ExchangeMgr_NumContextsInUse:       1
    MessageLayer_NumConnectionsInUse:       0
    ServiceMgr_NumRequestsInUse:        0
    WDMClient_NumViewInUse:     0
    WDMClient_NumSubscribeInUse:        0
    WDMClient_NumUpdateInUse:       0
    WDMClient_NumCancelInUse:       0
    WDMClient_NumBindingsInUse:     0
    End of delta resources in use
    some other log line
    '''

    parser_error, leak_detected = scan_for_leaks_and_parser_errors(test_output)
    result = "failed"
    if leak_detected and not parser_error:
        result = "passed"
    print result


class FaultInjectionOptions:
    """
    An object that handles CLI options for fault injection
    """

    def __init__(self, nodes = None):
        self.configuration = { 'faultid': None, 'faultskip': None, 'failonly': None, 'group': None, 'numgroups': None}
        self.getopt_config = [ "faultid=", "faultskip=", "failonly=", "group=", "numgroups=" ]
        self.nodes = ['client', 'server']
        if(nodes):
            self.nodes = nodes
        self.help_string = """\nfault-injection usage:
        --faultid <fault_name>      Inject only the fault specified; e.g. WeaveSys_PacketBufferNew
        --faultskip N               Inject the fault only once, skipping N instances
        --failonly                  Inject faults only to the node specified; allowed values: { """ + """, """.join(self.nodes) + """ }
        --group N  --numgroups M    Divide the list of tests in M groups and execute only the Nth (1 <= N <= M)
        """

    def is_node_enabled(self, node):
        """ Check if the user wants to inject faults only in the node given (usually 'client' or 'server') """
        return self.configuration['failonly'] in [None, node]

    def process_opts(self, opts):
        """ Process the fault-injection options.

        opts: the dictionary of options returned by getopt
        return: a copy of the list of options minus the fault-injection options
        """
        ret_opts = []
	for o, a in opts:
	    if o in ("--faultid"):
		self.configuration["faultid"] = a
	    elif o in ("--faultskip"):
		self.configuration["faultskip"] = int(a)
	    elif o in ("--failonly"):
		if a in self.nodes:
		    self.configuration["failonly"] = a
		else:
		    print self.help_string
		    sys.exit(1)
            elif o in ("--group"):
                num = int(a)
                if num <= 0:
                    print self.help_string
                    sys.exit(1)
                self.configuration["group"] = num
            elif o in ("--numgroups"):
                num = int(a)
                if num < 0:
                    print self.help_string
                    sys.exit(1)
                self.configuration["numgroups"] = num
            else:
                ret_opts.append((o, a))

        if (self.configuration["group"] != None) != (self.configuration["numgroups"] != None):
            print self.help_string
            sys.exit(1)

        if (self.configuration["group"] and self.configuration["group"] > self.configuration["numgroups"]):
            print self.help_string
            sys.exit(1)

        return ret_opts

    def parse_fault_instance_parameters(self, outputlog):
        parameters = {}

        for line in outputlog.splitlines():
            match = re.search(r'FI_instance_params: (\S+) maxArg: (\d+);', line.strip())
            if match:
                parameters[match.group(1)] = { 'maxArg': match.group(2) }

        return parameters

    def generate_faults_with_arguments(self, fault_config, parameters):
        """ Some fault ids can take arguments to customize the exception being
        injected.
        """
        ret_configs = []

        # Weave_WDMNotificationSize takes one optional positive argument
        # The deault in the production code is WDM_MAX_NOTIFICATION_SIZE
        # which by default is 1150
        if "Weave_WDMNotificationSize" in fault_config:
            for size in range(100, 1150, 100):
                ret_configs.append(fault_config + "_a" + str(size))
        # 0: expire exchange timers
        if "WeaveSys_AsyncEvent" in fault_config:
            # strip away the "_fN" part
            fault_config_tmp = re.sub(r'_f.*', "", fault_config)
            tmp = parameters[fault_config_tmp]
            num_of_events_to_inject = int(tmp['maxArg']) + 1
            for event in range(0, num_of_events_to_inject):
                ret_configs.append(fault_config + "_a" + str(event))
        if "Weave_FuzzExchangeHeaderTx" in fault_config:
            # strip away the "_fN" part
            fault_config_tmp = re.sub(r'_f.*', "", fault_config)
            tmp = parameters[fault_config_tmp]
            num_of_header_fuzzing_cases = int(tmp['maxArg']) + 1
            for size in range(0, num_of_header_fuzzing_cases):
                ret_configs.append(fault_config + "_a" + str(size))
        return ret_configs

    def generate_fault_config_list(self, node, outputlog, restart=False):
        """ Generate a list of fault injection options to be passed to the
        nodes under test, one per test.
        If a faultId was passed with the --faultid option, only that faultid is
        used to generate the list. If the --faultskip option was passed, only
        one configuration is generated per faultid with the skip value specified.

        node: the description of the node that generated the fault_counters, e.g. 'client' or 'server'
        fault_counters: a dictionary of fault counters as returned by parse_fault_injection_counters()
        restart: if true, fault configurations are generated twice each, once for the normal fault and
            the second time to trigger a node restart
        return: a list of fault configurations, e.g. [ "Inet_Send_s0_f1", "Inet_Send_s1_f1", ... ]
        """

        fault_ids = [ self.configuration['faultid'] ]
        ret_configs = []
        if self.is_node_enabled(node):
            fault_counters = parse_fault_injection_counters(outputlog)
            fault_instance_parameters = self.parse_fault_instance_parameters(outputlog)

            print node + ' fault counters: '
            pprint.pprint(fault_counters)

            if fault_ids[0] == None:
                fault_ids = sorted(fault_counters.keys())

            for fault_id in fault_ids:
                skip_range = self.__get_skip_range(fault_counters[fault_id])
                for event_number in skip_range:
                    fault_opt = fault_id + "_s" + str(event_number) + "_f1"
                    faults_with_arguments = self.generate_faults_with_arguments(fault_opt, fault_instance_parameters)
                    if faults_with_arguments:
                        ret_configs += faults_with_arguments
                    else:
                        ret_configs.append(fault_opt)
                    if restart and not "WeaveSys_AsyncEvent" == fault_id:
                        fault_opt += "_r"
                        ret_configs.append(fault_opt)

            group = self.configuration["group"]
            numgroups = self.configuration["numgroups"]
            if group:
                num_tests = len(ret_configs)
                num_tests_per_group = num_tests / numgroups
                index_first_test = num_tests_per_group * (group -1)
                index_first_test_next_group = num_tests_per_group * group
                if group == numgroups:
                    ret_configs = ret_configs[index_first_test:]
                else:
                    ret_configs = ret_configs[index_first_test:index_first_test_next_group]

        return ret_configs


    def __get_skip_range(self, fault_counter_value):
	skip_range = range(fault_counter_value)
	if self.configuration['faultskip'] is not None:
	    skip_range = [ self.configuration['faultskip'] ]

	return skip_range

def cleanup_after_exception():
    print "Deleting Happy state.."
    opts = happy.HappyStateDelete.option()
    state_delete = happy.HappyStateDelete.HappyStateDelete(opts)
    state_delete.run()
    print "Happy state deleted."


def run_unittest():
    """
    Wrapper for unittest.main() that ensures the Happy state is deleted in case
    of failure or error.
    This is meant to be used with mock-to-mock test cases that always delete the topology they create.
    If the test case can reuse an existing topology and therefore does not always delete the topology,
    it should handle exceptions in setUp and tearDown explicitly.

    Unittest traps all exceptions but not KeyboardInterrupt.
    So, a KeyboardInterrupt will cause the Happy state not to be deleted.
    An exception raised during a test results in the test ending with an "error"
    as opposed to a "failure" (see the docs for more details). tearDown is invoked
    in that case.
    If an exception is raised during setUp, tearDown is not invoked, and so the
    Happy state is not cleaned up.
    Note that an invocation of sys.exit() from the Happy code results in an
    exception itself (and then it depends if that is happening during setUp,
    during the test, or during tearDown).
    So,
      1. we must cleanup in case of KeyboardInterrupt, and
      2. to keep it simple, we cleanup in case of any SystemExit that carries
         an error (we don't care if unittest was able to run tearDown or not).
    """
    try:
        unittest.main()

    except KeyboardInterrupt:
        print "\n\nWeaveUtilities.run_unittest caught KeyboardInterrupt"
        cleanup_after_exception()
        raise

    except SystemExit as e:
        if e.args[0] not in [0, False]:
            print "\n\nWeaveUtilities.run_unittest caught some kind of test error or failure"
            cleanup_after_exception()
        raise e
    finally:
        Plaid.deletePlaidNetwork()


if __name__ == "__main__":

    test_scan_for_resource_leak()
    test_parse_fault_injection_counters()
    test_scan_for_leaks_and_parser_errors()
