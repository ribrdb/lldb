//===-- ClangASTTypeSystem.h ------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_ClangASTTypeSystem_h_
#define liblldb_ClangASTTypeSystem_h_

#include <string>

#include "clang/AST/Type.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"
#include "clang/AST/DeclGroup.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/Type.h"
#include "clang/AST/VTableBuilder.h"

#include "clang/Basic/Builtins.h"
#include "clang/Basic/IdentifierTable.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"

#include "llvm/Support/Format.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/raw_ostream.h"

#include "lldb/lldb-private.h"
#include "lldb/Core/ClangForward.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Scalar.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Symbol/ClangASTTypeSystem.h"
#include "lldb/Symbol/ClangExternalASTSourceCommon.h"
#include "lldb/Symbol/TypeSystem.h"
#include "lldb/Symbol/VerifyDecl.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/ObjCLanguageRuntime.h"
#include "lldb/Target/Process.h"

namespace lldb_private {
    
//----------------------------------------------------------------------
// A class that can carry around a clang ASTContext and a opaque clang
// QualType. A clang::QualType can be easily reconstructed from an
// opaque clang type and often the ASTContext is needed when doing
// various type related tasks, so this class allows both items to travel
// in a single very lightweight class that can be used. There are many
// static equivalents of the member functions that allow the ASTContext
// and the opaque clang QualType to be specified for ease of use and
// to avoid code duplication.
//----------------------------------------------------------------------
class ClangASTTypeSystem : public TypeSystem
{
public:
    //----------------------------------------------------------------------
    // Constructors and Destructors
    //----------------------------------------------------------------------
    ClangASTTypeSystem (clang::ASTContext *ast_context) : m_ast(ast_context)
    {
    }
    
    virtual ~ClangASTTypeSystem ()
    {
    }
    
    //----------------------------------------------------------------------
    // Tests
    //----------------------------------------------------------------------
    
    bool
    IsArrayType (void* type,
                 ClangASTType *element_type,
                 uint64_t *size,
                 bool *is_incomplete) const;
    
    bool
    IsVectorType (void* type,
                  ClangASTType *element_type,
                  uint64_t *size) const;
    
    bool
    IsAggregateType (void* type) const;
    
    bool
    IsBeingDefined (void* type) const;
    
    bool
    IsCharType (void* type) const;
    
    bool
    IsCompleteType (void* type) const;
    
    bool
    IsConst(void* type) const;
    
    bool
    IsCStringType (void* type, uint32_t &length) const;
    
    bool
    IsCXXClassType (void* type) const;
    
    bool
    IsDefined(void* type) const;
    
    bool
    IsFloatingPointType (void* type, uint32_t &count, bool &is_complex) const;
    
    bool
    IsFunctionType (void* type, bool *is_variadic_ptr = NULL) const;
    
    uint32_t
    IsHomogeneousAggregate (void* type, ClangASTType* base_type_ptr) const;
    
    size_t
    GetNumberOfFunctionArguments (void* type) const;
    
    ClangASTType
    GetFunctionArgumentAtIndex (void* type, const size_t index) const;
    
    bool
    IsFunctionPointerType (void* type) const;
    
    bool
    IsIntegerType (void* type, bool &is_signed) const;
    
    bool
    IsObjCClassType (void* type) const;
    
    bool
    IsObjCClassTypeAndHasIVars (void* type, bool check_superclass) const;
    
    bool
    IsObjCObjectOrInterfaceType (void* type) const;
    
    bool
    IsObjCObjectPointerType (void* type, ClangASTType *target_type = NULL);
    
    bool
    IsPolymorphicClass (void* type) const;
    
    bool
    IsPossibleDynamicType (void* type,
                           ClangASTType *target_type, // Can pass NULL
                           bool check_cplusplus,
                           bool check_objc) const;
    
    bool
    IsRuntimeGeneratedType (void* type) const;
    
    bool
    IsPointerType (void* type, ClangASTType *pointee_type = NULL) const;
    
    bool
    IsPointerOrReferenceType (void* type, ClangASTType *pointee_type = NULL) const;
    
