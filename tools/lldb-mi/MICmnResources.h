//===-- MICmnResources.h ----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#pragma once

// Third party headers
#include <map>

// In-house headers:
#include "MIUtilSingletonBase.h"
#include "MIUtilString.h"
#include "MICmnBase.h"

//++ ============================================================================
// Details: MI string test data resource definitions. These IDs match up with
//          actual string data in a map internal to CMICmnResources.
//          *** Be sure to update ms_pResourceId2TextData[] array ****
enum
{
    IDS_PROJNAME = 1,
    IDS_MI_VERSION_DESCRIPTION_DEBUG,
    IDS_MI_VERSION_DESCRIPTION,
    IDS_MI_APPNAME_SHORT,
    IDS_MI_APPNAME_LONG,
    IDS_MI_APP_FILEPATHNAME,
    IDS_MI_APP_ARGS,
    IDE_MI_VERSION_GDB,

    IDS_UTIL_FILE_ERR_INVALID_PATHNAME,
    IDS_UTIL_FILE_ERR_OPENING_FILE,
    IDS_UTIL_FILE_ERR_OPENING_FILE_UNKNOWN,
    IDE_UTIL_FILE_ERR_WRITING_FILE,
    IDE_UTIL_FILE_ERR_WRITING_NOTOPEN,

    IDS_RESOURCES_ERR_STRING_NOT_FOUND,
    IDS_RESOURCES_ERR_STRING_TABLE_INVALID,

    IDS_MI_CLIENT_MSG,

    IDS_LOG_MSG_CREATION_DATE,
    IDS_LOG_MSG_FILE_LOGGER_PATH,
    IDS_LOG_MSG_VERSION,
    IDS_LOG_ERR_FILE_LOGGER_DISABLED,
    IDS_LOG_MEDIUM_ERR_INIT,
    IDS_LOG_MEDIUM_ERR_WRITE_ANY,
    IDS_LOG_MEDIUM_ERR_WRITE_MEDIUMFAIL,
    IDE_MEDIUMFILE_ERR_GET_FILE_PATHNAME_SYS,

    IDS_MEDIUMFILE_NAME,
    IDS_MEDIUMFILE_ERR_INVALID_PATH,
    IDS_MEDIUMFILE_ERR_FILE_HEADER,
    IDS_MEDIUMFILE_NAME_LOG,

    IDE_OS_ERR_UNKNOWN,
    IDE_OS_ERR_RETRIEVING,

    IDS_DRIVERMGR_DRIVER_ERR_INIT,

    IDE_MEDIUMSTDERR_NAME,
    IDE_MEDIUMSTDOUT_NAME,

    IDE_MI_APP_DESCRIPTION,
    IDE_MI_APP_INFORMATION,
    IDE_MI_APP_ARG_USAGE,
    IDE_MI_APP_ARG_HELP,
    IDE_MI_APP_ARG_VERSION,
    IDE_MI_APP_ARG_VERSION_LONG,
    IDE_MI_APP_ARG_INTERPRETER,
    IDE_MI_APP_ARG_EXECUTEABLE,
    IDE_MI_APP_ARG_APP_LOG,
    IDE_MI_APP_ARG_APP_LOG_DIR,
    IDE_MI_APP_ARG_EXAMPLE,
    IDE_MI_APP_ARG_EXECUTABLE,

    IDS_STDIN_ERR_INVALID_PROMPT,
    IDS_STDIN_ERR_THREAD_CREATION_FAILED,
    IDS_STDIN_ERR_THREAD_DELETE,
    IDS_STDIN_ERR_CHKING_BYTE_AVAILABLE,
    IDS_STDIN_INPUT_CTRL_CHARS,

    IDS_CMD_QUIT_HELP,

    IDS_THREADMGR_ERR_THREAD_ID_INVALID,
    IDS_THREADMGR_ERR_THREAD_FAIL_CREATE,
    IDS_THREADMGR_ERR_THREAD_ID_NOT_FOUND,
    IDS_THREADMGR_ERR_THREAD_STILL_ALIVE,

    IDS_FALLTHRU_DRIVER_CMD_RECEIVED,

    IDS_CMDFACTORY_ERR_INVALID_CMD_NAME,
    IDS_CMDFACTORY_ERR_INVALID_CMD_CR8FN,
    IDS_CMDFACTORY_ERR_CMD_NOT_REGISTERED,
    IDS_CMDFACTORY_ERR_CMD_ALREADY_REGED,

    IDS_CMDMGR_ERR_CMD_FAILED_CREATE,
    IDS_CMDMGR_ERR_CMD_INVOKER,

