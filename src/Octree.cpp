// Octree.cpp

#include "stdafx.h"
#include "Octree.h"
#include "OctSolid.h"
#include "geometry.h"
#include <set>
#include <map>
#include "dxf.h"
#include "openglclass.h"
#include "colorshaderclass.h"

bool extensions_initialized = false;
OpenGLClass *OpenGL = NULL;
ColorShaderClass* ColorShader = NULL;
bool buffer_made = false;

int oct_ele_count = 0;

COctree::COctree(const CBox& box) :COctEle(box, 0)
{
	m_inside = false;
#ifdef OCTREE_USES_VERTEX_BUFFERS
#else
	m_display_list = 0;
#endif
}

void COctEle::Split()
{
	if ((m_level < MAX_OCTTREE_LEVEL) && (m_children[0] == NULL))
	{
		double mid[3];
		for (int i = 0; i < 3; i++)
		{
			mid[i] = (m_box.m_x[i] + m_box.m_x[i + 3]) * 0.5;
		}

		m_children[0] = new COctEle(CBox(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2], mid[0], mid[1], mid[2]), m_level + 1);
		m_children[1] = new COctEle(CBox(m_box.m_x[0], m_box.m_x[1], mid[2], mid[0], mid[1], m_box.m_x[5]), m_level + 1);
		m_children[2] = new COctEle(CBox(m_box.m_x[0], mid[1], m_box.m_x[2], mid[0], m_box.m_x[4], mid[2]), m_level + 1);
		m_children[3] = new COctEle(CBox(m_box.m_x[0], mid[1], mid[2], mid[0], m_box.m_x[4], m_box.m_x[5]), m_level + 1);
		m_children[4] = new COctEle(CBox(mid[0], m_box.m_x[1], m_box.m_x[2], m_box.m_x[3], mid[1], mid[2]), m_level + 1);
		m_children[5] = new COctEle(CBox(mid[0], m_box.m_x[1], mid[2], m_box.m_x[3], mid[1], m_box.m_x[5]), m_level + 1);
		m_children[6] = new COctEle(CBox(mid[0], mid[1], m_box.m_x[2], m_box.m_x[3], m_box.m_x[4], mid[2]), m_level + 1);
		m_children[7] = new COctEle(CBox(mid[0], mid[1], mid[2], m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]), m_level + 1);

		for (int i = 0; i < 8; i++)
		{
			m_children[i]->m_inside = m_inside;
			m_children[i]->m_color_r = rand() % 128;
			m_children[i]->m_color_g = rand() % 128;
			m_children[i]->m_color_b = rand() % 128;
		}
	}
}

void COctEle::DeleteChildren()
{
	if (m_children[0] != NULL)
	{
		for (int i = 0; i < 8; i++)
		{
			delete m_children[i];
		}
		m_children[0] = NULL;
	}
}

void COctEle::AddRemoveSolid(const COctSolid& s)
{
	if ((m_children[0] == NULL) && (m_inside == add))
	{
		// if already in or out
		return;
	}

	int inside = s.Inside(m_box);

	if (inside == 2)
	{
		DeleteChildren();
		m_inside = add;
		m_color_r = rand() % 128;
		m_color_g = rand() % 128;
		m_color_b = rand() % 128;
	}
	else if (inside == 1)
	{
		if (m_level < MAX_OCTTREE_LEVEL)
		{
			Split();
			for (int i = 0; i < 8; i++)
			{
				m_children[i]->AddRemoveSolid(s);
			}
		}
		else
		{
			m_inside = add || m_inside;
			s.SetElementsColor(*this);
		}
	}
}

static int triangle_count = 0;

#ifdef OCTREE_USES_VERTEX_BUFFERS
#define VERTEX_LIST_SIZE 27000000
#define BUFFER_SIZE 2700000
COctEle::VertexType* vertex_list = NULL;
int vertex_count = 0;
#endif