    bool
    IsReferenceType (void* type, ClangASTType *pointee_type = nullptr, bool* is_rvalue = nullptr) const;
    
    bool
    IsScalarType (void* type) const;
    
    bool
    IsTypedefType (void* type) const;
    
    bool
    IsVoidType (void* type) const;
    
    bool
    GetCXXClassName (void* type, std::string &class_name) const;
    
    bool
    GetObjCClassName (void* type, std::string &class_name);
    
    
    //----------------------------------------------------------------------
    // Type Completion
    //----------------------------------------------------------------------
    
    bool
    GetCompleteType (void* type) const;
    
    //----------------------------------------------------------------------
    // AST related queries
    //----------------------------------------------------------------------
    
    size_t
    GetPointerByteSize () const;
    
    //----------------------------------------------------------------------
    // Accessors
    //----------------------------------------------------------------------
    
    clang::ASTContext *
    GetASTContext() const
    {
        return m_ast;
    }
    
    ConstString
    GetTypeName (void* type) const;
    
    uint32_t
    GetTypeInfo (void* type, ClangASTType *pointee_or_element_clang_type = NULL) const;
    
    lldb::LanguageType
    GetMinimumLanguage (void* type);
    
    lldb::TypeClass
    GetTypeClass (void* type) const;
    
    unsigned
    GetTypeQualifiers(void* type) const;
    
    //----------------------------------------------------------------------
    // Creating related types
    //----------------------------------------------------------------------
    
    ClangASTType
    AddConstModifier (void* type) const;
    
    ClangASTType
    AddRestrictModifier (void* type) const;
    
    ClangASTType
    AddVolatileModifier (void* type) const;
    
    // Using the current type, create a new typedef to that type using "typedef_name"
    // as the name and "decl_ctx" as the decl context.
    ClangASTType
    CreateTypedefType (void* type, const char *typedef_name,
                       clang::DeclContext *decl_ctx) const;
    
    ClangASTType
    GetArrayElementType (void* type, uint64_t *stride = nullptr) const;
    
    ClangASTType
    GetCanonicalType (void* type) const;
    
    ClangASTType
    GetFullyUnqualifiedType (void* type) const;
    
    // Returns -1 if this isn't a function of if the function doesn't have a prototype
    // Returns a value >= 0 if there is a prototype.
    int
    GetFunctionArgumentCount (void* type) const;
    
    ClangASTType
    GetFunctionArgumentTypeAtIndex (void* type, size_t idx) const;
    
    ClangASTType
    GetFunctionReturnType (void* type) const;
    
    size_t
    GetNumMemberFunctions (void* type) const;
    
    TypeMemberFunctionImpl
    GetMemberFunctionAtIndex (void* type, size_t idx);
    
    ClangASTType
    GetLValueReferenceType (void* type) const;
    
    ClangASTType
    GetNonReferenceType (void* type) const;
    
    ClangASTType
    GetPointeeType (void* type) const;
    
    ClangASTType
    GetPointerType (void* type) const;
    
    ClangASTType
    GetRValueReferenceType (void* type) const;
    
    // If the current object represents a typedef type, get the underlying type
    ClangASTType
    GetTypedefedType (void* type) const;
    
    ClangASTType
    RemoveFastQualifiers (void* type) const;
    
    //----------------------------------------------------------------------
    // Create related types using the current type's AST
    //----------------------------------------------------------------------
    ClangASTType
    GetBasicTypeFromAST (void* type, lldb::BasicType basic_type) const;
    
    //----------------------------------------------------------------------
    // Exploring the type
    //----------------------------------------------------------------------
    
    uint64_t
    GetByteSize (void *type, ExecutionContextScope *exe_scope) const
    {
        return (GetBitSize (type, exe_scope) + 7) / 8;
    }
    
    uint64_t
    GetBitSize (void* type, ExecutionContextScope *exe_scope) const;
    
    lldb::Encoding
    GetEncoding (void* type, uint64_t &count) const;
    
    lldb::Format
    GetFormat (void* type) const;
    
    size_t
    GetTypeBitAlign (void* type) const;
    
