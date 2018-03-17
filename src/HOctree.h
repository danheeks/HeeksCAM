// HOctree.h
// Copyright (c) 2017, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#pragma once

#include "IdNamedObj.h"
#include "HeeksColor.h"
#include "Octree.h"

class HOctree : public IdNamedObj{
private:
	COctree m_octree;

public:
	HOctree(const CBox& box);
	~HOctree(void);
	HOctree(const HOctree &p);

	const HOctree& operator=(const HOctree &b);

	// HeeksObj's virtual functions
	int GetType()const{ return OctreeType; }
	long GetMarkingMask()const{ return MARKING_FILTER_OCTREE; }
	void glCommands(bool select, bool marked, bool no_color);
	void GetBox(CBox &box);
	const wxChar* GetTypeString(void)const{ return _("Octree"); }
	HeeksObj *MakeACopy(void)const;
	const wxBitmap &GetIcon();
	void ModifyByMatrix(const double *mat);
	void GetGripperPositions(std::list<GripData> *list, bool just_for_endof);
	void GetProperties(std::list<Property *> *list);
	void CopyFrom(const HeeksObj* object){ operator=(*((HOctree*)object)); }
	void WriteXML(TiXmlNode *root);

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
};


