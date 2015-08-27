//
//  GoASTContext.cpp
//  lldb
//
//  Created by Ryan Brown on 3/26/15.
//
//

#include <vector>
#include <utility>

#include "lldb/Core/ValueObject.h"
#include "lldb/DataFormatters/StringPrinter.h"
#include "lldb/Symbol/CompilerType.h"
#include "lldb/Symbol/GoASTContext.h"
#include "lldb/Symbol/Type.h"
#include "lldb/Target/ExecutionContext.h"

//#define ENABLE_DEBUG_PRINTF // COMMENT OUT THIS LINE PRIOR TO CHECKIN

#ifdef ENABLE_DEBUG_PRINTF
#include <stdio.h>
#define DEBUG_PRINTF(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define DEBUG_PRINTF(fmt, ...)
#endif

#define DW_AT_go_kind 0x2900
#define DW_AT_go_key  0x2901
#define DW_AT_go_elem 0x2902

using namespace lldb;

namespace lldb_private
{
class GoArray;
class GoFunction;
class GoStruct;
    
class GoType
{
public:
    enum {
        KIND_BOOL          = 1,
        KIND_INT           = 2,
        KIND_INT8          = 3,
        KIND_INT16         = 4,
        KIND_INT32         = 5,
        KIND_INT64         = 6,
        KIND_UINT          = 7,
        KIND_UINT8         = 8,
        KIND_UINT16        = 9,
        KIND_UINT32        = 10,
        KIND_UINT64        = 11,
        KIND_UINTPTR       = 12,
        KIND_FLOAT32       = 13,
        KIND_FLOAT64       = 14,
        KIND_COMPLEX64     = 15,
        KIND_COMPLEX128    = 16,
        KIND_ARRAY         = 17,
        KIND_CHAN          = 18,
        KIND_FUNC          = 19,
        KIND_INTERFACE     = 20,
        KIND_MAP           = 21,
        KIND_PTR           = 22,
        KIND_SLICE         = 23,
        KIND_STRING        = 24,
        KIND_STRUCT        = 25,
        KIND_UNSAFEPOINTER = 26,
        KIND_LLDB_VOID,          // Extension for LLDB, not used by go runtime.
        KIND_MASK = (1 << 5) - 1,
        KIND_DIRECT_IFACE = 1 << 5
    };
    GoType(int kind, const ConstString& name) : m_kind(kind & KIND_MASK), m_name(name) {
        if (m_kind == KIND_FUNC)
            m_kind = KIND_FUNC;
    }
    virtual ~GoType() { }
    
    int GetGoKind() const { return m_kind; }
    const ConstString& GetName() const { return m_name; }
    virtual CompilerType GetElementType() const
    {
        return CompilerType();
    }
    
    bool
    IsTypedef() const
    {
        switch (m_kind)
        {
            case KIND_CHAN:
            case KIND_MAP:
            case KIND_INTERFACE:
                return true;
            default:
                return false;
        }
    }
    
    GoArray* GetArray();
    GoFunction* GetFunction();
    GoStruct* GetStruct();
private:
    int m_kind;
    ConstString m_name;
    GoType(const GoType&) = delete;
    const GoType& operator=(const GoType&) = delete;
};

class GoElem : public GoType
{
public:
    GoElem(int kind, const ConstString& name, const CompilerType& elem) : GoType(kind, name), m_elem(elem) { }
    virtual CompilerType GetElementType() const { return m_elem; }

private:
    // TODO: should we store this differently?
    CompilerType m_elem;

    GoElem(const GoElem&) = delete;
    const GoElem& operator=(const GoElem&) = delete;
};
    
class GoArray : public GoElem
{
public:
    GoArray(const ConstString& name, int64_t length, const CompilerType& elem) : GoElem(KIND_ARRAY, name, elem), m_length(length) { }
    
    int64_t GetLength() const { return m_length; }
private:
    int64_t m_length;
    GoArray(const GoArray&) = delete;
    const GoArray& operator=(const GoArray&) = delete;
};

class GoFunction : public GoType
{
public:
    GoFunction(const ConstString& name, bool is_variadic) : GoType(KIND_FUNC, name), m_is_variadic(is_variadic) { }
    
    bool IsVariadic() const { return m_is_variadic; }
private:
    bool m_is_variadic;
    GoFunction(const GoFunction&) = delete;
    const GoFunction& operator=(const GoFunction&) = delete;
};

class GoStruct : public GoType
{
public:
    struct Field {
        Field(const ConstString& name, const CompilerType& type, uint64_t offset) : m_name(name), m_type(type), m_byte_offset(offset)
        {
        }
        ConstString m_name;
        CompilerType m_type;
        uint64_t m_byte_offset;
    };

    GoStruct(int kind, const ConstString& name, int64_t byte_size) : GoType(kind, name), m_is_complete(false), m_byte_size(byte_size) { }

    uint32_t GetNumFields() const { return m_fields.size(); }

    const Field* GetField(uint32_t i) const
    {
        if (i < m_fields.size())
            return &m_fields[i];
        return nullptr;
    }

    void
    AddField(const ConstString& name, const CompilerType& type, uint64_t offset)
    {
        m_fields.push_back(Field(name, type, offset));
    }
    
    bool
    IsComplete() const { return m_is_complete; }
    
    void SetComplete() { m_is_complete = true; }
    
    int64_t
    GetByteSize() const { return m_byte_size; }
    
private:
    bool m_is_complete;
    int64_t m_byte_size;
    std::vector<Field> m_fields;

    
    GoStruct(const GoStruct&) = delete;
    const GoStruct& operator=(const GoStruct&) = delete;
};
    
GoArray*
GoType::GetArray()
{
    if (this && m_kind == KIND_ARRAY) {
        return static_cast<GoArray*>(this);
    }
    return nullptr;
}
    
    
GoFunction*
GoType::GetFunction()
{
    if (this && m_kind == KIND_FUNC) {
        return static_cast<GoFunction*>(this);
    }
    return nullptr;
}

GoStruct*
GoType::GetStruct()
{
    if (this) {
        switch (m_kind)
        {
            case KIND_STRING:
            case KIND_STRUCT:
            case KIND_SLICE:
                return static_cast<GoStruct*>(this);
        }
    }
    return nullptr;
}
}  // namespace lldb_private
using namespace lldb_private;

GoASTContext::GoASTContext() : m_pointer_byte_size(0), m_int_byte_size(0), m_types(new TypeMap), m_complete_type(nullptr), m_complete_type_this(nullptr) { }
GoASTContext::~GoASTContext() { }

//----------------------------------------------------------------------
// Tests
//----------------------------------------------------------------------

bool
GoASTContext::IsArrayType (void * type,
                           CompilerType *element_type,
                           uint64_t *size,
                           bool *is_incomplete)
{
    if (element_type)
        element_type->Clear();
    if (size)
        *size = 0;
    if (is_incomplete)
        *is_incomplete = false;
    GoArray* array = static_cast<GoType*>(type)->GetArray();
    if (array) {
        if (array->GetLength() == -1)
        {
            if (is_incomplete)
            {
                *is_incomplete = true;
            }
        } else if (size)
        {
            *size = array->GetLength();
        }
        if (element_type)
            *element_type = array->GetElementType();
        return true;
    }
    return false;
}

bool
GoASTContext::IsVectorType (void * type,
              CompilerType *element_type,
              uint64_t *size)
{
    if (element_type)
        element_type->Clear();
    if (size)
        *size = 0;
    return false;
}

bool
GoASTContext::IsAggregateType (void * type)
{
    int kind = static_cast<GoType*>(type)->GetGoKind();
    if (kind < GoType::KIND_ARRAY)
        return false;
    if (kind == GoType::KIND_PTR)
        return false;
    if (kind == GoType::KIND_STRING)
        return false;
    if (kind == GoType::KIND_UNSAFEPOINTER)
        return false;
    return true;
}

bool
GoASTContext::IsBeingDefined (void * type)
{
    return false;
}

bool
GoASTContext::IsCharType (void * type)
{
    // Go's DWARF doesn't distinguish between rune and int32.
    return false;
}

bool
GoASTContext::IsCompleteType (void * type)
{
    if (!type)
        return false;
    GoType* t = static_cast<GoType*>(type);
    if (GoStruct* s = t->GetStruct())
        return s->IsComplete();
    if (t->IsTypedef() || t->GetGoKind() == GoType::KIND_PTR)
        return t->GetElementType().IsCompleteType();
    return true;
}

bool
GoASTContext::IsConst(void * type)
{
    return false;
}

bool
GoASTContext::IsCStringType (void * type, uint32_t &length)
{
    return false;
}