    uint32_t
    GetNumChildren (void* type, bool omit_empty_base_classes) const;
    
    lldb::BasicType
    GetBasicTypeEnumeration (void* type) const;
    
    static lldb::BasicType
    GetBasicTypeEnumeration (void* type, const ConstString &name);
    
    uint32_t
    GetNumDirectBaseClasses (void* type) const;
    
    uint32_t
    GetNumVirtualBaseClasses (void* type) const;
    
    uint32_t
    GetNumFields (void* type) const;
    
    ClangASTType
    GetDirectBaseClassAtIndex (void* type, size_t idx,
                               uint32_t *bit_offset_ptr) const;
    
    ClangASTType
    GetVirtualBaseClassAtIndex (void* type, size_t idx,
                                uint32_t *bit_offset_ptr) const;
    
    ClangASTType
    GetFieldAtIndex (void* type,
                     size_t idx,
                     std::string& name,
                     uint64_t *bit_offset_ptr,
                     uint32_t *bitfield_bit_size_ptr,
                     bool *is_bitfield_ptr) const;
    
    uint32_t
    GetNumPointeeChildren (void* type) const;
    
    ClangASTType
    GetChildClangTypeAtIndex (void* type,
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
                              ValueObject *valobj) const;
    
    // Lookup a child given a name. This function will match base class names
    // and member member names in "clang_type" only, not descendants.
    uint32_t
    GetIndexOfChildWithName (void* type,
                             const char *name,
                             bool omit_empty_base_classes) const;
    
    // Lookup a child member given a name. This function will match member names
    // only and will descend into "clang_type" children in search for the first
    // member in this class, or any base class that matches "name".
    // TODO: Return all matches for a given name by returning a vector<vector<uint32_t>>
    // so we catch all names that match a given child name, not just the first.
    size_t
    GetIndexOfChildMemberWithName (void* type,
                                   const char *name,
                                   bool omit_empty_base_classes,
                                   std::vector<uint32_t>& child_indexes) const;
    
    size_t
    GetNumTemplateArguments (void* type) const;
    
    ClangASTType
    GetTemplateArgument (void* type,
                         size_t idx,
                         lldb::TemplateArgumentKind &kind) const;
    
    
    //----------------------------------------------------------------------
    // Modifying RecordType
    //----------------------------------------------------------------------
    clang::FieldDecl *
    AddFieldToRecordType (void* type,
                          const char *name,
                          const ClangASTType &field_type,
                          lldb::AccessType access,
                          uint32_t bitfield_bit_size);
    
    void
    BuildIndirectFields (void* type);
    
    void
    SetIsPacked (void* type);
    
    clang::VarDecl *
    AddVariableToRecordType (void* type,
                             const char *name,
                             const ClangASTType &var_type,
                             lldb::AccessType access);
    
    clang::CXXMethodDecl *
    AddMethodToCXXRecordType (void* type,
                              const char *name,
                              const ClangASTType &method_type,
                              lldb::AccessType access,
                              bool is_virtual,
                              bool is_static,
                              bool is_inline,
                              bool is_explicit,
                              bool is_attr_used,
                              bool is_artificial);
    
    // C++ Base Classes
    clang::CXXBaseSpecifier *
    CreateBaseClassSpecifier (void* type,
                              lldb::AccessType access,
                              bool is_virtual,
                              bool base_of_class);
    
    static void
    DeleteBaseClassSpecifiers (void* type,
                               clang::CXXBaseSpecifier **base_classes,
                               unsigned num_base_classes);
    
    bool
    SetBaseClassesForClassType (void* type,
                                clang::CXXBaseSpecifier const * const *base_classes,
                                unsigned num_base_classes);
    
    
    bool
    SetObjCSuperClass (void* type,
                       const ClangASTType &superclass_clang_type);
    
    bool
    AddObjCClassProperty (void* type,
                          const char *property_name,
                          const ClangASTType &property_clang_type,
                          clang::ObjCIvarDecl *ivar_decl,
                          const char *property_setter_name,
                          const char *property_getter_name,
                          uint32_t property_attributes,
                          ClangASTMetadata *metadata);
    
