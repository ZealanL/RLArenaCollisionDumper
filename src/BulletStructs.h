#pragma once
#include <iostream>

// Stripped BulletPhysics structures with only the fields we need

struct __declspec(align(16)) btVector3 {
	float x, y, z;

	bool operator==(const btVector3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator!=(const btVector3& other) const {
		return !(*this == other);
	}

	friend std::ostream& operator <<(std::ostream& stream, btVector3 vec) {
		stream << "[ " << vec.x << ", " << vec.y << ", " << vec.z << " ]";
		return stream;
	}

	btVector3 operator +(const btVector3& other) const {
		return btVector3{ x + other.x, y + other.y, z + other.z };
	}

	btVector3 operator *(float f) const {
		return btVector3{ x * f, y * f, z * f };
	}
};

struct __declspec(align(16)) btMatrix3x3 {
	btVector3 el[3];

	bool operator==(const btMatrix3x3& other) const {
		return el[0] == other.el[0] && el[1] == other.el[1] && el[2] == other.el[2];
	}

	bool operator!=(const btMatrix3x3& other) const {
		return !(*this == other);
	}

	friend std::ostream& operator <<(std::ostream& stream, btMatrix3x3 mat) {
		stream << "{" << std::endl;
		stream << "\t" << mat.el[0] << ", " << std::endl;
		stream << "\t" << mat.el[1] << ", " << std::endl;
		stream << "\t" << mat.el[2] << std::endl;
		stream << "} ";
		return stream;
	}

	static btMatrix3x3 GetIdentity() {
		constexpr static btMatrix3x3 IDENTITY = {
			btVector3{1, 0, 0},
			btVector3{0, 1, 0},
			btVector3{0, 0, 1}
		};
		return IDENTITY;
	}

	btVector3 Rotate(const btVector3& vec) {
		return
			el[0] * vec.x +
			el[1] * vec.y +
			el[2] * vec.z;
	}
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

// Mesh proxytype enum from bullet (only the ones relevant to RL)
enum {
	BOX_SHAPE_PROXYTYPE = 0,
	TRIANGLE_SHAPE_PROXYTYPE = 1,
	TETRAHEDRAL_SHAPE_PROXYTYPE,
	CONVEX_HULL_SHAPE_PROXYTYPE = 4,
	SPHERE_SHAPE_PROXYTYPE = 8,
	CUSTOM_CONVEX_SHAPE_TYPE = 19,
	TRIANGLE_MESH_SHAPE_PROXYTYPE = 21,
	SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE = 22,
	EMPTY_SHAPE_PROXYTYPE = 27,
	STATIC_PLANE_PROXYTYPE = 28,
	CUSTOM_CONCAVE_SHAPE_TYPE = 29,
	COMPOUND_SHAPE_PROXYTYPE = 31
};