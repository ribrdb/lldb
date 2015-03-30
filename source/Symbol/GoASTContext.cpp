//
//  GoASTContext.cpp
//  lldb
//
//  Created by Ryan Brown on 3/26/15.
//
//

#include "lldb/Symbol/ClangASTType.h"
#include "lldb/Symbol/GoASTContext.h"
#include "Expression/gofrontend/expressions.h"
#include "Expression/gofrontend/types.h"

using namespace lldb_private;

namespace {

go::Type*
GetCanonicalGoType(void* type) {
    if (!type)
        return nullptr;
    go::Type* gotype = static_cast<go::Type*>(type);
    go::Type* new_type = gotype;
    do {
        gotype = new_type;
        if (gotype->named_type())
            new_type = gotype->named_type()->real_type();
        else
            new_type = gotype->forwarded();
    } while (new_type != gotype);
    return gotype;
}
}  // anonymous namespace

//----------------------------------------------------------------------
// Tests
//----------------------------------------------------------------------

bool
GoASTContext::IsArrayType (void * type,
             ClangASTType *element_type,
             uint64_t *size,
             bool *is_incomplete) const
{
    if (is_incomplete)
        *is_incomplete = false;

    go::Type* gotype = GetCanonicalGoType(type);
    if (gotype && gotype->array_type()) {
        go::Array_type* array = gotype->array_type();
        if (element_type)
            element_type->SetClangType(this, array->element_type());
        if (size) {
            go::Numeric_constant val;
            assert(array->length()->numeric_constant_value(&val));
            unsigned long ul;
            val.to_unsigned_long(&ul);
            *size = ul;
        }
        return true;
    }
    if (element_type)
        element_type->Clear();
    if (size)
        *size = 0;
    return false;
}

bool
GoASTContext::IsVectorType (void * type,
              ClangASTType *element_type,
              uint64_t *size) const
{
    return false;
}

bool
GoASTContext::IsAggregateType (void * type) const
{
}

bool
GoASTContext::IsBeingDefined (void * type) const
{
}

bool
GoASTContext::IsCharType (void * type) const
{
}

bool
GoASTContext::IsCompleteType (void * type) const
{
}

bool
GoASTContext::IsConst(void * type) const
{
}

bool
GoASTContext::IsCStringType (void * type, uint32_t &length) const
{
}

bool
GoASTContext::IsCXXClassType (void * type) const
{
}

bool
GoASTContext::IsDefined(void * type) const
{
}

bool
GoASTContext::IsFloatingPointType (void * type, uint32_t &count, bool &is_complex) const
{
}

bool
GoASTContext::IsFunctionType (void * type, bool *is_variadic_ptr = NULL) const
{
}

uint32_t
GoASTContext::IsHomogeneousAggregate (void * type, ClangASTType* base_type_ptr) const
{
}

size_t
GoASTContext::GetNumberOfFunctionArguments (void * type) const
{
}

ClangASTType
GoASTContext::GetFunctionArgumentAtIndex (void * type, const size_t index) const
{
}

bool
GoASTContext::IsFunctionPointerType (void * type) const
{
}

bool
GoASTContext::IsIntegerType (void * type, bool &is_signed) const
{
}

bool
GoASTContext::IsObjCClassType (void * type) const
{
}

bool
GoASTContext::IsObjCClassTypeAndHasIVars (void * type, bool check_superclass) const
{
}

bool
GoASTContext::IsObjCObjectOrInterfaceType (void * type) const
{
}

bool
GoASTContext::IsObjCObjectPointerType (void * type, ClangASTType *target_type = NULL)
{
}

bool
GoASTContext::IsPolymorphicClass (void * type) const
{
}

bool
GoASTContext::IsPossibleDynamicType (void * type,
                       ClangASTType *target_type, // Can pass NULL
                       bool check_cplusplus,
                       bool check_objc) const
{
}

bool
GoASTContext::IsRuntimeGeneratedType (void * type) const
{
}

bool
GoASTContext::IsPointerType (void * type, ClangASTType *pointee_type = NULL) const
{
}

bool
GoASTContext::IsPointerOrReferenceType (void * type, ClangASTType *pointee_type = NULL) const
{
}

bool
GoASTContext::IsReferenceType (void * type, ClangASTType *pointee_type = nullptr, bool* is_rvalue = nullptr) const
{
}

bool
GoASTContext::IsScalarType (void * type) const
{
}

