//
//  GoASTContext.h
//  lldb
//
//  Created by Ryan Brown on 3/26/15.
//
//

#ifndef __lldb__GoASTContext__
#define __lldb__GoASTContext__

#include <map>
#include "lldb/Core/ConstString.h"
#include "lldb/Symbol/TypeSystem.h"

namespace lldb_private {
    
class Declaration;
class GoType;

class GoASTContext : public TypeSystem
{
public:
    GoASTContext();
    ~GoASTContext();
    
    virtual clang::ASTContext *
    GetASTContext() const
    {
        return nullptr;
    };
    
    virtual ClangASTContext *
    AsClangASTContext()
    {
        return nullptr;
    }
    
    virtual GoASTContext*
    AsGoASTContext()
    {
        return this;
    }
    
    void SetAddressByteSize(int byte_size) { m_pointer_byte_size = byte_size; }
    
    //----------------------------------------------------------------------
    // Creating Types
    //----------------------------------------------------------------------
    
    ClangASTType
    CreateArrayType(const ConstString& name, const ClangASTType& element_type, int64_t length);
    
    ClangASTType
    CreateBaseType(int go_kind, const ConstString& type_name_const_str, uint64_t byte_size);
    
    // For interface, map, chan.
    ClangASTType
    CreateTypedef(int kind, const ConstString& name, ClangASTType impl);
    
    ClangASTType
    CreateVoidType(const ConstString& name);
    ClangASTType
    CreateFunctionType(const lldb_private::ConstString &name, ClangASTType* params, size_t params_count, bool is_variadic, ClangASTType return_type);
    
    ClangASTType
    CreateStructType(int kind, const ConstString& name, uint32_t byte_size);
    
    void
    CompleteStructType(const ClangASTType& type);
    
    void
    AddFieldToStruct(const ClangASTType& struct_type,
                     const ConstString& name,
                     const ClangASTType& field_type,
                     uint32_t byte_offset);
    
    typedef void (*CompleteTypeCallback)(void *baton, ClangASTType& type);
    void SetExternalSource(void* baton, CompleteTypeCallback callback) {
        m_complete_type = callback;
        m_complete_type_this = baton;
    }
    void RemoveExternalSource() {
        m_complete_type = nullptr;
        m_complete_type_this = nullptr;
    }
    
    //----------------------------------------------------------------------
    // Tests
    //----------------------------------------------------------------------
    
    static bool IsGoString(const ClangASTType& type);
    static bool IsGoSlice(const ClangASTType& type);
    
    virtual bool
    IsArrayType (void * type,
                 ClangASTType *element_type,
                 uint64_t *size,
                 bool *is_incomplete);
    
    virtual bool
    IsAggregateType (void * type);
    
    virtual bool
    IsCharType (void * type);
    
    virtual bool
    IsCompleteType (void * type);
    
    virtual bool
    IsDefined(void * type);
    
    virtual bool
    IsFloatingPointType (void * type, uint32_t &count, bool &is_complex);
    
    virtual bool
    IsFunctionType (void * type, bool *is_variadic_ptr = NULL);
    
    virtual size_t
    GetNumberOfFunctionArguments (void * type);
    
    virtual ClangASTType
    GetFunctionArgumentAtIndex (void * type, const size_t index);
    
    virtual bool
    IsFunctionPointerType (void * type);
    
    virtual bool
    IsIntegerType (void * type, bool &is_signed);
    
    virtual bool
    IsPossibleDynamicType (void * type,
                           ClangASTType *target_type, // Can pass NULL
                           bool check_cplusplus,
                           bool check_objc);
    
    virtual bool
    IsPointerType (void * type, ClangASTType *pointee_type = NULL);
    
    virtual bool
    IsScalarType (void * type);
    
    virtual bool
    IsVoidType (void * type);
    
    //----------------------------------------------------------------------
    // Type Completion
    //----------------------------------------------------------------------
    
    virtual bool
    GetCompleteType (void * type);
    
    //----------------------------------------------------------------------
    // AST related queries
    //----------------------------------------------------------------------
    
    virtual uint32_t
    GetPointerByteSize ();
    
    //----------------------------------------------------------------------
    // Accessors
    //----------------------------------------------------------------------
    
    virtual ConstString
    GetTypeName (void * type);
    
    virtual uint32_t
    GetTypeInfo (void * type, ClangASTType *pointee_or_element_clang_type = NULL);
    
    virtual lldb::LanguageType
    GetMinimumLanguage (void * type);
    
    virtual lldb::TypeClass
    GetTypeClass (void * type);
    
    //----------------------------------------------------------------------
    // Creating related types
    //----------------------------------------------------------------------
    
    virtual ClangASTType
    GetArrayElementType (void * type, uint64_t *stride = nullptr);
    
    virtual ClangASTType
    GetCanonicalType (void * type);
    
    // Returns -1 if this isn't a function of if the function doesn't have a prototype
    // Returns a value >= 0 if there is a prototype.
    virtual int
    GetFunctionArgumentCount (void * type);
    
    virtual ClangASTType
    GetFunctionArgumentTypeAtIndex (void * type, size_t idx);
    
    virtual ClangASTType
    GetFunctionReturnType (void * type);
    
    virtual size_t
    GetNumMemberFunctions (void * type);
    
    virtual TypeMemberFunctionImpl
    GetMemberFunctionAtIndex (void * type, size_t idx);
    
    virtual ClangASTType
    GetPointeeType (void * type);
    
    virtual ClangASTType
    GetPointerType (void * type);
    
    //----------------------------------------------------------------------
    // Exploring the type
    //----------------------------------------------------------------------
    
    virtual uint64_t
    GetBitSize (void * type, ExecutionContextScope *exe_scope);
    