    IDS_MI_INIT_ERR_LOG,
    IDS_MI_INIT_ERR_RESOURCES,
    IDS_MI_INIT_ERR_INIT,
    IDS_MI_INIT_ERR_STREAMSTDIN,
    IDS_MI_INIT_ERR_STREAMSTDIN_OSHANDLER,
    IDS_MI_INIT_ERR_OS_STDIN_HANDLER,
    IDS_MI_INIT_ERR_STREAMSTDOUT,
    IDS_MI_INIT_ERR_STREAMSTDERR,
    IDS_MI_INIT_ERR_FALLTHRUDRIVER,
    IDS_MI_INIT_ERR_THREADMGR,
    IDS_MI_INIT_ERR_CMDINTERPRETER,
    IDS_MI_INIT_ERR_CMDMGR,
    IDS_MI_INIT_ERR_CMDFACTORY,
    IDS_MI_INIT_ERR_CMDINVOKER,
    IDS_MI_INIT_ERR_CMDMONITOR,
    IDS_MI_INIT_ERR_LLDBDEBUGGER,
    IDS_MI_INIT_ERR_DRIVERMGR,
    IDS_MI_INIT_ERR_DRIVER,
    IDS_MI_INIT_ERR_OUTOFBANDHANDLER,
    IDS_MI_INIT_ERR_DEBUGSESSIONINFO,
    IDS_MI_INIT_ERR_THREADMANAGER,
    IDS_MI_INIT_ERR_CLIENT_USING_DRIVER,
    IDS_MI_INIT_ERR_LOCAL_DEBUG_SESSION,

    IDS_CODE_ERR_INVALID_PARAMETER_VALUE,
    IDS_CODE_ERR_INVALID_PARAM_NULL_POINTER,
    IDS_CODE_ERR_INVALID_ENUMERATION_VALUE,

    IDS_LLDBDEBUGGER_ERR_INVALIDLISTENER,
    IDS_LLDBDEBUGGER_ERR_INVALIDDEBUGGER,
    IDS_LLDBDEBUGGER_ERR_CLIENTDRIVER,
    IDS_LLDBDEBUGGER_ERR_STARTLISTENER,
    IDS_LLDBDEBUGGER_ERR_THREADCREATIONFAIL,
    IDS_LLDBDEBUGGER_ERR_THREAD_DELETE,
    IDS_LLDBDEBUGGER_ERR_INVALIDBROADCASTER,
    IDS_LLDBDEBUGGER_ERR_INVALIDCLIENTNAME,
    IDS_LLDBDEBUGGER_ERR_CLIENTNOTREGISTERD,
    IDS_LLDBDEBUGGER_ERR_STOPLISTENER,
    IDS_LLDBDEBUGGER_ERR_BROARDCASTER_NAME,
    IDS_LLDBDEBUGGER_WRN_UNKNOWN_EVENT,

    IDS_LLDBOUTOFBAND_ERR_UNKNOWN_EVENT,
    IDS_LLDBOUTOFBAND_ERR_PROCESS_INVALID,
    IDS_LLDBOUTOFBAND_ERR_BRKPT_NOTFOUND,
    IDS_LLDBOUTOFBAND_ERR_BRKPT_INFO_GET,
    IDS_LLDBOUTOFBAND_ERR_BRKPT_INFO_SET,
    IDS_LLDBOUTOFBAND_ERR_FORM_MI_RESPONSE,
    IDS_LLDBOUTOFBAND_ERR_FRAME_INFO_GET,
    IDS_LLDBOUTOFBAND_ERR_SETNEWDRIVERSTATE,
    IDS_LLDBOUTOFBAND_ERR_BRKPT_CNT_EXCEEDED,

    IDS_DBGSESSION_ERR_SHARED_DATA_RELEASE,
    IDS_DBGSESSION_ERR_SHARED_DATA_ADD,

