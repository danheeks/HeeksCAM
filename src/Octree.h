// Octree.h

#pragma once

#include <fstream>
#include <list>
#include <set>
#include "Box.h"
#include "geometry.h"

class CTri;
class COctSolid;
class COctEle;

#define MAX_OCTTREE_LEVEL 8
#define OCTREE_USES_VERTEX_BUFFERS


class COctEle
{
public:
	enum BooleanType
	{
		OCTELE_BOOLEAN_UNION,
		OCTELE_BOOLEAN_CUT,
		OCTELE_BOOLEAN_COMMON,
		OCTELE_BOOLEAN_XOR,
	};

#ifdef OCTREE_USES_VERTEX_BUFFERS
	struct VertexType
	{
		float x, y, z;
		float r, g, b;
		VertexType(float X, float Y, float Z){ x = X; y = Y; z = Z; r = 0.0; g = 1.0; b = 0.0; }
		VertexType(){}
	};
#endif

	CBox m_box;
	int m_level;
	COctEle* m_children[8];  // (1, 1,-1), (-1, 1,-1), (-1,-1,-1), (1,-1,-1), (1, 1, 1), (-1, 1, 1), (-1,-1, 1), (1,-1, 1)  
	double m_rad;
	bool m_inside; // valid if there are no children
	unsigned char m_color_r, m_color_g, m_color_b;
	static int m_child_order[8][8]; // [ray_type][child]
	static bool texture;
	static bool add;

	COctEle(const CBox& box, int level);
	~COctEle();

	void Split();
	void DeleteChildren();
	void AddRemoveSolid(const COctSolid& s);
	virtual void Render(bool no_color);
};

class COctree: public COctEle
{
#ifdef OCTREE_USES_VERTEX_BUFFERS
	int m_vertexCount, m_indexCount;
	unsigned int m_vertexArrayId, m_vertexBufferId[10], m_indexBufferId[10];
#else
	int m_display_list;
#endif

public:
	int m_triangle_count;

	COctree(const CBox& box);
	void AddRemoveSolid(bool add_remove, const COctSolid& s);
	void Render(bool no_color);
#ifdef OCTREE_USES_VERTEX_BUFFERS
	bool MakeBuffer();
#endif
};