    virtual lldb::Encoding
    GetEncoding (void * type, uint64_t &count);
    
    virtual lldb::Format
    GetFormat (void * type);
    
    virtual uint32_t
    GetNumChildren (void * type, bool omit_empty_base_classes);
    
    virtual lldb::BasicType
    GetBasicTypeEnumeration (void * type);
    
    virtual uint32_t
    GetNumFields (void * type);
    
    virtual ClangASTType
    GetFieldAtIndex (void * type,
                     size_t idx,
                     std::string& name,
                     uint64_t *bit_offset_ptr,
                     uint32_t *bitfield_bit_size_ptr,
                     bool *is_bitfield_ptr);
    
    virtual ClangASTType
    GetChildClangTypeAtIndex (void * type,
                              ExecutionContext *exe_ctx,
                              size_t idx,
                              bool transparent_pointers,
                              bool omit_empty_base_classes,
                              bool ignore_array_bounds,
                              std::string& child_name,
                              uint32_t &child_byte_size,
                              int32_t &child_byte_offset,
                              uint32_t &child_bitfield_bit_size,
                              uint32_t &child_bitfield_bit_offset,
                              bool &child_is_base_class,
                              bool &child_is_deref_of_parent,
                              ValueObject *valobj);
    
    // Lookup a child given a name. This function will match base class names
    // and member member names in "clang_type" only, not descendants.
    virtual uint32_t
    GetIndexOfChildWithName (void * type,
                             const char *name,
                             bool omit_empty_base_classes);
    
    // Lookup a child member given a name. This function will match member names
    // only and will descend into "clang_type" children in search for the first
    // member in this class, or any base class that matches "name".
    // TODO: Return all matches for a given name by returning a vector<vector<uint32_t>>
    // so we catch all names that match a given child name, not just the first.
    virtual size_t
    GetIndexOfChildMemberWithName (void * type,
                                   const char *name,
                                   bool omit_empty_base_classes,
                                   std::vector<uint32_t>& child_indexes);
    
    //----------------------------------------------------------------------
    // Dumping types
    //----------------------------------------------------------------------
    virtual void
    DumpValue (void * type,
               ExecutionContext *exe_ctx,
               Stream *s,
               lldb::Format format,
               const DataExtractor &data,
               lldb::offset_t data_offset,
               size_t data_byte_size,
               uint32_t bitfield_bit_size,
               uint32_t bitfield_bit_offset,
               bool show_types,
               bool show_summary,
               bool verbose,
               uint32_t depth);
    
    virtual bool
    DumpTypeValue (void * type,
                   Stream *s,
                   lldb::Format format,
                   const DataExtractor &data,
                   lldb::offset_t data_offset,
                   size_t data_byte_size,
                   uint32_t bitfield_bit_size,
                   uint32_t bitfield_bit_offset,
                   ExecutionContextScope *exe_scope);
    
    virtual void
    DumpTypeDescription (void * type); // Dump to stdout
    
    virtual void
    DumpTypeDescription (void * type, Stream *s);
    
    //----------------------------------------------------------------------
    // TODO: These methods appear unused. Should they be removed?
    //----------------------------------------------------------------------
    
    virtual bool
    IsRuntimeGeneratedType (void * type);
    
    virtual void
    DumpSummary (void * type,
                 ExecutionContext *exe_ctx,
                 Stream *s,
                 const DataExtractor &data,
                 lldb::offset_t data_offset,
                 size_t data_byte_size);
        
    // Converts "s" to a floating point value and place resulting floating
    // point bytes in the "dst" buffer.
    virtual size_t
    ConvertStringToFloatValue (void * type,
                               const char *s,
                               uint8_t *dst,
                               size_t dst_size);
    
    //----------------------------------------------------------------------
    // TODO: Determine if these methods should move to ClangASTContext.
    //----------------------------------------------------------------------
    
    virtual bool
    IsPointerOrReferenceType (void * type, ClangASTType *pointee_type = NULL);
    
    virtual unsigned
    GetTypeQualifiers(void * type);
    
    virtual bool
    IsCStringType (void * type, uint32_t &length);
    
    virtual size_t
    GetTypeBitAlign (void * type);
    
    virtual ClangASTType
    GetBasicTypeFromAST (void * type, lldb::BasicType basic_type);
    
    virtual bool
    IsBeingDefined (void * type);
    
    virtual bool
    IsConst(void * type);
    
    virtual uint32_t
    IsHomogeneousAggregate (void * type, ClangASTType* base_type_ptr);
    
    virtual bool
    IsPolymorphicClass (void * type);
    
    virtual bool
    IsTypedefType (void * type);
    
    // If the current object represents a typedef type, get the underlying type
    virtual ClangASTType
    GetTypedefedType (void * type);
    
    virtual bool
    IsVectorType (void * type,
                  ClangASTType *element_type,
                  uint64_t *size);
    
    virtual ClangASTType
    GetFullyUnqualifiedType (void * type);
    
    virtual ClangASTType
    GetNonReferenceType (void * type);
    
    virtual bool
    IsReferenceType (void * type, ClangASTType *pointee_type = nullptr, bool* is_rvalue = nullptr);
    
private:
    typedef std::map<ConstString, std::unique_ptr<GoType>> TypeMap;
    int m_pointer_byte_size;
    int m_int_byte_size;
    std::unique_ptr<TypeMap> m_types;
    CompleteTypeCallback m_complete_type;
    void* m_complete_type_this;

    GoASTContext(const GoASTContext&) = delete;
    const GoASTContext& operator=(const GoASTContext&) = delete;
};
}

#endif /* defined(__lldb__GoASTContext__) */
