#!/usr/bin/env python

"""
Defines all test parameter options for WeaveWdmNext automation.
"""

# Test Options
# Client nodes
CLIENTS = "clients"
# Whether to use plaid
PLAID = "plaid"
# Control test result output
QUIET = "quiet"
# Server nodes
SERVER = "server"
# Whether to use strace
STRACE = "strace"
# Name of test case
TEST_CASE_NAME = "test_case_name"
# Time to wait for test end
TIMEOUT = "timeout"
# Option for using persistent storage
USE_PERSISTENT_STORAGE = "use_persistent_storage"
# Type of weave subscription
WDM_OPTION = "wdm_option"
# Tag to denote test type
TEST_TAG = "test_tag"

"""
Client/Server SHARED Options
"""
# Resets the state of the data sink after each iteration
CLEAR_STATE_BETWEEN_ITERATIONS = "clear_state_between_iterations"
# Enable/disable flip trait data in HandleDataFlipTimeout
ENABLE_FLIP = "enable_flip"
# Controls whether the process will exit the iteration or not at the end of the iteration
ENABLE_STOP = "enable_stop"
# Controls the event generator (None | Debug | Livenesss | Security | Telemetry | TestTrait)
# running in the node's background.
EVENT_GENERATOR = "event_generator"
# Fault injections
FAULTS = "faults"
# Control what ends the test iteration
# 0(client cancel), 1(publisher cancel), 2(client abort), 3(Publisher abort), 4(Idle)
FINAL_STATUS = "final_status"
# Period of time (milliseconds) between events
INTER_EVENT_PERIOD = "inter_event_period"
# Specify time (seconds) between liveness checks in WDM subscription as publisher
LIVENESS_CHECK_PERIOD = "wdm_liveness_check_period"
# Save wdm perf data in files
SAVE_PERF = "save_perf"
TAP = "tap_device"
# Controls which traits are published and subscribed to, values must be same between client/server
TEST_CASE = "test_case"
# Period of time (milliseconds) between iterations
TEST_DELAY = "test_delay"
# Number of times test sequence is repeated
TEST_ITERATIONS = "test_iterations"
# Period of time between trait instance mutations
TIMER_PERIOD = "timer_period"
# Number of times the node will mutate the trait instance per iteration
TOTAL_COUNT = "total_count"
# Logs to verify on node
LOG_CHECK = "log_check"

"""
Client ONLY Options
"""
CASE = "case"
# File containing Weave certificate to authenticate the node
CASE_CERT_PATH = "node_cert"
# File containing private key to authenticate the node
CASE_KEY_PATH = "node_key"
# Enable/disable dictionary tests
ENABLE_DICTIONARY_TEST = "enable_dictionary_test"
# Enable automatic subscription retries by WDM
ENABLE_RETRY = "enable_retry"
# Enable mock event initial counter using timestamp
ENABLE_MOCK_EVENT_TIMESTAMP_INITIAL_COUNTER = "enable_mock_event_timestamp_initial_counter"
# Use group key to encrypt messages
GROUP_ENC = "group_enc"
# Key to encrypt messages
GROUP_ENC_KEY_ID = "group_enc_key_id"
# The conditionality of the update (conditional | unconditional | mixed | alternate)
UPDATE_CONDITIONALITY = "wdm_update_conditionality"
# Client discards the paths on which SetUpdated was called in case of error
UPDATE_DISCARD_ERROR = "wdm_update_discard_on_error"
# The first mutation to apply to each trait instance
UPDATE_MUTATION = "wdm_update_mutation"
# Number of mutations performed in same context
UPDATE_NUM_MUTATIONS = "wdm_update_number_of_mutations"
# How many times same mutation is applied before moving on
UPDATE_NUM_REPEATED_MUTATIONS = "wdm_update_number_of_repeated_mutations"
# Number of traits to mutate (1-4)
UPDATE_NUM_TRAITS = "wdm_update_number_of_traits"
# Controls when first mutation is applied and flushed
UPDATE_TIMING = "wdm_update_timing"


"""
Server ONLY Options
"""
# Node ID of the destination node
WDM_SUBLESS_NOTIFY_DEST_NODE = "wdm_subless_notify_dest_node"

# Options top level keys
CLIENT = "client_options"
SERVER = "server_options"
TEST = "test_options"

OPTIONS = {
    CLIENT: {
        CASE: False,
        CASE_CERT_PATH: None,
        CASE_KEY_PATH: None,
        CLEAR_STATE_BETWEEN_ITERATIONS: False,
        ENABLE_DICTIONARY_TEST: True,
        ENABLE_FLIP: None,
        ENABLE_RETRY: False,
        ENABLE_STOP: True,
        ENABLE_MOCK_EVENT_TIMESTAMP_INITIAL_COUNTER: True,
        EVENT_GENERATOR: None,
        FAULTS: None,
        FINAL_STATUS: None,
        GROUP_ENC: False,
        GROUP_ENC_KEY_ID: None,
        INTER_EVENT_PERIOD: None,
        LIVENESS_CHECK_PERIOD: None,
        SAVE_PERF: False,
        TAP: None,
        TEST_CASE: 1,
        TEST_DELAY: None,
        TEST_ITERATIONS: None,
        TIMER_PERIOD: None,
        TOTAL_COUNT: None,
        UPDATE_CONDITIONALITY: None,
        UPDATE_DISCARD_ERROR: False,
        UPDATE_MUTATION: None,
        UPDATE_NUM_MUTATIONS: None,
        UPDATE_NUM_REPEATED_MUTATIONS: None,
        UPDATE_NUM_TRAITS: None,
        UPDATE_TIMING: None,
        LOG_CHECK: []
    },
    SERVER: {
        CLEAR_STATE_BETWEEN_ITERATIONS: False,
        ENABLE_FLIP: None,
        ENABLE_STOP: False,
        EVENT_GENERATOR: None,
        FAULTS: None,
        FINAL_STATUS: None,
        INTER_EVENT_PERIOD: None,
        LIVENESS_CHECK_PERIOD: None,
        SAVE_PERF: False,
        TAP: None,
        TEST_CASE: 1,
        TEST_DELAY: 0,
        TEST_ITERATIONS: None,
        TIMER_PERIOD: None,
        TOTAL_COUNT: None,
        WDM_SUBLESS_NOTIFY_DEST_NODE: None,
        LOG_CHECK: []
    },
    TEST: {
        CLIENTS: None,
        PLAID: "auto",
        QUIET: True,
        SERVER: None,
        STRACE: False,
        TEST_CASE_NAME: [],
        TIMEOUT: 60 * 15,
        WDM_OPTION: None,
        USE_PERSISTENT_STORAGE: True,
        TEST_TAG: None
    }
}
