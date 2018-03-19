// HOctree.cpp
// Copyright (c) 2017, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "HOctree.h"
#include "GraphicsCanvas.h"
#include "OctCube.h"
#include "OctSphere.h"
#include "PropertyInt.h"

HOctree::HOctree(const CBox& box) :m_octree(box)
{
	double x25 = box.MinX() + box.Width() * 0.01;
	double x75 = box.MinX() + box.Width() * 0.99;
	double y25 = box.MinY() + box.Height() * 0.01;
	double y75 = box.MinY() + box.Height() * 0.99;
	double z25 = box.MinZ() + box.Depth() * 0.01;
	double z75 = box.MinZ() + box.Depth() * 0.99;
	CBox half_box(x25, y25, z25, x75, y75, z75);
	m_octree.AddRemoveSolid(true, COctCube(half_box));
	//m_octree.AddRemoveSolid(true, COctCube(CBox(10, -20, -10, 50, 20, 0)));

#if 0
	for (int i = 0; i < 500; i++)
	{
		double x = box.MinX() + box.Width() * rand() / 32768.0;
		double y = box.MinY() + box.Height() * rand() / 32768.0;
		double z = box.MinZ() + box.Depth() * rand() / 32768.0;
		m_octree.AddRemoveSolid(false, COctSphere(geoff_geometry::Point3d(x, y, z), 20.0));
	}
#else
	m_octree.AddRemoveSolid(false, COctSphere(geoff_geometry::Point3d(x75, y25, z75), 20.0));
#endif

}

HOctree::~HOctree(void){

}

HOctree::HOctree(const HOctree &p) :m_octree(p.m_octree)
{
	operator=(p);
}

const HOctree& HOctree::operator=(const HOctree &b)
{
	HeeksObj::operator =(b);

	m_octree = b.m_octree;

	return *this;
}

void HOctree::glCommands(bool select, bool marked, bool no_color)
{
	// render the octree
	m_octree.Render(no_color);

	//RenderTest();
}

void HOctree::GetBox(CBox &box)
{


}

HeeksObj *HOctree::MakeACopy(void)const
{
	return new HOctree(*this);
}

const wxBitmap &HOctree::GetIcon()
{
	static wxBitmap* icon = NULL;
	if (icon == NULL)icon = new wxBitmap(wxImage(wxGetApp().GetResFolder() + _T("/icons/octree.png")));
	return *icon;
}

void HOctree::ModifyByMatrix(const double *mat)
{

}

void HOctree::GetGripperPositions(std::list<GripData> *list, bool just_for_endof)
{

}

void HOctree::GetProperties(std::list<Property *> *list)
{
#if 0 // to do
	list->push_back(new PropertyInt(_("triangle_count"), m_octree.m_triangle_count, this));
#endif
	HeeksObj::GetProperties(list);
}

void HOctree::WriteXML(TiXmlNode *root)
{

}


//static
HeeksObj* HOctree::ReadFromXMLElement(TiXmlElement* pElem)
{
	// to do
	CBox test_box(-100, -100, -100, 100, 100, 100);
	HOctree* new_object = new HOctree(test_box);
	new_object->ReadBaseXML(pElem);

	return new_object;
}


	void RenderTest()
	{
	}
