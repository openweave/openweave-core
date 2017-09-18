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
#include <Weave/Profiles/data-management/Current/WdmManagedNamespace.h>
#include <Weave/Profiles/data-management/DataManagement.h>

namespace nl {
namespace Weave {
namespace Profiles {
namespace WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current) {

#if WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

static WEAVE_ERROR
ReadAndCheckPresence(nl::Weave::TLV::TLVReader &inReader,
                     uint64_t &inOutReceivedMask,
                     const uint64_t &inReceivedFieldFlag,
                     uint64_t &inOutValue);

EventProcessor::EventProcessor(uint64_t inLocalNodeId) :
    mLocalNodeId(inLocalNodeId),
    mLastEventId()
{
}

EventProcessor::~EventProcessor(void)
{
}

WEAVE_ERROR
EventProcessor::ProcessEvents(nl::Weave::TLV::TLVReader &inReader,
                              nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = ParseEventList(inReader, inClient);
    SuccessOrExit(err);

exit:
    return err;
}

WEAVE_ERROR
EventProcessor::ParseEventList(nl::Weave::TLV::TLVReader &inReader,
                               nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    StreamParsingContext parsingContext(inClient.GetBinding()->GetPeerNodeId());

    while (WEAVE_NO_ERROR == (err = inReader.Next()))
    {
        VerifyOrExit(nl::Weave::TLV::AnonymousTag == inReader.GetTag(), err = WEAVE_ERROR_TLV_TAG_NOT_FOUND);
        VerifyOrExit(nl::Weave::TLV::kTLVType_Structure == inReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

        {
            nl::Weave::TLV::TLVType outerContainerType;

            err = inReader.EnterContainer(outerContainerType);
            SuccessOrExit(err);

            err = ParseEvent(inReader, inClient, parsingContext);
            SuccessOrExit(err);

            err = inReader.ExitContainer(outerContainerType);
            SuccessOrExit(err);
        }
    }

exit:
    if (err == WEAVE_END_OF_TLV)
    {
        err = WEAVE_NO_ERROR;
    }

    return err;
}

WEAVE_ERROR
EventProcessor::ParseEvent(nl::Weave::TLV::TLVReader &inReader,
                           nl::Weave::Profiles::DataManagement::SubscriptionClient &inClient,
                           StreamParsingContext &inOutParsingContext)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    EventHeader eventHeader;
    uint64_t receivedMask = 0;

    while (WEAVE_NO_ERROR == (err = inReader.Next()))
    {
        uint32_t tag = 0;

        VerifyOrExit(nl::Weave::TLV::IsContextTag(inReader.GetTag()), err = WEAVE_ERROR_INVALID_TLV_TAG);

        tag = nl::Weave::TLV::TagNumFromTag(inReader.GetTag());

        switch (tag)
        {
            case Event::kCsTag_Source:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_Source,
                                           eventHeader.mSource);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_Importance:
            {
                uint64_t v = 0;

                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_Importance,
                                           v);
                SuccessOrExit(err);

                eventHeader.mImportance = (ImportanceType)v;
                break;
            }

            case Event::kCsTag_Id:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_Id,
                                           eventHeader.mId);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_RelatedImportance:
            {
                uint64_t v = 0;

                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_RelatedImportance,
                                           v);
                SuccessOrExit(err);

                eventHeader.mRelatedImportance = (ImportanceType)v;
                break;
            }

