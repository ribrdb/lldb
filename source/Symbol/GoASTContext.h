//
//  GoASTContext.h
//  lldb
//
//  Created by Ryan Brown on 3/26/15.
//
//

#ifndef __lldb__GoASTContext__
#define __lldb__GoASTContext__

#include "lldb/Symbol/TypeSystem.h"
namespace lldb_private {
    
class Declaration;

class GoASTContext : public TypeSystem
{
public:
    GoASTContext() { }
    
    virtual clang::ASTContext *
    GetASTContext() const
    {
        return nullptr;
    };
    
    //----------------------------------------------------------------------
    // Tests
    //----------------------------------------------------------------------
    
    virtual bool
    IsArrayType (void * type,
                 ClangASTType *element_type,
                 uint64_t *size,
                 bool *is_incomplete) const  = 0;
    
    virtual bool
    IsVectorType (void * type,
                  ClangASTType *element_type,
                  uint64_t *size) const  = 0;
    
    virtual bool
    IsAggregateType (void * type) const = 0;
    
    virtual bool
    IsBeingDefined (void * type) const = 0;
    
    virtual bool
    IsCharType (void * type) const = 0;
    
    virtual bool
    IsCompleteType (void * type) const = 0;
    
    virtual bool
    IsConst(void * type) const = 0;
    
    virtual bool
    IsCStringType (void * type, uint32_t &length) const = 0;
    
    virtual bool
    IsCXXClassType (void * type) const = 0;
    
    virtual bool
    IsDefined(void * type) const = 0;
    
    virtual bool
    IsFloatingPointType (void * type, uint32_t &count, bool &is_complex) const = 0;
    
    virtual bool
    IsFunctionType (void * type, bool *is_variadic_ptr = NULL) const = 0;
    
    virtual uint32_t
    IsHomogeneousAggregate (void * type, ClangASTType* base_type_ptr) const = 0;
    
    virtual size_t
    GetNumberOfFunctionArguments (void * type) const = 0;
    
    virtual ClangASTType
    GetFunctionArgumentAtIndex (void * type, const size_t index) const = 0;
    
    virtual bool
    IsFunctionPointerType (void * type) const = 0;
    
    virtual bool
    IsIntegerType (void * type, bool &is_signed) const = 0;
    
    virtual bool
    IsObjCClassType (void * type) const = 0;
    
    virtual bool
    IsObjCClassTypeAndHasIVars (void * type, bool check_superclass) const = 0;
    
    virtual bool
    IsObjCObjectOrInterfaceType (void * type) const = 0;
    
    virtual bool
    IsObjCObjectPointerType (void * type, ClangASTType *target_type = NULL) = 0;
    
    virtual bool
    IsPolymorphicClass (void * type) const = 0;
    
    virtual bool
    IsPossibleDynamicType (void * type,
                           ClangASTType *target_type, // Can pass NULL
                           bool check_cplusplus,
                           bool check_objc) const = 0;
    
    virtual bool
    IsRuntimeGeneratedType (void * type) const = 0;
    
    virtual bool
    IsPointerType (void * type, ClangASTType *pointee_type = NULL) const = 0;
    
    virtual bool
    IsPointerOrReferenceType (void * type, ClangASTType *pointee_type = NULL) const = 0;
    
    virtual bool
    IsReferenceType (void * type, ClangASTType *pointee_type = nullptr, bool* is_rvalue = nullptr) const = 0;
    
    virtual bool
    IsScalarType (void * type) const = 0;
    
    virtual bool
    IsTypedefType (void * type) const = 0;
    
    virtual bool
    IsVoidType (void * type) const = 0;
    
    virtual bool
    GetCXXClassName (void * type, std::string &class_name) const = 0;
    
    virtual bool
    GetObjCClassName (void * type, std::string &class_name) = 0;
    
    
    //----------------------------------------------------------------------
    // Type Completion
    //----------------------------------------------------------------------
    
    virtual bool
    GetCompleteType (void * type) const = 0;
    
    //----------------------------------------------------------------------
    // AST related queries
    //----------------------------------------------------------------------
    
    virtual size_t
    GetPointerByteSize () const = 0;
    
