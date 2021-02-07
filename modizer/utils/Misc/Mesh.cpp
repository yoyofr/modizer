#include "Mesh.h"
#include <stdio.h>
#include <map>
#include <assert.h>


namespace
{
	float positions[MaxVertices*3];
	float normals[MaxVertices*3];
	float texCoords[MaxVertices*2];

	std::map<uint64_t, int> foundVertices;

	uint64_t GetVertexKey(int p, int t, int n)
	{
		return p | (t << 12) | (n << 24);
	}
	
	void GetVertexData(Vertex& vertex, int p, int t, int n)
	{
		vertex.x = positions[p*3];
		vertex.y = positions[p*3+1];
		vertex.z = positions[p*3+2];

		vertex.nx = normals[n*3];
		vertex.ny = normals[n*3+1];
		vertex.nz = normals[n*3+2];

		vertex.u = texCoords[t*2];
		vertex.v = -texCoords[t*2+1];
	}

	void AddVertexAndIndex(int p, int t, int n, Mesh& mesh)
	{	
		const uint64_t vKey = GetVertexKey(p,t,n);
		if (foundVertices.find(vKey) == foundVertices.end())
		{
			const int index = mesh.m_vertexCount++;
			Vertex& vertex = mesh.m_vertices[index];
			GetVertexData(vertex, p-1, t-1, n-1);
			foundVertices[vKey] = index;
		}
		
		const int index = foundVertices[vKey];
		mesh.m_indices[mesh.m_indexCount++] = index;
	}
}



bool MeshUtils::Load(Mesh& mesh, const char* filename)
{
	FILE* fin = fopen(filename, "ra");
	if (fin == NULL)
		return false;

	foundVertices.clear();

	int posCount = 0;
	int normalCount = 0;
	int texCoordCount = 0;
	char line[256];
	while (fgets(line, 255, fin) != NULL)
	{
		float x, y, z;
		int p1,t1,n1, p2,t2,n2, p3,t3,n3;
		if (sscanf(line, "v %f %f %f", &x, &y, &z) == 3)
		{
			positions[posCount++] = x;
			positions[posCount++] = y;
			positions[posCount++] = z;
		}
		else if (sscanf(line, "vn %f %f %f", &x, &y, &z) == 3)
		{
			normals[normalCount++] = x;
			normals[normalCount++] = y;
			normals[normalCount++] = z;
		}
		else if (sscanf(line, "vt %f %f", &x, &y) == 2)
		{
			texCoords[texCoordCount++] = x;
			texCoords[texCoordCount++] = y;
		}
		else if (sscanf(line, "f %d/%d/%d %d/%d/%d %d/%d/%d", &p1, &t1, &n1, &p2, &t2, &n2, &p3, &t3, &n3) == 9)
		{
			assert (p1 < posCount && n1 < normalCount && t1 < texCoordCount);
			assert (p2 < posCount && n2 < normalCount && t2 < texCoordCount);
			assert (p3 < posCount && n3 < normalCount && t3 < texCoordCount);

			AddVertexAndIndex(p1, t1, n1, mesh);
			AddVertexAndIndex(p2, t2, n2, mesh);
			AddVertexAndIndex(p3, t3, n3, mesh);		
		}
		
		memset(line, 0, sizeof(line));
	}
	
	fclose(fin);
	return true;
}


