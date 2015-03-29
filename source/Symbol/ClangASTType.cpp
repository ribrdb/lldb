//===-- ClangASTType.cpp ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#include "lldb/Symbol/ClangASTType.h"

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
#include "lldb/Symbol/Type.h"
#include "lldb/Symbol/VerifyDecl.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/ObjCLanguageRuntime.h"
#include "lldb/Target/Process.h"

#include <iterator>
#include <mutex>

using namespace lldb;
using namespace lldb_private;

ClangASTType::ClangASTType (clang::ASTContext *ast,
                            lldb::clang_type_t type) :
m_type (type),
m_ast (ClangASTContext::GetASTContext(ast)->getTypeSystem())
{
}

ClangASTType::ClangASTType (clang::ASTContext *ast,
                            clang::QualType qual_type) :
    m_type (qual_type.getAsOpaquePtr()),
m_ast (ClangASTContext::GetASTContext(ast)->getTypeSystem())
{
}

ClangASTType::~ClangASTType()
{
}

//----------------------------------------------------------------------
// Tests
//----------------------------------------------------------------------

bool
ClangASTType::IsAggregateType () const
{
    if (IsValid())
        return m_ast->IsAggregateType(m_type);
    return false;
}

bool
ClangASTType::IsArrayType (ClangASTType *element_type_ptr,
                           uint64_t *size,
                           bool *is_incomplete) const
{
    if (IsValid())
        return m_ast->IsArrayType(m_type, element_type_ptr, size, is_incomplete);

    if (element_type_ptr)
        element_type_ptr->Clear();
    if (size)
        *size = 0;
    if (is_incomplete)
        *is_incomplete = false;
    return 0;
}

bool
ClangASTType::IsVectorType (ClangASTType *element_type,
                            uint64_t *size) const
{
    if (IsValid())
        return m_ast->IsVectorType(m_type, element_type, size);
    return false;
}

bool
ClangASTType::IsRuntimeGeneratedType () const
{
    if (IsValid())
        return m_ast->IsRuntimeGeneratedType(m_type);
    return false;
}

bool
ClangASTType::IsCharType () const
{
    if (IsValid())
        return m_ast->IsCharType(m_type);
    return false;
}


bool
ClangASTType::IsCompleteType () const
{
    if (IsValid())
        return m_ast->IsCompleteType(m_type);
    return false;
}

bool
ClangASTType::IsConst() const
{
    if (IsValid())
        return m_ast->IsConst(m_type);
    return false;
}

bool
ClangASTType::IsCStringType (uint32_t &length) const
{
    if (IsValid())
        return m_ast->IsCStringType(m_type, length);
    return false;
}

bool
ClangASTType::IsFunctionType (bool *is_variadic_ptr) const
{
    if (IsValid())
        return m_ast->IsFunctionType(m_type, is_variadic_ptr);
    return false;
}

// Used to detect "Homogeneous Floating-point Aggregates"
uint32_t
ClangASTType::IsHomogeneousAggregate (ClangASTType* base_type_ptr) const
{
    if (IsValid())
        return m_ast->IsHomogeneousAggregate(m_type, base_type_ptr);
    return 0;
}

size_t
ClangASTType::GetNumberOfFunctionArguments () const
{
    if (IsValid())
        return m_ast->GetNumberOfFunctionArguments(m_type);
    return 0;
}

ClangASTType
ClangASTType::GetFunctionArgumentAtIndex (const size_t index) const
{
    if (IsValid())
        return m_ast->GetFunctionArgumentAtIndex(m_type, index);
    return ClangASTType();
}

bool
ClangASTType::IsFunctionPointerType () const
{
    if (IsValid())
        return m_ast->IsFunctionPointerType(m_type);
    return false;

}

bool
ClangASTType::IsIntegerType (bool &is_signed) const
{
    if (IsValid())
        return m_ast->IsIntegerType(m_type, is_signed);
    return false;
}

bool
ClangASTType::IsPointerType (ClangASTType *pointee_type) const
{
    if (IsValid())
    {
        return m_ast->IsPointerType(m_type, pointee_type);
    }
    if (pointee_type)
        pointee_type->Clear();
    return false;
}


bool
ClangASTType::IsPointerOrReferenceType (ClangASTType *pointee_type) const
{
    if (IsValid())
    {
        return m_ast->IsPointerOrReferenceType(m_type, pointee_type);
    }
    if (pointee_type)
        pointee_type->Clear();
    return false;
}


