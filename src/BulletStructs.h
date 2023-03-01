#pragma once
#include <iostream>

// Stripped BulletPhysics structures with only the fields we need

struct __declspec(align(16)) btVector3 {
	float x, y, z;

	friend std::ostream& operator <<(std::ostream& stream, btVector3 vec) {
		stream << "[ " << vec.x << ", " << vec.y << ", " << vec.z << " ]";
		return stream;
	}
};

struct __declspec(align(16)) btMatrix3x3 {
	btVector3 el[3];
};

struct __declspec(align(16)) btTransform {
	btMatrix3x3 basis;
	btVector3 origin;
};

//////////////////////////////////////////////

struct __declspec(align(8)) btAlignedObjectArray {
	char pad_0[0x4]; // allocator
	int size;
	int capacity;
	void** data;
	bool ownsMemory;
};
static_assert(sizeof(btAlignedObjectArray) == 0x20, "btAlignedObjectArray size must be 0x20 bytes exactly");

//////////////////////////////////////////////

struct btDynamicsWorld {
	char pad_0[0x8]; // vtable
	btAlignedObjectArray collisionObjects;
};

struct btCollisionObject {
	char pad_0[0x8]; // vtable
	btTransform worldTransform;
	char pad_50[0x80];
	struct btCollisionShape* collisionShape;
};

struct btCollisionShape {
	char pad_0[0x8];
	int shapeType;
};

struct btTriangleMeshShape {
	char pad_0[0x20];
	btVector3 localAabbMin;
	btVector3 localAabbMax;
	struct btTriangleIndexVertexArray* stridingMeshInterface;
};

struct btTriangleIndexVertexArray {
	char pad_0[0x20];
	btAlignedObjectArray indexedMeshes;
};

enum PHY_ScalarType : __int32
{
	PHY_FLOAT = 0x0,
	PHY_DOUBLE = 0x1,
	PHY_INTEGER = 0x2,
	PHY_SHORT = 0x3,
	PHY_FIXEDPOINT88 = 0x4,
	PHY_UCHAR = 0x5,
};

struct __declspec(align(8)) btIndexedMesh
{
	int numTriangles;
	void* triangleIndexData;
	int triangleIndexStride;
	int numVertices;
	void* vertexData;
	int vertexStride;
	PHY_ScalarType indexType;
	PHY_ScalarType vertexType;
};