bool
GoASTContext::IsDefined(void * type)
{
    return type != nullptr;
}

bool
GoASTContext::IsFloatingPointType (void * type, uint32_t &count, bool &is_complex)
{
    int kind = static_cast<GoType*>(type)->GetGoKind();
    if (kind >= GoType::KIND_FLOAT32 && kind <= GoType::KIND_COMPLEX128) {
        if (kind >= GoType::KIND_COMPLEX64) {
            is_complex = true;
            count = 2;
        }
        else
        {
            is_complex = false;
            count = 1;
        }
        return true;
    }
    count = 0;
    is_complex = false;
    return false;
}

bool
GoASTContext::IsFunctionType (void * type, bool *is_variadic_ptr)
{
    GoFunction* func = static_cast<GoType*>(type)->GetFunction();
    if (func)
    {
        if (is_variadic_ptr)
            *is_variadic_ptr = func->IsVariadic();
        return true;
    }
    if (is_variadic_ptr)
        *is_variadic_ptr = false;
    return false;
}

uint32_t
GoASTContext::IsHomogeneousAggregate (void * type, CompilerType* base_type_ptr)
{
    return false;
}

size_t
GoASTContext::GetNumberOfFunctionArguments (void * type)
{
    assert(false);
}

CompilerType
GoASTContext::GetFunctionArgumentAtIndex (void * type, const size_t index)
{
    assert(false);
}

bool
GoASTContext::IsFunctionPointerType (void * type)
{
    return IsFunctionType(type);
}

bool
GoASTContext::IsIntegerType (void * type, bool &is_signed)
{
    is_signed = false;
    // TODO: Is bool an integer?
    if (type)
    {
        int kind = static_cast<GoType*>(type)->GetGoKind();
        if (kind <= GoType::KIND_UINTPTR) {
            is_signed = (kind != GoType::KIND_BOOL) & (kind <= GoType::KIND_INT64);
            return true;
        }
    }
    return false;
}

bool
GoASTContext::IsPolymorphicClass (void * type)
{
    return false;
}

bool
GoASTContext::IsPossibleDynamicType (void * type,
                                     CompilerType *target_type, // Can pass NULL
                                     bool check_cplusplus,
                                     bool check_objc)
{
    if (target_type)
        target_type->Clear();
    if (type)
        return static_cast<GoType*>(type)->GetGoKind() == GoType::KIND_INTERFACE;
    return false;
}

bool
GoASTContext::IsRuntimeGeneratedType (void * type)
{
    return false;
}

bool
GoASTContext::IsPointerType (void * type, CompilerType *pointee_type)
{
    if (!type)
        return false;
    GoType* t = static_cast<GoType*>(type);
    if (pointee_type)
    {
        *pointee_type = t->GetElementType();
    }
    switch (t->GetGoKind()) {
        case GoType::KIND_PTR:
        case GoType::KIND_UNSAFEPOINTER:
        case GoType::KIND_CHAN:
        // TODO: is map a pointer? string? function?
            return true;
        default:
            return false;
    }
}

bool
GoASTContext::IsPointerOrReferenceType (void * type, CompilerType *pointee_type)
{
    return IsPointerType(type, pointee_type);
}

bool
GoASTContext::IsReferenceType (void * type, CompilerType *pointee_type, bool* is_rvalue)
{
    return false;
}

bool
GoASTContext::IsScalarType (void * type)
{
    return !IsAggregateType(type);
}

bool
GoASTContext::IsTypedefType (void * type)
{
    if (type)
        return static_cast<GoType*>(type)->IsTypedef();
    return false;
}

bool
GoASTContext::IsVoidType (void * type)
{
    if (!type)
        return false;
    return static_cast<GoType*>(type)->GetGoKind() == GoType::KIND_LLDB_VOID;
}

//----------------------------------------------------------------------
// Type Completion
//----------------------------------------------------------------------

bool
GoASTContext::GetCompleteType (void * type)
{
    if (!type)
        return false;
    GoType* t = static_cast<GoType*>(type);
    if (t->IsTypedef() || t->GetGoKind() == GoType::KIND_PTR || t->GetArray())
        return t->GetElementType().GetCompleteType();
    if (GoStruct* s = t->GetStruct())
    {
        if (s->IsComplete())
            return true;
        if (!m_complete_type)
            return false;
        CompilerType clang_type(this, s);
        m_complete_type(m_complete_type_this, clang_type);
    }
            return true;
}

//----------------------------------------------------------------------
// AST related queries
//----------------------------------------------------------------------

uint32_t
GoASTContext::GetPointerByteSize ()
{
    return m_pointer_byte_size;
}

//----------------------------------------------------------------------
// Accessors
//----------------------------------------------------------------------

ConstString
GoASTContext::GetTypeName (void * type)
{
    if (type)
        return static_cast<GoType*>(type)->GetName();
    return ConstString();
}

uint32_t
GoASTContext::GetTypeInfo (void * type, CompilerType *pointee_or_element_clang_type)
{
    if (pointee_or_element_clang_type)
        pointee_or_element_clang_type->Clear();
    if (!type)
        return 0;
    GoType* t = static_cast<GoType*>(type);
    if (pointee_or_element_clang_type)
        *pointee_or_element_clang_type = t->GetElementType();
    int kind = t->GetGoKind();
    if (kind == GoType::KIND_ARRAY)
        return eTypeHasChildren | eTypeIsArray;
    if (kind < GoType::KIND_ARRAY)
    {
        uint32_t builtin_type_flags = eTypeIsBuiltIn | eTypeHasValue;
        if (kind < GoType::KIND_FLOAT32)
        {
            builtin_type_flags |= eTypeIsInteger | eTypeIsScalar;
            if (kind >= GoType::KIND_INT && kind <= GoType::KIND_INT64)
                builtin_type_flags |= eTypeIsSigned;
        }
        else
        {
            builtin_type_flags |= eTypeIsFloat;
            if (kind < GoType::KIND_COMPLEX64)
                builtin_type_flags |= eTypeIsComplex;
            else
                builtin_type_flags |= eTypeIsScalar;
        }
        return builtin_type_flags;
    }
    if (kind == GoType::KIND_STRING)
        return eTypeHasValue | eTypeIsBuiltIn;
    if (kind == GoType::KIND_FUNC)
        return eTypeIsFuncPrototype | eTypeHasValue;
    if (IsPointerType(type))
        return eTypeIsPointer | eTypeHasValue | eTypeHasChildren;
    if (kind == GoType::KIND_LLDB_VOID)
        return 0;
    return eTypeHasChildren | eTypeIsStructUnion;
}

lldb::TypeClass
GoASTContext::GetTypeClass (void * type)
{
    if (!type)
        return eTypeClassInvalid;
    int kind = static_cast<GoType*>(type)->GetGoKind();
    if (kind == GoType::KIND_FUNC)
        return eTypeClassFunction;
    if (IsPointerType(type))
        return eTypeClassPointer;
    if (kind < GoType::KIND_COMPLEX64)
        return eTypeClassBuiltin;
    if (kind <= GoType::KIND_COMPLEX128)
        return eTypeClassComplexFloat;
    if (kind == GoType::KIND_LLDB_VOID)
        return eTypeClassInvalid;
    return eTypeClassStruct;
}

    lldb::BasicType
GoASTContext::GetBasicTypeEnumeration (void * type)
{
    assert(false);
}

lldb::LanguageType
GoASTContext::GetMinimumLanguage (void * type)
{
    return lldb::eLanguageTypeGo;
}

unsigned
GoASTContext::GetTypeQualifiers(void * type)
{
    return 0;
}

//----------------------------------------------------------------------
// Creating related types
//----------------------------------------------------------------------

CompilerType
GoASTContext::GetArrayElementType (void * type, uint64_t *stride)
{
    GoArray* array = static_cast<GoType*>(type)->GetArray();
    if (array)
    {
        if (stride)
        {
            *stride = array->GetElementType().GetByteSize(nullptr);
        }
        return array->GetElementType();
    }
    return CompilerType();
}

CompilerType
GoASTContext::GetCanonicalType (void * type)
{
    GoType* t = static_cast<GoType*>(type);
    if (t->IsTypedef())
        return t->GetElementType();
    return CompilerType(this, type);
}

CompilerType
GoASTContext::GetFullyUnqualifiedType (void * type)
{
    return CompilerType(this, type);
}

// Returns -1 if this isn't a function of if the function doesn't have a prototype
// Returns a value >= 0 if there is a prototype.
int
GoASTContext::GetFunctionArgumentCount (void * type)
{
    return GetNumberOfFunctionArguments(type);
}

