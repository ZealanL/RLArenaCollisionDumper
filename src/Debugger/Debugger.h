#pragma once
#include "../Framework.h"

namespace Debugger {
	CONTEXT GrabContextWithBreakpoint(DWORD pid, void* instructionAddress);
}