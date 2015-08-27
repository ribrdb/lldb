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
    
    virtual ClangASTContext *
    AsClangASTContext() override
    {
        return nullptr;
    }
    
    virtual GoASTContext*
    AsGoASTContext() override
    {
        return this;
    }
    
    //----------------------------------------------------------------------
    // DWARF type parsing
    //----------------------------------------------------------------------
    virtual lldb::TypeSP
    ParseTypeFromDWARF (const SymbolContext& sc,
                        SymbolFileDWARF *dwarf,
                        DWARFCompileUnit* dwarf_cu,
                        const DWARFDebugInfoEntry *die,
                        Log *log,
                        bool *type_is_new_ptr) override;
    
    virtual Function *
    ParseFunctionFromDWARF (const SymbolContext& sc,
                            SymbolFileDWARF *dwarf,
                            DWARFCompileUnit* dwarf_cu,
                            const DWARFDebugInfoEntry *die) override;
    
    void SetAddressByteSize(int byte_size) { m_pointer_byte_size = byte_size; }
    
    //----------------------------------------------------------------------
    // CompilerDeclContext functions
    //----------------------------------------------------------------------
    
    virtual bool
    DeclContextIsStructUnionOrClass (void *opaque_decl_ctx) override
    {
        return false;
    }
    
    virtual ConstString
    DeclContextGetName (void *opaque_decl_ctx) override
    {
        return ConstString();
    }
    
    virtual bool
    DeclContextIsClassMethod (void *opaque_decl_ctx,
                              lldb::LanguageType *language_ptr,
                              bool *is_instance_method_ptr,
                              ConstString *language_object_name_ptr) override
    {
        return false;
    }
    
    //----------------------------------------------------------------------
    // Creating Types
    //----------------------------------------------------------------------
    
    CompilerType
    CreateArrayType(const ConstString& name, const CompilerType& element_type, int64_t length);
    
    CompilerType
    CreateBaseType(int go_kind, const ConstString& type_name_const_str, uint64_t byte_size);
    
    // For interface, map, chan.
    CompilerType
    CreateTypedef(int kind, const ConstString& name, CompilerType impl);
    
    CompilerType
    CreateVoidType(const ConstString& name);
    CompilerType
    CreateFunctionType(const lldb_private::ConstString &name, CompilerType* params, size_t params_count, bool is_variadic);
    
    CompilerType
    CreateStructType(int kind, const ConstString& name, uint32_t byte_size);
    
    void
    CompleteStructType(const CompilerType& type);
    
    void
    AddFieldToStruct(const CompilerType& struct_type,
                     const ConstString& name,
                     const CompilerType& field_type,
                     uint32_t byte_offset);
    
    typedef void (*CompleteTypeCallback)(void *baton, CompilerType& type);
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
    
    static bool IsGoString(const CompilerType& type);
    static bool IsGoSlice(const CompilerType& type);
    static bool IsGoInterface(const CompilerType& type);
    static bool IsDirectIface(uint8_t kind);
    static bool IsPointerKind(uint8_t kind);
    
    virtual bool
    IsArrayType (void * type,
                 CompilerType *element_type,
                 uint64_t *size,
                 bool *is_incomplete) override;
    
    virtual bool
    IsAggregateType (void * type) override;
    
    virtual bool
    IsCharType (void * type) override;
    
    virtual bool
    IsCompleteType (void * type) override;
    
    virtual bool
    IsDefined(void * type) override;
    
    virtual bool
    IsFloatingPointType (void * type, uint32_t &count, bool &is_complex) override;
    
    virtual bool
    IsFunctionType (void * type, bool *is_variadic_ptr = NULL) override;
    
    virtual size_t
    GetNumberOfFunctionArguments (void * type) override;
    
    virtual CompilerType
    GetFunctionArgumentAtIndex (void * type, const size_t index) override;
    
    virtual bool
    IsFunctionPointerType (void * type) override;
    
    virtual bool
    IsIntegerType (void * type, bool &is_signed) override;
    
    virtual bool
    IsPossibleDynamicType (void * type,
                           CompilerType *target_type, // Can pass NULL
                           bool check_cplusplus,
                           bool check_objc) override;
    
    virtual bool
    IsPointerType (void * type, CompilerType *pointee_type = NULL) override;
    
    virtual bool
    IsScalarType (void * type) override;
    
    virtual bool
    IsVoidType (void * type) override;
    
    //----------------------------------------------------------------------
    // Type Completion
    //----------------------------------------------------------------------
    
    virtual bool
    GetCompleteType (void * type) override;
    
    //----------------------------------------------------------------------
    // AST related queries
    //----------------------------------------------------------------------
    
    virtual uint32_t
    GetPointerByteSize () override;
    
    //----------------------------------------------------------------------
    // Accessors
    //----------------------------------------------------------------------
    
    virtual ConstString
    GetTypeName (void * type) override;
    
    virtual uint32_t
    GetTypeInfo (void * type, CompilerType *pointee_or_element_clang_type = NULL) override;
    
    virtual lldb::LanguageType
    GetMinimumLanguage (void * type) override;
    
    virtual lldb::TypeClass
    GetTypeClass (void * type) override;
    
    //----------------------------------------------------------------------
    // Creating related types
    //----------------------------------------------------------------------
    
    virtual CompilerType
    GetArrayElementType (void * type, uint64_t *stride = nullptr) override;
    
    virtual CompilerType
    GetCanonicalType (void * type) override;
    
    // Returns -1 if this isn't a function of if the function doesn't have a prototype
    // Returns a value >= 0 if there is a prototype.
    virtual int
    GetFunctionArgumentCount (void * type) override;
    
    virtual CompilerType
    GetFunctionArgumentTypeAtIndex (void * type, size_t idx) override;
    
    virtual CompilerType
    GetFunctionReturnType (void * type) override;
    
    virtual size_t
    GetNumMemberFunctions (void * type) override;
    
    virtual TypeMemberFunctionImpl
    GetMemberFunctionAtIndex (void * type, size_t idx) override;
    
    virtual CompilerType
    GetPointeeType (void * type) override;
    
    virtual CompilerType
    GetPointerType (void * type) override;
    
    //----------------------------------------------------------------------
    // Exploring the type
    //----------------------------------------------------------------------
    
    virtual uint64_t
    GetBitSize (void * type, ExecutionContextScope *exe_scope) override;
    
    virtual lldb::Encoding
    GetEncoding (void * type, uint64_t &count) override;
    
    virtual lldb::Format
    GetFormat (void * type) override;
    
    virtual uint32_t
    GetNumChildren (void * type, bool omit_empty_base_classes) override;
    
    virtual lldb::BasicType
    GetBasicTypeEnumeration (void * type) override;
    
    virtual uint32_t
    GetNumFields (void * type) override;
    
    virtual CompilerType
    GetFieldAtIndex (void * type,
                     size_t idx,
                     std::string& name,
                     uint64_t *bit_offset_ptr,
                     uint32_t *bitfield_bit_size_ptr,
                     bool *is_bitfield_ptr) override;
    
    virtual uint32_t
    GetNumDirectBaseClasses (void *type) override
    {
        return 0;
    }
    
    virtual uint32_t
    GetNumVirtualBaseClasses (void *type) override
    {
        return 0;
    }
    
    virtual CompilerType
    GetDirectBaseClassAtIndex (void *type,
                               size_t idx,
                               uint32_t *bit_offset_ptr) override
    {
        return CompilerType();
    }
    
    virtual CompilerType
    GetVirtualBaseClassAtIndex (void *type,
                                size_t idx,
                                uint32_t *bit_offset_ptr) override
    {
        return CompilerType();
    }
    
    virtual CompilerType
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
                              ValueObject *valobj) override;
    
    // Lookup a child given a name. This function will match base class names
    // and member member names in "clang_type" only, not descendants.
    virtual uint32_t
    GetIndexOfChildWithName (void * type,
                             const char *name,
                             bool omit_empty_base_classes) override;
    
    // Lookup a child member given a name. This function will match member names
    // only and will descend into "clang_type" children in search for the first
    // member in this class, or any base class that matches "name".
    // TODO: Return all matches for a given name by returning a vector<vector<uint32_t>>
    // so we catch all names that match a given child name, not just the first.
    virtual size_t
    GetIndexOfChildMemberWithName (void * type,
                                   const char *name,
                                   bool omit_empty_base_classes,
                                   std::vector<uint32_t>& child_indexes) override;
    
    virtual size_t
    GetNumTemplateArguments (void *type) override
    {
        return 0;
    }
    
    virtual CompilerType
    GetTemplateArgument (void *type,
                         size_t idx,
                         lldb::TemplateArgumentKind &kind) override
    {
        return CompilerType();
    }
    
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
               uint32_t depth) override;
    
    virtual bool
    DumpTypeValue (void * type,
                   Stream *s,
                   lldb::Format format,
                   const DataExtractor &data,
                   lldb::offset_t data_offset,
                   size_t data_byte_size,
                   uint32_t bitfield_bit_size,
                   uint32_t bitfield_bit_offset,
                   ExecutionContextScope *exe_scope) override;
    
    virtual void
    DumpTypeDescription (void * type) override; // Dump to stdout
    
    virtual void
    DumpTypeDescription (void * type, Stream *s) override;
    
    //----------------------------------------------------------------------
    // TODO: These methods appear unused. Should they be removed?
    //----------------------------------------------------------------------
    
    virtual bool
    IsRuntimeGeneratedType (void * type) override;
    
    virtual void
    DumpSummary (void * type,
                 ExecutionContext *exe_ctx,
                 Stream *s,
                 const DataExtractor &data,
                 lldb::offset_t data_offset,
                 size_t data_byte_size) override;
        
    // Converts "s" to a floating point value and place resulting floating
    // point bytes in the "dst" buffer.
    virtual size_t
    ConvertStringToFloatValue (void * type,
                               const char *s,
                               uint8_t *dst,
                               size_t dst_size) override;
    
    //----------------------------------------------------------------------
    // TODO: Determine if these methods should move to ClangASTContext.
    //----------------------------------------------------------------------
    
    virtual bool
    IsPointerOrReferenceType (void * type, CompilerType *pointee_type = NULL) override;
    
    virtual unsigned
    GetTypeQualifiers(void * type) override;
    
    virtual bool
    IsCStringType (void * type, uint32_t &length) override;
    
    virtual size_t
    GetTypeBitAlign (void * type) override;
    
    virtual CompilerType
    GetBasicTypeFromAST (lldb::BasicType basic_type) override;
    
    virtual bool
    IsBeingDefined (void * type) override;
    
    virtual bool
    IsConst(void * type) override;
    
    virtual uint32_t
    IsHomogeneousAggregate (void * type, CompilerType* base_type_ptr) override;
    
    virtual bool
    IsPolymorphicClass (void * type) override;
    
    virtual bool
    IsTypedefType (void * type) override;
    
    // If the current object represents a typedef type, get the underlying type
    virtual CompilerType
    GetTypedefedType (void * type) override;
    
    virtual bool
    IsVectorType (void * type,
                  CompilerType *element_type,
                  uint64_t *size) override;
    
    virtual CompilerType
    GetFullyUnqualifiedType (void * type) override;
    
    virtual CompilerType
    GetNonReferenceType (void * type) override;
    
    virtual bool
    IsReferenceType (void * type, CompilerType *pointee_type = nullptr, bool* is_rvalue = nullptr) override;
    
    bool
    CompleteTypeFromDWARF (SymbolFileDWARF *dwarf,
                           DWARFCompileUnit *dwarf_cu,
                           const DWARFDebugInfoEntry* die,
                           lldb_private::Type *type,
                           CompilerType &clang_type) override;
private:
    size_t
    ParseChildParameters (const SymbolContext& sc,
                          SymbolFileDWARF *dwarf,
                          DWARFCompileUnit* dwarf_cu,
                          const DWARFDebugInfoEntry *parent_die,
                          bool &is_variadic,
                          std::vector<CompilerType>& function_param_types);
    void
    ParseChildArrayInfo (const SymbolContext& sc,
                         SymbolFileDWARF *dwarf,
                         DWARFCompileUnit* dwarf_cu,
                         const DWARFDebugInfoEntry *parent_die,
                         int64_t& first_index,
                         std::vector<uint64_t>& element_orders,
                         uint32_t& byte_stride,
                         uint32_t& bit_stride);
    
    size_t
    ParseChildMembers (const lldb_private::SymbolContext& sc,
                       SymbolFileDWARF *dwarf,
                       DWARFCompileUnit* dwarf_cu,
                       const DWARFDebugInfoEntry *die,
                       lldb_private::CompilerType &class_clang_type);
    
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