            case Event::kCsTag_RelatedId:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_RelatedId,
                                           eventHeader.mRelatedId);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_UTCTimestamp:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_UTCTimestamp,
                                           eventHeader.mUTCTimestamp);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_SystemTimestamp:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_SystemTimestamp,
                                           eventHeader.mSystemTimestamp);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_ResourceId:
            {
                // Mandatory field.
                VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == inReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

                err = inReader.Get(eventHeader.mResourceId);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_TraitProfileId:
            {
                nl::Weave::TLV::TLVType outerContainerType;

                // Mandatory field.
                VerifyOrExit(nl::Weave::TLV::kTLVType_UnsignedInteger == inReader.GetType() || nl::Weave::TLV::kTLVType_Array == inReader.GetType(), err = WEAVE_ERROR_WRONG_TLV_TYPE);

                if (inReader.GetType() == nl::Weave::TLV::kTLVType_Array) {
                    err = inReader.EnterContainer(outerContainerType);
                    SuccessOrExit(err);

                    err = inReader.Next();
                    SuccessOrExit(err);

                    VerifyOrExit(inReader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                    err = inReader.Get(eventHeader.mTraitProfileId);
                    SuccessOrExit(err);

                    err = inReader.Next();
                    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                    // MaxVersion is only encoded if it isn't 1.
                    if (err == WEAVE_NO_ERROR) {
                        VerifyOrExit(inReader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                        err = inReader.Get(eventHeader.mDataSchemaVersionRange.mMaxVersion);
                        SuccessOrExit(err);
                    }

                    // Similarly, MinVersion is only encoded if it isn't 1.
                    err = inReader.Next();
                    VerifyOrExit(err == WEAVE_NO_ERROR || err == WEAVE_END_OF_TLV, );

                    if (err == WEAVE_NO_ERROR) {
                        VerifyOrExit(inReader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                        err = inReader.Get(eventHeader.mDataSchemaVersionRange.mMinVersion);
                        SuccessOrExit(err);
                    }

                    err = inReader.Next();
                    VerifyOrExit(err == WEAVE_END_OF_TLV, err = WEAVE_ERROR_WDM_MALFORMED_DATA_ELEMENT);

                    err = inReader.ExitContainer(outerContainerType);
                    SuccessOrExit(err);
                }
                else {
                    VerifyOrExit(inReader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger, err = WEAVE_ERROR_WRONG_TLV_TYPE);

                    err = inReader.Get(eventHeader.mTraitProfileId);
                }

                break;
            }

            case Event::kCsTag_TraitInstanceId:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_TraitInstanceId,
                                           eventHeader.mTraitInstanceId);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_Type:
            {
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_Type,
                                           eventHeader.mType);
                SuccessOrExit(err);
                break;
            }

            case Event::kCsTag_DeltaUTCTime:
            {
                uint64_t v = 0;
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_DeltaUTCTime,
                                           v);
                SuccessOrExit(err);
                eventHeader.mDeltaUTCTime = (int64_t)v;
                break;
            }

            case Event::kCsTag_DeltaSystemTime:
            {
                uint64_t v = 0;
                err = ReadAndCheckPresence(inReader,
                                           receivedMask,
                                           ReceivedEventHeaderFieldPresenceMask_DeltaSystemTime,
                                           v);
                SuccessOrExit(err);
                eventHeader.mDeltaSystemTime = (int64_t)v;
                break;
            }

            case Event::kCsTag_Data:
            {
                // check if this tag has appeared before
                VerifyOrExit((receivedMask & ReceivedEventHeaderFieldPresenceMask_Data) == 0, err = WEAVE_ERROR_INVALID_TLV_TAG);
                receivedMask |= ReceivedEventHeaderFieldPresenceMask_Data;

                // Mandatory field.  Make sure we're sending up a
                // fully-qualified header to the app.
                err = UpdateContextQualifyHeader(eventHeader, inOutParsingContext, receivedMask);
                SuccessOrExit(err);

                // Potentially this could be moved to outside the while(inReader.Next())?
                // but so could the call to ProcessEvent (this would enable data-less events).
                // For now leaving both together makes up for it in readability.
                err = UpdateGapDetection(eventHeader);
                SuccessOrExit(err);

                err = ProcessEvent(inReader, inClient, eventHeader);
                SuccessOrExit(err);

                break;
            }

            default:
            {
                // Unknown.  If a newly-added field is not optional,
                // it needs to be handled in a case above.
                WeaveLogError(EventLogging, "EventProcessor encountered unknown tag 0x%" PRIx32 " (%d)", tag, tag);
                break;
            }
        }
    }

    // almost all fields in an Event are optional
    if (err == WEAVE_END_OF_TLV)
    {
        err = WEAVE_NO_ERROR;
    }

exit:
    return err;
}

WEAVE_ERROR
EventProcessor::MapReceivedMaskToPublishedMask(const uint64_t &inReceivedMask,
                                               uint64_t &inOutPublishedMask)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    const uint8_t numFields = 6;
    const uint64_t headerFields[numFields] =
    {
        EventHeaderFieldPresenceMask_RelatedImportance,
        EventHeaderFieldPresenceMask_RelatedId,
        EventHeaderFieldPresenceMask_UTCTimestamp,
        EventHeaderFieldPresenceMask_SystemTimestamp,
        EventHeaderFieldPresenceMask_DeltaUTCTime,
        EventHeaderFieldPresenceMask_DeltaSystemTime
    };
    const uint64_t receivedHeaderFields[numFields] =
    {
        ReceivedEventHeaderFieldPresenceMask_RelatedImportance,
        ReceivedEventHeaderFieldPresenceMask_RelatedId,
        ReceivedEventHeaderFieldPresenceMask_UTCTimestamp,
        ReceivedEventHeaderFieldPresenceMask_SystemTimestamp,
        ReceivedEventHeaderFieldPresenceMask_DeltaUTCTime,
        ReceivedEventHeaderFieldPresenceMask_DeltaSystemTime
    };

    inOutPublishedMask = 0;

    for (uint8_t i = 0; i < numFields; i++)
    {
        if ((inReceivedMask & receivedHeaderFields[i]) != 0)
        {
            inOutPublishedMask |= headerFields[i];
        }
    }

    return err;
}

