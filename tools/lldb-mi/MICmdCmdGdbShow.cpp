//===-- MICmdCmdGdbShow.cpp -------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// Overview:    CMICmdCmdGdbShow implementation.

// In-house headers:
#include "MICmdCmdGdbShow.h"
#include "MICmnMIResultRecord.h"
#include "MICmnMIValueConst.h"
#include "MICmdArgValString.h"
#include "MICmdArgValListOfN.h"
#include "MICmdArgValOptionLong.h"
#include "MICmnLLDBDebugSessionInfo.h"

// Instantiations:
const CMICmdCmdGdbShow::MapGdbOptionNameToFnGdbOptionPtr_t CMICmdCmdGdbShow::ms_mapGdbOptionNameToFnGdbOptionPtr = {
    {"target-async", &CMICmdCmdGdbShow::OptionFnTargetAsync},
    {"fallback", &CMICmdCmdGdbShow::OptionFnFallback}};

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdGdbShow constructor.
// Type:    Method.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdGdbShow::CMICmdCmdGdbShow(void)
    : m_constStrArgNamedThreadGrp("thread-group")
    , m_constStrArgNamedGdbOption("option")
    , m_bGdbOptionRecognised(true)
    , m_bGdbOptionFnSuccessful(false)
    , m_bGbbOptionFnHasError(false)
    , m_strGdbOptionFnError(MIRSRC(IDS_WORD_ERR_MSG_NOT_IMPLEMENTED_BRKTS))
{
    // Command factory matches this name with that received from the stdin stream
    m_strMiCmd = "gdb-show";

    // Required by the CMICmdFactory when registering *this command
    m_pSelfCreatorFn = &CMICmdCmdGdbShow::CreateSelf;
}

//++ ------------------------------------------------------------------------------------
// Details: CMICmdCmdGdbShow destructor.
// Type:    Overrideable.
// Args:    None.
// Return:  None.
// Throws:  None.
//--
CMICmdCmdGdbShow::~CMICmdCmdGdbShow(void)
{
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The parses the command line options
//          arguments to extract values for each of those arguments.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Function succeeded.
//          MIstatus::failure - Function failed.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::ParseArgs(void)
{
    bool bOk = m_setCmdArgs.Add(
        *(new CMICmdArgValOptionLong(m_constStrArgNamedThreadGrp, false, false, CMICmdArgValListBase::eArgValType_ThreadGrp, 1)));
    bOk = bOk &&
          m_setCmdArgs.Add(
              *(new CMICmdArgValListOfN(m_constStrArgNamedGdbOption, true, true, CMICmdArgValListBase::eArgValType_StringAnything)));
    return (bOk && ParseValidateCmdOptions());
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command is executed in this function.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Function succeeded.
//          MIstatus::failure - Function failed.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::Execute(void)
{
    CMICMDBASE_GETOPTION(pArgGdbOption, ListOfN, m_constStrArgNamedGdbOption);
    const CMICmdArgValListBase::VecArgObjPtr_t &rVecWords(pArgGdbOption->GetExpectedOptions());

    // Get the gdb-show option to carry out. This option will be used as an action
    // which should be done. Further arguments will be used as parameters for it.
    CMICmdArgValListBase::VecArgObjPtr_t::const_iterator it = rVecWords.begin();
    const CMICmdArgValString *pOption = static_cast<const CMICmdArgValString *>(*it);
    const CMIUtilString strOption(pOption->GetValue());
    ++it;

    // Retrieve the parameter(s) for the option
    CMIUtilString::VecString_t vecWords;
    while (it != rVecWords.end())
    {
        const CMICmdArgValString *pWord = static_cast<const CMICmdArgValString *>(*it);
        vecWords.push_back(pWord->GetValue());

        // Next
        ++it;
    }

    FnGdbOptionPtr pPrintRequestFn = nullptr;
    if (!GetOptionFn(strOption, pPrintRequestFn))
    {
        // For unimplemented option handlers, fallback to a generic handler
        // ToDo: Remove this when ALL options have been implemented
        if (!GetOptionFn("fallback", pPrintRequestFn))
        {
            m_bGdbOptionRecognised = false;
            m_strGdbOptionName = "fallback"; // This would be the strOption name
            return MIstatus::success;
        }
    }

    m_bGdbOptionFnSuccessful = (this->*(pPrintRequestFn))(vecWords);
    if (!m_bGdbOptionFnSuccessful && !m_bGbbOptionFnHasError)
        return MIstatus::failure;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: The invoker requires this function. The command prepares a MI Record Result
//          for the work carried out in the Execute() method.
// Type:    Overridden.
// Args:    None.
// Return:  MIstatus::success - Function succeeded.
//          MIstatus::failure - Function failed.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::Acknowledge(void)
{
    // Print error if option isn't recognized:
    // ^error,msg="The request '%s' was not recognized, not implemented"
    if (!m_bGdbOptionRecognised)
    {
        const CMICmnMIValueConst miValueConst(
            CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_INFO_PRINTFN_NOT_FOUND), m_strGdbOptionName.c_str()));
        const CMICmnMIValueResult miValueResult("msg", miValueConst);
        const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Error, miValueResult);
        m_miResultRecord = miRecordResult;
        return MIstatus::success;
    }

    // ^done,value="%s"
    if (m_bGdbOptionFnSuccessful && !m_strValue.empty())
    {
        const CMICmnMIValueConst miValueConst(m_strValue);
        const CMICmnMIValueResult miValueResult("value", miValueConst);
        const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done, miValueResult);
        m_miResultRecord = miRecordResult;
        return MIstatus::success;
    }
    else if (m_bGdbOptionFnSuccessful)
    {
        // Ignore empty value (for fallback)
        const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Done);
        m_miResultRecord = miRecordResult;
        return MIstatus::success;
    }

    // Print error if request failed:
    // ^error,msg="The request '%s' failed.
    const CMICmnMIValueConst miValueConst(CMIUtilString::Format(MIRSRC(IDS_CMD_ERR_INFO_PRINTFN_FAILED), m_strGdbOptionFnError.c_str()));
    const CMICmnMIValueResult miValueResult("msg", miValueConst);
    const CMICmnMIResultRecord miRecordResult(m_cmdData.strMiCmdToken, CMICmnMIResultRecord::eResultClass_Error, miValueResult);
    m_miResultRecord = miRecordResult;

    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Required by the CMICmdFactory when registering *this command. The factory
