//===-- GoLanguageRuntime.cpp --------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "GoLanguageRuntime.h"

#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Scalar.h"
#include "lldb/Core/ValueObject.h"
#include "lldb/Core/ValueObjectMemory.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Symbol/ClangExternalASTSourceCommon.h"
#include "lldb/Symbol/Symbol.h"
#include "lldb/Symbol/TypeList.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/SectionLoadList.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"

#include <vector>

using namespace lldb;
using namespace lldb_private;

bool
GoLanguageRuntime::CouldHaveDynamicValue (ValueObject &in_value)
{
    const auto& type = in_value.GetClangType();
    clang::CXXRecordDecl *cxx_record_decl = type.GetCanonicalQualType()->getAsCXXRecordDecl();
    ClangASTMetadata *metadata = ClangASTContext::GetMetadata (type.GetASTContext(), cxx_record_decl);
    if (metadata) {
        return metadata->GetIsDynamicCXXType();
    }
    return false;
}

bool
GoLanguageRuntime::GetDynamicTypeAndAddress (ValueObject &in_value,
                                                     lldb::DynamicValueType use_dynamic,
                                                     TypeAndOrName &class_type_or_name,
                                                     Address &dynamic_address)
{
    class_type_or_name.Clear();
    if (CouldHaveDynamicValue (in_value))
    {
        ConstString tab_cs("tab");
        ConstString type_cs("_type");
        ConstString data_cs("data");
        ConstString string_cs("_string");
        ConstString str_cs("str");
        ConstString size_cs("size");
        ConstString len_cs("len");
        Error err;
        ValueObjectSP iface = in_value.GetStaticValue();
        ValueObjectSP type = iface->GetChildMemberWithName(tab_cs, true);
        if (type)
        {
            iface = type->Dereference(err);
            if (err.Fail()) {
                return false;
            }
        }
        type = iface->GetChildMemberWithName(type_cs, true);
        if (!type)
        {
            return false;
        }
        ValueObjectSP name = type->GetChildMemberWithName(string_cs, true);
        if (!name)
        {
            return false;
        }
        ValueObjectSP namedata = name->GetChildMemberWithName(str_cs, true);
        if (!namedata)
        {
            return false;
        }
        ExecutionContext exe_ctx (in_value.GetExecutionContextRef());
        
        Target *target = exe_ctx.GetTargetPtr();
        Process *process = exe_ctx.GetProcessPtr();

        
        AddressType address_type;
        lldb::addr_t name_ptr = namedata->GetPointerValue(&address_type);
        if (name_ptr == LLDB_INVALID_ADDRESS)
            return false;
        uint64_t len = name->GetChildMemberWithName(len_cs, true)->GetValueAsUnsigned(0);
        char* buf = new char[len + 1];
        buf[len] = 0;
        size_t bytes_read = process->ReadMemory (name_ptr,
                                                 buf,
                                                 len,
                                                 err);

        if (err.Fail() || (bytes_read != len))
        {
            return false;
        }

        ConstString const_typename(buf);
        delete[] buf;
        SymbolContext sc;
        TypeList type_list;
        uint32_t num_matches = target->GetImages().FindTypes (sc,
                                                 const_typename,
                                                 false,
                                                 2,
                                                 type_list);
        if (num_matches == 1) {
            TypeSP final_type = type_list.GetTypeAtIndex(0);
            class_type_or_name.SetTypeSP(final_type);
            
        } else {
            class_type_or_name.SetName(const_typename);
        }
        ValueObjectSP size_sp = type->GetChildMemberWithName(size_cs, true);
        if (!size_sp)
            return false;
        // size is a uintptr
        if (size_sp->GetValueAsUnsigned(0) > size_sp->GetByteSize())
        {
            if (class_type_or_name.HasTypeSP())
            {
                class_type_or_name.SetClangASTType(class_type_or_name.GetTypeSP()->GetClangLayoutType().GetPointeeType());
            }
        }
        dynamic_address.SetLoadAddress(iface->GetChildMemberWithName(data_cs, true)->GetAddressOf(), target);

        return true;
    }
    return false;
}

//------------------------------------------------------------------
// Static Functions
//------------------------------------------------------------------
LanguageRuntime *
GoLanguageRuntime::CreateInstance (Process *process, lldb::LanguageType language)
{
    if (language == eLanguageTypeGo)
        return new GoLanguageRuntime (process);
    else
        return NULL;
}

void
GoLanguageRuntime::Initialize()
{
    PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                   "Go Language Runtime",
                                   CreateInstance);
}

void
GoLanguageRuntime::Terminate()
{
    PluginManager::UnregisterPlugin (CreateInstance);
}

lldb_private::ConstString
GoLanguageRuntime::GetPluginNameStatic()
{
    static ConstString g_name("golang");
    return g_name;
}

//------------------------------------------------------------------
// PluginInterface protocol
//------------------------------------------------------------------
lldb_private::ConstString
GoLanguageRuntime::GetPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
GoLanguageRuntime::GetPluginVersion()
{
    return 1;
}

