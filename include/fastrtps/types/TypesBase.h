// Copyright 2018 Proyectos y Sistemas de Mantenimiento SL (eProsima).
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

#ifndef TYPES_BASE_H
#define TYPES_BASE_H

#include <fastrtps/rtps/common/Types.h>
#include <fastcdr/Cdr.h>
#include <bitset>
#include <string>
#include <map>
#include <vector>
#include <cctype>
#include <algorithm>
#include <memory>

namespace eprosima{
namespace fastrtps{

using namespace rtps;

namespace types{

const std::string ANNOTATION_KEY_ID = "@Key";
const std::string ANNOTATION_TOPIC_ID = "@Topic";

const std::string TKNAME_BOOLEAN    = "bool";
const std::string TKNAME_INT16      = "int16_t";
const std::string TKNAME_UINT16     = "uint16_t";
const std::string TKNAME_INT32      = "int32_t";
const std::string TKNAME_UINT32     = "uint32_t";
const std::string TKNAME_INT64      = "int64_t";
const std::string TKNAME_UINT64     = "uint64_t";
const std::string TKNAME_CHAR8      = "char";
const std::string TKNAME_BYTE       = "octet";
const std::string TKNAME_CHAR16     = "wchar";
const std::string TKNAME_FLOAT32    = "float";
const std::string TKNAME_FLOAT64    = "double";
const std::string TKNAME_FLOAT128   = "longdouble";

const std::string TKNAME_STRING8    = "string";
const std::string TKNAME_STRING16   = "wstring";
const std::string TKNAME_ALIAS      = "alias";
const std::string TKNAME_ENUM       = "enum";
const std::string TKNAME_BITMASK    = "bitmask";
const std::string TKNAME_ANNOTATION = "annotation";
const std::string TKNAME_STRUCTURE  = "structure";
const std::string TKNAME_UNION      = "union";
const std::string TKNAME_BITSET     = "bitset";
const std::string TKNAME_SEQUENCE   = "sequence";
const std::string TKNAME_ARRAY      = "array";
const std::string TKNAME_MAP        = "map";

// ---------- Equivalence Kinds ------------------
typedef octet EquivalenceKind;
const octet EK_MINIMAL   = 0xF1; // 0x1111 0001
const octet EK_COMPLETE  = 0xF2; // 0x1111 0010
const octet EK_BOTH      = 0xF3; // 0x1111 0011

// ---------- TypeKinds (begin) ------------------
typedef octet TypeKind;        // Primitive TKs

const octet TK_NONE       = 0x00;
const octet TK_BOOLEAN    = 0x01;
const octet TK_BYTE       = 0x02;
const octet TK_INT16      = 0x03;
const octet TK_INT32      = 0x04;
const octet TK_INT64      = 0x05;
const octet TK_UINT16     = 0x06;
const octet TK_UINT32     = 0x07;
const octet TK_UINT64     = 0x08;
const octet TK_FLOAT32    = 0x09;
const octet TK_FLOAT64    = 0x0A;
const octet TK_FLOAT128   = 0x0B;
const octet TK_CHAR8      = 0x10;
const octet TK_CHAR16     = 0x11;

// String TKs
const octet TK_STRING8    = 0x20;
const octet TK_STRING16   = 0x21;


// Constructed/Named types
const octet TK_ALIAS      = 0x30;

// Enumerated TKs
const octet TK_ENUM       = 0x40;
const octet TK_BITMASK    = 0x41;

// Structured TKs
const octet TK_ANNOTATION = 0x50;
const octet TK_STRUCTURE  = 0x51;
const octet TK_UNION      = 0x52;
const octet TK_BITSET     = 0x53;

// Collection TKs
const octet TK_SEQUENCE   = 0x60;
const octet TK_ARRAY      = 0x61;
const octet TK_MAP        = 0x62;

// ---------- TypeKinds (end) ------------------

// The name of some element (e.g. type, type member, module)
// Valid characters are alphanumeric plus the "_" cannot start with digit

const int32_t MEMBER_NAME_MAX_LENGTH = 256;
typedef std::string MemberName;

// Qualified type name includes the name of containing modules
// using "::" as separator. No leading "::". E.g. "MyModule::MyType"
const int32_t TYPE_NAME_MAX_LENGTH = 256;
typedef std::string QualifiedTypeName;

// Every type has an ID. Those of the primitive types are pre-defined.
typedef octet PrimitiveTypeId;

// First 4 bytes of MD5 of of a member name converted to bytes
// using UTF-8 encoding and without a 'nul' terminator.
// Example: the member name "color" has NameHash {0x70, 0xDD, 0xA5, 0xDF}
typedef std::array<uint8_t, 4> NameHash;

// Mask used to remove the flags that do no affect assignability
// Selects  T1, T2, O, M, K, D
const uint16_t MemberFlagMinimalMask = 0x003f;

/*!
 * @brief This class represents the enumeration ResponseCode.
 */
enum ResponseCode : uint32_t
{
    RETCODE_ERROR = (uint32_t)(-1),
    RETCODE_OK = 0,
    RETCODE_BAD_PARAMETER,
    RETCODE_PRECONDITION_NOT_MET,
    RETCODE_ALREADY_DELETED
};

typedef uint32_t MemberId;
#define MEMBER_ID_INVALID 0X0FFFFFFF
#define INDEX_INVALID UINT32_MAX

const int32_t MAX_BITMASK_LENGTH = 64;
const int32_t MAX_ELEMENTS_COUNT = 100;
const int32_t MAX_STRING_LENGTH = 255;

#define LENGTH_UNLIMITED        0

class TypeSupport
{
public:
    //int register_type(DomainParticipant domain, std::string type_name);
    //std::string get_type_name();
    //DynamicType get_type();
};

class DynamicTypeSupport : public TypeSupport
{
public:
    //static DynamicTypeSupport create_type_support(DynamicType type);
    //static ResponseCode delete_type_support(DynamicTypeSupport type_support);

