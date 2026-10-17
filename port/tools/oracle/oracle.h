// What the oracle's Platform layer (platform.cpp) reports to the program (oracle.cpp).
#ifndef PORT_TOOLS_ORACLE_ORACLE_H
#define PORT_TOOLS_ORACLE_ORACLE_H

namespace oracle {

// melonDS's log levels below this one are dropped (melonDS::Platform::LogLevel: 0 debug to 3 error)
int LogThreshold();
// The emulated console stopped itself (melonDS::Platform::StopReason)
void OnEmulatorStop(int reason, void *userdata);
// The game wrote to its save memory
void OnSaveWritten(void *userdata);

} // namespace oracle

#endif // PORT_TOOLS_ORACLE_ORACLE_H
