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
 *      This file implements objects commonly used for the
 *      processing of Weave messages.
 *
 */

#include <Weave/Core/WeaveCore.h>
#include <Weave/Core/WeaveMessageLayer.h>
#include <Weave/Profiles/ProfileCommon.h>
#include <Weave/Profiles/service-directory/ServiceDirectory.h>

using namespace ::nl::Weave;
using namespace ::nl::Weave::TLV;
using namespace ::nl::Weave::Profiles;
using namespace ::nl::Weave::Profiles::Common;

/*
 *-------------------- definitions for the message iterator --------------------
 *
 * here's the constructor method.
 * parameter: PacketBuffer *aBuffer, a message buffer to iterate over
 */

MessageIterator::MessageIterator(PacketBuffer *aBuffer)
{
    Retain(aBuffer);

    thePoint = aBuffer->Start();
}

/*
 * parameter: uint8_t &aDestination, a place to put a byte read off the buffer
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::readByte(uint8_t *aDestination)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasData(1))
    {
        err = WEAVE_NO_ERROR;

        *aDestination = READBYTE(thePoint);
    }

    return err;
}

/*
 * parameter: uint16_t *aDestination, a place to put a short read off the buffer
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::read16(uint16_t *aDestination)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasData(2))
    {
        err = WEAVE_NO_ERROR;

        READ16(thePoint, *aDestination);
    }

    return err;
}

/*
 * parameter: uint32_t *aDestination, a place to put an 32-bit value
 * read off the buffer
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::read32(uint32_t *aDestination)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasData(4))
    {
        err = WEAVE_NO_ERROR;

        READ32(thePoint, *aDestination);
    }

    return err;
}

/*
 * parameter: uint64_t *aDestination, a place to put an 64-bit value
 * read off the buffer
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::read64(uint64_t *aDestination)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    uint8_t *p;

    if (hasData(8))
    {
        err = WEAVE_NO_ERROR;
        p = (uint8_t *)aDestination;

        for (int i = 0; i < 8; i++)
            readByte(p++);
    }

    return err;
}

/*
 * parameters:
 * - uint16_t aLength, the length of the string to be read
 * - char* aString, a place to put the string
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::readString(uint16_t aLength, char* aString)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasData(aLength))
    {
        err = WEAVE_NO_ERROR;

        for (uint16_t i = 0; i < aLength; i++)
        {
            *aString = READBYTE(thePoint);
            aString++;
        }
    }

    return err;
}

/*
 * parameters:
 * - uint16_t aLength, the length of the byte string to be read
 * - uint8_t* aByteString, a place to put the bytes
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::readBytes(uint16_t aLength, uint8_t* aByteString)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasData(aLength))
    {
        err = WEAVE_NO_ERROR;

        for (uint16_t i = 0; i < aLength; i++)
        {
            *aByteString = READBYTE(thePoint);
            aByteString++;
        }
    }

    return err;
}

/*
 * parameters: - uint8_t aValue, a byte value to write out
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::writeByte(uint8_t aValue)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasRoom(1))
    {
        err = WEAVE_NO_ERROR;

        WRITEBYTE(thePoint, aValue);
        finishWriting();
    }

    return err;
}

/*
 * parameters: - uint16_t aValue, a short value to write out
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::write16(uint16_t aValue)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasRoom(2))
    {
        err = WEAVE_NO_ERROR;

        WRITE16(thePoint, aValue);
        finishWriting();
    }

    return err;
}

/*
 * parameters: - uint32_t aValue, a 32-bit value to write out
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::write32(uint32_t aValue)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasRoom(4))
    {
        err = WEAVE_NO_ERROR;

        WRITE32(thePoint, aValue);
        finishWriting();
    }

    return err;
}

/*
 * parameters: - uint64_t aValue, a 64-bit value to write out
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::write64(uint64_t aValue)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;
    uint8_t *p = (uint8_t *)&aValue;

    if (hasRoom(8))
    {
        err = WEAVE_NO_ERROR;

        for (int i = 0; i < 8; i++)
            writeByte(*p++);
    }

    return err;
}

/*
 * parameters:
 * - uint16_t aLength, the length of the string to write
 * - char *aString, the string itself
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::writeString(uint16_t aLength, char *aString)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasRoom(aLength))
    {
        err = WEAVE_NO_ERROR;

        for (uint16_t i = 0; i < aLength; i++)
        {
            WRITEBYTE(thePoint, *aString);
            aString++;
        }
        finishWriting();
    }

    return err;
}

/*
 * parameters:
 * - uint16_t aLength, the length of the byte string to write
 * - char *aByteString, the byte string  itself
 * return:
 * - WEAVE_NO_ERROR if it's all OK
 * - WEAVE_ERROR_BUFFER_TOO_SMALL if we're running past the end of the buffer
 */