    //ResponseCode register_type(DomainParticipant participant, std::string type_name);
    //std::string get_type_name();
    //DynamicType get_type();
};

// Long Bound of a collection type
typedef uint32_t LBound;
typedef std::vector<LBound> LBoundSeq;
const LBound INVALID_LBOUND = 0;

// Short Bound of a collection type
typedef octet SBound;
typedef std::vector<SBound> SBoundSeq;
const SBound INVALID_SBOUND = 0;

// Flags that apply to struct/union/collection/enum/bitmask/bitset
// members/elements and DO affect type assignability
// Depending on the flag it may not apply to members of all types

// When not all, the applicable member types are listed
class MemberFlag
{
private:
	std::bitset<16> m_MemberFlag;
public:
    MemberFlag() {}
    MemberFlag(const MemberFlag &x) : m_MemberFlag(x.m_MemberFlag) {}
    MemberFlag(MemberFlag &&x) : m_MemberFlag(std::move(x.m_MemberFlag)) {}
    MemberFlag& operator=(const MemberFlag &x)
	{
		m_MemberFlag = x.m_MemberFlag;
		return *this;
	}

    MemberFlag& operator=(MemberFlag &&x)
	{
		m_MemberFlag = std::move(x.m_MemberFlag);
		return *this;
	}
	// T1 | 00 = INVALID, 01 = DISCARD
	bool TRY_CONSTRUCT1() const { return m_MemberFlag.test(0); }
	void TRY_CONSTRUCT1(bool b) { b ? m_MemberFlag.set(0) : m_MemberFlag.reset(0); }
	// T2 | 10 = USE_DEFAULT, 11 = TRIM
	bool TRY_CONSTRUCT2() const { return m_MemberFlag.test(1); }
	void TRY_CONSTRUCT2(bool b) { b ? m_MemberFlag.set(1) : m_MemberFlag.reset(1); }
	// X  StructMember, UnionMember,
	//    CollectionElement
	bool IS_EXTERNAL() const { return m_MemberFlag.test(2); }
	void IS_EXTERNAL(bool b) { b ? m_MemberFlag.set(2) : m_MemberFlag.reset(2); }
	// O  StructMember
	bool IS_OPTIONAL() const { return m_MemberFlag.test(3); }
	void IS_OPTIONAL(bool b) { b ? m_MemberFlag.set(3) : m_MemberFlag.reset(3); }
	// M  StructMember
	bool IS_MUST_UNDERSTAND() const { return m_MemberFlag.test(4); }
	void IS_MUST_UNDERSTAND(bool b) { b ? m_MemberFlag.set(4) : m_MemberFlag.reset(4); }
	// K  StructMember, UnionDiscriminator
	bool IS_KEY() const { return m_MemberFlag.test(5); }
	void IS_KEY(bool b) { b ? m_MemberFlag.set(5) : m_MemberFlag.reset(5); }
	// D  UnionMember, EnumerationLiteral
	bool IS_DEFAULT() const { return m_MemberFlag.test(6); }
	void IS_DEFAULT(bool b) { b ? m_MemberFlag.set(6) : m_MemberFlag.reset(6); }

    void serialize(eprosima::fastcdr::Cdr &cdr) const
	{
		//cdr << m_MemberFlag;
		uint16_t bits = static_cast<uint16_t>(m_MemberFlag.to_ulong());
		cdr << bits;
	}

    void deserialize(eprosima::fastcdr::Cdr &cdr)
	{
		//cdr >> (uint16_t)m_MemberFlag;
		uint16_t bits;
		cdr >> bits;
		m_MemberFlag = std::bitset<16>(bits);
	}

