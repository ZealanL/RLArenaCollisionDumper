#include "Debugger.h"

// Convenient utility function
void WriteByteToTextSection(HANDLE handle, void* address, byte b) {
	// Change the page protection to PAGE_READWRITE (was hopefully PAGE_EXECUTE_READ before)
	DWORD originalProtection;
	if (!VirtualProtectEx(handle, address, 1, PAGE_READWRITE, &originalProtection))
		FATAL_ERROR("Failed to modify page protection of Rocket League for breakpoint functionality.");

	LOG("Patching instruction byte at " << address << "...");
	SIZE_T bytesWritten = 0;
	WriteProcessMemory(handle, address, &b, 1, &bytesWritten);

	if (!bytesWritten)
		FATAL_ERROR("Failed to write memory to Rocket League (patching an instruction for breakpoint functionality).");

	// Restore original page protection
	DWORD unused;
	if (!VirtualProtectEx(handle, address, 1, originalProtection, &unused))
		FATAL_ERROR("Failed to modify page protection of Rocket League for breakpoint functionality.");
}

CONTEXT Debugger::GrabContextWithBreakpoint(DWORD pid, void* instructionAddress) {

	// Special ntdll functions for suspending and resuming a process
	// We have to retrieve them using GetProcAddress because, as far as I'm aware, you can't #include them from Windows SDK
	static auto fnNtSuspendProcess	= (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll"), "NtSuspendProcess");
	static auto fnNtResumeProcess	= (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll"), "NtResumeProcess");

	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!handle)
		FATAL_ERROR("Failed to open an all-access handle to Rocket League to attach a custom debugger, try running as Administrator.");

	// Attach as debugger to Rocket League
	if (!DebugActiveProcess(pid))
		FATAL_ERROR("Failed to attach a custom debugger to Rocket League, is another debugger already attached?");
	
	// Don't close 
	DebugSetProcessKillOnExit(false);

	// Freeze Rocket League
	// We can't have Rocket League running while we try to write memory to its .text section
	fnNtSuspendProcess(handle);

	// Back up the instruction's first byte so we can restore it after the breakpoint is hit
	byte instructionRestoreByte;
	READMEM(handle, instructionAddress, &instructionRestoreByte, 1);

	// Put a breakpoint instruction at the targetted address
	WriteByteToTextSection(handle, instructionAddress, 0xCC);

	// Resume Rocket League
	fnNtResumeProcess(handle);

	// We will fill this struct with thread CPU registers when we hit the breakpoint
	CONTEXT result;
	result.ContextFlags = CONTEXT_ALL;

	bool done = false;

	// Loop and wait for the debugger event
	DEBUG_EVENT e = { NULL };
	while (true) {
		if (!WaitForDebugEvent(&e, INFINITE)) {
			CloseHandle(handle);
			FATAL_ERROR("WaitForDebugEvent() returned false, something went wrong while debugging.");
		}

		if (e.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
			DWORD exceptionCode = e.u.Exception.ExceptionRecord.ExceptionCode;
			if (exceptionCode == EXCEPTION_BREAKPOINT) {

				void* exceptionAddress = e.u.Exception.ExceptionRecord.ExceptionAddress;

				if (exceptionAddress == instructionAddress) {
					LOG("Breakpoint hit!");

					// No need to suspend Rocket League this time, we're in a debugger event so it's already suspended for us
					LOG("Restoring instruction to original byte (0x" << std::hex << (int)instructionRestoreByte << ")...");
					WriteByteToTextSection(handle, instructionAddress, instructionRestoreByte);

					LOG("Getting thread context...");
					HANDLE threadHandle = OpenThread(THREAD_ALL_ACCESS, false, e.dwThreadId);
					if (!threadHandle)
						FATAL_ERROR("Failed to open Rocket League's thread during debugger event (thread ID = " << e.dwThreadId << ").");

					if (!GetThreadContext(threadHandle, &result))
						FATAL_ERROR("Failed to get the thread context from Rocket League.");

					// Subtract back the instruction pointer (so that the original instruction isn't executed from partway through)
					// Otherwise, Rocket League will just crash
					result.Rip--;
					if (!SetThreadContext(threadHandle, &result))
						FATAL_ERROR("Failed to set the thread context in Rocket League.");

					CloseHandle(threadHandle);

					done = true;
				} else {
					LOG("Breakpoint hit, but not the one we set. Ignoring (at " << exceptionAddress << ").");
				}
			} else if (exceptionCode == EXCEPTION_ACCESS_VIOLATION || exceptionCode == EXCEPTION_IN_PAGE_ERROR) {

				// Serious memory error occoured, show an error

				std::stringstream errorStream;
				errorStream << "Rocket League encountered a fatal exception!";

				int exceptionSubType = e.u.Exception.ExceptionRecord.ExceptionInformation[0];

				errorStream << "\n - ";

				switch (exceptionSubType) {
				case 0:
					errorStream << "Failed to read memory";
					break;
				case 1:
					errorStream << "Failed to write memory";
					break;
				case 8:
					errorStream << "Failed to execute memory (DEP)";
					break;
				default:
					errorStream << "Unknown access violation";
				}

				void* exceptionAddress = (void*)e.u.Exception.ExceptionRecord.ExceptionInformation[1];
				errorStream << " at " << exceptionAddress;

				string errorStr = errorStream.str();
				FATAL_ERROR(errorStr);
			} else {
				LOG("Unhandled exception code: " << exceptionCode);
			}
		}
		
		// Continue the debugged process so it doesn't close
		ContinueDebugEvent(e.dwProcessId,
			e.dwThreadId,
			DBG_CONTINUE);

		if (done)
			break;
	}

	// Detach from the process, close handle so we're fully disconnected
	DebugActiveProcessStop(pid);
	CloseHandle(handle);

	LOG("Finished debugging process, retrieved CPU context.");
	return result;
}