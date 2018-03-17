// OctSphere.cpp

#include "OctSphere.h"
#include "Box.h"
#include "Octree.h"

COctSphere::COctSphere(const geoff_geometry::Point3d& c, double r)
{
	m_c = c;
	m_r = r;
	m_box.m_valid = true;
	m_box.m_x[0] = c.x - r;
	m_box.m_x[1] = c.y - r;
	m_box.m_x[2] = c.z - r;
	m_box.m_x[3] = c.x + r;
	m_box.m_x[4] = c.y + r;
	m_box.m_x[5] = c.z + r;
}

inline double squared(double v) { return v * v; }

int COctSphere::Inside(const CBox& box)const
{
	double dist_squared = m_r * m_r;
	/* assume C1 and C2 are element-wise sorted, if not, do that now */
	if (m_c.x < box.m_x[0]) dist_squared -= squared(m_c.x - box.m_x[0]);
	else if (m_c.x > box.m_x[3]) dist_squared -= squared(m_c.x - box.m_x[3]);
	if (m_c.y < box.m_x[1]) dist_squared -= squared(m_c.y - box.m_x[1]);
	else if (m_c.y > box.m_x[4]) dist_squared -= squared(m_c.y - box.m_x[4]);
	if (m_c.z < box.m_x[2]) dist_squared -= squared(m_c.z - box.m_x[2]);
	else if (m_c.z > box.m_x[5]) dist_squared -= squared(m_c.z - box.m_x[5]);
	if (dist_squared > 0)
	{
		for (int i = 0; i < 8; i++)
		{
			double x[3];
			box.vert(i, x);
			if (Inside(geoff_geometry::Point3d(x)) == false)
				return 1;
		}
		return 2;
	}

	return 0;
}

bool COctSphere::Inside(const geoff_geometry::Point3d& p)const
{
	return p.Dist(m_c) <= m_r;
}

void COctSphere::SetElementsColor(COctEle& ele)const
{
	double bc[3];
	ele.m_box.Centre(bc);
	geoff_geometry::Vector3d v(geoff_geometry::Point3d(bc), m_c);
	v.Normalize();
	double d = (v.getx() + v.gety() + v.getz()) / 2.2;
	if (!COctEle::add)d = -d;

	ele.m_color_r = 128 + d * 104;
	ele.m_color_g = 128 + d * 104;
	ele.m_color_b = 128 + d * 104;

	if (COctEle::texture)
	{
		ele.m_color_r += rand() % 12;
		ele.m_color_g += rand() % 12;
		ele.m_color_b += rand() % 12;
	}
}