CompilerType
GoASTContext::GetFunctionArgumentTypeAtIndex (void * type, size_t idx)
{
    return GetFunctionArgumentAtIndex(type, idx);
}

CompilerType
GoASTContext::GetFunctionReturnType (void * type)
{
    CompilerType result;
    if (type)
    {
        GoType* t = static_cast<GoType*>(type);
        if (t->GetGoKind() == GoType::KIND_FUNC)
            result = t->GetElementType();
    }
    return result;
}

size_t
GoASTContext::GetNumMemberFunctions (void * type)
{
    assert(false);
}

TypeMemberFunctionImpl
GoASTContext::GetMemberFunctionAtIndex (void * type, size_t idx)
{
    assert(false);
}

CompilerType
GoASTContext::GetNonReferenceType (void * type)
{
    return CompilerType(this, type);
}

CompilerType
GoASTContext::GetPointeeType (void * type)
{
    if (!type)
        return CompilerType();
    return static_cast<GoType*>(type)->GetElementType();
}

CompilerType
GoASTContext::GetPointerType (void * type)
{
    if (!type)
        return CompilerType();
    ConstString type_name = GetTypeName(type);
    ConstString pointer_name(std::string("*") + type_name.GetCString());
    GoType* pointer = (*m_types)[pointer_name].get();
    if (pointer == nullptr)
    {
        pointer = new GoElem(GoType::KIND_PTR, pointer_name, CompilerType(this, type));
        (*m_types)[pointer_name].reset(pointer);
    }
    return CompilerType(this, pointer);
}

// If the current object represents a typedef type, get the underlying type
CompilerType
GoASTContext::GetTypedefedType (void * type)
{
    if (IsTypedefType(type))
        return static_cast<GoType*>(type)->GetElementType();
    return CompilerType();
}

//----------------------------------------------------------------------
// Create related types using the current type's AST
//----------------------------------------------------------------------
CompilerType
GoASTContext::GetBasicTypeFromAST (lldb::BasicType basic_type)
{
    assert(false);
}

//----------------------------------------------------------------------
// Exploring the type
//----------------------------------------------------------------------

uint64_t
GoASTContext::GetBitSize (void * type, ExecutionContextScope *exe_scope)
{
    if (!type)
        return 0;
    if (!GetCompleteType(type))
        return 0;
    GoType* t = static_cast<GoType*>(type);
    GoArray* array = nullptr;
    switch (t->GetGoKind())
    {
        case GoType::KIND_BOOL:
        case GoType::KIND_INT8:
        case GoType::KIND_UINT8:
            return 8;
        case GoType::KIND_INT16:
        case GoType::KIND_UINT16:
            return 16;
        case GoType::KIND_INT32:
        case GoType::KIND_UINT32:
        case GoType::KIND_FLOAT32:
            return 32;
        case GoType::KIND_INT64:
        case GoType::KIND_UINT64:
        case GoType::KIND_FLOAT64:
        case GoType::KIND_COMPLEX64:
            return 64;
        case GoType::KIND_COMPLEX128:
            return 128;
        case GoType::KIND_INT:
        case GoType::KIND_UINT:
            return m_int_byte_size * 8;
        case GoType::KIND_UINTPTR:
        case GoType::KIND_FUNC: // I assume this is a pointer?
        case GoType::KIND_CHAN:
        case GoType::KIND_PTR:
        case GoType::KIND_UNSAFEPOINTER:
        case GoType::KIND_MAP:
            return m_pointer_byte_size * 8;
        case GoType::KIND_ARRAY:
            array = t->GetArray();
            return array->GetLength() * array->GetElementType().GetBitSize(exe_scope);
        case GoType::KIND_INTERFACE:
            return t->GetElementType().GetBitSize(exe_scope);
        case GoType::KIND_SLICE:
        case GoType::KIND_STRING:
        case GoType::KIND_STRUCT:
            return t->GetStruct()->GetByteSize() * 8;
        default:
            assert(false);
    }
}

lldb::Encoding
GoASTContext::GetEncoding (void * type, uint64_t &count)
{
    count = 1;
    bool is_signed;
    if (IsIntegerType(type, is_signed))
        return is_signed ? lldb::eEncodingSint : eEncodingUint;
    bool is_complex;
    uint32_t complex_count;
    if (IsFloatingPointType(type, complex_count, is_complex))
    {
        count = complex_count;
        return eEncodingIEEE754;
    }
    if (IsPointerType(type))
        return eEncodingUint;
    return eEncodingInvalid;
}

lldb::Format
GoASTContext::GetFormat (void * type)
{
    if (!type)
        return eFormatDefault;
    switch (static_cast<GoType*>(type)->GetGoKind())
    {
        case GoType::KIND_BOOL:
            return eFormatBoolean;
        case GoType::KIND_INT:
        case GoType::KIND_INT8:
        case GoType::KIND_INT16:
        case GoType::KIND_INT32:
        case GoType::KIND_INT64:
            return eFormatDecimal;
        case GoType::KIND_UINT:
        case GoType::KIND_UINT8:
        case GoType::KIND_UINT16:
        case GoType::KIND_UINT32:
        case GoType::KIND_UINT64:
            return eFormatUnsigned;
        case GoType::KIND_FLOAT32:
        case GoType::KIND_FLOAT64:
            return eFormatFloat;
        case GoType::KIND_COMPLEX64:
        case GoType::KIND_COMPLEX128:
            return eFormatComplexFloat;
        case GoType::KIND_UINTPTR:
        case GoType::KIND_CHAN:
        case GoType::KIND_PTR:
        case GoType::KIND_MAP:
        case GoType::KIND_UNSAFEPOINTER:
            return eFormatHex;
        case GoType::KIND_STRING:
            return eFormatCString;
        case GoType::KIND_ARRAY:
        case GoType::KIND_INTERFACE:
        case GoType::KIND_SLICE:
        case GoType::KIND_STRUCT:
        default:
            // Don't know how to display this.
            return eFormatBytes;
    }
}

size_t
GoASTContext::GetTypeBitAlign (void * type)
{
    assert(false);
}

uint32_t
GoASTContext::GetNumChildren (void * type, bool omit_empty_base_classes)
{
    if (!type || !GetCompleteType(type))
        return 0;
    GoType* t = static_cast<GoType*>(type);
    if (t->GetGoKind() == GoType::KIND_PTR)
    {
        CompilerType elem = t->GetElementType();
        uint32_t ptr_children = elem.GetNumChildren(omit_empty_base_classes);
        if (ptr_children != 0)
            return ptr_children;
        uint32_t flags = elem.GetTypeInfo();
        if (flags & (eTypeIsFuncPrototype | eTypeIsStructUnion))
            return 0;
        if (flags & eTypeHasValue)
            return 1;
    }
    else if (GoArray* array = t->GetArray())
    {
        return array->GetLength();
    }
    return GetNumFields(type);
}

uint32_t
GoASTContext::GetNumFields (void * type)
{
    if (!type || !GetCompleteType(type))
        return 0;
    GoType* t = static_cast<GoType*>(type);
    if (t->IsTypedef())
        return t->GetElementType().GetNumFields();
    GoStruct* s = t->GetStruct();
    if (s)
        return s->GetNumFields();
    return 0;
}

CompilerType
GoASTContext::GetFieldAtIndex (void * type,
                               size_t idx,
                               std::string& name,
                               uint64_t *bit_offset_ptr,
                               uint32_t *bitfield_bit_size_ptr,
                               bool *is_bitfield_ptr)
{
    if (bit_offset_ptr)
        *bit_offset_ptr = 0;
    if (bitfield_bit_size_ptr)
        *bitfield_bit_size_ptr = 0;
    if (is_bitfield_ptr)
        *is_bitfield_ptr = false;
    
    if (!type || !GetCompleteType(type))
        return CompilerType();
    
    GoType* t = static_cast<GoType*>(type);
    if (t->IsTypedef())
        return t->GetElementType().GetFieldAtIndex(idx, name, bit_offset_ptr, bitfield_bit_size_ptr, is_bitfield_ptr);
    
    GoStruct* s = t->GetStruct();
    if (s)
    {
        const auto* field = s->GetField(idx);
        if (field)
        {
            name = field->m_name.GetStringRef();
            if (bit_offset_ptr)
                *bit_offset_ptr = field->m_byte_offset * 8;
            return field->m_type;
        }
    }
    return CompilerType();
}

