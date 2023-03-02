#include "Interceptor.h"

// Convenient utility function
void WriteToTextSection(HANDLE handle, void* address, vector<byte> data) {

	assert(!data.empty());

	// Change the page protection to PAGE_READWRITE (was hopefully PAGE_EXECUTE_READ before)
	DWORD originalProtection;
	if (!VirtualProtectEx(handle, address, data.size(), PAGE_READWRITE, &originalProtection))
		FATAL_ERROR("Failed to modify page protection of Rocket League for breakpoint functionality.");

	LOG("Patching instruction byte at " << address << "...");
	SIZE_T bytesWritten = 0;
	WriteProcessMemory(handle, address, data.data(), data.size(), &bytesWritten);

	if (!bytesWritten)
		FATAL_ERROR("Failed to write memory to Rocket League (patching an instruction for breakpoint functionality).");

	// Restore original page protection
	DWORD unused;
	if (!VirtualProtectEx(handle, address, data.size(), originalProtection, &unused))
		FATAL_ERROR("Failed to modify page protection of Rocket League for breakpoint functionality.");
}


void* Interceptor::InterceptFunctionRCX(DWORD pid, void* funcAddress) {
	// Special ntdll functions for suspending and resuming a process
	// We have to retrieve them using GetProcAddress because, as far as I'm aware, you can't #include them from Windows SDK
	static auto fnNtSuspendProcess = (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll"), "NtSuspendProcess");
	static auto fnNtResumeProcess = (LONG(NTAPI*)(HANDLE))GetProcAddress(GetModuleHandleA("ntdll"), "NtResumeProcess");

	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!handle)
		FATAL_ERROR("Failed to open an all-access handle to Rocket League to patch memory, try running as Administrator.");

	// Allocate a pointer's worth of memory in Rocket League
	// This is where the patched function will copy RCX to
	// NOTE: Memory is zero'd by default according to WinAPI docs
	void* bufferPtr = VirtualAllocEx(handle, 0, sizeof(void*), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (!bufferPtr)
		FATAL_ERROR("Failed to allocate a tiny amount of memory in Rocket League.");

	// Create x86_64 machine code bytes to patch with
	std::vector<byte> patchBytes = {

		// movabs RAX, (zero placeholder)
		0x48, 0xB8, 0, 0, 0, 0, 0, 0, 0, 0,

		// mov [RAX], RCX
		0x48, 0x89, 0x08,

		// ret
		0xC3

	};

	// Set bufferPtr in patchBytes to the actual pointer's bytes
	for (int i = 0; i < sizeof(void*); i++)
		patchBytes[2 + i] = ((byte*)(&bufferPtr))[i];

	// Freeze Rocket League
	fnNtSuspendProcess(handle);

	// Patch function
	WriteToTextSection(handle, funcAddress, patchBytes);

	// Resume Rocket League
	fnNtResumeProcess(handle);

	// Wait until the memory is updated
	LOG("Waiting for function to be called (make sure you aren't paused in RL)...");
	void* caughtRCX = NULL;
	while (caughtRCX == NULL) {
		READMEM(handle, bufferPtr, &caughtRCX, sizeof(void*));
		Sleep(100);
	}

	LOG("Successfully found at " << caughtRCX);

	// Done! Return the pointer we found
	return caughtRCX;
}