void COctEle::Render(bool no_color)
{
	if (m_children[0] != NULL)
	{
		for (int i = 0; i < 8; i++)
			m_children[i]->Render(no_color);
	}
	else
	{
		if (m_inside && m_level == MAX_OCTTREE_LEVEL)
		{
			// render a cube
#ifdef OCTREE_USES_VERTEX_BUFFERS
			if (vertex_count > 7000000)return;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]); vertex_count++;

			// right
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]); vertex_count++;

			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]); vertex_count++;

			// left
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]); vertex_count++;

			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]); vertex_count++;

			// back
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]); vertex_count++;

			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]); vertex_count++;

			// top
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]); vertex_count++;

			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]); vertex_count++;

			// bottom
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]); vertex_count++;

			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
			vertex_list[vertex_count] = VertexType(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]); vertex_count++;
#else
			if (!no_color)glColor3ub(this->m_color_r, m_color_g, m_color_b);
			glBegin(GL_TRIANGLES);
			// front
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]);

			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]);
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]);

			// right
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]);

			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]);

			// left
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]);

			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]);
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]);

			// back
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]);

			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]);

			// top
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[5]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]);

			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[5]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[5]);
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[5]);

			// bottom
			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]);

			glVertex3d(m_box.m_x[0], m_box.m_x[4], m_box.m_x[2]);
			glVertex3d(m_box.m_x[3], m_box.m_x[1], m_box.m_x[2]);
			glVertex3d(m_box.m_x[0], m_box.m_x[1], m_box.m_x[2]);

			glEnd();
#endif
			triangle_count += 12;
		}
	}

}

//static 
//int COctEle::m_child_order[8][8] = {
//	{ 6, 2, 5, 7, 1, 3, 4, 0 },
//	{ 2, 1, 3, 6, 0, 5, 7, 4 },
//	{ 5, 1, 4, 6, 0, 2, 7, 3 },
//	{ 1, 0, 2, 5, 3, 4, 6, 7 },
//	{ 7, 3, 4, 6, 0, 2, 5, 1 },
//	{ 3, 0, 2, 7, 1, 4, 6, 5 },
//	{ 4, 0, 5, 7, 1, 3, 6, 2 },
//	{ 0, 1, 3, 4, 2, 5, 7, 6 },
//};

//int m_type;//  0 +x+y+z  1 +x+y-z  2 +x-y+z  3 +x-y-z  4 -x+y+z  5 -x+y-z  6 -x-y+z  7 -x-y-z

int COctEle::m_child_order[8][8] = {
	{0, 2, 1, 4, 6, 5, 3, 7},
	{1, 3, 5, 0, 2, 4, 7, 6},
	{2, 6, 0, 3, 1, 7, 4, 5},
	{3, 2, 7, 1, 5, 6, 0, 4},
	{4, 0, 6, 5, 1, 7, 2, 3},
	{5, 1, 7, 4, 3, 6, 0, 2},
	{6, 2, 4, 7, 3, 5, 0, 1},
	{7, 3, 5, 6, 1, 2, 4, 0},
};

COctEle::COctEle(const CBox& box, int level) : m_box(box), m_level(level), m_inside(false), m_color_r(0), m_color_g(0), m_color_b(0)
{
	oct_ele_count++;
	m_children[0] = NULL; // mark the first child as NULL to show there are no children yet
}

COctEle::~COctEle()
{
	if (m_children[0])
	{
		for (int i = 0; i < 8; i++)
			delete m_children[i];
	}
}

bool COctEle::texture = true;
bool COctEle::add = true;

void COctree::AddRemoveSolid(bool add_remove, const COctSolid& s)
{
	COctEle::add = add_remove;
	COctEle::AddRemoveSolid(s);
}

#ifdef OCTREE_USES_VERTEX_BUFFERS

