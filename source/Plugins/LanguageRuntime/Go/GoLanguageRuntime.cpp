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
#include "lldb/Symbol/GoASTContext.h"
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

namespace {
ValueObjectSP GetChild(ValueObject& obj, const char* name, bool dereference = true) {
    ConstString name_const_str(name);
    ValueObjectSP result = obj.GetChildMemberWithName(name_const_str, true);
    if (dereference && result && result->IsPointerType()) {
        Error err;
        result = result->Dereference(err);
        if (err.Fail())
            result.reset();
    }
    return result;
}

ConstString ReadString(ValueObject& str, Process* process) {
    ConstString result;
    ValueObjectSP data = GetChild(str, "str", false);
    ValueObjectSP len = GetChild(str, "len");
    if (len && data)
    {
        Error err;
        lldb::addr_t addr = data->GetPointerValue();
        if (addr == LLDB_INVALID_ADDRESS)
            return result;
        uint64_t byte_size = len->GetValueAsUnsigned(0);
        char* buf = new char[byte_size + 1];
        buf[byte_size] = 0;
        size_t bytes_read = process->ReadMemory (addr,
                                                 buf,
                                                 byte_size,
                                                 err);
        if (!(err.Fail() || bytes_read != byte_size))
            result = ConstString(buf, bytes_read);
        delete[] buf;
    }
    return result;
}
}

bool
GoLanguageRuntime::CouldHaveDynamicValue (ValueObject &in_value)
{
    return GoASTContext::IsGoInterface(in_value.GetClangType());
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
        ConstString data_cs("data");
        Error err;
        ValueObjectSP iface = in_value.GetStaticValue();
        ValueObjectSP type = GetChild(*iface, "tab");
        if (!type)
        {
            type = GetChild(*iface, "_type");
        }
        if (!type)
        {
            return false;
        }
        
        ValueObjectSP name = GetChild(*type, "_string");
        if (!name)
        {
            return false;
        }
        
        ExecutionContext exe_ctx (in_value.GetExecutionContextRef());
        
        Target *target = exe_ctx.GetTargetPtr();
        Process *process = exe_ctx.GetProcessPtr();

        ConstString const_typename = ReadString(*name, process);
        
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
        ValueObjectSP size_sp = GetChild(*type, "size");
        if (!size_sp)
            return false;
        ValueObjectSP data_sp = GetChild(*iface, "data", false);
        if (!data_sp)
            return false;
        
        // Note: runtime-gdb.py suggests that small interface values get inlined,
        // but that doesn't seem to be true.
        // uint64_t value_size_bytes = size_sp->GetValueAsUnsigned(0);
        
        // if (value_size_bytes > in_value.GetClangType().GetPointerByteSize())
        {
            if (class_type_or_name.HasTypeSP())
            {
                // Need to implement reference types to get rid of this pointer.
                class_type_or_name.SetClangASTType(class_type_or_name.GetTypeSP()->GetClangLayoutType().GetPointerType());
            }
        }
        dynamic_address.SetLoadAddress(data_sp->GetPointerValue(), target);

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

