#include "Reader.h"

#include "../BulletStructs.h"

void Reader::ReadArenaCollisionMeshes(HANDLE rpmHandle, void* btWorldPtr, vector<CollisionMeshFile>& meshFilesOut) {

	LOG("Parsing arena collision meshes...");

	// Read bullet world
	btDynamicsWorld bulletWorld;
	READMEM(rpmHandle, btWorldPtr, &bulletWorld, sizeof(btDynamicsWorld));

	LOG("Collision object amount: " << bulletWorld.collisionObjects.size);

	// Make sure Rocket League is on a normal car-soccer map, we can do this by checking the amount of collision objects
	// Each car is an additional collision object, thus why the user must be loaded into freeplay
	constexpr int SOCCAR_COLLISION_OBJECT_AMOUNT = 22;
	if (bulletWorld.collisionObjects.size != SOCCAR_COLLISION_OBJECT_AMOUNT)
		FATAL_ERROR(
			"Incorrect amount of collision objects (" <<
			bulletWorld.collisionObjects.size << "/" << SOCCAR_COLLISION_OBJECT_AMOUNT <<
			"), make sure you are in freeplay and the game is fully loaded!");

	for (int i = 0; i < bulletWorld.collisionObjects.size; i++) {
		void* collisionObjectPtr;
		READMEM(rpmHandle, (void*)(bulletWorld.collisionObjects.data + i), &collisionObjectPtr, sizeof(void*));

		if (!collisionObjectPtr)
			continue;

		btCollisionObject collisionObject;
		READMEM(rpmHandle, collisionObjectPtr, &collisionObject, sizeof(btCollisionObject));

		if (!collisionObject.collisionShape)
			continue;

		btCollisionShape collisionShape;
		READMEM(rpmHandle, collisionObject.collisionShape, &collisionShape, sizeof(btCollisionShape));

		// Check if this collision object's shape is a triangle mesh
		constexpr int TRIANGLE_MESH_SHAPE_PROXYTYPE = 21;
		if (collisionShape.shapeType == TRIANGLE_MESH_SHAPE_PROXYTYPE) {

			LOG(
				"Found btTriangleMeshShape at " << collisionObject.collisionShape << 
				" (collision object = " << collisionObjectPtr << "):");

			btVector3 pos = collisionObject.worldTransform.origin;
			LOG(" > Origin: " << pos);

			btTriangleMeshShape triangleMeshShape;
			READMEM(rpmHandle, collisionObject.collisionShape, &triangleMeshShape, sizeof(btTriangleMeshShape));

			LOG(" > AABB min: " << triangleMeshShape.localAabbMin);
			LOG(" > AABB max: " << triangleMeshShape.localAabbMax);

			// The stridingMeshInterface field will contain the vertex and face data we need
			if (triangleMeshShape.stridingMeshInterface) {
				LOG(" > Striding mesh interface: " << triangleMeshShape.stridingMeshInterface);

				btTriangleIndexVertexArray stridingIterface;
				READMEM(rpmHandle, triangleMeshShape.stridingMeshInterface, &stridingIterface, sizeof(btTriangleIndexVertexArray));

				btIndexedMesh indexedMesh;
				READMEM(rpmHandle, stridingIterface.indexedMeshes.data, &indexedMesh, sizeof(btIndexedMesh));
				LOG(" > Indexed mesh at " << stridingIterface.indexedMeshes.data);
				LOG(" > Number of vertices: " << indexedMesh.numVertices);
				LOG(" > Number of triangle faces: " << indexedMesh.numTriangles);

				constexpr int
					INDEXES_PER_TRI = 3,
					FLOATS_PER_VERTEX = 3;

				CollisionMeshFile meshFileOut;

				LOG(" > Reading triangles...");
				meshFileOut.tris.resize(indexedMesh.numTriangles);
				READMEM(rpmHandle,
					indexedMesh.triangleIndexData,
					meshFileOut.tris.data(),
					indexedMesh.numTriangles * sizeof(CollisionMeshFile::Triangle));

				LOG(" > Reading vertices...");
				meshFileOut.vertices.resize(indexedMesh.numVertices);
				READMEM(rpmHandle,
					indexedMesh.vertexData,
					meshFileOut.vertices.data(),
					indexedMesh.numVertices * sizeof(CollisionMeshFile::Vertex));

				LOG(" > Offsetting...");
				meshFileOut.Offset({ pos.x, pos.y, pos.z });

				LOG(" > Center of geometry (average vertex): " << meshFileOut.GetCenterOfGeometry());

				LOG(" > Done.");

				meshFilesOut.push_back(meshFileOut);

			} else {
				LOG(" > No striding mesh interface, ignoring.");
			}
		}

	}
}