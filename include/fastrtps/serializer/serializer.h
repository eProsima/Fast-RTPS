#include <fastrtps/types/DynamicTypePtr.h>
#include <fastrtps/types/TypeObjectHashId.h>
#include <fastrtps/types/TypeObject.h>
namespace eprosima{

namespace fastcdr{
    class Cdr ;
}
namespace fastrtps{

namespace types{
    class DynamicData ;
}

namespace serializer{

/*********************
 *     SERIALIZER    *
 *********************/
template<class Serializable>
class Serializer{
public:
    Serializer(Serializable &r):r_(r){}
    void serialize(
            eprosima::fastcdr::Cdr& cdr
            )
    {
        serialize(&r_, cdr) ;
    }
    void deserialize(
            eprosima::fastcdr::Cdr& cdr
            )
    {
        deserialize(&r_, cdr) ;
    }
    size_t getCdrSerializedSize(
            size_t current_alignment
    )
    {
        return getCdrSerializedSize( r_,current_alignment) ;
    }

private:

/*DynamicData Methods*/
    void serialize(
            eprosima::fastrtps::types::DynamicData *dd, 
            eprosima::fastcdr::Cdr& cdr
            ) const ;

    void serialize_discriminator(
            eprosima::fastrtps::types::DynamicData *dd, 
            eprosima::fastcdr::Cdr& cdr
            ) const ;

    void serialize_empty_data(
            eprosima::fastrtps::types::DynamicType *dt, 
            eprosima::fastcdr::Cdr& cdr
            ) const ;

    bool deserialize(
            eprosima::fastrtps::types::DynamicData *dd, 
            eprosima::fastcdr::Cdr& cdr
            ) const ;

    bool deserialize_discriminator(
            eprosima::fastcdr::Cdr& cdr
            );

    static size_t getCdrSerializedSize(

            const eprosima::fastrtps::types::DynamicData* data,

            size_t current_alignment = 0
            );

    static size_t getEmptyCdrSerializedSize(

            const eprosima::fastrtps::types::DynamicType* type,

            size_t current_alignment = 0
            );

    static size_t getKeyMaxCdrSerializedSize(

            const eprosima::fastrtps::types::DynamicType_ptr type,

            size_t current_alignment = 0
            );

    static size_t getMaxCdrSerializedSize(

            const eprosima::fastrtps::types::DynamicType_ptr type,

            size_t current_alignment = 0
            );

    void serializeKey(
            eprosima::fastrtps::types::DynamicData *dd, 
            eprosima::fastcdr::Cdr& cdr
            ) const;

    /*MemberFlag Methods*/
    void serialize(
            eprosima::fastrtps::types::MemberFlag *mf, 
            eprosima::fastcdr::Cdr &cdr
            ) const ;

    void deserialize(
            eprosima::fastrtps::types::MemberFlag *mf, 
            eprosima::fastcdr::Cdr &cdr
            );

    size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MemberFlag&, 
            size_t current_alignment
            );

    /*TypeFlag Methods*/
    void serialize(
            eprosima::fastrtps::types::TypeFlag *tf, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    void deserialize(
            eprosima::fastrtps::types::TypeFlag *tf, 
            eprosima::fastcdr::Cdr &cdr
            );

    size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeFlag&, 
            size_t current_alignment
            );

    /*TypeObjectHashId Methods*/
    static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeObjectHashId& data, 
            size_t current_alignment = 0
            );

