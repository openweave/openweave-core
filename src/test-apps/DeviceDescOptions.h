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
 *      Implementation of DeviceDescOptions object, which handles parsing of command line options
 *      that specify descriptive information about the simulated "device" used in test applications.
 *
 */

#ifndef DEVICEDESCCONFIG_H_
#define DEVICEDESCCONFIG_H_

#include <Weave/Profiles/device-description/DeviceDescription.h>

#include "ToolCommonOptions.h"

using nl::Weave::Profiles::DeviceDescription::WeaveDeviceDescriptor;

class DeviceDescOptions : public OptionSetBase
{
public:
    DeviceDescOptions();

    // The base values for the test device descriptor, not including fields
    // that change dynamically (e.g. FabricId).
    WeaveDeviceDescriptor BaseDeviceDesc;

    void GetDeviceDesc(WeaveDeviceDescriptor& deviceDesc);

    bool ParseOption(int optId);

    virtual bool HandleOption(const char *progName, OptionSet *optSet, int id, const char *name, const char *arg);
};

extern DeviceDescOptions gDeviceDescOptions;

#endif // DEVICEDESCCONFIG_H_
