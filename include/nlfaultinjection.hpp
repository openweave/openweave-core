/*
 *
 *    Copyright 2016 The nlfaultinjection Authors.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

/**
 *    @file
 *      Header file for the fault-injection utilities.
 *      This module provides an object to manager a set of fault IDs,
 *      and a macro to simplify the insertion of fault code in
 *      production code.
 */

#ifndef FAULT_INJECTION_H_
#define FAULT_INJECTION_H_

#include <stddef.h>
#include <stdint.h>
#include <errno.h>

namespace nl {

namespace FaultInjection {

typedef struct _Record Record;
typedef struct _Callback Callback;

typedef uint32_t Identifier;

enum {
    kMaxFaultArgs = 8 /**< The max number of arguments that can be stored in a fault */
};

/**
 * A fault-injection callback function.
 * A function of this type can be attached to a fault ID, and will be invoked every time
 * FaultInjectionMgr::CheckFault is called on the fault ID.
 * The main purpose of registering a callback is to be able to turn on lower-level faults from
 * higher level events; e.g. "fail in SendMessage for the next WDM ViewRequest".
 * The callback can also be used to let the application decide if the fault is supposed to be
 * triggered at each invocation. If the callback returns true, the fault is triggered.
 *
 * @param[in]   aFaultID        The fault ID
 * @param[in]   aFaultRecord    Pointer to the Record for aFaultID;
 *                              This allows the callback to check how the fault is configured
 *                              before taking action.
 * @param[in]   aContext        The pointer stored by the application in the Callback
 *                              structure.
 * @return      true if the fault is to be triggered. false if this callback does not want to
 *              force the fault to be triggered.
 */
typedef bool (*CallbackFn)(Identifier aId, Record *aFaultRecord, void *aContext);

/**
 * A linked-list node to hold a callback function to be attached to a fault ID.
 * The application can store a pointer in the mContext member.
 */
struct _Callback
{
    CallbackFn mCallBackFn;   /**< Callback function pointer */
    void      *mContext;      /**< Pointer for the application to store a context for mCallbackFn */
    Callback  *mNext;         /**< Linked list next pointer */
};

/**
 * Structure that stores the configuration of a given fault ID.
 * The module defining the fault-injection API needs to provide an array of Record
 * and pass it to its Manager instance via the Init method.
 */
struct _Record
{
    uint32_t  mNumCallsToSkip;    /**< The number of times this fault should not trigger before it
                                       starts failing */
    uint32_t  mNumCallsToFail;    /**< The number of times this fault should fail, before disabling
                                       itself */
    uint8_t   mPercentage;        /**< A number between 0 and 100 that indicates the percentage of
                                       times the fault should be triggered. */
    uint8_t   mReboot;            /**< This fault should reboot the system */

    Callback *mCallbackList;      /**< A list of callbacks */

    uint32_t  mNumTimesChecked;   /**< The number of times the fault location was executed */

    uint16_t  mLengthOfArguments; /**< The length of the array pointed to by mArguments */

    uint16_t  mNumArguments;      /**< The number of items currently stored in the array pointed to by mArguments */

    int32_t  *mArguments;         /**< A pointer to an array of integers to store extra arguments; this array is meant to
                                       be populated by either of the following:
                                       - the ParseFaultInjectionStr, so the values are available at the fault injection site
                                         and when the fault is injected
                                       - the logic around the fault injection site, to save useful values that can then
                                         be logged by a callback installed by the application, and so made available for use
                                         in subsequent test runs as arguments to the injected code.
                                         For example, the values can be exact arguments to be passed in, or ranges to be
                                         iterated on (like the length of a byte array to be fuzzed). */
};

/**
 * The module that provides a fault-injection API needs to provide an instance of Manager,
 * and initialize it with an array of Record.
 */
class Manager
{
public:

    int32_t Init(size_t inNumFaults, Record *inFaultArray, const char *inName, const char **inFaultNames);

    int32_t FailRandomlyAtFault(Identifier inId, uint8_t inPercentage = 10);

    int32_t FailAtFault(Identifier inId,
                        uint32_t inNumCallsToSkip,
                        uint32_t inNumCallsToFail);

