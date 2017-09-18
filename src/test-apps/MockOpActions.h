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
 *      This file defines a class for managing and finding Weave mock
 *      device command line functional testing tool commands or
 *      operations.
 *
 */

#ifndef MOCKOPACTIONS_H_
#define MOCKOPACTIONS_H_

#include <stdbool.h>
#include <stdint.h>

class MockOpActions
{
public:
    MockOpActions();
    ~MockOpActions();

    bool SetDelay(const char *opName, uint32_t delay);
    uint32_t GetDelay(const char *opName);

    bool SetAbort(const char *opName, bool abort);
    bool GetAbort(const char *opName);

private:

    class Op
    {
    public:
        const char *OpName;
        uint32_t Delay;
        bool Abort;
    };

    enum
    {
        kMaxOps = 32
    };

    Op mOps[kMaxOps];
    uint32_t mOpCount;

    Op *FindOp(const char *opName, bool add = false);
    static const char *NormalizeOpName(const char *opName);
};


#endif /* MOCKOPACTIONS_H_ */