    clang::ObjCMethodDecl *
    AddMethodToObjCObjectType (void* type,
                               const char *name,  // the full symbol name as seen in the symbol table (void* type, "-[NString stringWithCString:]")
                               const ClangASTType &method_clang_type,
                               lldb::AccessType access,
                               bool is_artificial);
    
    bool
    SetHasExternalStorage (void* type, bool has_extern);

    
    //------------------------------------------------------------------
    // Tag Declarations
    //------------------------------------------------------------------
    bool
    StartTagDeclarationDefinition (void* type);
    
    bool
    CompleteTagDeclarationDefinition (void* type);
    
    //----------------------------------------------------------------------
    // Modifying Enumeration types
    //----------------------------------------------------------------------
    bool
    AddEnumerationValueToEnumerationType (void* type,
                                          const ClangASTType &enumerator_qual_type,
                                          const Declaration &decl,
                                          const char *name,
                                          int64_t enum_value,
                                          uint32_t enum_value_bit_size);
    
    
    
    ClangASTType
    GetEnumerationIntegerType (void* type) const;
    
    
    //------------------------------------------------------------------
    // Pointers & References
    //------------------------------------------------------------------
    
    // Call this function using the class type when you want to make a
    // member pointer type to pointee_type.
    ClangASTType
    CreateMemberPointerType (void* type, const ClangASTType &pointee_type) const;
    
    
    // Converts "s" to a floating point value and place resulting floating
    // point bytes in the "dst" buffer.
    size_t
    ConvertStringToFloatValue (void* type,
                               const char *s,
                               uint8_t *dst,
                               size_t dst_size) const;
    //----------------------------------------------------------------------
    // Dumping types
    //----------------------------------------------------------------------
    void
    DumpValue (void* type,
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
    
    bool
    DumpTypeValue (void* type,
                   Stream *s,
                   lldb::Format format,
                   const DataExtractor &data,
                   lldb::offset_t data_offset,
                   size_t data_byte_size,
                   uint32_t bitfield_bit_size,
                   uint32_t bitfield_bit_offset,
                   ExecutionContextScope *exe_scope);
    
    void
    DumpSummary (void* type,
                 ExecutionContext *exe_ctx,
                 Stream *s,
                 const DataExtractor &data,
                 lldb::offset_t data_offset,
                 size_t data_byte_size);
    
    virtual void
    DumpTypeDescription (void* type) const; // Dump to stdout
    
    void
    DumpTypeDescription (void* type, Stream *s) const;
    
    bool
    GetValueAsScalar (void* type,
                      const DataExtractor &data,
                      lldb::offset_t data_offset,
                      size_t data_byte_size,
                      Scalar &value) const;
    
    bool
    SetValueFromScalar (void* type,
                        const Scalar &value,
                        Stream &strm);
    
    bool
    ReadFromMemory (void* type,
                    ExecutionContext *exe_ctx,
                    lldb::addr_t addr,
                    AddressType address_type,
                    DataExtractor &data);
    
    bool
    WriteToMemory (void* type,
                   ExecutionContext *exe_ctx,
                   lldb::addr_t addr,
                   AddressType address_type,
                   StreamString &new_value);
    
    clang::EnumDecl *
    GetAsEnumDecl (void* type) const;
    
    
    clang::RecordDecl *
    GetAsRecordDecl (void* type) const;
    
    clang::CXXRecordDecl *
    GetAsCXXRecordDecl (void* type) const;
    
    clang::ObjCInterfaceDecl *
    GetAsObjCInterfaceDecl (void* type) const;
    
    clang::QualType
    GetQualType (void* type) const
    {
        if (type)
            return clang::QualType::getFromOpaquePtr(type);
        return clang::QualType();
    }
    clang::QualType
    GetCanonicalQualType (void* type) const
    {
        if (type)
            return clang::QualType::getFromOpaquePtr(type).getCanonicalType();
        return clang::QualType();
    }
private:
    clang::ASTContext *m_ast;

};
    
} // namespace lldb_private

#endif // #ifndef liblldb_ClangASTTypeSystem_h_
