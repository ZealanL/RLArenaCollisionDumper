#include "Memory.h"

#include <TlHelp32.h>
#include <wtsapi32.h>
#pragma comment(lib,"wtsapi32.lib")

#define WILDCARD -1

vector<int> ParseStringBytePattern(const char* pattern) {

	vector<int> result;

	for (const char* s = pattern; *s != NULL; s++) {
		char c = *s;
		if (isblank(c))
			continue;

		if (c == '?') {
			result.push_back(WILDCARD);

			if (*(s + 1) == '?') {
				// Two question marks in a row is still just one wildcard byte
				// Skip this next character so we don't add two wildcards
				s++;
			}
		} else if (isalnum(c)) {
			// Read hex bytes
			result.push_back(strtoul(s, NULL, 16));
			s++; // Go past first character, second will be skipped on next loop
		}
	}

	return result;
}

DWORD Memory::PatternScan(void* moduleMemory, DWORD moduleSize, const char* patternStr) {
	LOG("Searching for pattern \"" << patternStr << "\"...");

	vector<int> pattern = ParseStringBytePattern(patternStr);
	if (pattern.empty() || pattern[0] == WILDCARD)
		return NULL; // Invalid pattern

	byte* binaryStart = (byte*)moduleMemory;
	byte* binaryEnd = binaryStart + moduleSize - (pattern.size() - 1);

	int* patternData = pattern.data();
	int patternSize = pattern.size();
	for (byte* i = binaryStart; i < binaryEnd; i++) {
		bool found = true;
		for (int j = 0; j < patternSize; j++) {
			if (pattern[j] == WILDCARD)
				continue;

			if (pattern[j] != i[j]) {
				found = false;
				break;
			}
		}

		if (found) {
			DWORD offset = (DWORD)(i - binaryStart);
			LOG(" > Found at offset " << (void*)offset << ".");
			return offset;
		}
	}

	LOG(" > No match.");
	return NULL;
}

DWORD Memory::FindProcess(const char* name) {
	WTS_PROCESS_INFOA* processInfos = NULL;
	DWORD processAmount = 0;

	if (WTSEnumerateProcessesA(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &processInfos, &processAmount)) {
		for (int i = 0; i < processAmount; i++) {

			WTS_PROCESS_INFOA processInfo = processInfos[i];
			
			if (!_stricmp(name, processInfo.pProcessName))
				return processInfo.ProcessId;
		}
	}

	WTSFreeMemory(processInfos);
	return NULL;
}

Memory::ModuleInfo Memory::GetProcessMainModule(DWORD pid) {
	HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);

	MODULEENTRY32W moduleEntry;
	moduleEntry.dwSize = sizeof(moduleEntry);

	if (Module32FirstW(moduleSnapshot, &moduleEntry)) {
		wstring pathWideStr = moduleEntry.szExePath;
		LOG("Found main module of process #" << pid << ": \"" << string(pathWideStr.begin(), pathWideStr.end()) << "\"");
		CloseHandle(moduleSnapshot);
		return ModuleInfo{ (void*)moduleEntry.modBaseAddr, moduleEntry.modBaseSize };
	} else {
		FATAL_ERROR("Failed to get main module of process #" << pid);
		CloseHandle(moduleSnapshot);
		return ModuleInfo{};
	}
}