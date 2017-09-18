/*
 *
 *    Copyright (c) 2015-2017 Nest Labs, Inc.
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
#include <stdarg.h>

#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

using namespace nl::Weave::TLV;

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {


/**
 * @brief
 *   A helper function that translates an already serialized eventdata element into the event buffer.
 *
 * @param ioWriter[inout] The writer to use for writing out the event
 *
 * @param appData[in]     A pointer to the TLVReader that holds serialized event data.
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the ioWriter.
 *
 */
static WEAVE_ERROR EventWriterTLVCopy(TLVWriter & ioWriter, uint8_t inDataTag, void *appData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    TLVReader *reader = static_cast<TLVReader *>(appData);

    VerifyOrExit(reader != NULL, err = WEAVE_ERROR_INVALID_ARGUMENT);

    err = reader->Next();
    SuccessOrExit(err);

    err = ioWriter.CopyElement(nl::Weave::TLV::ContextTag(kTag_EventData), *reader);

exit:
    return err;
}

event_id_t LogEvent(const EventSchema &inSchema, TLVReader &inData)
{
    return LogEvent(inSchema, inData, NULL);
}

event_id_t LogEvent(const EventSchema &inSchema, TLVReader &inData, const EventOptions *inOptions)
{
    return LogEvent(inSchema, EventWriterTLVCopy, &inData, inOptions);
}

event_id_t LogEvent(const EventSchema &inSchema, EventWriterFunct inEventWriter, void *inAppData)
{
    return LogEvent(inSchema, inEventWriter, inAppData, NULL);
}

event_id_t LogEvent(const EventSchema &inSchema, EventWriterFunct inEventWriter, void *inAppData, const EventOptions *inOptions)
{

    LoggingManagement &logManager = LoggingManagement::GetInstance();

    return logManager.LogEvent(inSchema, inEventWriter, inAppData, inOptions);
}

struct DebugLogContext
{
    const char *mRegion;
    const char *mFmt;
    va_list mArgs;
};

/**
 * @brief
 *   A helper function for emitting a freeform text as a debug
 *   event. The debug event is a structure with a logregion and a
 *   freeform text.
 *
 * @param ioWriter[inout] The writer to use for writing out the event
 *
 * @param appData[in]     A pointer to the DebugLogContext, a structure
 *                        that holds a string format, arguments, and a
 *                        log region
 *
 * @param[in]             inDataTag A context tag for the TLV we're writing out.  Unused here,
 *                        but required by the typedef for EventWriterFunct.
 *
 * @retval #WEAVE_NO_ERROR On success.
 *
 * @retval other          Other errors that mey be returned from the ioWriter.
 *
 */
WEAVE_ERROR PlainTextWriter(TLVWriter & ioWriter, uint8_t inDataTag, void *appData)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    DebugLogContext *context = static_cast<DebugLogContext *>(appData);
    TLVType outer;

    err = ioWriter.StartContainer(nl::Weave::TLV::ContextTag(kTag_EventData), kTLVType_Structure, outer);
    SuccessOrExit(err);

    err = ioWriter.PutString(nl::Weave::TLV::ContextTag(kTag_Region), context->mRegion);
    SuccessOrExit(err);

    err = ioWriter.VPutStringF(nl::Weave::TLV::ContextTag(kTag_Message), reinterpret_cast<const char*>(context->mFmt), context->mArgs);
    SuccessOrExit(err);

    err = ioWriter.EndContainer(outer);
    SuccessOrExit(err);

    ioWriter.Finalize();

exit:
    return err;
}

event_id_t LogFreeform(ImportanceType inImportance, const char * inFormat, ...)
{
    DebugLogContext context;
    event_id_t eid;
    EventSchema schema = { kWeaveProfile_NestDebug, kNestDebug_StringLogEntryEvent, inImportance, 1, 1 };

    va_start(context.mArgs, inFormat);
    context.mRegion = "";
    context.mFmt = inFormat;

    eid = LogEvent(schema, PlainTextWriter, &context, NULL);

    va_end(context.mArgs);

    return eid;
}

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