bool
ClangASTType::IsReferenceType (ClangASTType *pointee_type, bool* is_rvalue) const
{
    if (IsValid())
    {
        return m_ast->IsReferenceType(m_type, pointee_type, is_rvalue);
    }
    if (pointee_type)
        pointee_type->Clear();
    return false;
}

bool
ClangASTType::IsFloatingPointType (uint32_t &count, bool &is_complex) const
{
    if (IsValid())
    {
        return m_ast->IsFloatingPointType(m_type, count, is_complex);
    }
    count = 0;
    is_complex = false;
    return false;
}


bool
ClangASTType::IsDefined() const
{
    if (IsValid())
        return m_ast->IsDefined(m_type);
    return true;
}

bool
ClangASTType::IsObjCClassType () const
{
    if (IsValid())
    {
        m_ast->IsObjCClassType(m_type);
    }
    return false;
}

bool
ClangASTType::IsObjCObjectOrInterfaceType () const
{
    if (IsValid())
        return m_ast->IsObjCObjectOrInterfaceType(m_type);
    return false;
}

bool
ClangASTType::IsPolymorphicClass () const
{
    if (IsValid())
    {
        return m_ast->IsPolymorphicClass(m_type);
    }
    return false;
}

bool
ClangASTType::IsPossibleDynamicType (ClangASTType *dynamic_pointee_type,
                                     bool check_cplusplus,
                                     bool check_objc) const
{
    if (IsValid())
        return m_ast->IsPossibleDynamicType(m_type, dynamic_pointee_type, check_cplusplus, check_objc);
    return false;
}


bool
ClangASTType::IsScalarType () const
{
    if (!IsValid())
        return false;

    return m_ast->IsScalarType(m_type);
}

bool
ClangASTType::IsTypedefType () const
{
    if (!IsValid())
        return false;
    return m_ast->IsTypedefType(m_type);
}

bool
ClangASTType::IsVoidType () const
{
    if (!IsValid())
        return false;
    return m_ast->IsVoidType(m_type);
}

bool
ClangASTType::IsPointerToScalarType () const
{
    if (!IsValid())
        return false;
    
    return IsPointerType() && GetPointeeType().IsScalarType();
}

bool
ClangASTType::IsArrayOfScalarType () const
{
    ClangASTType element_type;
    if (IsArrayType(&element_type, nullptr, nullptr))
        return element_type.IsScalarType();
    return false;
}


bool
ClangASTType::GetCXXClassName (std::string &class_name) const
{
    if (IsValid())
    {
        return m_ast->GetCXXClassName(m_type, class_name);
    }
    class_name.clear();
    return false;
}


bool
ClangASTType::IsCXXClassType () const
{
    if (!IsValid())
        return false;
    
    return m_ast->IsCXXClassType(m_type);
}

bool
ClangASTType::IsBeingDefined () const
{
    if (!IsValid())
        return false;
    return m_ast->IsBeingDefined(m_type);
}

bool
ClangASTType::IsObjCObjectPointerType (ClangASTType *class_type_ptr)
{
    if (!IsValid())
        return false;
    
    return m_ast->IsObjCObjectPointerType(m_type, class_type_ptr);
}

bool
ClangASTType::GetObjCClassName (std::string &class_name)
{
    if (!IsValid())
        return false;
    
    return m_ast->GetObjCClassName(m_type, class_name);
}


//----------------------------------------------------------------------
// Type Completion
//----------------------------------------------------------------------

bool
ClangASTType::GetCompleteType () const
{
    if (!IsValid())
        return false;
    return m_ast->GetCompleteType(m_type);
}

//----------------------------------------------------------------------
// AST related queries
//----------------------------------------------------------------------
size_t
ClangASTType::GetPointerByteSize () const
{
    if (m_ast)
        return m_ast->GetPointerByteSize();
    return 0;
}

ConstString
ClangASTType::GetConstQualifiedTypeName () const
{
    return GetConstTypeName ();
}

ConstString
ClangASTType::GetConstTypeName () const
{
    if (IsValid())
    {
        ConstString type_name (GetTypeName());
        if (type_name)
            return type_name;
    }
    return ConstString("<invalid>");
}