CompilerType
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
                                        ValueObject *valobj)
{
    child_name.clear();
    child_byte_size = 0;
    child_byte_offset = 0;
    child_bitfield_bit_size = 0;
    child_bitfield_bit_offset = 0;
    child_is_base_class = false;
    child_is_deref_of_parent = false;

    if (!type || !GetCompleteType(type))
        return CompilerType();

    GoType* t = static_cast<GoType*>(type);
    if (t->GetStruct())
    {
        uint64_t bit_offset;
        CompilerType ret = GetFieldAtIndex(type, idx, child_name, &bit_offset, nullptr, nullptr);
        child_byte_size = ret.GetByteSize(exe_ctx ? exe_ctx->GetBestExecutionContextScope() : nullptr);
        child_byte_offset = bit_offset / 8;
        return ret;
    }
    else if (t->GetGoKind() == GoType::KIND_PTR)
    {
        CompilerType pointee = t->GetElementType();
        if (!pointee.IsValid() || pointee.IsVoidType())
            return CompilerType();
        if (transparent_pointers && pointee.IsAggregateType())
        {
            bool tmp_child_is_deref_of_parent = false;
            return pointee.GetChildClangTypeAtIndex(exe_ctx, idx,
                                                    transparent_pointers,
                                                    omit_empty_base_classes,
                                                    ignore_array_bounds,
                                                    child_name,
                                                    child_byte_size,
                                                    child_byte_offset,
                                                    child_bitfield_bit_size,
                                                    child_bitfield_bit_offset,
                                                    child_is_base_class,
                                                    tmp_child_is_deref_of_parent,
                                                    valobj);
        }
        else
        {
            child_is_deref_of_parent = true;
            const char *parent_name = valobj ? valobj->GetName().GetCString() : NULL;
            if (parent_name)
            {
                child_name.assign(1, '*');
                child_name += parent_name;
            }
            
            // We have a pointer to an simple type
            if (idx == 0 && pointee.GetCompleteType())
            {
                child_byte_size = pointee.GetByteSize(exe_ctx ? exe_ctx->GetBestExecutionContextScope() : NULL);
                child_byte_offset = 0;
                return pointee;
            }
        }
    }
    else if (GoArray* a = t->GetArray())
    {
        if (ignore_array_bounds || idx < a->GetLength())
        {
            CompilerType element_type = a->GetElementType();
            if (element_type.GetCompleteType())
            {
                char element_name[64];
                ::snprintf (element_name, sizeof (element_name), "[%zu]", idx);
                child_name.assign(element_name);
                child_byte_size = element_type.GetByteSize(exe_ctx ? exe_ctx->GetBestExecutionContextScope() : NULL);
                child_byte_offset = (int32_t)idx * (int32_t)child_byte_size;
                return element_type;
            }
        }
    }
    else if (t->IsTypedef())
    {
        return t->GetElementType().GetChildClangTypeAtIndex(exe_ctx,
                                                            idx,
                                                            transparent_pointers,
                                                            omit_empty_base_classes,
                                                            ignore_array_bounds,
                                                            child_name,
                                                            child_byte_size,
                                                            child_byte_offset,
                                                            child_bitfield_bit_size,
                                                            child_bitfield_bit_offset,
                                                            child_is_base_class,
                                                            child_is_deref_of_parent,
                                                            valobj);
    }
    return CompilerType();
}

// Lookup a child given a name. This function will match base class names
// and member member names in "clang_type" only, not descendants.
uint32_t
GoASTContext::GetIndexOfChildWithName (void * type,
                                       const char *name,
                                       bool omit_empty_base_classes)
{
    GoType* t = static_cast<GoType*>(type);
    GoStruct* s = t->GetStruct();
    if (s)
    {
        for (uint32_t i = 0; i < s->GetNumFields(); ++i)
        {
            const GoStruct::Field* f = s->GetField(i);
            if (f->m_name.GetStringRef() == name)
                return i;
        }
    }
    else if (t->GetGoKind() == GoType::KIND_PTR || t->IsTypedef())
    {
        return t->GetElementType().GetIndexOfChildWithName(name, omit_empty_base_classes);
    }
    return UINT_MAX;
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
                                             std::vector<uint32_t>& child_indexes)
{
    uint32_t index = GetIndexOfChildWithName(type, name, omit_empty_base_classes);
    if (index == UINT_MAX)
        return 0;
    child_indexes.push_back(index);
    return 1;
}

// Converts "s" to a floating point value and place resulting floating
// point bytes in the "dst" buffer.
size_t
GoASTContext::ConvertStringToFloatValue (void * type,
                           const char *s,
                           uint8_t *dst,
                           size_t dst_size)
{
    assert(false);
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
    assert(false);
}

bool
GoASTContext::DumpTypeValue (void * type,
               Stream *s,
               lldb::Format format,
               const DataExtractor &data,
               lldb::offset_t byte_offset,
               size_t byte_size,
               uint32_t bitfield_bit_size,
               uint32_t bitfield_bit_offset,
               ExecutionContextScope *exe_scope)
{
    if (!type)
        return false;
    if (IsAggregateType(type))
    {
        return false;
    }
    else
    {
        GoType* t = static_cast<GoType*>(type);
        if (t->IsTypedef())
        {
            CompilerType typedef_clang_type = t->GetElementType();
            if (format == eFormatDefault)
                format = typedef_clang_type.GetFormat();
            uint64_t typedef_byte_size = typedef_clang_type.GetByteSize(exe_scope);
            
            return typedef_clang_type.DumpTypeValue (s,
                                                     format,                 // The format with which to display the element
                                                     data,                   // Data buffer containing all bytes for this type
                                                     byte_offset,            // Offset into "data" where to grab value from
                                                     typedef_byte_size,      // Size of this type in bytes
                                                     bitfield_bit_size,      // Size in bits of a bitfield value, if zero don't treat as a bitfield
                                                     bitfield_bit_offset,    // Offset in bits of a bitfield value if bitfield_bit_size != 0
                                                     exe_scope);

        }
        
        uint32_t item_count = 1;
        // A few formats, we might need to modify our size and count for depending
        // on how we are trying to display the value...
        switch (format)
        {
            default:
            case eFormatBoolean:
            case eFormatBinary:
            case eFormatComplex:
            case eFormatCString:         // NULL terminated C strings
            case eFormatDecimal:
            case eFormatEnum:
            case eFormatHex:
            case eFormatHexUppercase:
            case eFormatFloat:
            case eFormatOctal:
            case eFormatOSType:
            case eFormatUnsigned:
            case eFormatPointer:
            case eFormatVectorOfChar:
            case eFormatVectorOfSInt8:
            case eFormatVectorOfUInt8:
            case eFormatVectorOfSInt16:
            case eFormatVectorOfUInt16:
            case eFormatVectorOfSInt32:
            case eFormatVectorOfUInt32:
            case eFormatVectorOfSInt64:
            case eFormatVectorOfUInt64:
            case eFormatVectorOfFloat32:
            case eFormatVectorOfFloat64:
            case eFormatVectorOfUInt128:
                break;
                
            case eFormatChar:
            case eFormatCharPrintable:
            case eFormatCharArray:
            case eFormatBytes:
            case eFormatBytesWithASCII:
                item_count = byte_size;
                byte_size = 1;
                break;
                
            case eFormatUnicode16:
                item_count = byte_size / 2;
                byte_size = 2;
                break;
                
            case eFormatUnicode32:
                item_count = byte_size / 4;
                byte_size = 4;
                break;
        }
        return data.Dump (s,
                          byte_offset,
                          format,
                          byte_size,
                          item_count,
                          UINT32_MAX,
                          LLDB_INVALID_ADDRESS,
                          bitfield_bit_size,
                          bitfield_bit_offset,
                          exe_scope);
    }
    return 0;
}

void
GoASTContext::DumpSummary (void * type,
             ExecutionContext *exe_ctx,
             Stream *s,
             const DataExtractor &data,
             lldb::offset_t data_offset,
             size_t data_byte_size)
{
    assert(false);
}

void
GoASTContext::DumpTypeDescription (void * type)
{
    assert(false);
} // Dump to stdout

void
GoASTContext::DumpTypeDescription (void * type, Stream *s)
{
    assert(false);
}

CompilerType
GoASTContext::CreateArrayType(const ConstString& name,
                              const CompilerType& element_type,
                              int64_t length)
{
    GoType* type = new GoArray(name, length, element_type);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}

CompilerType
GoASTContext::CreateBaseType(int go_kind, const lldb_private::ConstString &name, uint64_t byte_size)
{
    if (go_kind == GoType::KIND_UINT || go_kind == GoType::KIND_INT)
        m_int_byte_size = byte_size;
    GoType* type = new GoType(go_kind, name);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}
    
