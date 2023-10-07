#include "Reader.h"

#include "../BulletStructs.h"

vector<CollisionMeshFile> Reader::ReadArenaCollisionMeshes(HANDLE rpmHandle, void* btWorldPtr, int& gameModeOut) {

	vector<CollisionMeshFile> meshFiles;

	LOG("Parsing arena collision meshes...");

	// Read bullet world
	btDynamicsWorld bulletWorld;
	READMEM(rpmHandle, btWorldPtr, &bulletWorld, sizeof(btDynamicsWorld));

	LOG("Collision object amount: " << bulletWorld.collisionObjects.size);

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
		if (collisionShape.shapeType == TRIANGLE_MESH_SHAPE_PROXYTYPE) {

			DLOG(
				"Found btTriangleMeshShape at " << collisionObject.collisionShape << 
				" (collision object = " << collisionObjectPtr << "):");

			btVector3 pos = collisionObject.worldTransform.origin;
			DLOG(" > Origin: " << pos);

			btMatrix3x3 basis = collisionObject.worldTransform.basis;
			DLOG(" > Forward: " << basis.el[0]);

			bool rotated = basis != btMatrix3x3::GetIdentity();

			btTriangleMeshShape triangleMeshShape;
			READMEM(rpmHandle, collisionObject.collisionShape, &triangleMeshShape, sizeof(btTriangleMeshShape));

			DLOG(" > AABB min: " << triangleMeshShape.localAabbMin);
			DLOG(" > AABB max: " << triangleMeshShape.localAabbMax);

			// The stridingMeshInterface field will contain the vertex and face data we need
			if (triangleMeshShape.stridingMeshInterface) {
				DLOG(" > Striding mesh interface: " << triangleMeshShape.stridingMeshInterface);

				btTriangleIndexVertexArray stridingIterface;
				READMEM(rpmHandle, triangleMeshShape.stridingMeshInterface, &stridingIterface, sizeof(btTriangleIndexVertexArray));

				btIndexedMesh indexedMesh;
				READMEM(rpmHandle, stridingIterface.indexedMeshes.data, &indexedMesh, sizeof(btIndexedMesh));
				DLOG(" > Indexed mesh at " << stridingIterface.indexedMeshes.data);
				DLOG(" > Number of vertices: " << indexedMesh.numVertices);
				DLOG(" > Number of triangle faces: " << indexedMesh.numTriangles);

				constexpr int
					INDEXES_PER_TRI = 3,
					FLOATS_PER_VERTEX = 3;

				CollisionMeshFile meshFileOut;

				DLOG(" > Reading triangles...");
				meshFileOut.tris.resize(indexedMesh.numTriangles);
				READMEM(rpmHandle,
					indexedMesh.triangleIndexData,
					meshFileOut.tris.data(),
					indexedMesh.numTriangles * sizeof(CollisionMeshFile::Triangle));

				DLOG(" > Reading vertices...");
				meshFileOut.vertices.resize(indexedMesh.numVertices);
				READMEM(rpmHandle,
					indexedMesh.vertexData,
					meshFileOut.vertices.data(),
					indexedMesh.numVertices * sizeof(CollisionMeshFile::Vertex));

				if (rotated) {
					DLOG(" > Rotating...");
					for (auto& vertex : meshFileOut.vertices) {
						btVector3 rotated = basis.Rotate(btVector3{ vertex.x, vertex.y, vertex.z });
						vertex.x = rotated.x;
						vertex.y = rotated.y;
						vertex.z = rotated.z;
					}
				}

				DLOG(" > Offsetting...");
				meshFileOut.Offset({ pos.x, pos.y, pos.z });

				DLOG(" > Center of geometry (average vertex): " << meshFileOut.GetCenterOfGeometry());

				DLOG(" > Done.");

				meshFiles.push_back(meshFileOut);

			} else {
				DLOG(" > No striding mesh interface, ignoring.");
			}
		} else if (collisionShape.shapeType == STATIC_PLANE_PROXYTYPE) {
			DLOG(
				"Found btStaticPlaneShape at " << collisionObject.collisionShape <<
				" (collision object = " << collisionObjectPtr << "):");

			btVector3 pos = collisionObject.worldTransform.origin;
			DLOG(" > Origin (UU): " << pos * 50);
		}
	}
		
	gameModeOut = GAMEMODE_INVALID;

	LOG("Meshes found: " << meshFiles.size());

	constexpr int GAMEMODE_MESH_AMOUNTS[GAMEMODE_AMOUNT] = {
		16, // Soccar
		12 // Hoops
	};
	
	for (int i = 0; i < GAMEMODE_AMOUNT; i++) {
		if (meshFiles.size() == GAMEMODE_MESH_AMOUNTS[i]) {
			gameModeOut = i;
			break;
		}
	}

	if (gameModeOut != GAMEMODE_INVALID) {
		LOG("Detected game mode: " << GAMEMODE_STRS[gameModeOut]);
	} else {
		std::stringstream expectedMeshCountsStream;
		for (int i = 0; i < GAMEMODE_AMOUNT; i++) {
			if (i > 0) {
				if (GAMEMODE_AMOUNT > 2)
					expectedMeshCountsStream << ',';

				expectedMeshCountsStream << ' ';

				if (i == GAMEMODE_AMOUNT - 1)
					expectedMeshCountsStream << "or ";
			}

			expectedMeshCountsStream << GAMEMODE_MESH_AMOUNTS[i];
		}

		string expectedMeshCounts = expectedMeshCountsStream.str();

		FATAL_ERROR("Unknown number of meshes: " << meshFiles.size() << "\nExpected " << expectedMeshCounts << " meshes.");
	}

	return meshFiles;
}