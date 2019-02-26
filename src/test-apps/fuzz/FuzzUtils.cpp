/*
 *
 *    Copyright (c) 2018 Google LLC.
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
 *      This file implements a set of utilities for fuzzing tests
 */

#include "FuzzUtils.h"
#include <stdio.h>
#include <vector>

#include "FuzzUtils.h"
#include "ToolCommon.h"
#include <Weave/Support/ErrorStr.h>
#include <Weave/Profiles/security/WeaveSecurity.h>
#include <Weave/Profiles/security/WeavePASE.h>
#include <Weave/Support/RandUtils.h>
using namespace nl::Weave::Profiles::Security;
using namespace nl::Weave::Profiles::Security::PASE;
using System::PacketBuffer;

void saveCorpus(const uint8_t *inBuf, size_t size, char *fileName)
{
    //Todo create better method of creating these.
    FILE* file = fopen(fileName, "wb+" );
    fwrite(inBuf, 1, size, file );
    fclose(file);
}

void printCorpus(const uint8_t *inBuf, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (i % 12 == 0) { printf("\n"); }
        printf("0x%02X, ", inBuf[i]);
    }
    printf("\n");
}