    IDS_MI_SHTDWN_ERR_LOG,
    IDS_MI_SHUTDOWN_ERR,
    IDE_MI_SHTDWN_ERR_RESOURCES,
    IDE_MI_SHTDWN_ERR_STREAMSTDIN,
    IDS_MI_SHTDWN_ERR_OS_STDIN_HANDLER,
    IDS_MI_SHTDWN_ERR_STREAMSTDOUT,
    IDS_MI_SHTDWN_ERR_STREAMSTDERR,
    IDS_MI_SHTDWN_ERR_THREADMGR,
    IDS_MI_SHTDWN_ERR_CMDINTERPRETER,
    IDS_MI_SHTDWN_ERR_CMDMGR,
    IDS_MI_SHTDWN_ERR_CMDFACTORY,
    IDS_MI_SHTDWN_ERR_CMDINVOKER,
    IDS_MI_SHTDWN_ERR_CMDMONITOR,
    IDS_MI_SHTDWN_ERR_LLDBDEBUGGER,
    IDS_MI_SHTDWN_ERR_DRIVERMGR,
    IDS_MI_SHTDWN_ERR_DRIVER,
    IDS_MI_SHTDWN_ERR_OUTOFBANDHANDLER,
    IDS_MI_SHTDWN_ERR_DEBUGSESSIONINFO,
    IDE_MI_SHTDWN_ERR_THREADMANAGER,

    IDS_DRIVER_ERR_PARSE_ARGS,
    IDS_DRIVER_ERR_PARSE_ARGS_UNKNOWN,
    IDS_DRIVER_ERR_CURRENT_NOT_SET,
    IDS_DRIVER_ERR_NON_REGISTERED,
    IDS_DRIVER_SAY_DRIVER_USING,
    IDS_DRIVER_ERR_ID_INVALID,
    IDS_DRIVER_ERR_FALLTHRU_DRIVER_ERR,
    IDS_DRIVER_CMD_RECEIVED,
    IDS_DRIVER_CMD_NOT_IN_FACTORY,
    IDS_DRIVER_ERR_DRIVER_STATE_ERROR,
    IDS_DRIVER_ERR_MAINLOOP,
    IDS_DRIVER_ERR_LOCAL_DEBUG_NOT_IMPL,
    IDS_DRIVER_ERR_LOCAL_DEBUG_INIT,

    IDS_DRIVER_WAITING_STDIN_DATA,

    IDS_STDERR_ERR_NOT_ALL_DATA_WRITTEN,

    IDS_CMD_ARGS_ERR_N_OPTIONS_REQUIRED,
    IDS_CMD_ARGS_ERR_OPTION_NOT_FOUND,
    IDS_CMD_ARGS_ERR_VALIDATION_MANDATORY,
    IDS_CMD_ARGS_ERR_VALIDATION_INVALID,
    IDS_CMD_ARGS_ERR_VALIDATION_MAN_INVALID,
    IDS_CMD_ARGS_ERR_VALIDATION_MISSING_INF,
    IDS_CMD_ARGS_ERR_CONTEXT_NOT_ALL_EATTEN,
    IDS_CMD_ARGS_ERR_PREFIX_MSG,

    IDS_VARIANT_ERR_USED_BASECLASS,
    IDS_VARIANT_ERR_MAP_KEY_INVALID,

    IDS_WORD_INVALIDBRKTS,
    IDS_WORD_NONE,
    IDS_WORD_NOT,
    IDS_WORD_INVALIDEMPTY,
    IDS_WORD_INVALIDNULLPTR,
    IDS_WORD_UNKNOWNBRKTS,
    IDS_WORD_NOT_IMPLEMENTED,
    IDS_WORD_NOT_IMPLEMENTED_BRKTS,
    IDS_WORD_UNKNOWNTYPE_BRKTS,
    IDS_WORD_ERR_MSG_NOT_IMPLEMENTED_BRKTS,

