#pragma once
#include "../Framework.h"
#include "../CollisionMeshFile.h"

namespace Reader {
	// Read all of the collision meshes to vector<CollisionMeshFile> given the BulletPhysics world pointer
	vector<CollisionMeshFile> ReadArenaCollisionMeshes(HANDLE rpmHandle, void* btWorldPtr, int& gameModeOut);
}