ConstString
ClangASTType::GetTypeName () const
{
    std::string type_name;
    if (IsValid())
    {
        m_ast->GetTypeName(m_type);
    }
    return ConstString(type_name);
}

ConstString
ClangASTType::GetDisplayTypeName () const
{
    return GetTypeName();
}

uint32_t
ClangASTType::GetTypeInfo (ClangASTType *pointee_or_element_clang_type) const
{
    if (!IsValid())
        return 0;
    
    return m_ast->GetTypeInfo(m_type);
}



lldb::LanguageType
ClangASTType::GetMinimumLanguage ()
{
    if (!IsValid())
        return lldb::eLanguageTypeC;
    
    return m_ast->GetMinimumLanguage(m_type);
}

lldb::TypeClass
ClangASTType::GetTypeClass () const
{
    if (!IsValid())
        return lldb::eTypeClassInvalid;
    
    return m_ast->GetTypeClass(m_type);
    
}

void
ClangASTType::SetClangType (clang::ASTContext *ast, lldb::clang_type_t  type)
{
    m_ast = ClangASTContext::GetASTContext(ast)->getTypeSystem();
    m_type = type;
}

void
ClangASTType::SetClangType (clang::ASTContext *ast, clang::QualType qual_type)
{
    m_ast = ClangASTContext::GetASTContext(ast)->getTypeSystem();
    m_type = qual_type.getAsOpaquePtr();
}

unsigned
ClangASTType::GetTypeQualifiers() const
{
    if (IsValid())
        return m_ast->GetTypeQualifiers(m_type);
    return 0;
}

//----------------------------------------------------------------------
// Creating related types
//----------------------------------------------------------------------

ClangASTType
ClangASTType::AddConstModifier () const
{
    if (IsValid())
        return m_ast->AddConstModifier(m_type);
    return ClangASTType();
}

ClangASTType
ClangASTType::AddRestrictModifier () const
{
    if (IsValid())
        return m_ast->AddRestrictModifier(m_type);
    return ClangASTType();
}

ClangASTType
ClangASTType::AddVolatileModifier () const
{
    if (IsValid())
        return m_ast->AddVolatileModifier(m_type);
    return ClangASTType();
}

