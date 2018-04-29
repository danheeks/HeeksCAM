// HeeksObj.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "HeeksObj.h"
#include "PropertyString.h"
#include "PropertyInt.h"
#include "PropertyColor.h"
#include "PropertyCheck.h"
#include "tinyxml.h"
#include "ObjList.h"
#include "../src/Gripper.h"
#include "../src/HeeksFrame.h"
#include "../src/ObjPropsCanvas.h"
#include "../src/Sketch.h"

HeeksObj::HeeksObj(void): m_owner(NULL), m_skip_for_undo(false), m_id(0), m_layer(0), m_visible(true), m_preserving_id(false), m_index(0)
{
}

HeeksObj::HeeksObj(const HeeksObj& ho): m_owner(NULL), m_skip_for_undo(false), m_id(0), m_layer(0), m_visible(true),m_preserving_id(false), m_index(0)
{
	operator=(ho);
}

const HeeksObj& HeeksObj::operator=(const HeeksObj &ho)
{
	// don't copy the ID or the owner
	m_layer = ho.m_layer;
	m_visible = ho.m_visible;
	m_skip_for_undo = ho.m_skip_for_undo;

	if(ho.m_preserving_id)
		m_id = ho.m_id;

	return *this;
}

HeeksObj::~HeeksObj()
{
	if(m_owner)m_owner->Remove(this);

	if (m_index) wxGetApp().ReleaseIndex(m_index);
}

HeeksObj* HeeksObj::MakeACopyWithID()
{
	m_preserving_id = true;
	HeeksObj* ret = MakeACopy();
	m_preserving_id = false;
	return ret;
}


class PropertyObjectTitle :public Property{
public:
	PropertyObjectTitle(HeeksObj* object) :Property(object, _("object title")){ m_editable = object->CanEditString(); }
	// Property's virtual functions
	int get_property_type(){ return StringPropertyType; }
	Property *MakeACopy(void)const{ return new PropertyObjectTitle(*this); }
	const wxChar* GetString()const{ return m_object->GetShortString(); }
	void Set(const wxChar* value){ m_object->OnEditString(value); }
};

class PropertyObjectColor :public Property{
public:
	PropertyObjectColor(HeeksObj* object) :Property(object, _("color")){}
	// Property's virtual functions
	int get_property_type(){ return ColorPropertyType; }
	Property *MakeACopy(void)const{ return new PropertyObjectColor(*this); }
	const HeeksColor &GetColor()const{ return *(m_object->GetColor()); }
	void Set(const HeeksColor& value){ m_object->SetColor(value); }
};



static void on_set_color(HeeksColor value, HeeksObj* object)
{
	object->SetColor(value);
	wxGetApp().m_frame->m_properties->OnApply2();
	wxGetApp().Repaint();
}

static void on_set_id(int value, HeeksObj* object)
{
	object->SetID(value);
}

static void on_set_visible(bool value, HeeksObj* object)
{
	object->m_visible = value;
	wxGetApp().Repaint();
}

const wxBitmap &HeeksObj::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(wxGetApp().GetResFolder() + _T("/icons/unknown.png")));
	return *icon;
}

static bool test_bool = false;

void HeeksObj::GetProperties(std::list<Property *> *list)
{
	bool editable = CanEditString();
	list->push_back(new PropertyStringReadOnly(_("object type"), GetTypeString()));

	if (GetShortString())list->push_back(new PropertyObjectTitle(this));
	if(UsesID())list->push_back(new PropertyInt(this, _("ID"), (int*)(&m_id)));
	const HeeksColor* c = GetColor();
	if(c)list->push_back ( new PropertyObjectColor(this) );
	list->push_back(new PropertyCheck(this, _("visible"), &m_visible));
}

bool HeeksObj::GetScaleAboutMatrix(double *m)
{
	// return the bottom left corner of the box
	CBox box;
	GetBox(box);
	if(!box.m_valid)return false;
	gp_Trsf mat;
	mat.SetTranslationPart(gp_Vec(box.m_x[0], box.m_x[1], box.m_x[2]));
	extract(mat, m);
	return true;
}

void HeeksObj::GetGripperPositionsTransformed(std::list<GripData> *list, bool just_for_endof)
{
	GetGripperPositions(list,just_for_endof);
}

void HeeksObj::GetGripperPositions(std::list<GripData> *list, bool just_for_endof)
{
	CBox box;
	GetBox(box);
	if(!box.m_valid)return;

	//TODO: This is a tab bit of a strange thing to do. Especially for planar objects like faces
	//ones that are on a plane like y-z or x-z will have all gripper merged togeather.
	list->push_back(GripData(GripperTypeTranslate,box.m_x[0],box.m_x[1],box.m_x[2],NULL));
	list->push_back(GripData(GripperTypeRotateObject,box.m_x[3],box.m_x[1],box.m_x[2],NULL));
	list->push_back(GripData(GripperTypeRotateObject,box.m_x[0],box.m_x[4],box.m_x[2],NULL));
	list->push_back(GripData(GripperTypeScale,box.m_x[3],box.m_x[4],box.m_x[2],NULL));
}

bool HeeksObj::Add(HeeksObj* object, HeeksObj* prev_object)
{
	object->m_owner = this;
	object->OnAdd();
	return true;
}

void HeeksObj::OnRemove()
{
	if(m_owner == NULL)KillGLLists();
}

void HeeksObj::SetID(int id)
{
	wxGetApp().SetObjectID(this, id);
}

void HeeksObj::WriteBaseXML(TiXmlElement *element)
{
	wxGetApp().ObjectWriteBaseXML(this, element);
}

void HeeksObj::ReadBaseXML(TiXmlElement* element)
{
	wxGetApp().ObjectReadBaseXML(this, element);
}

bool HeeksObj::OnVisibleLayer()
{
	// to do, support multiple layers.
	return true;
}

HeeksObj *HeeksObj::Find( const int type, const unsigned int id )
{
	if ((type == this->GetType()) && (this->m_id == id)) return(this);
	return(NULL);
}

#ifdef WIN32
#define snprintf _snprintf
#endif

void HeeksObj::ToString(char *str, unsigned int* rlen, unsigned int len)
{
	unsigned int printed;
	*rlen = 0;

	printed = snprintf(str,len,"ID: 0x%X, Type: 0x%X, MarkingMask: 0x%X, IDGroup: 0x%X\n",GetID(),GetType(),(unsigned int)GetMarkingMask(),GetIDGroupType());
	if(printed >= len)
		goto abort;
	*rlen += printed; len -= printed;

abort:
	*rlen = 0;
}

unsigned int HeeksObj::GetIndex() {
	if (!m_index) m_index = wxGetApp().GetIndex(this);
	return m_index;
}
