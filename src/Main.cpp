#include "Framework.h"

#include "Memory/Memory.h"
#include "Reader/Reader.h"
#include "Interceptor/Interceptor.h"

int main() {

	LOG("Looking for Rocket League...");

	// Find Rocket League process by the executable name
	DWORD pid = NULL;
	while (pid == NULL) {
		pid = Memory::FindProcess("RocketLeague.exe");

		static bool firstNotFound = true;
		if (!pid && firstNotFound) {
			LOG("Can't find Rocket League, waiting for you to start it...");
			firstNotFound = false;
		}

		Sleep(250);
	}

	LOG("Found Rocket League! (PID = " << pid << ")");

	// Get the main module, aka the "RocketLeague.exe" module
	Memory::ModuleInfo mainModule = Memory::GetProcessMainModule(pid);
	
	// Open a read handle
	HANDLE rpmHandle = OpenProcess(PROCESS_VM_READ, false, pid);
	if (!rpmHandle)
		FATAL_ERROR("Failed to open a read-access handle to Rocket League, try running as Administrator.");

	// Allocate enough memory for all of RocketLeague.exe's binary image
	void* binaryMemory = malloc(mainModule.size);
	if (!binaryMemory)
		FATAL_ERROR("Failed to allocate memory to copy Rocket League's binary data (size = " << (void*)mainModule.size << ").");
	
	// Copy over into allocated
	LOG("Copying Rocket League's main binary (size = " << (void*)mainModule.size << ")...");
	READMEM(rpmHandle, mainModule.baseAddress, binaryMemory, mainModule.size);

	// Find a function that has the BulletPhysics world pointer as the first argument (__thiscall instance pointer)
	DWORD functionWithBtWorldOffset = 
		Memory::PatternScan(
			binaryMemory, 
			mainModule.size, 
			"48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 8B F1"
		);

	void* functionWithBtWorld = (void*)((uintptr_t)mainModule.baseAddress + functionWithBtWorldOffset);

	LOG("Make sure you are in-game for this next part to work, just load into freeplay.");
	// Since we grabbed the thread context when the function was executed, the BulletPhysics world pointer will be in RCX
	// The RCX register is what passes the thisptr of __thiscall functions (also first argument of __fastcall)
	void* btWorldPtr = Interceptor::InterceptFunctionRCX(pid, functionWithBtWorld);

	if (btWorldPtr == NULL)
		FATAL_ERROR("BulletPhysics world pointer was NULL! Thread context must have been invalid...");
	LOG("BulletPhysics world pointer: " << btWorldPtr);

	// Now that we have the BulletPhysics world, we can finally go through and read each collision mesh
	vector<CollisionMeshFile> collisionMeshes;
	Reader::ReadArenaCollisionMeshes(rpmHandle, btWorldPtr, collisionMeshes);
	
	// Save all of the meshes to the meshes folder
	LOG("Saving meshes to \"" << COLLISION_MESH_SOCCAR_PATH << "\"...");
	for (int i = 0; i < collisionMeshes.size(); i++) {
		string name = "mesh_" + std::to_string(i);;
		LOG("Saving collision mesh \"" << name << COLLISION_MESH_FILE_EXTENSION << "...");
		collisionMeshes[i].WriteToFile(name);
	}

	// Clean up
	CloseHandle(rpmHandle);
	free(binaryMemory);

	LOG("====== Done! ======");
	LOG(" - Press enter to exit.");
	getchar();

	return EXIT_SUCCESS;
}