    //----------------------------------------------------------------------
    // Accessors
    //----------------------------------------------------------------------
    
    virtual ConstString
    GetTypeName (void * type) const = 0;
    
    virtual uint32_t
    GetTypeInfo (void * type, ClangASTType *pointee_or_element_clang_type = NULL) const = 0;
    
    virtual lldb::LanguageType
    GetMinimumLanguage (void * type) = 0;
    
    virtual lldb::TypeClass
    GetTypeClass (void * type) const = 0;
    
    virtual unsigned
    GetTypeQualifiers(void * type) const = 0;
    
    //----------------------------------------------------------------------
    // Creating related types
    //----------------------------------------------------------------------
    
    virtual ClangASTType
    AddConstModifier (void * type) const = 0;
    
    virtual ClangASTType
    AddRestrictModifier (void * type) const = 0;
    
    virtual ClangASTType
    AddVolatileModifier (void * type) const = 0;
    
    // Using the current type, create a new typedef to that type using "typedef_name"
    // as the name and "decl_ctx" as the decl context.
    virtual ClangASTType
    CreateTypedefType (void * type, const char *typedef_name,
                       clang::DeclContext *decl_ctx) const = 0;
    
    virtual ClangASTType
    GetArrayElementType (void * type, uint64_t *stride = nullptr) const = 0;
    
    virtual ClangASTType
    GetCanonicalType (void * type) const = 0;
    
    virtual ClangASTType
    GetFullyUnqualifiedType (void * type) const = 0;
    
    // Returns -1 if this isn't a function of if the function doesn't have a prototype
    // Returns a value >= 0 if there is a prototype.
    virtual int
    GetFunctionArgumentCount (void * type) const = 0;
    
    virtual ClangASTType
    GetFunctionArgumentTypeAtIndex (void * type, size_t idx) const = 0;
    
    virtual ClangASTType
    GetFunctionReturnType (void * type) const = 0;
    
    virtual size_t
    GetNumMemberFunctions (void * type) const = 0;
    
    virtual TypeMemberFunctionImpl
    GetMemberFunctionAtIndex (void * type, size_t idx) = 0;
    
    virtual ClangASTType
    GetLValueReferenceType (void * type) const = 0;
    
    virtual ClangASTType
    GetNonReferenceType (void * type) const = 0;
    
    virtual ClangASTType
    GetPointeeType (void * type) const = 0;
    
    virtual ClangASTType
    GetPointerType (void * type) const = 0;
    
    virtual ClangASTType
    GetRValueReferenceType (void * type) const = 0;
    
    // If the current object represents a typedef type, get the underlying type
    virtual ClangASTType
    GetTypedefedType (void * type) const = 0;
    
    virtual ClangASTType
    RemoveFastQualifiers (void * type) const = 0;
    
    //----------------------------------------------------------------------
    // Create related types using the current type's AST
    //----------------------------------------------------------------------
    virtual ClangASTType
    GetBasicTypeFromAST (void * type, lldb::BasicType basic_type) const = 0;
    
    //----------------------------------------------------------------------
    // Exploring the type
    //----------------------------------------------------------------------
    
    virtual uint64_t
    GetBitSize (void * type, ExecutionContextScope *exe_scope) const = 0;
    
    virtual lldb::Encoding
    GetEncoding (void * type, uint64_t &count) const = 0;
    
    virtual lldb::Format
    GetFormat (void * type) const = 0;
    
    virtual size_t
    GetTypeBitAlign (void * type) const = 0;
    
    virtual uint32_t
    GetNumChildren (void * type, bool omit_empty_base_classes) const = 0;
    
    virtual lldb::BasicType
    GetBasicTypeEnumeration (void * type) const = 0;
    
    virtual uint32_t
    GetNumDirectBaseClasses (void * type) const = 0;
    
    virtual uint32_t
    GetNumVirtualBaseClasses (void * type) const = 0;
    
    virtual uint32_t
    GetNumFields (void * type) const = 0;
    
    virtual ClangASTType
    GetDirectBaseClassAtIndex (void * type, size_t idx,
                               uint32_t *bit_offset_ptr) const = 0;
    
