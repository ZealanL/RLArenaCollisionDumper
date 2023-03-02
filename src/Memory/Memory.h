#include "../Framework.h"

#include <Windows.h>

namespace Memory {
	// Pattern string example: "0F 29 74 24 ? 0F 28 F1 FF 90 ? ? ? ? 48"
	// Returns offset from module base
	DWORD PatternScan(void* moduleMemory, DWORD moduleSize, const char* patternStr);

	DWORD FindProcess(const char* name);
	
	struct ModuleInfo {
		void* baseAddress = NULL;
		DWORD size;

		operator bool() {
			return baseAddress != NULL;
		}
	};
	ModuleInfo GetProcessMainModule(DWORD pid);
}