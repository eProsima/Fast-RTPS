// Copyright 2016 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * DataMsg.hpp
 *
 */

#include <fastrtps/qos/ParameterTypes.h>

#include <fastdds/core/policy/ParameterSerializer.hpp>
#include <fastdds/core/policy/ParameterList.hpp>

namespace eprosima {
namespace fastrtps {
namespace rtps {

bool RTPSMessageCreator::addMessageData(
        CDRMessage_t* msg,
        GuidPrefix_t& guidprefix,
        const CacheChange_t* change,
        TopicKind_t topicKind,
        const EntityId_t& readerId,
        bool expectsInlineQos,
        InlineQosWriter* inlineQos)
{

    RTPSMessageCreator::addHeader(msg, guidprefix);

    RTPSMessageCreator::addSubmessageInfoTS_Now(msg, false);

    bool is_big_submessage;
    octet* pending_payload = nullptr;
    uint32_t pending_size = 0;
    uint32_t pending_padding = 0;
    RTPSMessageCreator::addSubmessageData(msg, change, topicKind, readerId, expectsInlineQos, inlineQos,
            is_big_submessage, true, pending_payload, pending_size, pending_padding);

    msg->length = msg->pos;

    return true;
}

bool RTPSMessageCreator::addSubmessageData(
        CDRMessage_t* msg,
        const CacheChange_t* change,
        TopicKind_t topicKind,
        const EntityId_t& readerId,
        bool expectsInlineQos,
        InlineQosWriter* inlineQos,
        bool& is_big_submessage,
        bool copy_data,
        octet*& pending_payload,
        uint32_t& pending_size,
        uint32_t& pending_padding)
{
    // Initialize output parameters
    is_big_submessage = false;
    pending_payload = nullptr;
    pending_size = 0;
    pending_padding = 0;

    octet flags = 0x0;
    //Find out flags
    bool dataFlag = false;
    bool keyFlag = false;
    bool inlineQosFlag = false;

    Endianness_t old_endianess = msg->msg_endian;
#if FASTDDS_IS_BIG_ENDIAN_TARGET
    msg->msg_endian = BIGEND;
#else
    flags = flags | BIT(0);
    msg->msg_endian = LITTLEEND;
#endif // if FASTDDS_IS_BIG_ENDIAN_TARGET

    if (change->kind == ALIVE && change->serializedPayload.length > 0 && change->serializedPayload.data != NULL)
    {
        dataFlag = true;
        keyFlag = false;
    }
    else
    {
        dataFlag = false;
        keyFlag = true;
    }
    if (topicKind == NO_KEY)
    {
        keyFlag = false;
    }
    inlineQosFlag = false;
    // cout << "expects inline qos: " << expectsInlineQos << " topic KIND: " << (topicKind == WITH_KEY) << endl;
    if (inlineQos != NULL || expectsInlineQos || change->kind != ALIVE) //expects inline qos
    {
        if (topicKind == WITH_KEY)
        {
            flags = flags | BIT(1);
            inlineQosFlag = true;
            //cout << "INLINE QOS FLAG TO 1 " << endl;
            keyFlag = false;
        }
    }
    // Maybe the inline QoS because a WriteParam.
    else if (change->write_params.related_sample_identity() != SampleIdentity::unknown())
    {
        inlineQosFlag = true;
        flags = flags | BIT(1);
    }

    if (dataFlag)
    {
        flags = flags | BIT(2);
    }
    if (keyFlag)
    {
        flags = flags | BIT(3);
    }

    octet status = 0;
    if (change->kind == NOT_ALIVE_DISPOSED)
    {
        status = status | BIT(0);
    }
    if (change->kind == NOT_ALIVE_UNREGISTERED)
    {
        status = status | BIT(1);
    }
    if (change->kind == NOT_ALIVE_DISPOSED_UNREGISTERED)
    {
        status = status | BIT(0);
        status = status | BIT(1);
    }

    bool added_no_error = true;

    // Submessage header.
    CDRMessage::addOctet(msg, DATA);
    CDRMessage::addOctet(msg, flags);
    uint32_t submessage_size_pos = msg->pos;
    uint16_t submessage_size = 0;
    CDRMessage::addUInt16(msg, submessage_size);
    uint32_t position_size_count_size = msg->pos;

    //extra flags. not in this version.
    added_no_error &= CDRMessage::addUInt16(msg, 0);
    //octet to inline Qos is 12, may change in future versions
    added_no_error &= CDRMessage::addUInt16(msg, RTPSMESSAGE_OCTETSTOINLINEQOS_DATASUBMSG);
    //Entity ids
    added_no_error &= CDRMessage::addEntityId(msg, &readerId);
    added_no_error &= CDRMessage::addEntityId(msg, &change->writerGUID.entityId);
    //Add Sequence Number
    added_no_error &= CDRMessage::addSequenceNumber(msg, &change->sequenceNumber);
    //Add INLINE QOS AND SERIALIZED PAYLOAD DEPENDING ON FLAGS:


    if (inlineQosFlag) //inlineQoS
    {
        if (change->write_params.related_sample_identity() != SampleIdentity::unknown())
        {
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_sample_identity(msg,
                    change->write_params.related_sample_identity());
        }

        if (topicKind == WITH_KEY)
        {
            //cout << "ADDDING PARAMETER KEY " << endl;
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_key(msg, change->instanceHandle);
        }

        if (change->kind != ALIVE)
        {
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_status(msg, status);
        }

        if (inlineQos != nullptr)
        {
            inlineQos->writeQosToCDRMessage(msg);
        }

        fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_sentinel(msg);
    }

    //Add Serialized Payload
    if (dataFlag)
    {
        if (copy_data)
        {
            added_no_error &=
                    CDRMessage::addData(msg, change->serializedPayload.data, change->serializedPayload.length);
        }
        else if (msg->pos + change->serializedPayload.length > msg->max_size)
        {
            return false;
        }
        else
        {
            pending_payload = change->serializedPayload.data;
            pending_size = change->serializedPayload.length;
            msg->pos += pending_size;
        }
    }

    if (keyFlag)
    {
        added_no_error &= CDRMessage::addOctet(msg, 0); //ENCAPSULATION
        if (msg->msg_endian == BIGEND)
        {
            added_no_error &= CDRMessage::addOctet(msg, PL_CDR_BE); //ENCAPSULATION
        }
        else
        {
            added_no_error &= CDRMessage::addOctet(msg, PL_CDR_LE); //ENCAPSULATION
        }

        added_no_error &= CDRMessage::addUInt16(msg, 0); //ENCAPSULATION OPTIONS
        added_no_error &=
                fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_key(msg,
                        change->instanceHandle);
        added_no_error &= fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_status(msg, status);
        added_no_error &= fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_sentinel(msg);
    }

    // Align submessage to rtps alignment (4).
    uint32_t align = (4 - msg->pos % 4) & 3;
    if (copy_data)
    {
        for (uint32_t count = 0; count < align; ++count)
        {
            added_no_error &= CDRMessage::addOctet(msg, 0);
        }
    }
    else
    {
        pending_padding = align;
        msg->pos += align;
    }

    uint32_t size32 = msg->pos - position_size_count_size;
    if (size32 <= std::numeric_limits<uint16_t>::max())
    {
        submessage_size = static_cast<uint16_t>(size32);
        octet* o = reinterpret_cast<octet*>(&submessage_size);
        if (msg->msg_endian == DEFAULT_ENDIAN)
        {
            msg->buffer[submessage_size_pos] = *(o);
            msg->buffer[submessage_size_pos + 1] = *(o + 1);
        }
        else
        {
            msg->buffer[submessage_size_pos] = *(o + 1);
            msg->buffer[submessage_size_pos + 1] = *(o);
        }
    }
    else
    {
        // Submessage > 64KB
        is_big_submessage = true;
    }

    // Rewind position when not copying data
    if (!copy_data)
    {
        msg->pos -= pending_padding;
        msg->pos -= pending_size;
    }

    msg->msg_endian = old_endianess;

    return added_no_error;
}

bool RTPSMessageCreator::addMessageDataFrag(
        CDRMessage_t* msg,
        GuidPrefix_t& guidprefix,
        const CacheChange_t* change,
        uint32_t fragment_number,
        TopicKind_t topicKind,
        const EntityId_t& readerId,
        bool expectsInlineQos,
        InlineQosWriter* inlineQos)
{
    RTPSMessageCreator::addHeader(msg, guidprefix);

    RTPSMessageCreator::addSubmessageInfoTS_Now(msg, false);

    // Calculate fragment start
    uint32_t fragment_start = change->getFragmentSize() * (fragment_number - 1);
    // Calculate fragment size. If last fragment, size may be smaller
    uint32_t fragment_size = fragment_number < change->getFragmentCount() ? change->getFragmentSize() :
            change->serializedPayload.length - fragment_start;

    // TODO (Ricardo). Check to create special wrapper.
    SerializedPayload_t payload;
    payload.data = change->serializedPayload.data + fragment_start;
    payload.length = fragment_size;

    octet* pending_payload = nullptr;
    uint32_t pending_size = 0;
    uint32_t pending_padding = 0;
    RTPSMessageCreator::addSubmessageDataFrag(msg, change, fragment_number, payload,
            topicKind, readerId, expectsInlineQos, inlineQos, false, pending_payload, pending_size, pending_padding);

    payload.data = NULL;

    msg->length = msg->pos;
    return true;
}

bool RTPSMessageCreator::addSubmessageDataFrag(
        CDRMessage_t* msg,
        const CacheChange_t* change,
        uint32_t fragment_number,
        const SerializedPayload_t& payload,
        TopicKind_t topicKind,
        const EntityId_t& readerId,
        bool expectsInlineQos,
        InlineQosWriter* inlineQos,
        bool copy_data,
        octet*& pending_payload,
        uint32_t& pending_size,
        uint32_t& pending_padding)
{
    // Initialize output parameters
    pending_payload = nullptr;
    pending_size = 0;
    pending_padding = 0;

    octet flags = 0x0;
    //Find out flags
    bool keyFlag = false;
    bool inlineQosFlag = false;

    Endianness_t old_endianess = msg->msg_endian;
#if FASTDDS_IS_BIG_ENDIAN_TARGET
    msg->msg_endian = BIGEND;
#else
    flags = flags | BIT(0);
    msg->msg_endian = LITTLEEND;
#endif // if FASTDDS_IS_BIG_ENDIAN_TARGET

    if (change->kind == ALIVE && payload.length > 0 && payload.data != NULL)
    {
        keyFlag = false;
    }
    else
    {
        keyFlag = true;
    }

    if (topicKind == NO_KEY)
    {
        keyFlag = false;
    }

    // cout << "expects inline qos: " << expectsInlineQos << " topic KIND: " << (topicKind == WITH_KEY) << endl;
    if (inlineQos != NULL || expectsInlineQos || change->kind != ALIVE) //expects inline qos
    {
        if (topicKind == WITH_KEY)
        {
            flags = flags | BIT(1);
            inlineQosFlag = true;
            //cout << "INLINE QOS FLAG TO 1 " << endl;
            keyFlag = false;
        }
    }
    // Maybe the inline QoS because a WriteParam.
    else if (change->write_params.related_sample_identity() != SampleIdentity::unknown())
    {
        inlineQosFlag = true;
        flags = flags | BIT(1);
    }

    if (keyFlag)
    {
        flags = flags | BIT(2);
    }

    octet status = 0;
    if (change->kind == NOT_ALIVE_DISPOSED)
    {
        status = status | BIT(0);
    }
    if (change->kind == NOT_ALIVE_UNREGISTERED)
    {
        status = status | BIT(1);
    }

    if (change->kind == NOT_ALIVE_DISPOSED_UNREGISTERED)
    {
        status = status | BIT(0);
        status = status | BIT(1);
    }

    bool added_no_error = true;

    // Submessage header.
    CDRMessage::addOctet(msg, DATA_FRAG);
    CDRMessage::addOctet(msg, flags);
    uint32_t submessage_size_pos = msg->pos;
    uint16_t submessage_size = 0;
    CDRMessage::addUInt16(msg, submessage_size);
    uint32_t position_size_count_size = msg->pos;

    //extra flags. not in this version.
    added_no_error &= CDRMessage::addUInt16(msg, 0);

    //octet to inline Qos is 28, may change in future versions
    added_no_error &= CDRMessage::addUInt16(msg, RTPSMESSAGE_OCTETSTOINLINEQOS_DATAFRAGSUBMSG);

    //Entity ids
    added_no_error &= CDRMessage::addEntityId(msg, &readerId);
    added_no_error &= CDRMessage::addEntityId(msg, &change->writerGUID.entityId);

    //Add Sequence Number
    added_no_error &= CDRMessage::addSequenceNumber(msg, &change->sequenceNumber);

    // Add fragment starting number
    added_no_error &= CDRMessage::addUInt32(msg, fragment_number); // fragments start in 1

    // Add fragments in submessage
    added_no_error &= CDRMessage::addUInt16(msg, 1); // we are sending one fragment

    // Add fragment size
    added_no_error &= CDRMessage::addUInt16(msg, change->getFragmentSize());

    // Add total sample size
    added_no_error &= CDRMessage::addUInt32(msg, change->serializedPayload.length);

    //Add INLINE QOS AND SERIALIZED PAYLOAD DEPENDING ON FLAGS:
    if (inlineQosFlag) //inlineQoS
    {
        if (change->write_params.related_sample_identity() != SampleIdentity::unknown())
        {
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_sample_identity(msg,
                    change->write_params.related_sample_identity());
        }

        if (topicKind == WITH_KEY)
        {
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_key(msg, change->instanceHandle);
        }

        if (change->kind != ALIVE)
        {
            fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_status(msg, status);
        }

        if (inlineQos != nullptr)
        {
            inlineQos->writeQosToCDRMessage(msg);
        }

        fastdds::dds::ParameterSerializer<Parameter_t>::add_parameter_sentinel(msg);
    }

    //Add Serialized Payload XXX TODO
    if (!keyFlag) // keyflag = 0 means that the serializedPayload SubmessageElement contains the serialized Data
    {
        if (copy_data)
        {
            added_no_error &= CDRMessage::addData(msg, payload.data, payload.length);
        }
        else if (msg->pos + payload.length > msg->max_size)
        {
            return false;
        }
        else
        {
            pending_payload = payload.data;
            pending_size = payload.length;
            msg->pos += pending_size;
        }
    }
    else
    {
        // keyflag = 1 means that the serializedPayload SubmessageElement contains the serialized Key
        /*
            added_no_error &= CDRMessage::addOctet(&submsgElem, 0); //ENCAPSULATION
            if (submsgElem.msg_endian == BIGEND)
            added_no_error &= CDRMessage::addOctet(&submsgElem, PL_CDR_BE); //ENCAPSULATION
            else
            added_no_error &= CDRMessage::addOctet(&submsgElem, PL_CDR_LE); //ENCAPSULATION

            added_no_error &= CDRMessage::addUInt16(&submsgElem, 0); //ENCAPSULATION OPTIONS
            added_no_error &= Parameter_t::addParameterKey(&submsgElem, &change->instanceHandle);
            added_no_error &= Parameter_t::addParameterStatus(&submsgElem, status);
            added_no_error &= Parameter_t::addParameterSentinel(&submsgElem);
         */
        msg->msg_endian = old_endianess;
        return false;
    }

    // TODO(Ricardo) This should be on cachechange.
    // Align submessage to rtps alignment (4).
    submessage_size = uint16_t(msg->pos - position_size_count_size);
    for (; 0 != (submessage_size & 3); ++submessage_size)
    {
        if (copy_data)
        {
            added_no_error &= CDRMessage::addOctet(msg, 0);
        }
        else
        {
            ++pending_padding;
        }
    }

    if (!copy_data)
    {
        msg->pos -= pending_size;
    }

    //TODO(Ricardo) Improve.
    octet* o = reinterpret_cast<octet*>(&submessage_size);
    if (msg->msg_endian == DEFAULT_ENDIAN)
    {
        msg->buffer[submessage_size_pos] = *(o);
        msg->buffer[submessage_size_pos + 1] = *(o + 1);
    }
    else
    {
        msg->buffer[submessage_size_pos] = *(o + 1);
        msg->buffer[submessage_size_pos + 1] = *(o);
    }

    msg->msg_endian = old_endianess;

    return added_no_error;
}

} // namespace rtps
} // namespace fastrtps
} // namespace eprosima