WEAVE_ERROR MessageIterator::writeBytes(uint16_t aLength, uint8_t *aByteString)
{
    WEAVE_ERROR err = WEAVE_ERROR_BUFFER_TOO_SMALL;

    if (hasRoom(aLength))
    {
        err = WEAVE_NO_ERROR;

        for (uint16_t i = 0; i < aLength; i++)
        {
            WRITEBYTE(thePoint, *aByteString);
            aByteString++;
        }
        finishWriting();
    }

    return err;
}

/*
 * increment a message iterator by 1 if there's room
 */

MessageIterator& MessageIterator::operator ++(void)
{
    if (hasRoom(1))
        ++thePoint;

    return *this;
}

/*
 * parameter: uint16_t inc, an increment to apply to the message iterator
 * return: the iterator incremented either by the given increment, if there's
 * room or else slammed right up against the end if there's not.
 */

MessageIterator& MessageIterator::operator +(uint16_t inc)
{
    if (hasRoom(inc))
        thePoint += inc;

    else
        thePoint += mBuffer->AvailableDataLength();

    return *this;
}

/*
 *
 * parameter: uint16_t dec, an decrement to apply to the message iterator
 * return: the iterator either decremented by the given value if there's
 * room or else slammed right up against the beginning if there's not.
 */

MessageIterator& MessageIterator::operator -(uint16_t dec)
{
    if (mBuffer->DataLength() > dec)
        thePoint -= dec;
    else
        thePoint = mBuffer->Start();
    return *this;
}

/*
 * parameter: const MessageIterator& another, another message iterator to compare with
 */

bool MessageIterator::operator==(const MessageIterator &aMessageIterator)
{
    return(thePoint == aMessageIterator.thePoint && mBuffer == aMessageIterator.mBuffer);
}

/*
 * parameter: const MessageIterator &aMessageIterator, another message iterator to compare with
 */

bool MessageIterator::operator!=(const MessageIterator &aMessageIterator)
{
    return !(thePoint == aMessageIterator.thePoint && mBuffer == aMessageIterator.mBuffer);
}

/*
 * return: what we're looking at in the buffer
 */

uint8_t& MessageIterator::operator *(void)
{
    return *thePoint;
}

/*
 * set the point to after any data currently in the buffer.
 */

void MessageIterator::append(void)
{
    thePoint = mBuffer->Start() + mBuffer->DataLength();
}

/*
 * parameter: uint16_t inc, an integer amount that may be read from the
 * buffer.
 * return: true if the buffer's current data length is greater than or equal
 * to the given increment. false otherwise.
 */

bool MessageIterator::hasData(uint16_t inc)
{
    return inc <= (mBuffer->DataLength());
}

/*
 * parameter: uint16_t inc, an integer amount that may be written to the
 * buffer.
 * return: true if the difference between the buffer's current data
 * length and its maximum allowable data length, i.e. its available data
 * length,  is less than or equal to the given increment.
 */

bool MessageIterator::hasRoom(uint16_t inc)
{
    return inc <= (mBuffer->AvailableDataLength());
}

/*
 * adjust the buffer after writing.
 */

void MessageIterator::finishWriting(void)
{
    mBuffer->SetDataLength((uint16_t)(thePoint - mBuffer->Start()));
}

/*
 *-------------------- definitions for referenced strings --------------------
 *
 * the no-arg constructor for referenced strings.
 */

ReferencedString::ReferencedString(void) :
    RetainedPacketBuffer()
{
    theLength = 0;
    theString = NULL;
    isShort = false;
}

