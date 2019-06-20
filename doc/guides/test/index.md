# Test Cases

OpenWeave includes a number of Python scripts for testing Weave functionality
over simulated Happy topologies. These test cases ensure the topology is
properly configured for network connectivity and Weave deployment.

Test case scripts are found in the OpenWeave repository at
[`/src/test-apps/happy/tests`](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/tests).
There are two types of tests:

*   [Service](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/tests/service)
    — Tests that interface with a Service
*   [Standalone](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/tests/standalone)
    — Tests that run on local topologies

## Run

1.  Install OpenWeave. See the OpenWeave [Build](https://github.com/openweave/openweave-core/tree/master/BUILDING.md)
    guide for instructions.
1.  Install Happy. See the Happy [Setup](https://openweave.io/happy/setup) guide
    for instructions.
1.  Navigate to the directory containing the target test case. For example, to
    run an Echo profile test case:

        $ cd {path-to-openweave-core}/src/test-apps/happy/tests/standalone/echo
        $ python test_weave_echo_01.py

## Change test topology

OpenWeave test cases run against the sample Happy topologies found in
[`/src/test-apps/happy/topologies/standalone`](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/topologies/standalone).
To use your own custom Happy topology in a test case:

1.  After constructing your custom topology, save it in JSON format:

        $ happy-state -s my_topology.json

    This saves the topology state file in the `$HOME` directory.
1.  In the test case script, locate the topology file being used. Topologies in
    test cases are typically assigned to the `self.topology_file` variable. For
    example,
    [`test_weave_echo_01.py`](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/tests/standalone/echo/test_weave_echo_01.py#L45)
    uses the following topology for a default OpenWeave build:

        self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
            "/../../../topologies/standalone/three_nodes_on_thread_weave.json"

1.  Update the topology path to point to your custom topology state file:

        self.topology_file = "~/my_topology.json"

1.  Alternatively, place your custom topology state file in the same location
    as those included with OpenWeave:

        self.topology_file = os.path.dirname(os.path.realpath(__file__)) + \
            "/../../../topologies/standalone/my_topology.json"

1.  [Run the test case](#run).

> Caution: Some test case scripts, such as
[`test_weave_pairing_01.py`](https://github.com/openweave/openweave-core/tree/master/src/test-apps/happy/tests/standalone/pairing/test_weave_pairing_01.py#L78)
use hard-coded Happy node names. Before modifying a script to use a custom
topology, review the code and ensure your topology and the existing script are
aligned.