    virtual ClangASTType
    GetVirtualBaseClassAtIndex (void * type, size_t idx,
                                uint32_t *bit_offset_ptr) const = 0;
    
    virtual ClangASTType
    GetFieldAtIndex (void * type,
                     size_t idx,
                     std::string& name,
                     uint64_t *bit_offset_ptr,
                     uint32_t *bitfield_bit_size_ptr,
                     bool *is_bitfield_ptr) const = 0;
    
    virtual uint32_t
    GetNumPointeeChildren (void * type) const = 0;
    
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
                              ValueObject *valobj) const = 0;
    
    // Lookup a child given a name. This function will match base class names
    // and member member names in "clang_type" only, not descendants.
    virtual uint32_t
    GetIndexOfChildWithName (void * type,
                             const char *name,
                             bool omit_empty_base_classes) const = 0;
    
    // Lookup a child member given a name. This function will match member names
    // only and will descend into "clang_type" children in search for the first
    // member in this class, or any base class that matches "name".
    // TODO: Return all matches for a given name by returning a vector<vector<uint32_t>>
    // so we catch all names that match a given child name, not just the first.
    virtual size_t
    GetIndexOfChildMemberWithName (void * type,
                                   const char *name,
                                   bool omit_empty_base_classes,
                                   std::vector<uint32_t>& child_indexes) const = 0;
    
    virtual size_t
    GetNumTemplateArguments (void * type) const = 0;
    
    virtual ClangASTType
    GetTemplateArgument (void * type,
                         size_t idx,
                         lldb::TemplateArgumentKind &kind) const = 0;
    
    //------------------------------------------------------------------
    // Pointers & References
    //------------------------------------------------------------------
    
    // Call this function using the class type when you want to make a
    // member pointer type to pointee_type.
    virtual ClangASTType
    CreateMemberPointerType (void * type, const ClangASTType &pointee_type) const = 0;
    
    
    // Converts "s" to a floating point value and place resulting floating
    // point bytes in the "dst" buffer.
    virtual size_t
    ConvertStringToFloatValue (void * type,
                               const char *s,
                               uint8_t *dst,
                               size_t dst_size) const = 0;
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
               uint32_t depth) = 0;
    
    virtual bool
    DumpTypeValue (void * type,
                   Stream *s,
                   lldb::Format format,
                   const DataExtractor &data,
                   lldb::offset_t data_offset,
                   size_t data_byte_size,
                   uint32_t bitfield_bit_size,
                   uint32_t bitfield_bit_offset,
                   ExecutionContextScope *exe_scope) = 0;
    
    virtual void
    DumpSummary (void * type,
                 ExecutionContext *exe_ctx,
                 Stream *s,
                 const DataExtractor &data,
                 lldb::offset_t data_offset,
                 size_t data_byte_size) = 0;
    
    virtual void
    DumpTypeDescription (void * type) const = 0; // Dump to stdout
    
    virtual void
    DumpTypeDescription (void * type, Stream *s) const = 0;
    
    virtual bool
    GetValueAsScalar (void * type,
                      const DataExtractor &data,
                      lldb::offset_t data_offset,
                      size_t data_byte_size,
                      Scalar &value) const = 0;
    
    virtual bool
    SetValueFromScalar (void * type,
                        const Scalar &value,
                        Stream &strm) = 0;
    
    virtual bool
    ReadFromMemory (void * type,
                    ExecutionContext *exe_ctx,
                    lldb::addr_t addr,
                    AddressType address_type,
                    DataExtractor &data) = 0;
    
    virtual bool
    WriteToMemory (void * type,
                   ExecutionContext *exe_ctx,
                   lldb::addr_t addr,
                   AddressType address_type,
                   StreamString &new_value) = 0;
    
    virtual clang::EnumDecl *
    GetAsEnumDecl (void * type) const = 0;
    
    virtual clang::QualType
    GetQualType (void * type) const = 0;
    
    virtual clang::QualType
    GetCanonicalQualType (void * type) const = 0;
    
private:
    GoASTContext(const GoASTContext&) = delete;
    const GoASTContext& operator=(const GoASTContext&) = delete;
};
}

#endif /* defined(__lldb__GoASTContext__) */