//          calls this function to create an instance of *this command.
// Type:    Static method.
// Args:    None.
// Return:  CMICmdBase * - Pointer to a new command.
// Throws:  None.
//--
CMICmdBase *
CMICmdCmdGdbShow::CreateSelf(void)
{
    return new CMICmdCmdGdbShow();
}

//++ ------------------------------------------------------------------------------------
// Details: Retrieve the print function's pointer for the matching print request.
// Type:    Method.
// Args:    vrPrintFnName   - (R) The info requested.
//          vrwpFn          - (W) The print function's pointer of the function to carry out
// Return:  bool    - True = Print request is implemented, false = not found.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::GetOptionFn(const CMIUtilString &vrPrintFnName, FnGdbOptionPtr &vrwpFn) const
{
    vrwpFn = nullptr;

    const MapGdbOptionNameToFnGdbOptionPtr_t::const_iterator it = ms_mapGdbOptionNameToFnGdbOptionPtr.find(vrPrintFnName);
    if (it != ms_mapGdbOptionNameToFnGdbOptionPtr.end())
    {
        vrwpFn = (*it).second;
        return true;
    }

    return false;
}

//++ ------------------------------------------------------------------------------------
// Details: Carry out work to complete the GDB show option 'target-async' to prepare
//          and send back the requested information.
// Type:    Method.
// Args:    vrWords - (R) List of additional parameters used by this option.
// Return:  MIstatus::success - Function succeeded.
//          MIstatus::failure - Function failed.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::OptionFnTargetAsync(const CMIUtilString::VecString_t &vrWords)
{
    MIunused(vrWords);

    // Get async mode
    CMICmnLLDBDebugSessionInfo &rSessionInfo(CMICmnLLDBDebugSessionInfo::Instance());
    const bool bAsyncMode = rSessionInfo.GetDebugger().GetAsync();

    m_strValue = bAsyncMode ? "on" : "off";
    return MIstatus::success;
}

//++ ------------------------------------------------------------------------------------
// Details: Carry out work to complete the GDB show option to prepare and send back the
//          requested information.
// Type:    Method.
// Args:    None.
// Return:  MIstatus::success - Function succeeded.
//          MIstatus::failure - Function failed.
// Throws:  None.
//--
bool
CMICmdCmdGdbShow::OptionFnFallback(const CMIUtilString::VecString_t &vrWords)
{
    MIunused(vrWords);

    // Do nothing - intentional. This is a fallback function to do nothing.
    // This allows the search for gdb-show options to always succeed when the option is not
    // found (implemented).

    return MIstatus::success;
}
