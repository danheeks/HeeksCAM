// Vertex.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
#include "stdafx.h"
#include "Vertex.h"
#include "Face.h"
#include "Solid.h"
#include "Gripper.h"
#include "BRepTools_ReShape.hxx"

HVertex::HVertex(const TopoDS_Vertex &vertex):m_topods_vertex(vertex){
	gp_Pnt pos = BRep_Tool::Pnt(vertex);
	extract(pos, m_point);
}

HVertex::~HVertex(){
}

void HVertex::FindEdges()
{
	CShape* body = GetParentBody();
	if(body)
	{
		for(HeeksObj* object = body->m_edges->GetFirstChild(); object; object = body->m_edges->GetNextChild())
		{
			CEdge* e = (CEdge*)object;
			HVertex* v0 = e->GetVertex0();
			HVertex* v1 = e->GetVertex1();
			if(v0 == this || v1 == this)m_edges.push_back(e);
		}
	}
}

const wxBitmap &HVertex::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(wxGetApp().GetResFolder() + _T("/icons/vertex.png")));
	return *icon;
}

void HVertex::glCommands(bool select, bool marked, bool no_color){
	// don't render anything, but put a point for selection
	glRasterPos3dv(m_point);
}

void HVertex::GetGripperPositions(std::list<GripData> *list, bool just_for_endof){
	list->push_back(GripData(GripperTypeTranslate,m_point[0],m_point[1],m_point[2],NULL));
}

void HVertex::ModifyByMatrix(const double* m)
{
#if 1
	TopLoc_Location loc(make_matrix(m));
	m_topods_vertex.Move(loc);
#else
	gp_Trsf trans = make_matrix(m);
	BRepBuilderAPI_Transform brepTrans(m_topods_vertex, trans);
	TopoDS_Vertex newVertex = TopoDS::Vertex(brepTrans.Shape());

	BRepTools_ReShape reshape;
	reshape.Replace(m_topods_vertex, newVertex);
	reshape.Apply(m_topods_vertex);

	//BRepTools_ReShape::Apply(oldEdge);
	//BRepTools_ReShape::Replace(oldEdge, newEdge);
	//BRepTools_ReShape::Apply(oldCompund);
#endif
}

CEdge* HVertex::GetFirstEdge()
{
	if (m_edges.size()==0)FindEdges();
	if (m_edges.size()==0) return NULL;
	m_edgeIt = m_edges.begin();
	return *m_edgeIt;
}

CEdge* HVertex::GetNextEdge()
{
	if (m_edges.size()==0 || m_edgeIt==m_edges.end()) return NULL;
	m_edgeIt++;
	if (m_edgeIt==m_edges.end()) return NULL;
	return *m_edgeIt;
}

CShape* HVertex::GetParentBody()
{
	if(m_owner == NULL)return NULL;
	if(m_owner->m_owner == NULL)return NULL;
	if(m_owner->m_owner->GetType() != SolidType)return NULL;
	return (CShape*)(m_owner->m_owner);
}
