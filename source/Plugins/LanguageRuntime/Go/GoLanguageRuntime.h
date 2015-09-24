//===-- GoLanguageRuntime.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_GoLanguageRuntime_h_
#define liblldb_GoLanguageRuntime_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/lldb-private.h"
#include "lldb/Breakpoint/BreakpointResolver.h"
#include "lldb/Target/LanguageRuntime.h"
#include "lldb/Core/Value.h"

namespace lldb_private {
    
    class GoLanguageRuntime :
    public lldb_private::LanguageRuntime
    {
    public:
        ~GoLanguageRuntime() { }
      
        virtual lldb::LanguageType
        GetLanguageType () const {
            return lldb::eLanguageTypeGo;
        }

        virtual bool
        GetObjectDescription (Stream &str, ValueObject &object) {
            // TODO(ribrdb): Maybe call String() method?
            return false;
        }
        
        virtual bool
        GetObjectDescription (Stream &str, Value &value, ExecutionContextScope *exe_scope)  {
            return false;
        }

        
        virtual bool
        GetDynamicTypeAndAddress (ValueObject &in_value,
                                  lldb::DynamicValueType use_dynamic,
                                  TypeAndOrName &class_type_or_name,
                                  Address &address);
        
        virtual bool
        CouldHaveDynamicValue (ValueObject &in_value);
        
        virtual lldb::BreakpointResolverSP
        CreateExceptionResolver (Breakpoint *bkpt, bool catch_bp, bool throw_bp) {
            return lldb::BreakpointResolverSP();
        }
        
        //------------------------------------------------------------------
        // Static Functions
        //------------------------------------------------------------------
        static void
        Initialize();
        
        static void
        Terminate();
        
        static lldb_private::LanguageRuntime *
        CreateInstance (Process *process, lldb::LanguageType language);
        
        static lldb_private::ConstString
        GetPluginNameStatic();
        
        //------------------------------------------------------------------
        // PluginInterface protocol
        //------------------------------------------------------------------
        virtual lldb_private::ConstString
        GetPluginName();
        
        virtual uint32_t
        GetPluginVersion();
        
    private:
        GoLanguageRuntime(Process *process) : lldb_private::LanguageRuntime(process) { } // Call CreateInstance instead.
    };
    
} // namespace lldb_private

#endif  // liblldb_GoLanguageRuntime_h_
