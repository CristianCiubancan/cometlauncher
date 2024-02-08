//
// Spirited � All Rights Reserved
//
// This source code is govered by a BSD-3 software license, included in the top directory
// of the project. Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the license is met.
//

#include "launch.h"
#include "launch_process.h"

#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <vector>

using namespace std;

// Dragon's launcher is a code injector. The code injector is isolated from the patcher to allow
// for client repairs after anti-virus scanners delete the launcher. The patcher can detect that
// the launcher executable is missing and prompt the user with additional instructions.
int main(int argc, char *argv[])
{
    if (argc < 3) {
        printf("%s\n", "Invalid arguments. Expected:");
        printf("%s %s\n", argv[0], "client_path client_exe [<client_argument>...]");
        return ERROR_BAD_ARGUMENTS;
    }

    // Create the client process in a suspended state
    vector<LPCSTR> arguments(argv, argv + argc);
    LPCSTR clientDirectory = arguments[1];
    LPCSTR clientExecutable = arguments[2];

    PROCESS_INFORMATION processInfo;
    ostringstream processArguments;
    copy(arguments.begin() + 3, arguments.end(), ostream_iterator<string>(processArguments, " "));

    DWORD result = dragon::StartProcess(
        clientDirectory, clientExecutable, processArguments.str(), processInfo);
    if (result != ERROR_SUCCESS) {
        return result;
    }

    // Find all dragon modules to be injected into the client process
    vector<string> modules;
    result = dragon::FindModules(modules);
    if (result != ERROR_SUCCESS) {
        TerminateProcess(processInfo.hProcess, ERROR_PROCESS_ABORTED);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        return result;
    }

    // Inject modules into the running client process
    vector<PVOID> addresses;
    result = dragon::LoadModules(modules, processInfo, addresses);
    if (result != ERROR_SUCCESS) {
        TerminateProcess(processInfo.hProcess, ERROR_PROCESS_ABORTED);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        return result;
    }

    // Resume the client process
    result = dragon::ResumeProcess(modules, processInfo, addresses);
    if (result != ERROR_SUCCESS) {
        TerminateProcess(processInfo.hProcess, ERROR_PROCESS_ABORTED);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
        return result;
    }

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);
    return result;
}