/*
 * for initializing these things, we have a couple of choices. the intention is
 * that, when you parse one of these things, there's a message buffer available
 * and that's where the string data should be. BUT, if you're making one of these
 * in order to send it then it's silly and wasteful to require that we set aside
 * a message buffer and stuff a string into it in order to do this. so, there are
 * two initializers. the first one is the one we use if there's actually a message
 * buffer.
 * parameters:
 * - uint16_t aLength, a length for the referenced string
 * - char *aString, a pointer to the string data (in the buffer)
 * - PacketBuffer *aBuffer, a messsage buffer in which the string resides
 * return:
 * - WEAVE_NO_ERROR if AOK
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if... uhhh the string length is invalid
 */

WEAVE_ERROR ReferencedString::init(uint16_t aLength, char *aString, PacketBuffer *aBuffer)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aLength > (aBuffer->AvailableDataLength() - aBuffer->DataLength()))
        err = WEAVE_ERROR_INVALID_STRING_LENGTH;

    else
    {
        Retain(aBuffer);

        theLength = aLength;
        theString = aString;
        isShort = false;
    }

    return err;
}

/*
 * this initializer is the one we use if there's no message buffer because we're
 * creating one of these to send. the string data here can really come from
 * anywhere.
 * NOTE!!! if the string passed in here is stack-allocated, any outgoing message
 * created in this way must be sent before the stack context in which it was
 * created is exited.
 * parameters:
 * - uint16_t aLength, a length for the referenced string
 * - char *aString, a pointer to the string data
 * return:
 * - WEAVE_NO_ERROR if AOK
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if... uhhh the string length is invalid
 */

WEAVE_ERROR ReferencedString::init(uint16_t aLength, char *aString)
{
    theLength = aLength;
    theString = aString;

    Release();

    isShort = false;

    return WEAVE_NO_ERROR;
}

/*
 * and now we have the same thing for a single-byte length.
 * parameters:
 * - uint8_t aLength, a length for the referenced string
 * - char *aString, a pointer to the string data (in the buffer)
 * - PacketBuffer *aBuffer, a messsage buffer in which the string resides
 * return:
 * - WEAVE_NO_ERROR if AOK
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if... uhhh the string length is invalid
 */

WEAVE_ERROR ReferencedString::init(uint8_t aLength, char *aString, PacketBuffer *aBuffer)
{
    if (aLength > (aBuffer->AvailableDataLength() - aBuffer->DataLength())) return WEAVE_ERROR_INVALID_STRING_LENGTH;

    Retain(aBuffer);

    theLength = (uint16_t)aLength;
    theString = aString;
    isShort = true;

    return WEAVE_NO_ERROR;
}

/*
 * parameters:
 * - uint8_t aLength, a length for the referenced string
 * - char *aString, a pointer to the string data
 * return:
 * - WEAVE_NO_ERROR if AOK
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if... uhhh the string length is invalid
 */

WEAVE_ERROR ReferencedString::init(uint8_t aLength, char *aString)
{
    theLength = (uint16_t)aLength;
    theString = aString;

    Release();

    isShort = true;

    return WEAVE_NO_ERROR;
}

/*
 * parameter: MessageIterator &i, an iterator over the message being packed.
 * return: error/status
 */

WEAVE_ERROR ReferencedString::pack(MessageIterator &i)
{
    WEAVE_ERROR e;

    if (isShort)
        e = i.writeByte((uint8_t)theLength);

    else
        e = i.write16(theLength);

    if (e == WEAVE_NO_ERROR)
        e = i.writeString(theLength, theString);

    return e;
}

/*
 * parameters:
 * - MessageIterator &i, an iterator over the message being parsed.
 * - ReferencedString &aString, a place to put the result of parsing
 * return:
 * - WEAVE_NO_ERROR if it's all good
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if the string is too long for the buffer
 * (this should never happen).
 */

WEAVE_ERROR ReferencedString::parse(MessageIterator &i, ReferencedString &aString)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    uint16_t len = 0;

    if (aString.isShort)
        i.readByte((uint8_t *)&len);

    else
        i.read16(&len);

    if (i.hasRoom(len))
    {
        aString.theLength = len;
        aString.theString = (char *)i.thePoint;
        aString.Retain(i.GetBuffer());

        // we need to skip over the string

        (i.thePoint) += len;
    }

    else
        err = WEAVE_ERROR_INVALID_STRING_LENGTH;

    return err;
}

