set(LLVM_NO_RTTI 1)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
  include_directories(${LIBXML2_INCLUDE_DIR})
endif()

add_lldb_library(lldbPluginProcessGDBRemote
  GDBRemoteCommunication.cpp
  GDBRemoteCommunicationClient.cpp
  GDBRemoteCommunicationServer.cpp
  GDBRemoteCommunicationServerCommon.cpp
  GDBRemoteCommunicationServerLLGS.cpp
  GDBRemoteCommunicationServerPlatform.cpp
  GDBRemoteRegisterContext.cpp
  ProcessGDBRemote.cpp
  ProcessGDBRemoteLog.cpp
  ThreadGDBRemote.cpp
  )