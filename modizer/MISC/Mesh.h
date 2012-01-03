#ifndef st_Mesh_h
#define st_Mesh_h

#include <stdint.h>
#include <cstddef>


const int MaxVertices = 10000;
const int MaxIndices = 10000;

struct Vertex
{
	float x, y, z;
	float nx, ny, nz;
	float u, v;
};


struct Mesh
{
	Mesh() 
		: m_vertexCount(0)
		, m_indexCount(0)
	{}

	int m_vertexCount;
	int m_indexCount;
	
	Vertex m_vertices[MaxVertices];
	uint16_t m_indices[MaxIndices];
};


namespace MeshUtils
{
	bool Load(Mesh& mesh, const char* filename);
}

#endif
