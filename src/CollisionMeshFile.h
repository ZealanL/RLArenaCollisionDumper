#pragma once
#include "Framework.h"

#define COLLISION_MESH_BASE_PATH "./collision_meshes/"
#define COLLISION_MESH_FILE_EXTENSION ".cmf"

// Basic collision mesh file struct I made for this program
struct CollisionMeshFile {

	struct Triangle {
		int vertexIndexes[3];
	};

	struct Vertex {
		float x, y, z;

		friend std::ostream& operator <<(std::ostream& stream, Vertex vertex) {
			stream << "[ " << vertex.x << ", " << vertex.y << ", " << vertex.z << " ]";
			return stream;
		}
	};

	vector<Triangle> tris;
	vector<Vertex> vertices;

	void Offset(Vertex amount) {
		for (Vertex& v : vertices) {
			v.x += amount.x;
			v.y += amount.y;
			v.z += amount.z;
		}
	}

	void Scale(Vertex scale) {
		for (Vertex& v : vertices) {
			v.x *= scale.x;
			v.y *= scale.y;
			v.z *= scale.z;
		}
	}

	// Average of all vertices
	Vertex GetCenterOfGeometry() {
		Vertex sum = { 0,0,0 };
		for (Vertex& v : vertices) {
			sum.x += v.x;
			sum.y += v.y;
			sum.z += v.z;
		}

		sum.x /= vertices.size();
		sum.y /= vertices.size();
		sum.z /= vertices.size();

		return sum;
	}

	void WriteToFile(string path, string name) {
		string filePath = path + name + COLLISION_MESH_FILE_EXTENSION;

		// Ensure directories exist
		CreateDirectoryA(COLLISION_MESH_BASE_PATH, NULL);
		CreateDirectoryA(path.c_str(), NULL);

		std::ofstream outStream = std::ofstream(filePath, std::ios::binary);
		
		assert(outStream.good());
		assert(tris.size() > 0 && vertices.size() > 0);

		// Write amount of tris and vertices
		int numTris = tris.size();
		outStream.write((const char*)&numTris, sizeof(int));
		int numVertices = vertices.size();
		outStream.write((const char*)&numVertices, sizeof(int));

		// Write raw data
		outStream.write((const char*)tris.data(), tris.size() * sizeof(Triangle));
		outStream.write((const char*)vertices.data(), vertices.size() * sizeof(Vertex));

		outStream.close();
	}
};