ClangASTType
ClangASTType::GetArrayElementType (uint64_t *stride) const
{
    if (IsValid())
    {
        return m_ast->GetArrayElementType(m_type, stride);
        
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetCanonicalType () const
{
    if (IsValid())
        return m_ast->GetCanonicalType(m_type);
    return ClangASTType();
}

ClangASTType
ClangASTType::GetFullyUnqualifiedType () const
{
    if (IsValid())
        return m_ast->GetFullyUnqualifiedType(m_type);
    return ClangASTType();
}


int
ClangASTType::GetFunctionArgumentCount () const
{
    if (IsValid())
    {
        return m_ast->GetFunctionArgumentCount(m_type);
    }
    return -1;
}

ClangASTType
ClangASTType::GetFunctionArgumentTypeAtIndex (size_t idx) const
{
    if (IsValid())
    {
        return m_ast->GetFunctionArgumentTypeAtIndex(m_type, idx);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetFunctionReturnType () const
{
    if (IsValid())
    {
        return m_ast->GetFunctionReturnType(m_type);
    }
    return ClangASTType();
}

size_t
ClangASTType::GetNumMemberFunctions () const
{
    if (IsValid())
    {
        return m_ast->GetNumMemberFunctions(m_type);
    }
    return 0;
}

TypeMemberFunctionImpl
ClangASTType::GetMemberFunctionAtIndex (size_t idx)
{
    if (IsValid())
    {
        return m_ast->GetMemberFunctionAtIndex(m_type, idx);
    }
    return TypeMemberFunctionImpl();
}

ClangASTType
ClangASTType::GetLValueReferenceType () const
{
    if (IsValid())
    {
        m_ast->GetLValueReferenceType(m_type);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetRValueReferenceType () const
{
    if (IsValid())
    {
        return m_ast->GetRValueReferenceType(m_type);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetNonReferenceType () const
{
    if (IsValid())
        return m_ast->GetNonReferenceType(m_type);
    return ClangASTType();
}

ClangASTType
ClangASTType::CreateTypedefType (const char *typedef_name,
                                 clang::DeclContext *decl_ctx) const
{
    if (IsValid() && typedef_name && typedef_name[0])
    {
        return m_ast->CreateTypedefType(m_type, typedef_name, decl_ctx);
    }
    return ClangASTType();

}

ClangASTType
ClangASTType::GetPointeeType () const
{
    if (IsValid())
    {
        return m_ast->GetPointeeType(m_type);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetPointerType () const
{
    if (IsValid())
    {
        return m_ast->GetPointerType(m_type);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::GetTypedefedType () const
{
    if (IsValid())
    {
        return m_ast->GetTypedefedType(m_type);
    }
    return ClangASTType();
}

ClangASTType
ClangASTType::RemoveFastQualifiers () const
{
    if (IsValid())
    {
        return m_ast->RemoveFastQualifiers(m_type);
    }
    return ClangASTType();
}


//----------------------------------------------------------------------
// Create related types using the current type's AST
//----------------------------------------------------------------------

ClangASTType
ClangASTType::GetBasicTypeFromAST (lldb::BasicType basic_type) const
{
    if (IsValid())
        return m_ast->GetBasicTypeFromAST(m_type, basic_type);
    return ClangASTType();
}
//----------------------------------------------------------------------
// Exploring the type
//----------------------------------------------------------------------

uint64_t
ClangASTType::GetBitSize (ExecutionContextScope *exe_scope) const
{
    if (IsValid())
    {
        return m_ast->GetBitSize(m_type, exe_scope);
    }
    return 0;
}

uint64_t
ClangASTType::GetByteSize (ExecutionContextScope *exe_scope) const
{
    return (GetBitSize (exe_scope) + 7) / 8;
}


size_t
ClangASTType::GetTypeBitAlign () const
{
    if (IsValid())
        return m_ast->GetTypeBitAlign(m_type);
    return 0;
}


lldb::Encoding
ClangASTType::GetEncoding (uint64_t &count) const
{
    if (!IsValid())
        return lldb::eEncodingInvalid;
    
    return m_ast->GetEncoding(m_type, count);
}

lldb::Format
ClangASTType::GetFormat () const
{
    if (!IsValid())
        return lldb::eFormatDefault;
    
    return m_ast->GetFormat(m_type);
}

uint32_t
ClangASTType::GetNumChildren (bool omit_empty_base_classes) const
{
    if (!IsValid())
        return 0;
    return m_ast->GetNumChildren(m_type, omit_empty_base_classes);
}

lldb::BasicType
ClangASTType::GetBasicTypeEnumeration () const
{
    if (IsValid())
    {
        return m_ast->GetBasicTypeEnumeration(m_type);
    }
    return eBasicTypeInvalid;
}


#pragma mark Aggregate Types

uint32_t
ClangASTType::GetNumDirectBaseClasses () const
{
    if (!IsValid())
        return 0;
    return m_ast->GetNumDirectBaseClasses(m_type);
}

uint32_t
ClangASTType::GetNumVirtualBaseClasses () const
{
    if (!IsValid())
        return 0;
    return m_ast->GetNumVirtualBaseClasses(m_type);
}

uint32_t
ClangASTType::GetNumFields () const
{
    if (!IsValid())
        return 0;
    return m_ast->GetNumFields(m_type);
}

ClangASTType
ClangASTType::GetDirectBaseClassAtIndex (size_t idx, uint32_t *bit_offset_ptr) const
{
    if (!IsValid())
        return ClangASTType();
    return m_ast->GetDirectBaseClassAtIndex(m_type, idx, bit_offset_ptr);
}

ClangASTType
ClangASTType::GetVirtualBaseClassAtIndex (size_t idx, uint32_t *bit_offset_ptr) const
{
    if (!IsValid())
        return ClangASTType();
    return m_ast->GetVirtualBaseClassAtIndex(m_type, idx, bit_offset_ptr);
}

ClangASTType
ClangASTType::GetFieldAtIndex (size_t idx,
                               std::string& name,
                               uint64_t *bit_offset_ptr,
                               uint32_t *bitfield_bit_size_ptr,
                               bool *is_bitfield_ptr) const
{
    if (!IsValid())
        return ClangASTType();
    return m_ast->GetFieldAtIndex(m_type, idx, name, bit_offset_ptr, bitfield_bit_size_ptr, is_bitfield_ptr);
}

uint32_t
ClangASTType::GetIndexOfFieldWithName (const char* name,
                                       ClangASTType* field_clang_type_ptr,
                                       uint64_t *bit_offset_ptr,
                                       uint32_t *bitfield_bit_size_ptr,
                                       bool *is_bitfield_ptr) const
{
    unsigned count = GetNumFields();
    std::string field_name;
    for (unsigned index = 0; index < count; index++)
    {
        ClangASTType field_clang_type (GetFieldAtIndex(index, field_name, bit_offset_ptr, bitfield_bit_size_ptr, is_bitfield_ptr));
        if (strcmp(field_name.c_str(), name) == 0)
        {
            if (field_clang_type_ptr)
                *field_clang_type_ptr = field_clang_type;
            return index;
        }
    }
    return UINT32_MAX;
}

uint32_t
ClangASTType::GetNumPointeeChildren () const
{
    if (!IsValid())
        return 0;
    return m_ast->GetNumPointeeChildren(m_type);
}


ClangASTType
ClangASTType::GetChildClangTypeAtIndex (ExecutionContext *exe_ctx,
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
                                        ValueObject *valobj) const
{
    if (!IsValid())
        return ClangASTType();
    return m_ast->GetChildClangTypeAtIndex(m_type, exe_ctx, idx, transparent_pointers, omit_empty_base_classes, ignore_array_bounds, child_name, child_byte_size, child_byte_offset, child_bitfield_bit_size, child_bitfield_bit_offset, child_is_base_class, child_is_deref_of_parent, valobj);
}



size_t
ClangASTType::GetIndexOfChildMemberWithName (const char *name,
                                             bool omit_empty_base_classes,
                                             std::vector<uint32_t>& child_indexes) const
{
    if (IsValid() && name && name[0])
    {
        return m_ast->GetIndexOfChildMemberWithName(m_type, name, omit_empty_base_classes, child_indexes);
    }
    return 0;
}


// Get the index of the child of "clang_type" whose name matches. This function
// doesn't descend into the children, but only looks one level deep and name
// matches can include base class names.

uint32_t
ClangASTType::GetIndexOfChildWithName (const char *name, bool omit_empty_base_classes) const
{
    if (IsValid() && name && name[0])
        return m_ast->GetIndexOfChildWithName(m_type, name, omit_empty_base_classes);
    return UINT32_MAX;
}


size_t
ClangASTType::GetNumTemplateArguments () const
{
    if (IsValid())
        return m_ast->GetNumTemplateArguments(m_type);
    return 0;
}

ClangASTType
ClangASTType::GetTemplateArgument (size_t arg_idx, lldb::TemplateArgumentKind &kind) const
{
    if (IsValid())
        return m_ast->GetTemplateArgument(m_type, arg_idx, kind);
    kind = eTemplateArgumentKindNull;
    return ClangASTType ();
}

clang::EnumDecl *
ClangASTType::GetAsEnumDecl () const
{
    if (IsValid())
        return m_ast->GetAsEnumDecl(m_type);
    return NULL;
}

#pragma mark C++ Base Classes

void
ClangASTType::DeleteBaseClassSpecifiers (clang::CXXBaseSpecifier **base_classes, unsigned num_base_classes)
{
    for (unsigned i=0; i<num_base_classes; ++i)
    {
        delete base_classes[i];
        base_classes[i] = nullptr;
    }
}

bool
ClangASTType::IsObjCClassTypeAndHasIVars (bool check_superclass) const
{
    if (IsValid())
        return m_ast->IsObjCClassTypeAndHasIVars(m_type, check_superclass);
    return false;
}

#pragma mark TagDecl


ClangASTType
ClangASTType::CreateMemberPointerType (const ClangASTType &pointee_type) const
{
    if (IsValid() && pointee_type.IsValid())
    {
        return m_ast->CreateMemberPointerType(m_type, pointee_type);
    }
    return ClangASTType();
}


size_t
ClangASTType::ConvertStringToFloatValue (const char *s, uint8_t *dst, size_t dst_size) const
{
    if (IsValid())
        return m_ast->ConvertStringToFloatValue(m_type, s, dst, dst_size);
    return 0;
}



//----------------------------------------------------------------------
// Dumping types
//----------------------------------------------------------------------

void
ClangASTType::DumpValue (ExecutionContext *exe_ctx,
                         Stream *s,
                         lldb::Format format,
                         const lldb_private::DataExtractor &data,
                         lldb::offset_t data_byte_offset,
                         size_t data_byte_size,
                         uint32_t bitfield_bit_size,
                         uint32_t bitfield_bit_offset,
                         bool show_types,
                         bool show_summary,
                         bool verbose,
                         uint32_t depth)
{
    if (!IsValid())
        return;
    m_ast->DumpValue(m_type, exe_ctx, s, format, data, data_byte_offset, data_byte_size, bitfield_bit_size, bitfield_bit_offset, show_types, show_summary, verbose, depth);
}




bool
ClangASTType::DumpTypeValue (Stream *s,
                             lldb::Format format,
                             const lldb_private::DataExtractor &data,
                             lldb::offset_t byte_offset,
                             size_t byte_size,
                             uint32_t bitfield_bit_size,
                             uint32_t bitfield_bit_offset,
                             ExecutionContextScope *exe_scope)
{
    if (!IsValid())
        return false;
    return m_ast->DumpTypeValue(m_type, s, format, data, byte_offset, byte_size, bitfield_bit_size, bitfield_bit_offset, exe_scope);
}



void
ClangASTType::DumpSummary (ExecutionContext *exe_ctx,
                           Stream *s,
                           const lldb_private::DataExtractor &data,
                           lldb::offset_t data_byte_offset,
                           size_t data_byte_size)
{
    if (IsValid())
        m_ast->DumpSummary(m_type, exe_ctx, s, data, data_byte_offset, data_byte_size);
}

void
ClangASTType::DumpTypeDescription () const
{
    if (IsValid())
        m_ast->DumpTypeDescription(m_type);
}

void
ClangASTType::DumpTypeDescription (Stream *s) const
{
    if (IsValid())
    {
        m_ast->DumpTypeDescription(m_type, s);
    }
}

bool
ClangASTType::GetValueAsScalar (const lldb_private::DataExtractor &data,
                                lldb::offset_t data_byte_offset,
                                size_t data_byte_size,
                                Scalar &value) const
{
    if (!IsValid())
        return false;

   return m_ast->GetValueAsScalar(m_type, data, data_byte_offset, data_byte_size, value);
}

bool
ClangASTType::SetValueFromScalar (const Scalar &value, Stream &strm)
{
    // Aggregate types don't have scalar values
    if (IsValid())
        return m_ast->SetValueFromScalar(m_type, value, strm);
    return false;
}

bool
ClangASTType::ReadFromMemory (lldb_private::ExecutionContext *exe_ctx,
                              lldb::addr_t addr,
                              AddressType address_type,
                              lldb_private::DataExtractor &data)
{
    if (!IsValid())
        return false;

    return m_ast->ReadFromMemory(m_type, exe_ctx, addr, address_type, data);
}

bool
ClangASTType::WriteToMemory (lldb_private::ExecutionContext *exe_ctx,
                             lldb::addr_t addr,
                             AddressType address_type,
                             StreamString &new_value)
{
    if (!IsValid())
        return false;

    return m_ast->WriteToMemory(m_type, exe_ctx, addr, address_type, new_value);
}

clang::QualType
ClangASTType::GetQualType() const {
    if (m_type && m_ast)
        return m_ast->GetQualType(m_type);
    return clang::QualType();
}

clang::QualType
ClangASTType::GetCanonicalQualType() const {
    if (m_type && m_ast)
        return m_ast->GetCanonicalQualType(m_type);
        return clang::QualType();
}

clang::ASTContext *
ClangASTType::GetASTContext() const {
    if (m_ast)
    {
        return m_ast->GetASTContext();
    }
    return nullptr;
}

//clang::CXXRecordDecl *
//ClangASTType::GetAsCXXRecordDecl (lldb::clang_type_t opaque_clang_qual_type)
//{
//    if (opaque_clang_qual_type)
//        return clang::QualType::getFromOpaquePtr(opaque_clang_qual_type)->getAsCXXRecordDecl();
//    return NULL;
//}

bool
lldb_private::operator == (const lldb_private::ClangASTType &lhs, const lldb_private::ClangASTType &rhs)
{
    return lhs.GetASTContext() == rhs.GetASTContext() && lhs.GetOpaqueQualType() == rhs.GetOpaqueQualType();
}


bool
lldb_private::operator != (const lldb_private::ClangASTType &lhs, const lldb_private::ClangASTType &rhs)
{
    return lhs.GetASTContext() != rhs.GetASTContext() || lhs.GetOpaqueQualType() != rhs.GetOpaqueQualType();
}