    void serialize(
            eprosima::fastrtps::types::TypeObjectHashId *tohi, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    void deserialize(
            eprosima::fastrtps::types::TypeObjectHashId *tohi, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonStructMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonStructMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonStructMember *csm, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonStructMember *csm, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteMemberDetail Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteMemberDetail& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteMemberDetail *cmd,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteMemberDetail *cmd,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalMemberDetail Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalMemberDetail& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalMemberDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalMemberDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteStructMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteStructMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteStructMember *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteStructMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalStructMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalStructMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalStructMember *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalStructMember *v, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*AppliedBuiltinTypeAnnotations Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::AppliedBuiltinTypeAnnotations& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::AppliedBuiltinTypeAnnotations *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::AppliedBuiltinTypeAnnotations *v, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalTypeDetail Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalTypeDetail& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalTypeDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalTypeDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteTypeDetail Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteTypeDetail& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteTypeDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteTypeDetail *v, 
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteStructHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteStructHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteStructHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteStructHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalStructHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalStructHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalStructHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalStructHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteStructType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteStructType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteStructType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteStructType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalStructType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalStructType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalStructType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalStructType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonUnionMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonUnionMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteUnionMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteUnionMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalUnionMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalUnionMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalUnionMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonDiscriminatorMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonDiscriminatorMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteDiscriminatorMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteDiscriminatorMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalDiscriminatorMember Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalDiscriminatorMember& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalDiscriminatorMember *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteUnionHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteUnionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteUnionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteUnionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalUnionHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalUnionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalUnionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalUnionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteUnionType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteUnionType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteUnionType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteUnionType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalUnionType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalUnionType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalUnionType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalUnionType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonAnnotationParameter Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonAnnotationParameter& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAnnotationParameter Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAnnotationParameter& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAnnotationParameter Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAnnotationParameter& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAnnotationHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAnnotationHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAnnotationHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAnnotationHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAnnotationHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAnnotationHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAnnotationHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAnnotationHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAnnotationType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAnnotationType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAnnotationType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAnnotationType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAnnotationType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAnnotationType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAnnotationType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAnnotationType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonAliasBody Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonAliasBody& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAliasBody Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAliasBody& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAliasBody Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAliasBody& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAliasBody *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAliasHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAliasHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAliasHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAliasHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAliasHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAliasHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAliasHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAliasHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteAliasType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteAliasType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteAliasType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteAliasType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalAliasType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalAliasType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalAliasType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalAliasType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteElementDetail Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteElementDetail& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteElementDetail *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteElementDetail *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonCollectionElement Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonCollectionElement& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteCollectionElement Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteCollectionElement& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalCollectionElement Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalCollectionElement& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalCollectionElement *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonCollectionHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonCollectionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteCollectionHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteCollectionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalCollectionHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalCollectionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteSequenceType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteSequenceType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteSequenceType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteSequenceType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalSequenceType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalSequenceType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalSequenceType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalSequenceType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonArrayHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonArrayHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteArrayHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteArrayHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalArrayHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalArrayHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalArrayHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteArrayType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteArrayType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteArrayType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteArrayType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalArrayType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalArrayType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalArrayType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalArrayType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteMapType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteMapType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteMapType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteMapType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalMapType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalMapType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalMapType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalMapType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonEnumeratedLiteral Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonEnumeratedLiteral& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteEnumeratedLiteral Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteEnumeratedLiteral& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*MinimalEnumeratedLiteral Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalEnumeratedLiteral& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalEnumeratedLiteral *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonEnumeratedHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonEnumeratedHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteEnumeratedHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteEnumeratedHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalEnumeratedHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalEnumeratedHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalEnumeratedHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteEnumeratedType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteEnumeratedType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteEnumeratedType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteEnumeratedType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalEnumeratedType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalEnumeratedType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalEnumeratedType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalEnumeratedType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonBitflag Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonBitflag& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteBitflag Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteBitflag& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalBitflag Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalBitflag& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalBitflag *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonBitmaskHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonBitmaskHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonBitmaskHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonBitmaskHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteBitmaskType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteBitmaskType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteBitmaskType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteBitmaskType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalBitmaskType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalBitmaskType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalBitmaskType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalBitmaskType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CommonBitfield Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CommonBitfield& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CommonBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CommonBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteBitfield Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteBitfield& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalBitfield Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalBitfield& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalBitfield *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteBitsetHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteBitsetHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteBitsetHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteBitsetHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalBitsetHeader Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalBitsetHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalBitsetHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalBitsetHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteBitsetType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteBitsetType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteBitsetType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteBitsetType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalBitsetType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalBitsetType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalBitsetType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalBitsetType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*CompleteExtendedType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteExtendedType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteExtendedType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteExtendedType *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalExtendedType Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalExtendedType& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalExtendedType *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalExtendedType *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*CompleteTypeObject Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::CompleteTypeObject& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::CompleteTypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::CompleteTypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*MinimalTypeObject Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::MinimalTypeObject& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::MinimalTypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::MinimalTypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeObject Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeObject& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeObject *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeIdentifierTypeObjectPair Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeIdentifierTypeObjectPair& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeIdentifierTypeObjectPair *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeIdentifierTypeObjectPair *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeIdentifierPair Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeIdentifierPair& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeIdentifierPair *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeIdentifierPair *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeIdentifierWithSize Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeIdentifierWithSize& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeIdentifierWithSize *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeIdentifierWithSize *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeIdentifierWithDependencies Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeIdentifierWithDependencies& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeIdentifierWithDependencies *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeIdentifierWithDependencies *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*TypeInformation Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeInformation& data, 
            size_t current_alignment = 0
            );
    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeInformation *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;
    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeInformation *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*StringSTypeDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::StringSTypeDefn& data, 
            size_t current_alignment = 0
            );
  
    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::StringSTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;
 
    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::StringSTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*StringLTypeDefn Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::StringLTypeDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::StringLTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::StringLTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*PlainCollectionHeader Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainCollectionHeader& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainCollectionHeader *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainSequenceSElemDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainSequenceSElemDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainSequenceSElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainSequenceSElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainSequenceLElemDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainSequenceLElemDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainSequenceLElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainSequenceLElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainArraySElemDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainArraySElemDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainArraySElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainArraySElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainArrayLElemDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainArrayLElemDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainArrayLElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainArrayLElemDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainMapSTypeDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainMapSTypeDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainMapSTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainMapSTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*PlainMapLTypeDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::PlainMapLTypeDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::PlainMapLTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::PlainMapLTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*StronglyConnectedComponentId Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::StronglyConnectedComponentId& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::StronglyConnectedComponentId *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::StronglyConnectedComponentId *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*ExtendedTypeDefn Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::ExtendedTypeDefn& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::ExtendedTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::ExtendedTypeDefn *v,
            eprosima::fastcdr::Cdr &cdr
            );
    /*TypeIdentifier Methods*/

    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::TypeIdentifier& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::TypeIdentifier *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::TypeIdentifier *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*ExtendedAnnotationParameterValue Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::ExtendedAnnotationParameterValue& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::ExtendedAnnotationParameterValue *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::ExtendedAnnotationParameterValue *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*AnnotationParameterValue Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::AnnotationParameterValue& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::AnnotationParameterValue *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::AnnotationParameterValue *v,
            eprosima::fastcdr::Cdr &cdr
            );

    /*AppliedAnnotationParameter Methods*/
    RTPS_DllAPI static size_t getCdrSerializedSize(
            const eprosima::fastrtps::types::AppliedAnnotationParameter& data, 
            size_t current_alignment = 0
            );

    RTPS_DllAPI void serialize(
            eprosima::fastrtps::types::AppliedAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            ) const;

    RTPS_DllAPI void deserialize(
            eprosima::fastrtps::types::AppliedAnnotationParameter *v,
            eprosima::fastcdr::Cdr &cdr
            );

private: //just for readibility: above methods are private as well

    Serializable &r_ ;
};

} //namespace serializer
} //namespace fastrtps
} //namespace eprosima