    //static size_t getMaxCdrSerializedSize(size_t current_alignment = 0)
	//{
	//	return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
	//}

    static size_t getCdrSerializedSize(const MemberFlag&, size_t current_alignment = 0)
	{
		return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
	}
};

typedef MemberFlag CollectionElementFlag;   // T1, T2, X
typedef MemberFlag StructMemberFlag;        // T1, T2, O, M, K, X
typedef MemberFlag UnionMemberFlag;         // T1, T2, D, X
typedef MemberFlag UnionDiscriminatorFlag;  // T1, T2, K
typedef MemberFlag EnumeratedLiteralFlag;   // D
typedef MemberFlag AnnotationParameterFlag; // Unused. No flags apply
typedef MemberFlag AliasMemberFlag;         // Unused. No flags apply
typedef MemberFlag BitflagFlag;             // Unused. No flags apply
typedef MemberFlag BitsetMemberFlag;        // Unused. No flags apply

// Flags that apply to type declarationa and DO affect assignability
// Depending on the flag it may not apply to all types
// When not all, the applicable  types are listed
class TypeFlag
{
private:
	std::bitset<16> m_TypeFlag;
public:
    TypeFlag() {}
    TypeFlag(const TypeFlag &x) : m_TypeFlag(x.m_TypeFlag) {}
    TypeFlag(TypeFlag &&x) : m_TypeFlag(std::move(x.m_TypeFlag)) {}
    TypeFlag& operator=(const TypeFlag &x)
	{
		m_TypeFlag = x.m_TypeFlag;
		return *this;
	}

    TypeFlag& operator=(TypeFlag &&x)
	{
		m_TypeFlag = std::move(x.m_TypeFlag);
		return *this;
	}

	// F |
	bool IS_FINAL() const { return m_TypeFlag.test(0); }
	void IS_FINAL(bool b) { b ? m_TypeFlag.set(0) : m_TypeFlag.reset(0); }
	// A |-  Struct, Union
	bool IS_APPENDABLE() const { return m_TypeFlag.test(1); }
	void IS_APPENDABLE(bool b) { b ? m_TypeFlag.set(1) : m_TypeFlag.reset(1); }
	// M |   (exactly one flag)
	bool IS_MUTABLE() const { return m_TypeFlag.test(2); }
	void IS_MUTABLE(bool b) { b ? m_TypeFlag.set(2) : m_TypeFlag.reset(2); }
	// N     Struct, Union
	bool IS_NESTED() const { return m_TypeFlag.test(3); }
	void IS_NESTED(bool b) { b ? m_TypeFlag.set(3) : m_TypeFlag.reset(3); }
	// H     Struct
	bool IS_AUTOID_HASH() const { return m_TypeFlag.test(4); }
	void IS_AUTOID_HASH(bool b) { b ? m_TypeFlag.set(4) : m_TypeFlag.reset(4); }

    void serialize(eprosima::fastcdr::Cdr &cdr) const
	{
		//cdr << m_TypeFlag;
		uint16_t bits = static_cast<uint16_t>(m_TypeFlag.to_ulong());
		cdr << bits;
	}

    void deserialize(eprosima::fastcdr::Cdr &cdr)
	{
		//cdr >> (uint16_t)m_TypeFlag;
		uint16_t bits;
		cdr >> bits;
		m_TypeFlag = std::bitset<16>(bits);
	}

    //static size_t getMaxCdrSerializedSize(size_t current_alignment = 0)
	//{
	//	return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
	//}

    static size_t getCdrSerializedSize(const TypeFlag&, size_t current_alignment = 0)
	{
		return 2 + eprosima::fastcdr::Cdr::alignment(current_alignment, 2);
	}
};

typedef TypeFlag   StructTypeFlag;      // All flags apply
typedef TypeFlag   UnionTypeFlag;       // All flags apply
typedef TypeFlag   CollectionTypeFlag;  // Unused. No flags apply
typedef TypeFlag   AnnotationTypeFlag;  // Unused. No flags apply
typedef TypeFlag   AliasTypeFlag;       // Unused. No flags apply
typedef TypeFlag   EnumTypeFlag;        // Unused. No flags apply
typedef TypeFlag   BitmaskTypeFlag;     // Unused. No flags apply
typedef TypeFlag   BitsetTypeFlag;      // Unused. No flags apply

// Mask used to remove the flags that do no affect assignability
const uint16_t TypeFlagMinimalMask = 0x0007; // Selects  M, A, F

// --- Annotation usage: ----------------------------------------------

// ID of a type member
const uint32_t ANNOTATION_STR_VALUE_MAX_LEN = 128;
const uint32_t ANNOTATION_OCTETSEC_VALUE_MAX_LEN = 128;

} // namespace types
} // namespace fastrtps
} // namespace eprosima

#endif // TYPES_BASE_H