    IDS_CMD_ERR_N_OPTIONS_REQUIRED,
    IDS_CMD_ERR_OPTION_NOT_FOUND,
    IDS_CMD_ERR_ARGS,
    IDS_CMD_WRN_ARGS_NOT_HANDLED,
    IDS_CMD_ERR_FNFAILED,
    IDS_CMD_ERR_SHARED_DATA_NOT_FOUND,
    IDS_CMD_ERR_LLDBPROCESS_DETACH,
    IDS_CMD_ERR_LLDBPROCESS_DESTROY,
    IDS_CMD_ERR_SETWKDIR,
    IDS_CMD_ERR_INVALID_TARGET,
    IDS_CMD_ERR_INVALID_TARGET_CURRENT,
    IDS_CMD_ERR_INVALID_TARGET_TYPE,
    IDS_CMD_ERR_INVALID_TARGET_PLUGIN,
    IDS_CMD_ERR_CONNECT_TO_TARGET,
    IDS_CMD_ERR_INVALID_TARGETPLUGINCURRENT,
    IDS_CMD_ERR_NOT_IMPLEMENTED,
    IDS_CMD_ERR_NOT_IMPLEMENTED_DEPRECATED,
    IDS_CMD_ERR_CREATE_TARGET,
    IDS_CMD_ERR_BRKPT_LOCATION_FORMAT,
    IDS_CMD_ERR_BRKPT_INVALID,
    IDS_CMD_ERR_BRKPT_CNT_EXCEEDED,
    IDS_CMD_ERR_SOME_ERROR,
    IDS_CMD_ERR_THREAD_INVALID,
    IDS_CMD_ERR_THREAD_FRAME_RANGE_INVALID,
    IDS_CMD_ERR_FRAME_INVALID,
    IDS_CMD_ERR_VARIABLE_DOESNOTEXIST,
    IDS_CMD_ERR_VARIABLE_ENUM_INVALID,
    IDS_CMD_ERR_VARIABLE_EXPRESSIONPATH,
    IDS_CMD_ERR_VARIABLE_CREATION_FAILED,
    IDS_CMD_ERR_CMD_RUN_BUT_NO_ACTION,
    IDS_CMD_ERR_EVENT_HANDLED_BUT_NO_ACTION,
    IDS_CMD_ERR_DISASM_ADDR_START_INVALID,
    IDS_CMD_ERR_DISASM_ADDR_END_INVALID,
    IDS_CMD_ERR_MEMORY_ALLOC_FAILURE,
    IDS_CMD_ERR_LLDB_ERR_NOT_READ_WHOLE_BLK,
    IDS_CMD_ERR_LLDB_ERR_READ_MEM_BYTES,
    IDS_CMD_ERR_INVALID_PROCESS,
    IDS_CMD_ERR_INVALID_PRINT_VALUES,
    IDS_CMD_ERR_INVALID_FORMAT_TYPE,
    IDS_CMD_ERR_BRKPT_INFO_OBJ_NOT_FOUND,
    IDS_CMD_ERR_LLDB_ERR_WRITE_MEM_BYTES,
    IDS_CMD_ERR_LLDB_ERR_NOT_WRITE_WHOLEBLK,
    IDS_CMD_ERR_SET_NEW_DRIVER_STATE,
    IDS_CMD_ERR_INFO_PRINTFN_NOT_FOUND,
    IDS_CMD_ERR_INFO_PRINTFN_FAILED,
    IDS_CMD_ERR_GDBSET_OPT_TARGETASYNC,
    IDS_CMD_ERR_GDBSET_OPT_SOLIBSEARCHPATH,
    IDS_CMD_ERR_GDBSET_OPT_PRINT_BAD_ARGS,
    IDS_CMD_ERR_GDBSET_OPT_PRINT_UNKNOWN_OPTION,
    IDS_CMD_ERR_GDBSHOW_OPT_PRINT_BAD_ARGS,
    IDS_CMD_ERR_GDBSHOW_OPT_PRINT_UNKNOWN_OPTION
};

//++ ============================================================================
// Details: MI common code implementation class. Handle application resources
//          and locality.
//          Singleton class.
// Gotchas: None.
// Authors: Illya Rudkin 29/01/2014.
// Changes: None.
//--
class CMICmnResources : public CMICmnBase, public MI::ISingleton<CMICmnResources>
{
    friend class MI::ISingleton<CMICmnResources>;

    // Methods:
  public:
    bool Initialize(void);
    bool Shutdown(void);

    CMIUtilString GetString(const MIuint vResourceId) const;
    bool HasString(const MIuint vResourceId) const;

    // Typedef:
  private:
    typedef std::map<MIuint, const MIchar *> MapRscrIdToTextData_t;
    typedef std::pair<MIuint, const MIchar *> MapPairRscrIdToTextData_t;

    // Enumerations:
  private:
    enum Buffer_e
    {
        eBufferSize = 2048
    };

    // Structs:
  private:
    struct SRsrcTextData
    {
        MIuint id;
        const MIchar *pTextData;
    };

    // Methods:
  private:
    /* ctor */ CMICmnResources(void);
    /* ctor */ CMICmnResources(const CMICmnResources &);
    void operator=(const CMICmnResources &);

    bool GetStringFromResource(const MIuint vResourceId, CMIUtilString &vrwResourceString) const;
    bool ReadResourceStringData(void);

    // Overridden:
  private:
    // From CMICmnBase
    /* dtor */ virtual ~CMICmnResources(void);

    // Attributes:
  private:
    static const SRsrcTextData ms_pResourceId2TextData[];
    //
    MIuint m_nResourceId2TextDataSize;
    MapRscrIdToTextData_t m_mapRscrIdToTextData;
};

//++ =========================================================================
// Details: Macro short cut for retrieving a text data resource
//--
#define MIRSRC(x) CMICmnResources::Instance().GetString(x).c_str()