bool
GoASTContext::IsTypedefType (void * type) const
{
}

bool
GoASTContext::IsVoidType (void * type) const
{
}

bool
GoASTContext::GetCXXClassName (void * type, std::string &class_name) const
{
}

bool
GoASTContext::GetObjCClassName (void * type, std::string &class_name)
{
}


//----------------------------------------------------------------------
// Type Completion
//----------------------------------------------------------------------

bool
GoASTContext::GetCompleteType (void * type) const
{
}

//----------------------------------------------------------------------
// AST related queries
//----------------------------------------------------------------------

size_t
GoASTContext::GetPointerByteSize () const
{
}

//----------------------------------------------------------------------
// Accessors
//----------------------------------------------------------------------

ConstString
GoASTContext::GetTypeName (void * type) const
{
}

uint32_t
GoASTContext::GetTypeInfo (void * type, ClangASTType *pointee_or_element_clang_type = NULL) const
{
}

lldb::LanguageType
GoASTContext::GetMinimumLanguage (void * type)
{
}

lldb::TypeClass
GoASTContext::GetTypeClass (void * type) const
{
}

unsigned
GoASTContext::GetTypeQualifiers(void * type) const
{
}

//----------------------------------------------------------------------
// Creating related types
//----------------------------------------------------------------------

ClangASTType
GoASTContext::AddConstModifier (void * type) const
{
}

ClangASTType
GoASTContext::AddRestrictModifier (void * type) const
{
}

ClangASTType
GoASTContext::AddVolatileModifier (void * type) const
{
}

// Using the current type, create a new typedef to that type using "typedef_name"
// as the name and "decl_ctx" as the decl context.
ClangASTType
GoASTContext::CreateTypedefType (void * type, const char *typedef_name,
                   clang::DeclContext *decl_ctx) const
{
}

ClangASTType
GoASTContext::GetArrayElementType (void * type, uint64_t *stride = nullptr) const
{
}

ClangASTType
GoASTContext::GetCanonicalType (void * type) const
{
}

ClangASTType
GoASTContext::GetFullyUnqualifiedType (void * type) const
{
}

// Returns -1 if this isn't a function of if the function doesn't have a prototype
// Returns a value >= 0 if there is a prototype.
int
GoASTContext::GetFunctionArgumentCount (void * type) const
{
}

ClangASTType
GoASTContext::GetFunctionArgumentTypeAtIndex (void * type, size_t idx) const
{
}

ClangASTType
GoASTContext::GetFunctionReturnType (void * type) const
{
}

size_t
GoASTContext::GetNumMemberFunctions (void * type) const
{
}

TypeMemberFunctionImpl
GoASTContext::GetMemberFunctionAtIndex (void * type, size_t idx)
{
}

ClangASTType
GoASTContext::GetLValueReferenceType (void * type) const
{
}

ClangASTType
GoASTContext::GetNonReferenceType (void * type) const
{
}

ClangASTType
GoASTContext::GetPointeeType (void * type) const
{
}

ClangASTType
GoASTContext::GetPointerType (void * type) const
{
}

ClangASTType
GoASTContext::GetRValueReferenceType (void * type) const
{
}

// If the current object represents a typedef type, get the underlying type
ClangASTType
GoASTContext::GetTypedefedType (void * type) const
{
}

ClangASTType
GoASTContext::RemoveFastQualifiers (void * type) const
{
}

//----------------------------------------------------------------------
// Create related types using the current type's AST
//----------------------------------------------------------------------
ClangASTType
GoASTContext::GetBasicTypeFromAST (void * type, lldb::BasicType basic_type) const
{
}

//----------------------------------------------------------------------
// Exploring the type
//----------------------------------------------------------------------

uint64_t
GoASTContext::GetBitSize (void * type, ExecutionContextScope *exe_scope) const
{
}

lldb::Encoding
GoASTContext::GetEncoding (void * type, uint64_t &count) const
{
}

lldb::Format
GoASTContext::GetFormat (void * type) const
{
}

size_t
GoASTContext::GetTypeBitAlign (void * type) const
{
}

uint32_t
GoASTContext::GetNumChildren (void * type, bool omit_empty_base_classes) const
{
}

lldb::BasicType
GoASTContext::GetBasicTypeEnumeration (void * type) const
{
}

uint32_t
GoASTContext::GetNumDirectBaseClasses (void * type) const
{
}

