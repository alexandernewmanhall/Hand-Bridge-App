#pragma once
#include <windows.h>
#include <DbgHelp.h>
#include <string>

// Writes a minidump to the specified file on crash
inline LONG WINAPI WriteCrashDump(EXCEPTION_POINTERS* ExceptionInfo) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    wchar_t filename[MAX_PATH];
    swprintf(filename, MAX_PATH, L"crash_%04d%02d%02d_%02d%02d%02d.dmp",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HANDLE hFile = CreateFileW(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mdei;
        mdei.ThreadId = GetCurrentThreadId();
        mdei.ExceptionPointers = ExceptionInfo;
        mdei.ClientPointers = FALSE;
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mdei, nullptr, nullptr);
        CloseHandle(hFile);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

// Registers the crash dump handler
inline void EnableCrashDumps() {
    SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)WriteCrashDump);
}