CompilerType
GoASTContext::CreateTypedef(int kind, const ConstString& name, CompilerType impl)
{
    GoType* type = new GoElem(kind, name, impl);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}
    
CompilerType
GoASTContext::CreateVoidType(const lldb_private::ConstString &name)
{
    GoType* type = new GoType(GoType::KIND_LLDB_VOID, name);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}
    
CompilerType
GoASTContext::CreateStructType(int kind, const lldb_private::ConstString &name, uint32_t byte_size)
{
    GoType* type = new GoStruct(kind, name, byte_size);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}

void
GoASTContext::AddFieldToStruct(const lldb_private::CompilerType &struct_type, const lldb_private::ConstString &name, const lldb_private::CompilerType &field_type, uint32_t byte_offset)
{
    if (!struct_type)
        return;
    GoASTContext* ast = struct_type.GetTypeSystem()->AsGoASTContext();
    if (!ast)
        return;
    GoType* type = static_cast<GoType*>(struct_type.GetOpaqueQualType());
    if (GoStruct* s = type->GetStruct())
        s->AddField(name, field_type, byte_offset);
}

void
GoASTContext::CompleteStructType(const lldb_private::CompilerType &struct_type)
{
    if (!struct_type)
        return;
    GoASTContext* ast = struct_type.GetTypeSystem()->AsGoASTContext();
    if (!ast)
        return;
    GoType* type = static_cast<GoType*>(struct_type.GetOpaqueQualType());
    if (GoStruct* s = type->GetStruct())
        s->SetComplete();
}
    
CompilerType
GoASTContext::CreateFunctionType(const lldb_private::ConstString &name, CompilerType* params, size_t params_count, bool is_variadic)
{
    GoType* type = new GoFunction(name, is_variadic);
    (*m_types)[name].reset(type);
    return CompilerType(this, type);
}
    
bool
GoASTContext::IsGoString(const lldb_private::CompilerType &type)
{
    if (!type.IsValid() || !type.GetTypeSystem()->AsGoASTContext())
        return false;
    return GoType::KIND_STRING == static_cast<GoType*>(type.GetOpaqueQualType())->GetGoKind();
}
    
bool
GoASTContext::IsGoSlice(const lldb_private::CompilerType &type)
{
    if (!type.IsValid() || !type.GetTypeSystem()->AsGoASTContext())
        return false;
    return GoType::KIND_SLICE == static_cast<GoType*>(type.GetOpaqueQualType())->GetGoKind();
}
    
bool
GoASTContext::IsGoInterface(const lldb_private::CompilerType &type)
{
    if (!type.IsValid() || !type.GetTypeSystem()->AsGoASTContext())
        return false;
    return GoType::KIND_INTERFACE == static_cast<GoType*>(type.GetOpaqueQualType())->GetGoKind();
}
    
bool
GoASTContext::IsPointerKind(uint8_t kind)
{
    return (kind & GoType::KIND_MASK) == GoType::KIND_PTR;
}
    
bool
GoASTContext::IsDirectIface(uint8_t kind)
{
    return (kind & GoType::KIND_DIRECT_IFACE) == GoType::KIND_DIRECT_IFACE;
}

// DWARF parsing functions
#pragma mark DWARF Parsing
    
#include "lldb/Core/Module.h"
#include "lldb/Symbol/CompileUnit.h"
#include "lldb/Symbol/Function.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/TypeList.h"
    
#include "Plugins/SymbolFile/DWARF/DWARFCompileUnit.h"
#include "Plugins/SymbolFile/DWARF/DWARFDebugInfo.h"
#include "Plugins/SymbolFile/DWARF/DWARFDebugInfoEntry.h"
#include "Plugins/SymbolFile/DWARF/DWARFDeclContext.h"
#include "Plugins/SymbolFile/DWARF/DWARFDefines.h"
#include "Plugins/SymbolFile/DWARF/DWARFDIECollection.h"
#include "Plugins/SymbolFile/DWARF/SymbolFileDWARF.h"
#include "Plugins/SymbolFile/DWARF/SymbolFileDWARFDebugMap.h"
#include "Plugins/SymbolFile/DWARF/UniqueDWARFASTType.h"

