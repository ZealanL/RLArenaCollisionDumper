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

	// btDynamicsWorld member functions that are called every tick
	// We just need to find one of these to do our shellcode injection
	const char* BTWORLD_FUNC_PATTERNS[] = {
		"48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 70 48 8B 05 ? ? ? ? 48 33 C4 48 89 44 24 ? 48 8B F1",
		"40 53 48 83 EC 30 48 8B 01 48 8B D9 0F 29 74 24 ? 0F 28 F1 FF 90 ? ? ? ? 48 8B 03 0F 28 CE 48 8B CB FF 90",
		"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 56 41 57 48 83 EC 20 48 63 99",
		"40 53 57 41 57 48 81 EC ? ? ? ? 0F 29 B4 24"
	};

	// Find a function that has the BulletPhysics world pointer as the first argument (__thiscall instance pointer)
	DWORD funcOffset = 0;
	for (const char* pattern : BTWORLD_FUNC_PATTERNS) {
		funcOffset = Memory::PatternScan(
			binaryMemory,
			mainModule.size,
			pattern
		);

		if (funcOffset)
			break;
	}

	if (!funcOffset)
		FATAL_ERROR("Failed to find any of the " << (sizeof(BTWORLD_FUNC_PATTERNS) / sizeof(*BTWORLD_FUNC_PATTERNS)) << " patterns!");

	void* functionWithBtWorld = (void*)((uintptr_t)mainModule.baseAddress + funcOffset);

	LOG("Make sure you are in-game for this next part to work, just load into freeplay.");
	// Since we grabbed the thread context when the function was executed, the BulletPhysics world pointer will be in RCX
	// The RCX register is what passes the thisptr of __thiscall functions (also first argument of __fastcall)
	void* btWorldPtr = Interceptor::InterceptFunctionRCX(pid, functionWithBtWorld);

	if (btWorldPtr == NULL)
		FATAL_ERROR("BulletPhysics world pointer was NULL! Thread context must have been invalid...");
	LOG("BulletPhysics world pointer: " << btWorldPtr);

	// Now that we have the BulletPhysics world, we can finally go through and read each collision mesh
	int gameMode = GAMEMODE_INVALID;
	vector<CollisionMeshFile> collisionMeshes = Reader::ReadArenaCollisionMeshes(rpmHandle, btWorldPtr, gameMode);
	
	if (gameMode == GAMEMODE_INVALID)
		FATAL_ERROR("Finished Reader::ReadArenaCollisionMeshes() with no game mode! This should NOT happen!");

	string meshFolderName = GAMEMODE_STRS[gameMode];
	for (char& c : meshFolderName) {
		c = tolower(c);
		if (c == ' ')
			c == '_';
	}

	string savePath = COLLISION_MESH_BASE_PATH + meshFolderName + "/";

	// Save all of the meshes to the meshes folder
	LOG("Saving meshes to \"" << savePath << "\"...");

	for (int i = 0; i < collisionMeshes.size(); i++) {
		string name = "mesh_" + std::to_string(i);;
		LOG("Saving collision mesh \"" << name << COLLISION_MESH_FILE_EXTENSION << "...");
		collisionMeshes[i].WriteToFile(savePath, name);
	}

	// Clean up
	CloseHandle(rpmHandle);
	free(binaryMemory);

	LOG("====== Done! ======");
	LOG(" - Press enter to exit.");
	getchar();

	return EXIT_SUCCESS;
}