uint32_t
GoASTContext::GetNumVirtualBaseClasses (void * type) const
{
}

uint32_t
GoASTContext::GetNumFields (void * type) const
{
}

ClangASTType
GoASTContext::GetDirectBaseClassAtIndex (void * type, size_t idx,
                           uint32_t *bit_offset_ptr) const
{
}

ClangASTType
GoASTContext::GetVirtualBaseClassAtIndex (void * type, size_t idx,
                            uint32_t *bit_offset_ptr) const
{
}

ClangASTType
GoASTContext::GetFieldAtIndex (void * type,
                 size_t idx,
                 std::string& name,
                 uint64_t *bit_offset_ptr,
                 uint32_t *bitfield_bit_size_ptr,
                 bool *is_bitfield_ptr) const
{
}

uint32_t
GoASTContext::GetNumPointeeChildren (void * type) const
{
}

ClangASTType
GoASTContext::GetChildClangTypeAtIndex (void * type,
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
                          ValueObject *valobj) const
{
}

// Lookup a child given a name. This function will match base class names
// and member member names in "clang_type" only, not descendants.
uint32_t
GoASTContext::GetIndexOfChildWithName (void * type,
                         const char *name,
                         bool omit_empty_base_classes) const
{
}

// Lookup a child member given a name. This function will match member names
// only and will descend into "clang_type" children in search for the first
// member in this class, or any base class that matches "name".
// TODO: Return all matches for a given name by returning a vector<vector<uint32_t>>
// so we catch all names that match a given child name, not just the first.
size_t
GoASTContext::GetIndexOfChildMemberWithName (void * type,
                               const char *name,
                               bool omit_empty_base_classes,
                               std::vector<uint32_t>& child_indexes) const
{
}

size_t
GoASTContext::GetNumTemplateArguments (void * type) const
{
}

ClangASTType
GoASTContext::GetTemplateArgument (void * type,
                     size_t idx,
                     lldb::TemplateArgumentKind &kind) const
{
}

//------------------------------------------------------------------
// Pointers & References
//------------------------------------------------------------------

// Call this function using the class type when you want to make a
// member pointer type to pointee_type.
ClangASTType
GoASTContext::CreateMemberPointerType (void * type, const ClangASTType &pointee_type) const
{
}


// Converts "s" to a floating point value and place resulting floating
// point bytes in the "dst" buffer.
size_t
GoASTContext::ConvertStringToFloatValue (void * type,
                           const char *s,
                           uint8_t *dst,
                           size_t dst_size) const
{
}
//----------------------------------------------------------------------
// Dumping types
//----------------------------------------------------------------------
void
GoASTContext::DumpValue (void * type,
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
           uint32_t depth)
{
}

bool
GoASTContext::DumpTypeValue (void * type,
               Stream *s,
               lldb::Format format,
               const DataExtractor &data,
               lldb::offset_t data_offset,
               size_t data_byte_size,
               uint32_t bitfield_bit_size,
               uint32_t bitfield_bit_offset,
               ExecutionContextScope *exe_scope)
{
}

void
GoASTContext::DumpSummary (void * type,
             ExecutionContext *exe_ctx,
             Stream *s,
             const DataExtractor &data,
             lldb::offset_t data_offset,
             size_t data_byte_size)
{
}

void
GoASTContext::DumpTypeDescription (void * type) const
{
} // Dump to stdout

void
GoASTContext::DumpTypeDescription (void * type, Stream *s) const
{
}

bool
GoASTContext::GetValueAsScalar (void * type,
                  const DataExtractor &data,
                  lldb::offset_t data_offset,
                  size_t data_byte_size,
                  Scalar &value) const
{
}

bool
GoASTContext::SetValueFromScalar (void * type,
                    const Scalar &value,
                    Stream &strm)
{
}

bool
GoASTContext::ReadFromMemory (void * type,
                ExecutionContext *exe_ctx,
                lldb::addr_t addr,
                AddressType address_type,
                DataExtractor &data)
{
}

bool
GoASTContext::WriteToMemory (void * type,
               ExecutionContext *exe_ctx,
               lldb::addr_t addr,
               AddressType address_type,
               StreamString &new_value)
{
}

clang::EnumDecl *
GoASTContext::GetAsEnumDecl (void * type) const
{
}

clang::QualType
GoASTContext::GetQualType (void * type) const
{
}

clang::QualType
GoASTContext::GetCanonicalQualType (void * type) const
{
}
