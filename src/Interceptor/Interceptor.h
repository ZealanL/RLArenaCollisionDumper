#pragma once
#include "../Framework.h"

// The interceptor's job is to patch the start of the BulletPhysics function to copy the RCX register to some allocated memory
namespace Interceptor {
	void* InterceptFunctionRCX(DWORD pid, void* funcAddress);
}