/*
 * parameter: ReferencedString &another, a string to check against
 * return: true if they're equal, false otherwise
 */

bool ReferencedString::operator== (const ReferencedString &aReferencedString) const
{
    bool result = false;

    if (theLength == aReferencedString.theLength)
    {
        for (int i = 0; i < theLength; i++)
        {
            if (theString[i] != aReferencedString.theString[i])
                goto exit;
        }

        result = true;
    }

exit:

    return result;
}

/*
 * return: a printable string
 */

char *ReferencedString::printString(void)
{
    theString[theLength] = 0;

    return theString;
}

/*
 *-------------------- definitions for TLV data --------------------
 */

/**
 * The no-arg constructor for TLV data. Delivers a free/uninitialized
 * object which must be subjected to one of the init() methods defined
 * here in order to be useful.
 */

ReferencedTLVData::ReferencedTLVData(void) :
    RetainedPacketBuffer()
{
    theLength = 0;
    theMaxLength = 0;
    theData = NULL;
    theWriteCallback = NULL;
    theAppState = NULL;
}

/**
 * Initialize a ReferencedTLVData object given a buffer full of
 * TLV. This assumes that the buffer ONLY contains TLV.
 *
 * @param [in] PacketBuffer *aBuffer, a message buffer in which the TLV resides.
 *
 * @return WEAVE_NO_ERROR
 */

