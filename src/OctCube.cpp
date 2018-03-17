// OctCube.cpp

#include "OctCube.h"
#include "Box.h"
#include "Octree.h"

COctCube::COctCube(const CBox& box)
{
	m_box = box;
}

int COctCube::Inside(const CBox& box)const
{
	if (m_box.Contains(box))
		return 2;

	if (m_box.Intersects(box))
		return 1;

	return 0;
}

void COctCube::SetElementsColor(COctEle& ele)const
{
	double v[3], v2[3];
	double c[3], c2[3];
	ele.m_box.Centre(c);
	m_box.Centre(c2);
	for (int i = 0; i < 3; i++)
	{
		v[i] = fabs(c[i] - c2[i]);
		v2[i] = m_box.m_x[i + 3] - c2[i];
	}

	geoff_geometry::Vector3d vv(v);
	geoff_geometry::Vector3d vv2(v2);

	ele.m_color_r = 128;
	ele.m_color_g = 128;
	ele.m_color_b = 128;

	if (COctEle::texture)
	{
		ele.m_color_r += rand() % 12;
		ele.m_color_g += rand() % 12;
		ele.m_color_b += rand() % 12;
	}

	double size = ele.m_box.Width() * 0.5;

	if (vv.getx() + size > vv2.getx())
	{
		if (c[0] < c2[0])
		{
			ele.m_color_r += 64;
			ele.m_color_g += 64;
			ele.m_color_b += 64;
		}
		else{
			ele.m_color_r -= 64;
			ele.m_color_g -= 64;
			ele.m_color_b -= 64;
		}
	}
	else if (vv.gety() + size > vv2.gety())
	{
		if (c[1] < c2[1])
		{
			ele.m_color_r += 32;
			ele.m_color_g += 32;
			ele.m_color_b += 32;
		}
		else{
			ele.m_color_r -= 32;
			ele.m_color_g -= 32;
			ele.m_color_b -= 32;
		}
	}
	else if (vv.getz() + size > vv2.getz())
	{
		if (c[2] < c2[2])
		{
			ele.m_color_r += 16;
			ele.m_color_g += 16;
			ele.m_color_b += 16;
		}
		else{
			ele.m_color_r -= 16;
			ele.m_color_g -= 16;
			ele.m_color_b -= 16;
		}
	}
}