    int32_t RebootAtFault(Identifier inId);

    int32_t StoreArgsAtFault(Identifier inId, uint16_t inNumArgs, int32_t *inArgs);

    int32_t InsertCallbackAtFault(Identifier inId, Callback *inCallBack);

    int32_t RemoveCallbackAtFault(Identifier inId, Callback *inCallBack);

    bool CheckFault(Identifier inId);
    bool CheckFault(Identifier inId, uint16_t &outNumArgs, int32_t *&outArgs);

    /**
     * Get the number of fault IDs defined by the Manager.
     *
     * @return  the number of fault IDs defined by the Manager.
     */
    size_t GetNumFaults(void) const
    {
        return mNumFaults;
    }

    /**
     * Get the name of the Manager. Every Manager object is initialized with a name,
     * so that faults can be configured using human readable strings.
     *
     * @return  The Manager's name, as a pointer to a const null-terminated string.
     */
    const char *GetName(void) const
    {
        return mName;
    }

    /**
     * Get a pointer to the array of fault names.
     *
     * @return  A pointer to a const char pointer. The array length
     *          is the number of faults defined by the Manager; see GetNumFaults.
     */
    const char **GetFaultNames(void) const
    {
        return mFaultNames;
    }

    const Record *GetFaultRecords(void) const
    {
        return mFaultRecords;
    }

    void ResetFaultCounters(void);

    int32_t ResetFaultConfigurations(void);
    int32_t ResetFaultConfigurations(Identifier inId);

private:

    size_t  mNumFaults;
    Record *mFaultRecords;
    const char *mName;
    const char **mFaultNames;
};

/**
 * The type of a function that returns a reference to a Manager
 * The module is expected to provide such a function so that
 * it can be added to an array of GetManagerFn instances and passed to
 * ParseFaultInjectionStr.
 */
typedef Manager &(*GetManagerFn)(void);

/**
 * A callback for the application to implement support for restarting
 * the system.
 */
typedef void (*RebootCallbackFn)(void);

/**
 * A callback to inform the application that a Manager has decided to inject a fault.
 * The main use of this type of callback is to print a log statement.
 */
typedef void (*PostInjectionCallbackFn)(Manager *aManager, Identifier aId, Record *aFaultRecord);

/**
 * A table of callbacks used by all managers
 */
typedef struct _GlobalCallbackTable {
    RebootCallbackFn        mRebootCb;        /**< See RebootCallbackFn */
    PostInjectionCallbackFn mPostInjectionCb; /**< PostInjectionCallbackFn */
} GlobalCallbackTable;

/**
 * A structure to hold global state that is used
 * by all Managers
 */
typedef struct _GlobalContext {
    GlobalCallbackTable mCbTable;             /**< A table of callbacks */
} GlobalContext;

void SetGlobalContext(GlobalContext *inGlobalContext);

bool ParseFaultInjectionStr(char *inStr, GetManagerFn *inTable, size_t inTableSize);

} // namespace FaultInjection

} // namespace nl

/**
 * The macro to inject the fault code.
 * Typically the module offering a fault-injection API
 * wraps this macro into a macro that:
 * 1. translates to a no-op if faults are disabled at compile time;
 * 2. hardcodes aManager to the module's own.
 *
 * @param[in] aManager    The Manager
 * @param[in] aId         The fault ID
 * @param[in] aStatements C++ code to be executed if the fault is to be injected.
 *                        For example:
 *                            - a single statement without terminating ";"
 *                            - two or more statements, separated by ";"
 *                            - a whole C++ block, enclosed in "{}"
 *                        The statements can refer to a local array of int32_t
 *                        args called faultArgs, of length numFaultArgs.
 */
#define nlFAULT_INJECT( aManager, aId, aStatements ) \
    do { \
        uint16_t numFaultArgs = 0; \
        int32_t *faultArgs = NULL; \
        if ((aManager).CheckFault(aId, numFaultArgs, faultArgs)) \
        { \
            aStatements; \
        } \
    } while (0)

#endif // FAULT_INJECTION_H_