WEAVE_ERROR
EventProcessor::UpdateContextQualifyHeader(EventHeader &inOutEventHeader,
                                           StreamParsingContext &inOutContext,
                                           uint64_t inReceivedMask)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    err = MapReceivedMaskToPublishedMask(inReceivedMask, inOutEventHeader.mPresenceMask);
    SuccessOrExit(err);

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_Source) == 0)
    {
        // No source present.  Use our default.
        inOutEventHeader.mSource = inOutContext.mPublisherSourceId;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_Importance) != 0)
    {
        // Importance is present.
        inOutContext.mCurrentEventImportance = inOutEventHeader.mImportance;
    }
    else
    {
        // No importance is present.
        inOutEventHeader.mImportance = inOutContext.mCurrentEventImportance;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_Id) != 0)
    {
        // Event ID is present.
        inOutContext.mCurrentEventId = inOutEventHeader.mId;
    }
    else
    {
        // No event ID is present.
        inOutEventHeader.mId = ++inOutContext.mCurrentEventId;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_Type) != 0)
    {
        // Event type is present.
        inOutContext.mCurrentEventType = inOutEventHeader.mType;
    }
    else
    {
        // No event type is present.
        inOutEventHeader.mType = inOutContext.mCurrentEventType;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_TraitInstanceId) == 0)
    {
        // No trait instance ID was present, default is 0.
        inOutEventHeader.mTraitInstanceId = 0;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_SystemTimestamp) != 0)
    {
        // System timestamp is present, save it off.
        inOutContext.mCurrentSystemTimestamp = inOutEventHeader.mSystemTimestamp;
    }
    else if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_DeltaSystemTime) != 0)
    {
        // No system timestamp present, but delta is, so system
        // timestamp is our saved value plus any delta.
        inOutEventHeader.mSystemTimestamp = inOutContext.mCurrentSystemTimestamp + inOutEventHeader.mDeltaSystemTime;

        // Update mCurrentSystemTimestamp.
        inOutContext.mCurrentSystemTimestamp = inOutEventHeader.mSystemTimestamp;
    }

    if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_UTCTimestamp) != 0)
    {
        // UTC timestamp is present, save it off.
        inOutContext.mCurrentUTCTimestamp = inOutEventHeader.mUTCTimestamp;
    }
    else if ((inReceivedMask & ReceivedEventHeaderFieldPresenceMask_DeltaUTCTime) != 0)
    {
        // No UTC timestamp present, but delta is, so system timestamp
        // is our saved value plus any delta.
        inOutEventHeader.mUTCTimestamp = inOutContext.mCurrentUTCTimestamp + inOutEventHeader.mDeltaUTCTime;

        // Update mCurrentUTCTimestamp.
        inOutContext.mCurrentUTCTimestamp = inOutEventHeader.mUTCTimestamp;
    }

exit:
    return err;
}

WEAVE_ERROR
EventProcessor::UpdateGapDetection(const EventHeader &inEventHeader)
{
    // If any event was ever received for that importance
    if (mLastEventId[inEventHeader.mImportance] != 0)
    {
        if (inEventHeader.mId != (mLastEventId[inEventHeader.mImportance] + 1))
        {
            WeaveLogDetail(DataManagement, "EventProcessor found gap for importance: %u (0x%" PRIx32 " -> 0x%" PRIx64 ") NodeId=0x%" PRIx64,
                inEventHeader.mImportance,
                mLastEventId[inEventHeader.mImportance],
                inEventHeader.mId,
                inEventHeader.mSource);
            GapDetected(inEventHeader);
        }

        mLastEventId[inEventHeader.mImportance] = inEventHeader.mId;
    }
    else
    {
        WeaveLogDetail(DataManagement, "EventProcessor stream for importance: %u initialized with id: 0x%" PRIx64 , inEventHeader.mImportance, inEventHeader.mId);
        mLastEventId[inEventHeader.mImportance] = inEventHeader.mId;
    }

    return WEAVE_NO_ERROR;
}

WEAVE_ERROR
ReadAndCheckPresence(nl::Weave::TLV::TLVReader &inReader,
                     uint64_t &inOutReceivedMask,
                     const uint64_t &inReceivedFieldFlag,
                     uint64_t &inOutValue)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    // Check if this tag has appeared before.
    VerifyOrExit((inOutReceivedMask & inReceivedFieldFlag) == 0, err = WEAVE_ERROR_INVALID_TLV_TAG);

    // The only two types we should see here.
    VerifyOrExit((inReader.GetType() == nl::Weave::TLV::kTLVType_UnsignedInteger) ||
                 (inReader.GetType() == nl::Weave::TLV::kTLVType_SignedInteger),
                 err = WEAVE_ERROR_WRONG_TLV_TYPE);

    err = inReader.Get(inOutValue);
    SuccessOrExit(err);

    inOutReceivedMask |= inReceivedFieldFlag;

exit:
    return err;
}

#endif // WEAVE_CONFIG_SERIALIZATION_ENABLE_DESERIALIZATION

} // WeaveMakeManagedNamespaceIdentifier(DataManagement, kWeaveManagedNamespaceDesignation_Current)
} // Profiles
} // Weave
} // nl
