/*
 *
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file implements the Weave command line tool.
 *
 *      The Weave command line tool is used, primarily, for generating
 *      and manipulating Weave security certificate material.
 *
 */

#include <stdio.h>
#include <string.h>

#include <Weave/WeaveVersion.h>

#include "weave-tool.h"

static const char *const sHelp =
        "Usage: weave <command> [ <args...> ]\n"
        "\n"
        "Commands:\n"
        "\n"
        "    gen-ca-cert -- Generate a Weave CA certificate.\n"
        "\n"
        "    gen-device-cert -- Generate a Weave device certificate.\n"
        "\n"
        "    gen-code-signing-cert -- Generate a Weave code signing certificate.\n"
        "\n"
        "    gen-service-endpoint-cert -- Generate a Weave service endpoint certificate.\n"
        "\n"
        "    gen-general-cert -- Generate a general Weave certificate with a string subject.\n"
        "\n"
        "    gen-provisioning-data -- Generate manufacturing provisioning data for one or more devices.\n"
        "\n"
        "    convert-cert -- Convert a certificate between Weave and X509 form.\n"
        "\n"
        "    convert-key -- Convert a private key between Weave and PEM/DER form.\n"
        "\n"
        "    convert-provisioning-data -- Perform various conversions on a device provisioning data file.\n"
        "\n"
        "    resign-cert -- Resign a weave certificate using a new CA key.\n"
        "\n"
        "    make-service-config -- Make a service config object.\n"
        "\n"
        "    validate-cert -- Validate a Weave certificate chain.\n"
        "\n"
        "    print-cert -- Print a Weave certificate.\n"
        "\n"
        "    print-sig -- Print a Weave signature.\n"
        "\n"
        "    print-tlv -- Print a Weave TLV object.\n"
        "\n"
        "    version -- Print the program version and exit.\n"
        "\n"
        ;

/**
 * Print to standard output the program version information.
 *
 * @return Unconditionally returns true.
 *
 */
static bool PrintVersion(void)
{
    printf("weave " WEAVE_VERSION_STRING "\n" COPYRIGHT_STRING);

    return true;
}

extern "C"
int main(int argc, char *argv[])
{
    bool res = false;

    if (argc == 1)
        fprintf(stderr, "weave: Please specify a command, or 'help' for help.\n");

    else if (strcasecmp(argv[1], "help") == 0 || strcasecmp(argv[1], "--help") == 0 || strcasecmp(argv[1], "-h") == 0)
        res = (fputs(sHelp, stdout) != EOF);

    else if (strcasecmp(argv[1], "version") == 0 || strcasecmp(argv[1], "--version") == 0 || strcasecmp(argv[1], "-v") == 0)
        res = PrintVersion();

    else if (strcasecmp(argv[1], "gen-ca-cert") == 0 || strcasecmp(argv[1], "gencacert") == 0)
        res = Cmd_GenCACert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "gen-device-cert") == 0 || strcasecmp(argv[1], "gendevicecert") == 0)
        res = Cmd_GenDeviceCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "gen-code-signing-cert") == 0 || strcasecmp(argv[1], "gencodesigningcert") == 0)
        res = Cmd_GenCodeSigningCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "gen-service-endpoint-cert") == 0 || strcasecmp(argv[1], "genserviceendpointcert") == 0)
        res = Cmd_GenServiceEndpointCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "gen-general-cert") == 0 || strcasecmp(argv[1], "gengeneralcert") == 0)
        res = Cmd_GenGeneralCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "convert-cert") == 0 || strcasecmp(argv[1], "convertcert") == 0)
        res = Cmd_ConvertCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "convert-key") == 0 || strcasecmp(argv[1], "convertkey") == 0)
        res = Cmd_ConvertKey(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "convert-provisioning-data") == 0 || strcasecmp(argv[1], "convertprovisioningdata") == 0)
        res = Cmd_ConvertProvisioningData(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "resign-cert") == 0 || strcasecmp(argv[1], "resigncert") == 0)
        res = Cmd_ResignCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "make-service-config") == 0 || strcasecmp(argv[1], "makeserviceconfig") == 0)
        res = Cmd_MakeServiceConfig(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "make-access-token") == 0 || strcasecmp(argv[1], "makeaccesstoken") == 0)
        res = Cmd_MakeAccessToken(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "gen-provisioning-data") == 0 || strcasecmp(argv[1], "genprovisioningdata") == 0)
        res = Cmd_GenProvisioningData(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "validate-cert") == 0 || strcasecmp(argv[1], "validatecert") == 0)
        res = Cmd_ValidateCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "print-cert") == 0 || strcasecmp(argv[1], "printcert") == 0)
        res = Cmd_PrintCert(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "print-sig") == 0 || strcasecmp(argv[1], "printsig") == 0)
        res = Cmd_PrintSig(argc - 1, argv + 1);

    else if (strcasecmp(argv[1], "print-tlv") == 0 || strcasecmp(argv[1], "printtlv") == 0)
        res = Cmd_PrintTLV(argc - 1, argv + 1);

    else
        fprintf(stderr, "weave: Unrecognized command: %s\n", argv[1]);

    return res ? 0 : -1;
}
