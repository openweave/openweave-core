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
 *      This file implements a class for managing and finding Weave
 *      mock device command line functional testing tool commands or
 *      operations.
 *
 */

#include "ToolCommon.h"
#include "MockOpActions.h"

MockOpActions::MockOpActions()
{
    memset(mOps, 0, sizeof(mOps));
    mOpCount = 0;
}

MockOpActions::~MockOpActions()
{
}

bool MockOpActions::SetDelay(const char* opName, uint32_t delay)
{
    Op *op = FindOp(opName, true);
    if (op == NULL)
        return false;
    op->Delay = delay;
    return true;
}

uint32_t MockOpActions::GetDelay(const char* opName)
{
    Op *op = FindOp(opName);
    if (op == NULL)
        return 0;
    return op->Delay;
}

bool MockOpActions::SetAbort(const char* opName, bool abort)
{
    Op *op = FindOp(opName, true);
    if (op == NULL)
        return false;
    op->Abort = abort;
    return true;
}

bool MockOpActions::GetAbort(const char* opName)
{
    Op *op = FindOp(opName);
    return op != NULL && op->Delay;
}

MockOpActions::Op *MockOpActions::FindOp(const char *opName, bool add)
{
    opName = NormalizeOpName(opName);
    if (opName == NULL)
        return NULL;

    for (uint32_t i = 0; i < mOpCount; i++)
    {
        Op& op = mOps[i];
        if (strcmp(op.OpName, opName) == 0)
            return &op;
    }

    if (!add || mOpCount == kMaxOps)
        return NULL;

    Op *op = &mOps[mOpCount++];
    op->OpName = opName;
    return op;
}

const char *MockOpActions::NormalizeOpName(const char *opName)
{
    if (strcasecmp(opName, "scan-networks") == 0 || strcasecmp(opName, "scannetworks") == 0)
        return "scan-networks";
    if (strcasecmp(opName, "add-networks") == 0 || strcasecmp(opName, "addnetwork") == 0)
        return "add-networks";
    if (strcasecmp(opName, "update-networks") == 0 || strcasecmp(opName, "updatenetwork") == 0)
        return "update-networks";
    if (strcasecmp(opName, "remove-networks") == 0 || strcasecmp(opName, "removenetwork") == 0)
        return "remove-networks";
    if (strcasecmp(opName, "enable-network") == 0 || strcasecmp(opName, "enablenetwork") == 0)
        return "enable-network";
    if (strcasecmp(opName, "disable-network") == 0 || strcasecmp(opName, "disablenetwork") == 0)
        return "disable-network";
    if (strcasecmp(opName, "test-connectivity") == 0 || strcasecmp(opName, "testconnectivity") == 0 || strcasecmp(opName, "testcon") == 0)
        return "test-connectivity";
    if (strcasecmp(opName, "set-rendezvous-mode") == 0 || strcasecmp(opName, "setrendezvousmode") == 0)
        return "set-rendezvous-mode";
    if (strcasecmp(opName, "get-networks") == 0 || strcasecmp(opName, "getnetworks") == 0)
        return "get-networks";
    return NULL;
}
