// Copyright 2020 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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
 * @file SharedDumpFunctions.cpp
 *
 */

#include <json.hpp>

#include <fastdds/rtps/common/CacheChange.h>
#include <rtps/builtin/discovery/database/backup/SharedBackupFunctions.hpp>

namespace eprosima {
namespace fastdds {
namespace rtps {
namespace ddb {

using json = nlohmann::json;

void to_json(
        json& j,
        const eprosima::fastrtps::rtps::CacheChange_t& change)
{
    j["kind"] = change.kind;
    j["writer_GUID"] = object_to_string(change.writerGUID);
    j["instance_handle"] = object_to_string(change.instanceHandle);
    j["sequence_number"] = object_to_string(change.sequenceNumber);
    j["isRead"] = change.isRead;
    j["source_timestamp"] = object_to_string(change.sourceTimestamp);
    j["reception_timestamp"] = object_to_string(change.reader_info.receptionTimestamp);
    j["sample_identity"] = object_to_string(change.write_params.sample_identity());
    j["related_sample_identity"] = object_to_string(change.write_params.related_sample_identity());

    // serialize payload
    j["serialized_payload"]["encapsulation"] = change.serializedPayload.encapsulation;
    j["serialized_payload"]["length"] = change.serializedPayload.length;
    j["serialized_payload"]["data"] = b64encode(change.serializedPayload.data, change.serializedPayload.length);
}

void from_json(
        const json& j,
        eprosima::fastrtps::rtps::CacheChange_t& change)
{
    change.kind = static_cast<fastrtps::rtps::ChangeKind_t>(j["kind"].get<uint8_t>());
    std::istringstream(j["writer_GUID"].get<std::string>()) >> change.writerGUID;
    std::istringstream(j["instance_handle"].get<std::string>()) >> change.instanceHandle;
    std::istringstream(j["sequence_number"].get<std::string>()) >> change.sequenceNumber;
    change.isRead = static_cast<fastrtps::rtps::ChangeKind_t>(j["isRead"].get<bool>());
    std::istringstream(j["source_timestamp"].get<std::string>()) >> change.sourceTimestamp;
    std::istringstream(j["reception_timestamp"].get<std::string>()) >> change.reader_info.receptionTimestamp;

    // set sample identity
    fastrtps::rtps::SampleIdentity si;
    std::istringstream(j["sample_identity"].get<std::string>()) >> si;
    change.write_params.sample_identity(si);
    change.write_params.related_sample_identity(si);

    // set related sample identity
    fastrtps::rtps::SampleIdentity rsi;
    std::istringstream(j["related_sample_identity"].get<std::string>()) >> rsi;
    change.write_params.sample_identity(rsi);
    change.write_params.related_sample_identity(rsi);

    // deserialize SerializedPayload
    change.serializedPayload.encapsulation = j["serialized_payload"]["encapsulation"];
    change.serializedPayload.length = j["serialized_payload"]["length"];
    b64decode(change.serializedPayload.data, j["serialized_payload"]["data"].get<std::string>());
}

// stack overflow
// @polfosol-ఠ-ఠ
// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c/37109258#37109258

// Array to convert binary to b64
// Each 6 bits has the conversion to a value in this array. Numeric value of byte is index of its representation
const char* B64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Array to the numeric value from a b64 char to binary value
// In case the input string has a char not included in B64chars it will have Undefined Behaviour
// 255 spaces are taken to avoid seg fault in this case
const int B64index[255] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 16
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  62, 63, 62, 62, 63, // 32
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0,  0,  0,  0,  0,  0, // 48
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, // 64
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 0,  0,  0,  0,  63, // 80
    0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 //112
};

// encode a binary data into a string
const std::string b64encode(
        const unsigned char* data,
        const size_t& len)
{
    std::string result((len + 2) / 3 * 4, '=');
    char* str = &result[0];
    size_t j = 0, pad = len % 3;
    const size_t last = len - pad;

    for (size_t i = 0; i < last; i += 3)
    {
        int n = int(data[i]) << 16 | int(data[i + 1]) << 8 | data[i + 2];
        str[j++] = B64chars[n >> 18];
        str[j++] = B64chars[n >> 12 & 0x3F];
        str[j++] = B64chars[n >> 6 & 0x3F];
        str[j++] = B64chars[n & 0x3F];
    }
    if (pad)  /// set padding
    {
        int n = --pad ? int(data[last]) << 8 | data[last + 1] : data[last];
        str[j++] = B64chars[pad ? n >> 10 & 0x3F : n >> 2];
        str[j++] = B64chars[pad ? n >> 4 & 0x03F : n << 4 & 0x3F];
        str[j++] = pad ? B64chars[n << 2 & 0x3F] : '=';
    }
    return result;
}

// decode a string into another string
void b64decode(
        unsigned char* data,
        const std::string input)
{
    size_t len = input.size();

    if (len == 0)
    {
        return;
    }

    const char* p = input.c_str();
    size_t j = 0,
            pad1 = len % 4 || p[len - 1] == '=',
            pad2 = pad1 && (len % 4 > 2 || p[len - 2] != '=');
    const size_t last = (len - pad1) / 4 << 2;
    std::string result(last / 4 * 3 + pad1 + pad2, '\0');

    for (size_t i = 0; i < last; i += 4)
    {
        int n = B64index[(int) p[i]] << 18 |
                B64index[(int) p[i + 1]] << 12 |
                B64index[(int) p[i + 2]] << 6 |
                B64index[(int) p[i + 3]];
        data[j++] = (n >> 16) & 0xFF;
        data[j++] = (n >> 8) & 0xFF;
        data[j++] = n & 0xFF;
    }
    if (pad1)
    {
        int n = B64index[(int) p[last]] << 18 | B64index[(int) p[last + 1]] << 12;
        data[j++] = (n >> 16) & 0xFF;
        if (pad2)
        {
            n |= B64index[(int) p[last + 2]] << 6;
            data[j++] = (n >> 8) & 0xFF;
        }
    }
}

} /* ddb */
} /* namespace rtps */
} /* namespace fastdds */
} /* namespace eprosima */