bool COctree::MakeBuffer()
{
#if 1
	return false;
#else
	vertex_list = new VertexType[VERTEX_LIST_SIZE];
	if (!vertex_list)
	{
		return false;
	}

	triangle_count = 0;
	vertex_count = 0;
	COctEle::Render(false);

	m_triangle_count = triangle_count;
	
		unsigned int* indices;


		// Set the number of vertices in the vertex array.

		// Set the number of indices in the index array.
		m_indexCount = vertex_count;

		// Create the index array.
		indices = new unsigned int[m_indexCount];
		if (!indices)
		{
			return false;
		}

		// Load the vertex array with data.

		
		for (int i = 0; i < vertex_count; i++)
		{
			indices[i] = i;
		}

		// Allocate an OpenGL vertex array object.
		OpenGL->glGenVertexArrays(1, &m_vertexArrayId);

		// Bind the vertex array object to store all the buffers and vertex attributes we create here.
		OpenGL->glBindVertexArray(m_vertexArrayId);

		// Generate an ID for the vertex buffer.
		OpenGL->glGenBuffers(10, m_vertexBufferId);

		COctEle::VertexType* pvertex_list = vertex_list;
		unsigned int* pindices = indices;

		for (int i = 0; i < 10; i++)
		{
			// Bind the vertex buffer and load the vertex (position and color) data into the vertex buffer.
			OpenGL->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
			OpenGL->glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexType), vertex_list, GL_STATIC_DRAW);

			// Enable the two vertex array attributes.
			OpenGL->glEnableVertexAttribArray(0);  // Vertex position.
			OpenGL->glEnableVertexAttribArray(1);  // Vertex color.

			// Specify the location and format of the position portion of the vertex buffer.
			OpenGL->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
			OpenGL->glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(VertexType), 0);

			// Specify the location and format of the color portion of the vertex buffer.
			OpenGL->glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
			OpenGL->glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(VertexType), (unsigned char*)NULL + (3 * sizeof(float)));

			// Generate an ID for the index buffer.
			OpenGL->glGenBuffers(1, &m_indexBufferId);

			// Bind the index buffer and load the index data into it.
			OpenGL->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
			OpenGL->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indexCount* sizeof(unsigned int), indices, GL_STATIC_DRAW);
		}

		// Now that the buffers have been loaded we can release the array data.
		delete[] indices;
		indices = 0;

		return true;
#endif
	}
#endif

void COctree::Render(bool no_color)
{
#ifdef OCTREE_USES_VERTEX_BUFFERS
	if (!extensions_initialized)
	{
		if (OpenGL == NULL)
			OpenGL = new OpenGLClass;

		bool result = OpenGL->InitializeExtensions();
		extensions_initialized = true;
		if (!result)
		{
			wxMessageBox(L"Could not initialize the OpenGL extensions.", L"Error", MB_OK);
			return;
		}
		// Create the color shader object.
		ColorShader = new ColorShaderClass;

		// Initialize the color shader object.
		result = ColorShader->Initialize(OpenGL);
		if (!result)
		{
			wxMessageBox(L"Could not initialize the color shader object.", L"Error", MB_OK);
			return;
		}

	}

	if (!buffer_made)
	{
		buffer_made = true;
		bool result = MakeBuffer();
	}
	//float worldMatrix[16];
	//float viewMatrix[16];
	//float projectionMatrix[16];

	//OpenGL->GetWorldMatrix(worldMatrix);
	//OpenGL->GetWorldMatrix(viewMatrix);
	//OpenGL->GetProjectionMatrix(projectionMatrix);

	// Set the color shader as the current shader program and set the matrices that it will use for rendering.
	//ColorShader->SetShader(OpenGL);
	//ColorShader->SetShaderParameters(OpenGL, worldMatrix, viewMatrix, projectionMatrix);

	// Render the model using the color shader.

	// Bind the vertex array object that stored all the information about the vertex and index buffers.
	OpenGL->glBindVertexArray(m_vertexArrayId);

	// Render the vertex buffer using the index buffer.
	glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0);
#else
	if (m_display_list)
	{
		glCallList(m_display_list);
	}
	else{
		m_display_list = glGenLists(1);
		glNewList(m_display_list, GL_COMPILE_AND_EXECUTE);

		triangle_count = 0;
		COctEle::Render(false);
		m_triangle_count = triangle_count;

		glEndList();
	}
#endif
}