TypeSP
GoASTContext::ParseTypeFromDWARF (const SymbolContext& sc,
                                  SymbolFileDWARF *dwarf,
                                  DWARFCompileUnit* dwarf_cu,
                                  const DWARFDebugInfoEntry *die,
                                  Log *log,
                                  bool *type_is_new_ptr)
{
    TypeSP type_sp;

    if (type_is_new_ptr)
        *type_is_new_ptr = false;

    AccessType accessibility = eAccessNone;
    if (die != NULL)
    {
        if (log)
        {
            dwarf->GetObjectFile()->GetModule()->LogMessage (log, "GoASTContext::ParseTypeFromDWARF (die = 0x%8.8x) %s name = '%s')",
                                                             die->GetOffset(),
                                                             DW_TAG_value_to_name(die->Tag()),
                                                             die->GetName(dwarf, dwarf_cu));
        }

        Type *type_ptr = dwarf->m_die_to_type.lookup (die);
        TypeList* type_list = dwarf->GetTypeList();
        if (type_ptr == NULL)
        {
            if (type_is_new_ptr)
                *type_is_new_ptr = true;

            const dw_tag_t tag = die->Tag();

            bool is_forward_declaration = false;
            DWARFDebugInfoEntry::Attributes attributes;
            const char *type_name_cstr = NULL;
            ConstString type_name_const_str;
            Type::ResolveState resolve_state = Type::eResolveStateUnresolved;
            uint64_t byte_size = 0;
            uint64_t go_kind = 0;
            Declaration decl;

            Type::EncodingDataType encoding_data_type = Type::eEncodingIsUID;
            CompilerType clang_type;
            DWARFFormValue form_value;

            dw_attr_t attr;

            switch (tag)
            {
                case DW_TAG_base_type:
                case DW_TAG_pointer_type:
                case DW_TAG_typedef:
                case DW_TAG_unspecified_type:
                {
                    // Set a bit that lets us know that we are currently parsing this
                    dwarf->m_die_to_type[die] = DIE_IS_BEING_PARSED;

                    const size_t num_attributes = die->GetAttributes(dwarf,
                                                                     dwarf_cu,
                                                                     DWARFFormValue::FixedFormSizes(),
                                                                     attributes);
                    uint32_t encoding = 0;
                    lldb::user_id_t encoding_uid = LLDB_INVALID_UID;

                    if (num_attributes > 0)
                    {
                        uint32_t i;
                        for (i=0; i<num_attributes; ++i)
                        {
                            attr = attributes.AttributeAtIndex(i);
                            if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                            {
                                switch (attr)
                                {
                                    case DW_AT_name:
                                        type_name_cstr = form_value.AsCString(dwarf);
                                        if (type_name_cstr)
                                            type_name_const_str.SetCString(type_name_cstr);
                                        break;
                                    case DW_AT_byte_size:   byte_size = form_value.Unsigned(); break;
                                    case DW_AT_encoding:    encoding = form_value.Unsigned(); break;
                                    case DW_AT_type:        encoding_uid = form_value.Reference(); break;
                                    case DW_AT_go_kind:     go_kind = form_value.Unsigned(); break;
                                    default:
                                        // Do we care about DW_AT_go_key or DW_AT_go_elem?
                                        break;
                                }
                            }
                        }
                    }

                    DEBUG_PRINTF ("0x%8.8" PRIx64 ": %s (\"%s\") type => 0x%8.8lx\n", dwarf->MakeUserID(die->GetOffset()), DW_TAG_value_to_name(tag), type_name_cstr, encoding_uid);

                    switch (tag)
                    {
                        default:
                            break;

                        case DW_TAG_unspecified_type:
                            resolve_state = Type::eResolveStateFull;
                            clang_type = CreateVoidType(type_name_const_str);
                            break;

                        case DW_TAG_base_type:
                            resolve_state = Type::eResolveStateFull;
                            clang_type = CreateBaseType (go_kind, type_name_const_str, byte_size);
                            break;

                        case DW_TAG_pointer_type:
                            encoding_data_type = Type::eEncodingIsPointerUID;
                            break;
                        case DW_TAG_typedef:
                            encoding_data_type = Type::eEncodingIsTypedefUID;
                            CompilerType impl;
                            Type *type = dwarf->ResolveTypeUID(encoding_uid);
                            if (type)
                            {
                                if (go_kind == 0 && type->GetName() == type_name_const_str) {
                                    // Go emits extra typedefs as a forward declaration. Ignore these.
                                    dwarf->m_die_to_type[die] = type;
                                    return type->shared_from_this();
                                }
                                impl = type->GetForwardCompilerType();
                                clang_type = CreateTypedef(go_kind, type_name_const_str, impl);

                            }
                            break;
                    }

                    type_sp.reset( new Type (dwarf->MakeUserID(die->GetOffset()),
                                             dwarf,
                                             type_name_const_str,
                                             byte_size,
                                             NULL,
                                             encoding_uid,
                                             encoding_data_type,
                                             &decl,
                                             clang_type,
                                             resolve_state));

                    dwarf->m_die_to_type[die] = type_sp.get();
                }
                    break;

                case DW_TAG_structure_type:
                {
                    // Set a bit that lets us know that we are currently parsing this
                    dwarf->m_die_to_type[die] = DIE_IS_BEING_PARSED;
                    bool byte_size_valid = false;

                    const size_t num_attributes = die->GetAttributes(dwarf,
                                                                     dwarf_cu,
                                                                     DWARFFormValue::FixedFormSizes(),
                                                                     attributes);
                    if (num_attributes > 0)
                    {
                        uint32_t i;
                        for (i=0; i<num_attributes; ++i)
                        {
                            attr = attributes.AttributeAtIndex(i);
                            if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                            {
                                switch (attr)
                                {
                                    case DW_AT_name:
                                        type_name_cstr = form_value.AsCString(dwarf);
                                        type_name_const_str.SetCString(type_name_cstr);
                                        break;

                                    case DW_AT_byte_size:
                                        byte_size = form_value.Unsigned();
                                        byte_size_valid = true;
                                        break;

                                    case DW_AT_go_kind:
                                        go_kind = form_value.Unsigned();
                                        break;
                                    
                                    // TODO: Should we use SLICETYPE's DW_AT_go_elem?
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                    
                    // TODO(ribrdb): Do we need this?

                    // UniqueDWARFASTType is large, so don't create a local variables on the
                    // stack, put it on the heap. This function is often called recursively
                    // and clang isn't good and sharing the stack space for variables in different blocks.
                    std::unique_ptr<UniqueDWARFASTType> unique_ast_entry_ap(new UniqueDWARFASTType());

                    // Only try and unique the type if it has a name.
                    if (type_name_const_str &&
                        dwarf->GetUniqueDWARFASTTypeMap().Find (type_name_const_str,
                                                                dwarf,
                                                                dwarf_cu,
                                                                die,
                                                                decl,
                                                                byte_size_valid ? byte_size : -1,
                                                                *unique_ast_entry_ap))
                    {
                        // We have already parsed this type or from another
                        // compile unit. GCC loves to use the "one definition
                        // rule" which can result in multiple definitions
                        // of the same class over and over in each compile
                        // unit.
                        type_sp = unique_ast_entry_ap->m_type_sp;
                        if (type_sp)
                        {
                            dwarf->m_die_to_type[die] = type_sp.get();
                            return type_sp;
                        }
                    }

                    DEBUG_PRINTF ("0x%8.8" PRIx64 ": %s (\"%s\")\n", dwarf->MakeUserID(die->GetOffset()), DW_TAG_value_to_name(tag), type_name_cstr);

                    bool clang_type_was_created = false;
                    clang_type.SetCompilerType(this, dwarf->m_forward_decl_die_to_clang_type.lookup (die));
                    if (!clang_type)
                    {
                        clang_type_was_created = true;
                        clang_type = CreateStructType(go_kind, type_name_const_str, byte_size);
                    }

                    type_sp.reset (new Type (dwarf->MakeUserID(die->GetOffset()),
                                             dwarf,
                                             type_name_const_str,
                                             byte_size,
                                             NULL,
                                             LLDB_INVALID_UID,
                                             Type::eEncodingIsUID,
                                             &decl,
                                             clang_type,
                                             Type::eResolveStateForward));

                    // Add our type to the unique type map so we don't
                    // end up creating many copies of the same type over
                    // and over in the ASTContext for our module
                    unique_ast_entry_ap->m_type_sp = type_sp;
                    unique_ast_entry_ap->m_symfile = dwarf;
                    unique_ast_entry_ap->m_cu = dwarf_cu;
                    unique_ast_entry_ap->m_die = die;
                    unique_ast_entry_ap->m_declaration = decl;
                    unique_ast_entry_ap->m_byte_size = byte_size;
                    dwarf->GetUniqueDWARFASTTypeMap().Insert (type_name_const_str,
                                                              *unique_ast_entry_ap);

                    if (!is_forward_declaration)
                    {
                        // Always start the definition for a class type so that
                        // if the class has child classes or types that require
                        // the class to be created for use as their decl contexts
                        // the class will be ready to accept these child definitions.
                        if (die->HasChildren() == false)
                        {
                            // No children for this struct/union/class, lets finish it
                            CompleteStructType(clang_type);
                        }
                        else if (clang_type_was_created)
                        {
                            // Leave this as a forward declaration until we need
                            // to know the details of the type. lldb_private::Type
                            // will automatically call the SymbolFile virtual function
                            // "SymbolFileDWARF::CompleteType(Type *)"
                            // When the definition needs to be defined.
                            dwarf->m_forward_decl_die_to_clang_type[die] = clang_type.GetOpaqueQualType();
                            dwarf->m_forward_decl_clang_type_to_die[ClangASTContext::RemoveFastQualifiers(clang_type).GetOpaqueQualType()] = die;
                            //SetHasExternalStorage (clang_type.GetOpaqueQualType(), true);
                        }
                    }

                }
                    break;

                case DW_TAG_subprogram:
                case DW_TAG_subroutine_type:
                {
                    // Set a bit that lets us know that we are currently parsing this
                    dwarf->m_die_to_type[die] = DIE_IS_BEING_PARSED;

                    dw_offset_t type_die_offset = DW_INVALID_OFFSET;
                    bool is_variadic = false;
                    clang::StorageClass storage = clang::SC_None;//, Extern, Static, PrivateExtern


                    const size_t num_attributes = die->GetAttributes(dwarf,
                                                                     dwarf_cu,
                                                                     DWARFFormValue::FixedFormSizes(),
                                                                     attributes);
                    if (num_attributes > 0)
                    {
                        uint32_t i;
                        for (i=0; i<num_attributes; ++i)
                        {
                            attr = attributes.AttributeAtIndex(i);
                            if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                            {
                                switch (attr)
                                {
                                    case DW_AT_name:
                                        type_name_cstr = form_value.AsCString(dwarf);
                                        type_name_const_str.SetCString(type_name_cstr);
                                        break;

                                    case DW_AT_external:
                                        if (form_value.Unsigned())
                                        {
                                            if (storage == clang::SC_None)
                                                storage = clang::SC_Extern;
                                            else
                                                storage = clang::SC_PrivateExtern;
                                        }
                                        break;

                                    case DW_AT_high_pc:
                                    case DW_AT_low_pc:
                                        break;
                                }
                            }
                        }
                    }

                    DEBUG_PRINTF ("0x%8.8" PRIx64 ": %s (\"%s\")\n", dwarf->MakeUserID(die->GetOffset()), DW_TAG_value_to_name(tag), type_name_cstr);

                    Type *func_type = NULL;

                    std::vector<CompilerType> function_param_types;

                    // Parse the function children for the parameters

                    if (die->HasChildren())
                    {
                        ParseChildParameters (sc,
                                              dwarf,
                                              dwarf_cu,
                                              die,
                                              is_variadic,
                                              function_param_types);
                    }

                    // clang_type will get the function prototype clang type after this call
                    clang_type = CreateFunctionType (type_name_const_str,
                                                     function_param_types.data(),
                                                     function_param_types.size(),
                                                     is_variadic);

                    type_sp.reset( new Type (dwarf->MakeUserID(die->GetOffset()),
                                             dwarf,
                                             type_name_const_str,
                                             0,
                                             NULL,
                                             LLDB_INVALID_UID,
                                             Type::eEncodingIsUID,
                                             &decl,
                                             clang_type,
                                             Type::eResolveStateFull));
                    assert(type_sp.get());
                }
                    break;

                case DW_TAG_array_type:
                {
                    // Set a bit that lets us know that we are currently parsing this
                    dwarf->m_die_to_type[die] = DIE_IS_BEING_PARSED;

                    lldb::user_id_t type_die_offset = DW_INVALID_OFFSET;
                    int64_t first_index = 0;
                    uint32_t byte_stride = 0;
                    uint32_t bit_stride = 0;
                    bool is_vector = false;
                    const size_t num_attributes = die->GetAttributes(dwarf,
                                                                     dwarf_cu,
                                                                     DWARFFormValue::FixedFormSizes(),
                                                                     attributes);

                    if (num_attributes > 0)
                    {
                        uint32_t i;
                        for (i=0; i<num_attributes; ++i)
                        {
                            attr = attributes.AttributeAtIndex(i);
                            if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                            {
                                switch (attr)
                                {
                                    case DW_AT_name:
                                        type_name_cstr = form_value.AsCString(dwarf);
                                        type_name_const_str.SetCString(type_name_cstr);
                                        break;

                                    case DW_AT_type:            type_die_offset = form_value.Reference(); break;
                                    case DW_AT_byte_size:       break; // byte_size = form_value.Unsigned(); break;
                                    case DW_AT_go_kind:         go_kind = form_value.Unsigned(); break;
                                    default:
                                        break;
                                }
                            }
                        }

                        DEBUG_PRINTF ("0x%8.8" PRIx64 ": %s (\"%s\")\n", dwarf->MakeUserID(die->GetOffset()), DW_TAG_value_to_name(tag), type_name_cstr);

                        Type *element_type = dwarf->ResolveTypeUID(type_die_offset);

                        if (element_type)
                        {
                            std::vector<uint64_t> element_orders;
                            ParseChildArrayInfo(sc, dwarf, dwarf_cu, die, first_index, element_orders, byte_stride, bit_stride);
                            if (byte_stride == 0)
                                byte_stride = element_type->GetByteSize();
                            CompilerType array_element_type = element_type->GetForwardCompilerType ();
                            if (element_orders.size() > 0)
                            {
                                printf("golong: unsupported multi-dimensional array %s\n", type_name_cstr);
                                clang_type = CreateArrayType(type_name_const_str, array_element_type, element_orders[0]);
                            }
                            else
                            {
                                clang_type = CreateArrayType (type_name_const_str, array_element_type, 0);
                            }
                            type_sp.reset( new Type (dwarf->MakeUserID(die->GetOffset()),
                                                     dwarf,
                                                     type_name_const_str,
                                                     byte_stride,
                                                     NULL, 
                                                     type_die_offset, 
                                                     Type::eEncodingIsUID, 
                                                     &decl, 
                                                     clang_type, 
                                                     Type::eResolveStateFull));
                            type_sp->SetEncodingType (element_type);
                        }
                    }
                }
                    break;
                    
                default:
                    dwarf->GetObjectFile()->GetModule()->ReportError ("{0x%8.8x}: unhandled type tag 0x%4.4x (%s), please file a bug and attach the file at the start of this error message",
                                                               die->GetOffset(),
                                                               tag,
                                                               DW_TAG_value_to_name(tag));
                    break;
            }
            
            if (type_sp.get())
            {
                const DWARFDebugInfoEntry *sc_parent_die = SymbolFileDWARF::GetParentSymbolContextDIE(die);
                dw_tag_t sc_parent_tag = sc_parent_die ? sc_parent_die->Tag() : 0;
                
                SymbolContextScope * symbol_context_scope = NULL;
                if (sc_parent_tag == DW_TAG_compile_unit)
                {
                    symbol_context_scope = sc.comp_unit;
                }
                else if (sc.function != NULL && sc_parent_die)
                {
                    symbol_context_scope = sc.function->GetBlock(true).FindBlockByID(dwarf->MakeUserID(sc_parent_die->GetOffset()));
                    if (symbol_context_scope == NULL)
                        symbol_context_scope = sc.function;
                }
                
                if (symbol_context_scope != NULL)
                {
                    type_sp->SetSymbolContextScope(symbol_context_scope);
                }
                
                // We are ready to put this type into the uniqued list up at the module level
                type_list->Insert (type_sp);
                
                dwarf->m_die_to_type[die] = type_sp.get();
            }
        }
        else if (type_ptr != DIE_IS_BEING_PARSED)
        {
            type_sp = type_ptr->shared_from_this();
        }
    }
    return type_sp;
}

size_t
GoASTContext::ParseChildParameters (const SymbolContext& sc,
                                    SymbolFileDWARF *dwarf,
                                    DWARFCompileUnit* dwarf_cu,
                                    const DWARFDebugInfoEntry *parent_die,
                                    bool &is_variadic,
                                    std::vector<CompilerType>& function_param_types)
{
    if (parent_die == NULL)
        return 0;
    
    DWARFFormValue::FixedFormSizes fixed_form_sizes =
    DWARFFormValue::GetFixedFormSizesForAddressSize (dwarf_cu->GetAddressByteSize(),
                                                     dwarf_cu->IsDWARF64());
    
    size_t arg_idx = 0;
    const DWARFDebugInfoEntry *die;
    for (die = parent_die->GetFirstChild(); die != NULL; die = die->GetSibling())
    {
        dw_tag_t tag = die->Tag();
        switch (tag)
        {
            case DW_TAG_formal_parameter:
            {
                DWARFDebugInfoEntry::Attributes attributes;
                const size_t num_attributes = die->GetAttributes(dwarf, dwarf_cu, fixed_form_sizes, attributes);
                if (num_attributes > 0)
                {
                    const char *name = NULL;
                    Declaration decl;
                    dw_offset_t param_type_die_offset = DW_INVALID_OFFSET;
                    // one of None, Auto, Register, Extern, Static, PrivateExtern
                    
                    clang::StorageClass storage = clang::SC_None;
                    uint32_t i;
                    for (i=0; i<num_attributes; ++i)
                    {
                        const dw_attr_t attr = attributes.AttributeAtIndex(i);
                        DWARFFormValue form_value;
                        if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                        {
                            switch (attr)
                            {
                                case DW_AT_name:        name = form_value.AsCString(dwarf);
                                    break;
                                case DW_AT_type:        param_type_die_offset = form_value.Reference(); break;
                                case DW_AT_location:
                                    //                          if (form_value.BlockData())
                                    //                          {
                                    //                              const DWARFDataExtractor& debug_info_data = debug_info();
                                    //                              uint32_t block_length = form_value.Unsigned();
                                    //                              DWARFDataExtractor location(debug_info_data, form_value.BlockData() - debug_info_data.GetDataStart(), block_length);
                                    //                          }
                                    //                          else
                                    //                          {
                                    //                          }
                                    //                          break;
                                default:
                                    break;
                            }
                        }
                    }
                    
                    Type *type = dwarf->ResolveTypeUID(param_type_die_offset);
                    if (type)
                    {
                        function_param_types.push_back (type->GetForwardCompilerType ());
                    }
                }
                arg_idx++;
            }
                break;
                
            case DW_TAG_unspecified_parameters:
                is_variadic = true;
                break;
                
            default:
                break;
        }
    }
    return arg_idx;
}

void
GoASTContext::ParseChildArrayInfo (const SymbolContext& sc,
                                   SymbolFileDWARF *dwarf,
                                   DWARFCompileUnit* dwarf_cu,
                                   const DWARFDebugInfoEntry *parent_die,
                                   int64_t& first_index,
                                   std::vector<uint64_t>& element_orders,
                                   uint32_t& byte_stride,
                                   uint32_t& bit_stride)
{
    if (parent_die == NULL)
        return;
    
    const DWARFDebugInfoEntry *die;
    DWARFFormValue::FixedFormSizes fixed_form_sizes =
    DWARFFormValue::GetFixedFormSizesForAddressSize (dwarf_cu->GetAddressByteSize(),
                                                     dwarf_cu->IsDWARF64());
    for (die = parent_die->GetFirstChild(); die != NULL; die = die->GetSibling())
    {
        const dw_tag_t tag = die->Tag();
        switch (tag)
        {
            case DW_TAG_subrange_type:
            {
                DWARFDebugInfoEntry::Attributes attributes;
                const size_t num_child_attributes = die->GetAttributes(dwarf, dwarf_cu, fixed_form_sizes, attributes);
                if (num_child_attributes > 0)
                {
                    uint64_t num_elements = 0;
                    uint32_t i;
                    for (i=0; i<num_child_attributes; ++i)
                    {
                        const dw_attr_t attr = attributes.AttributeAtIndex(i);
                        DWARFFormValue form_value;
                        if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                        {
                            switch (attr)
                            {
                                case DW_AT_count:
                                    num_elements = form_value.Unsigned();
                                    break;
                                    
                                default:
                                case DW_AT_type:
                                    break;
                            }
                        }
                    }
                    
                    element_orders.push_back (num_elements);
                }
            }
                break;
        }
    }
}

bool
GoASTContext::CompleteTypeFromDWARF (SymbolFileDWARF *dwarf,
                                     DWARFCompileUnit *dwarf_cu,
                                     const DWARFDebugInfoEntry* die,
                                     lldb_private::Type *type,
                                     CompilerType &clang_type)
{    
    if (dwarf == nullptr || dwarf_cu == nullptr || die == nullptr)
        return false;
    
    const dw_tag_t tag = die->Tag();
    
    Log *log = nullptr; // (LogChannelDWARF::GetLogIfAny(DWARF_LOG_DEBUG_INFO|DWARF_LOG_TYPE_COMPLETION));
    if (log)
        dwarf->GetObjectFile()->GetModule()->LogMessageVerboseBacktrace (log,
                                                                         "0x%8.8" PRIx64 ": %s '%s' resolving forward declaration...",
                                                                         dwarf->MakeUserID(die->GetOffset()),
                                                                         DW_TAG_value_to_name(tag),
                                                                         type->GetName().AsCString());
    assert (clang_type);
    DWARFDebugInfoEntry::Attributes attributes;
    
    switch (tag)
    {
        case DW_TAG_structure_type:
        {
            {
                if (die->HasChildren())
                {
                    SymbolContext sc(dwarf->GetCompUnitForDWARFCompUnit(dwarf_cu));
                    
                    ParseChildMembers (sc,
                                       dwarf,
                                       dwarf_cu,
                                       die,
                                       clang_type);
                }
            }
            CompleteStructType(clang_type);
            return (bool)clang_type;
        }
            
        default:
            assert(false && "not a forward go type decl!");
            break;
    }
    
    return false;
}

size_t
GoASTContext::ParseChildMembers (const SymbolContext& sc,
                                 SymbolFileDWARF *dwarf,
                                 DWARFCompileUnit* dwarf_cu,
                                 const DWARFDebugInfoEntry *parent_die,
                                 CompilerType &class_clang_type)
{
    if (parent_die == NULL)
        return 0;
    
    size_t count = 0;
    const DWARFDebugInfoEntry *die;
    DWARFFormValue::FixedFormSizes fixed_form_sizes =
    DWARFFormValue::GetFixedFormSizesForAddressSize (dwarf_cu->GetAddressByteSize(),
                                                     dwarf_cu->IsDWARF64());
    uint32_t member_idx = 0;
    ModuleSP module_sp = dwarf->GetObjectFile()->GetModule();
    ClangASTContext* ast = class_clang_type.GetTypeSystem()->AsClangASTContext();
    if (ast == nullptr)
        return 0;
    
    for (die = parent_die->GetFirstChild(); die != NULL; die = die->GetSibling())
    {
        dw_tag_t tag = die->Tag();
        
        switch (tag)
        {
            case DW_TAG_member:
            {
                DWARFDebugInfoEntry::Attributes attributes;
                const size_t num_attributes = die->GetAttributes (dwarf,
                                                                  dwarf_cu,
                                                                  fixed_form_sizes,
                                                                  attributes);
                if (num_attributes > 0)
                {
                    Declaration decl;
                    const char *name = NULL;
                    
                    lldb::user_id_t encoding_uid = LLDB_INVALID_UID;
                    uint32_t member_byte_offset = UINT32_MAX;
                    uint32_t i;
                    for (i=0; i<num_attributes; ++i)
                    {
                        const dw_attr_t attr = attributes.AttributeAtIndex(i);
                        DWARFFormValue form_value;
                        if (attributes.ExtractFormValueAtIndex(dwarf, i, form_value))
                        {
                            switch (attr)
                            {
                                case DW_AT_name:        name = form_value.AsCString(dwarf); break;
                                case DW_AT_type:        encoding_uid = form_value.Reference(); break;
                                case DW_AT_data_member_location:
                                    if (form_value.BlockData())
                                    {
                                        Value initialValue(0);
                                        Value memberOffset(0);
                                        const DWARFDataExtractor& debug_info_data = dwarf->get_debug_info_data();
                                        uint32_t block_length = form_value.Unsigned();
                                        uint32_t block_offset = form_value.BlockData() - debug_info_data.GetDataStart();
                                        if (DWARFExpression::Evaluate(NULL, // ExecutionContext *
                                                                      NULL, // ClangExpressionVariableList *
                                                                      NULL, // ClangExpressionDeclMap *
                                                                      NULL, // RegisterContext *
                                                                      module_sp,
                                                                      debug_info_data,
                                                                      dwarf_cu,
                                                                      block_offset,
                                                                      block_length,
                                                                      eRegisterKindDWARF,
                                                                      &initialValue,
                                                                      memberOffset,
                                                                      NULL))
                                        {
                                            member_byte_offset = memberOffset.ResolveValue(NULL).UInt();
                                        }
                                    }
                                    else
                                    {
                                        // With DWARF 3 and later, if the value is an integer constant,
                                        // this form value is the offset in bytes from the beginning
                                        // of the containing entity.
                                        member_byte_offset = form_value.Unsigned();
                                    }
                                    break;
                            
                                default:
                                    break;
                            }
                        }
                    }
                
                    Type* member_type = dwarf->ResolveTypeUID(encoding_uid);
                    if (member_type)
                    {
                        CompilerType member_go_type = member_type->GetFullCompilerType();
                        ConstString name_const_str(name);
                        AddFieldToStruct(class_clang_type, name_const_str, member_go_type, member_byte_offset);
                    }
                }
                ++member_idx;
            }
                break;
                
            default:
                break;
        }
    }
    
    return count;
}

Function *
GoASTContext::ParseFunctionFromDWARF (const SymbolContext& sc,
                                      SymbolFileDWARF *dwarf,
                                      DWARFCompileUnit* dwarf_cu,
                                      const DWARFDebugInfoEntry *die)
{
    DWARFDebugRanges::RangeList func_ranges;
    const char *name = NULL;
    const char *mangled = NULL;
    int decl_file = 0;
    int decl_line = 0;
    int decl_column = 0;
    int call_file = 0;
    int call_line = 0;
    int call_column = 0;
    DWARFExpression frame_base(dwarf_cu);
    
    assert (die->Tag() == DW_TAG_subprogram);
    
    if (die->Tag() != DW_TAG_subprogram)
        return NULL;
    
    if (die->GetDIENamesAndRanges (dwarf,
                                   dwarf_cu,
                                   name,
                                   mangled,
                                   func_ranges,
                                   decl_file,
                                   decl_line,
                                   decl_column,
                                   call_file,
                                   call_line,
                                   call_column,
                                   &frame_base))
    {
        // Union of all ranges in the function DIE (if the function is discontiguous)
        AddressRange func_range;
        lldb::addr_t lowest_func_addr = func_ranges.GetMinRangeBase (0);
        lldb::addr_t highest_func_addr = func_ranges.GetMaxRangeEnd (0);
        if (lowest_func_addr != LLDB_INVALID_ADDRESS && lowest_func_addr <= highest_func_addr)
        {
            ModuleSP module_sp (dwarf->GetObjectFile()->GetModule());
            func_range.GetBaseAddress().ResolveAddressUsingFileSections (lowest_func_addr, module_sp->GetSectionList());
            if (func_range.GetBaseAddress().IsValid())
                func_range.SetByteSize(highest_func_addr - lowest_func_addr);
        }
        
        if (func_range.GetBaseAddress().IsValid())
        {
            Mangled func_name;
            func_name.SetValue(ConstString(name), false);
            
            FunctionSP func_sp;
            std::unique_ptr<Declaration> decl_ap;
            if (decl_file != 0 || decl_line != 0 || decl_column != 0)
                decl_ap.reset(new Declaration (sc.comp_unit->GetSupportFiles().GetFileSpecAtIndex(decl_file),
                                               decl_line,
                                               decl_column));
            
            // Supply the type _only_ if it has already been parsed
            Type *func_type = dwarf->m_die_to_type.lookup (die);
            
            assert(func_type == NULL || func_type != DIE_IS_BEING_PARSED);
            
            if (dwarf->FixupAddress (func_range.GetBaseAddress()))
            {
                const user_id_t func_user_id = dwarf->MakeUserID(die->GetOffset());
                func_sp.reset(new Function (sc.comp_unit,
                                            dwarf->MakeUserID(func_user_id),       // UserID is the DIE offset
                                            dwarf->MakeUserID(func_user_id),
                                            func_name,
                                            func_type,
                                            func_range));           // first address range
                
                if (func_sp.get() != NULL)
                {
                    if (frame_base.IsValid())
                        func_sp->GetFrameBaseExpression() = frame_base;
                    sc.comp_unit->AddFunction(func_sp);
                    return func_sp.get();
                }
            }
        }
    }
    return NULL;
}