WEAVE_ERROR ReferencedTLVData::init(PacketBuffer *aBuffer)
{
    Retain(aBuffer);

    theData = mBuffer->Start();
    theLength = mBuffer->DataLength();
    theMaxLength = mBuffer->MaxDataLength();

    theWriteCallback = NULL;
    theAppState = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Initialize a ReferencedTLVData object given a MessageIterator. In
 * this case, the TV is that last portion of the buffer and we pass in
 * a message iterator that's pointing to it.
 *
 * @param [in] MessageIterator &i , A message iterator pointing to TLV
 * to be extracted.
 *
 * @return: WEAVE_NO_ERROR
*/

WEAVE_ERROR ReferencedTLVData::init(MessageIterator &i)
{
    System::PacketBuffer *theBuffer = i.GetBuffer();

    Retain(theBuffer);

    theData = i.thePoint;
    theLength = theBuffer->DataLength() - (i.thePoint - mBuffer->Start());
    theMaxLength = theBuffer->MaxDataLength();

    theWriteCallback = NULL;
    theAppState = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Initialize ReferencedTLVData object with a byte string containing
 * TLV. This initializer is the one we use if there's no inet buffer
 * because we're creating one of these to pack and send.
 *
 * NOTE!!! if the string passed in here is stack-allocated, any
 * outgoing message created in this way must be sent before the stack
 * context in which it was created is exited.
 *
 * - uint16_t aLength, a length for the TLV data
 * - uint16_t aMaxLength, the total length of the buffer
 * - uint8_t *aByteString, a pointer to the string data
 * return:
 * - WEAVE_NO_ERROR if AOK
 * - WEAVE_ERROR_INVALID_STRING_LENGTH if... uhhh the string length is invalid
 */

WEAVE_ERROR ReferencedTLVData::init(uint16_t aLength, uint16_t aMaxLength, uint8_t *aByteString)
{
    theLength = aLength;
    theMaxLength = aMaxLength;
    theData = aByteString;

    Release();

    theWriteCallback = NULL;
    theAppState = NULL;

    return WEAVE_NO_ERROR;
}

/**
 * Initialize a ReferencedTLVData object. Instead of explicitly
 * supplying the data, this version provides function, the write
 * callback, and a reference object, which will be passed to it, along
 * with a TLVWriter object, when the referenced data is supposed to be
 * packed and sent. The signature of that callback is:
 *
 *   typedef void (*TLVWriteCallback)(TLV::TLVWriter &aWriter, void *aAppState);
 *
 * @param [in] TLVWriteCallback aWriteCallback, the function to be
 * called when it's time to write some TLV.
 *
 * @param [in] void *anAppState, an application state object to be
 * passed to the callback along with the writer.
 *
 * @return WEAVE_NO_ERROR if all went well, otherwise
 * WEAVE_ERROR_INVALID_ARGUMENT if thw write callback is not supplied.
 */

WEAVE_ERROR ReferencedTLVData::init(TLVWriteCallback aWriteCallback, void *anAppState)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;

    if (aWriteCallback != NULL)
    {
        theWriteCallback = aWriteCallback;
        theAppState = anAppState;
    }

    else
        err = WEAVE_ERROR_INVALID_ARGUMENT;

    theLength = 0;
    theMaxLength = 0;
    theData = NULL;
    mBuffer = NULL;

    return err;
}

/**
 * Free a ReferencedTLVData object, which is to say, undefine it.
 *
 * @return void
 */
void ReferencedTLVData::free(void)
{
    RetainedPacketBuffer::Release();

    /*
     * in this case you have to clear out the write callback and app
     * state as well since that may be how the data is getting generated.
     */

    theWriteCallback = NULL;
    theAppState = NULL;

    // and the rest of it for good measure

    theLength = 0;
    theMaxLength = 0;
    theData = NULL;
}

/**
 * Check if a ReferencedTLVData object is "free", i.e. undefined.
 *
 * @return true if the object is undefined, false otherwise.
 */

bool ReferencedTLVData::isFree(void)
{
    return (mBuffer == NULL && theWriteCallback == NULL && theAppState == NULL);
}

/**
 * Pack a ReferenceDTLVData object using a TLVWriter.
 *
 * @param [in] MessageIterator &i, an iterator over the message being packed.
 *
 * @return a WEAVE_ERROR - WEAVE_NO_ERROR if all goes well, otherwise
 * an error reflecting an inability of the writer to write the
 * relevant bytes. Note that the write callback is not allowed to
 * return an error and so fails silently.
 */

WEAVE_ERROR ReferencedTLVData::pack(MessageIterator &i)
{
    WEAVE_ERROR err = WEAVE_NO_ERROR;
    System::PacketBuffer *theBuffer = i.GetBuffer();
    uint16_t oldDataLength = theBuffer->DataLength();
    TLVWriter writer;

    if (theWriteCallback != NULL)
    {
        theData = i.thePoint;
        writer.Init(theBuffer);
        theWriteCallback(writer, theAppState);
        theLength = theBuffer->DataLength() - oldDataLength;
        i.thePoint += theLength;
    }
    else
        err = i.writeBytes(theLength, theData);

    return err;
}

/**
 * Parse a ReferenceTLVData object from a MessageIterator object
 * assumed to be pointing at the TLV portion of a message.
 *
 * Note that no actual "parsing" is done here since the TLV is left in
 * the buffer and not manipulated at all. This method mainly just sets
 * up the ReferencedTLVData structure for later use.
 *
 * @param [in] MessageIterator &i, an iterator over the message being
 * parsed.
 *
 * @param [out] ReferencedTLVData &aTarget, a place to put the result
 * of parsing.
 *
 * @return WEAVE_NO_ERROR.
 */

WEAVE_ERROR ReferencedTLVData::parse(MessageIterator &i, ReferencedTLVData &aTarget)
{
    PacketBuffer *buff = i.GetBuffer();

    aTarget.Retain(buff);

    aTarget.theLength = buff->DataLength() - (i.thePoint - buff->Start());

    if (aTarget.theLength != 0)
        aTarget.theData = i.thePoint;

    else
        aTarget.theData = NULL;

    // we need to skip over the data

    (i.thePoint) += aTarget.theLength;

    return WEAVE_NO_ERROR;
}

/**
 * Check a ReferencedTLVData object against another for equality.
 *
 * Note that this only really makes sense in the case of two objects
 * that have actual data in them backed by a buffer or string.
 *
 * @param [in] const ReferencedTLVData &another, an object to check against
 *
 * @return true if they're equal, false otherwise
 */

bool ReferencedTLVData::operator== (const ReferencedTLVData &another) const
{
    bool result = false;

    if (theLength == another.theLength)
    {
        for (int i = 0; i < theLength; i++)
        {
            if (theData[i] != another.theData[i])
                goto exit;
        }

        result = true;
    }

exit:

    return result;
}
