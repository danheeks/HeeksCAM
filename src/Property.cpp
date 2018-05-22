// Property.cpp
// Copyright (c) 2018, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"

#include "Property.h"
#include "HeeksConfig.h"

Property::Property(const Property& rhs) : m_editable(false), m_highlighted(false), m_object(NULL)
{
	operator=(rhs);
}

const Property& Property::operator=(const Property &rhs)
{
	m_editable = rhs.m_editable;
	m_highlighted = rhs.m_highlighted;
	m_object = rhs.m_object;

	return *this;
}

Property *PropertyCheck::MakeACopy(void)const{
	PropertyCheck* new_object = new PropertyCheck(*this);
	return new_object;
}

void PropertyCheckWithConfig::Set(bool value)
{
	PropertyCheck::Set(value);
	HeeksConfig config;
	config.Write(m_config_name, wxGetApp().m_sketch_reorder_tol);
}

Property *PropertyCheckWithConfig::MakeACopy(void)const
{
	return new PropertyCheckWithConfig(*this);
}


Property *PropertyChoice::MakeACopy(void)const{
	PropertyChoice* new_object = new PropertyChoice(*this);
	return new_object;
}

Property *PropertyColor::MakeACopy(void)const{
	PropertyColor* new_object = new PropertyColor(*this);
	return new_object;
}

Property *PropertyDouble::MakeACopy(void)const{
	PropertyDouble* new_object = new PropertyDouble(*this);
	return new_object;
}

Property *PropertyDoubleLimited::MakeACopy(void)const{
	PropertyDoubleLimited* new_object = new PropertyDoubleLimited(*this);
	return new_object;
}

Property *PropertyFile::MakeACopy(void)const{
	PropertyFile* new_object = new PropertyFile(*this);
	return new_object;
}

Property *PropertyInt::MakeACopy(void)const{
	PropertyInt* new_object = new PropertyInt(*this);
	return new_object;
}

Property *PropertyLength::MakeACopy(void)const{
	PropertyLength* new_object = new PropertyLength(*this);
	return new_object;
}

void PropertyLengthWithConfig::Set(double value)
{
	PropertyLength::Set(value);
	HeeksConfig config;
	config.Write(m_config_name, *m_pvar);
}

Property *PropertyLengthWithConfig::MakeACopy(void)const{
	PropertyLength* new_object = new PropertyLengthWithConfig(*this);
	return new_object;
}


Property *PropertyList::MakeACopy(void)const{
	PropertyList* new_object = new PropertyList(*this);
	return new_object;
}

Property *PropertyString::MakeACopy(void)const{
	PropertyString* new_object = new PropertyString(*this);
	return new_object;
}

Property *PropertyStringReadOnly::MakeACopy(void)const{
	PropertyStringReadOnly* new_object = new PropertyStringReadOnly(*this);
	return new_object;
}

Property *PropertyStringWithConfig::MakeACopy(void)const{
	PropertyStringWithConfig* new_object = new PropertyStringWithConfig(*this);
	return new_object;
}

void PropertyStringWithConfig::Set(const wxChar* value)
{
	PropertyString::Set(value);
	HeeksConfig config;
	config.Write(m_config_name, wxGetApp().m_sketch_reorder_tol);
}

PropertyList* PropertyVertex(HeeksObj* object, const wxChar* title, double* x)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyDouble(object, _("x"), &x[0]));
	p->m_list.push_back(new PropertyDouble(object, _("y"), &x[1]));
	p->m_list.push_back(new PropertyDouble(object, _("z"), &x[2]));
	return p;
}

PropertyList* PropertyVertex(HeeksObj* object, const wxChar* title, const double* x)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyDouble(object, _("x"), &x[0]));
	p->m_list.push_back(new PropertyDouble(object, _("y"), &x[1]));
	p->m_list.push_back(new PropertyDouble(object, _("z"), &x[2]));
	return p;
}



class PropertyPntCoord :public Property{
protected:
	gp_Pnt* m_pnt;
public:
	PropertyPntCoord(HeeksObj* object, const wxChar* title, gp_Pnt *pnt) :Property(object, title), m_pnt(pnt){ }
};

class PropertyPntX :public PropertyPntCoord
{
public:
	PropertyPntX(HeeksObj* object, gp_Pnt *pnt) :PropertyPntCoord(object, _("x"), pnt){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ m_pnt->SetX(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_pnt->X(); }
	Property* MakeACopy()const{ return new PropertyPntX(*this); }
};

class PropertyPntY :public PropertyPntCoord
{
public:
	PropertyPntY(HeeksObj* object, gp_Pnt *pnt) :PropertyPntCoord(object, _("y"), pnt){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ m_pnt->SetY(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_pnt->Y(); }
	Property* MakeACopy()const{ return new PropertyPntY(*this); }
};

class PropertyPntZ :public PropertyPntCoord
{
public:
	PropertyPntZ(HeeksObj* object, gp_Pnt *pnt) :PropertyPntCoord(object, _("z"), pnt){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ m_pnt->SetZ(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_pnt->Z(); }
	Property* MakeACopy()const{ return new PropertyPntZ(*this); }
};

PropertyList* PropertyPnt(HeeksObj* object, const wxChar* title, gp_Pnt* pnt)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyPntX(object, pnt));
	p->m_list.push_back(new PropertyPntY(object, pnt));
	p->m_list.push_back(new PropertyPntZ(object, pnt));
	return p;
}

void PropertyPnt(std::list<Property *> *list, HeeksObj* object, gp_Pnt* pnt)
{
	list->push_back(new PropertyPntX(object, pnt));
	list->push_back(new PropertyPntY(object, pnt));
	list->push_back(new PropertyPntZ(object, pnt));
}

class PropertyDirCoord :public Property{
protected:
	gp_Dir* m_dir;
public:
	PropertyDirCoord(HeeksObj* object, const wxChar* title, gp_Dir *dir) :Property(object, title), m_dir(dir){ }
};

class PropertyDirX :public PropertyDirCoord
{
public:
	PropertyDirX(HeeksObj* object, gp_Dir *dir) :PropertyDirCoord(object, _("x"), dir){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ m_dir->SetX(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_dir->X(); }
	Property* MakeACopy()const{ return new PropertyDirX(*this); }
};

class PropertyDirY :public PropertyDirCoord
{
public:
	PropertyDirY(HeeksObj* object, gp_Dir *dir) :PropertyDirCoord(object, _("y"), dir){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ m_dir->SetY(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_dir->Y(); }
	Property* MakeACopy()const{ return new PropertyDirY(*this); }
};

class PropertyDirZ :public PropertyDirCoord
{
public:
	PropertyDirZ(HeeksObj* object, gp_Dir *dir) :PropertyDirCoord(object, _("z"), dir){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ m_dir->SetZ(value); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_dir->Z(); }
	Property* MakeACopy()const{ return new PropertyDirZ(*this); }
};

PropertyList* PropertyDir(HeeksObj* object, const wxChar* title, gp_Dir* dir)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyDirX(object, dir));
	p->m_list.push_back(new PropertyDirY(object, dir));
	p->m_list.push_back(new PropertyDirZ(object, dir));
	return p;
}

void PropertyDir(std::list<Property *> *list, HeeksObj* object, gp_Dir* dir)
{
	list->push_back(new PropertyDirX(object, dir));
	list->push_back(new PropertyDirY(object, dir));
	list->push_back(new PropertyDirZ(object, dir));
}

class PropertyAxisDirCoord :public Property{
protected:
	gp_Ax1* m_a;
public:
	PropertyAxisDirCoord(HeeksObj* object, const wxChar* title, gp_Ax1 *a) :Property(object, title), m_a(a){ }
};

class PropertyAxisDirX :public PropertyAxisDirCoord
{
public:
	PropertyAxisDirX(HeeksObj* object, gp_Ax1 *a) :PropertyAxisDirCoord(object, _("x"), a){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ gp_Dir dir = m_a->Direction(); dir.SetX(value); m_a->SetDirection(dir); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Direction().X(); }
	Property* MakeACopy()const{ return new PropertyAxisDirX(*this); }
};

class PropertyAxisDirY :public PropertyAxisDirCoord
{
public:
	PropertyAxisDirY(HeeksObj* object, gp_Ax1 *a) :PropertyAxisDirCoord(object, _("y"), a){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ gp_Dir dir = m_a->Direction(); dir.SetY(value); m_a->SetDirection(dir); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Direction().X(); }
	Property* MakeACopy()const{ return new PropertyAxisDirY(*this); }
};

class PropertyAxisDirZ :public PropertyAxisDirCoord
{
public:
	PropertyAxisDirZ(HeeksObj* object, gp_Ax1 *a) :PropertyAxisDirCoord(object, _("z"), a){ }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	void Set(double value){ gp_Dir dir = m_a->Direction(); dir.SetZ(value); m_a->SetDirection(dir); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Direction().Z(); }
	Property* MakeACopy()const{ return new PropertyAxisDirZ(*this); }
};

PropertyList* PropertyAxisDir(HeeksObj* object, const wxChar* title, gp_Ax1* a)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyAxisDirX(object, a));
	p->m_list.push_back(new PropertyAxisDirY(object, a));
	p->m_list.push_back(new PropertyAxisDirZ(object, a));
	return p;
}

class PropertyAxisLocCoord :public Property{
protected:
	gp_Ax1* m_a;
public:
	PropertyAxisLocCoord(HeeksObj* object, const wxChar* title, gp_Ax1 *a) :Property(object, title), m_a(a){ }
};

class PropertyAxisLocX :public PropertyAxisLocCoord
{
public:
	PropertyAxisLocX(HeeksObj* object, gp_Ax1 *a) :PropertyAxisLocCoord(object, _("x"), a){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt loc = m_a->Location(); loc.SetX(value); m_a->SetLocation(loc); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Location().X(); }
	Property* MakeACopy()const{ return new PropertyAxisLocX(*this); }
};

class PropertyAxisLocY :public PropertyAxisLocCoord
{
public:
	PropertyAxisLocY(HeeksObj* object, gp_Ax1 *a) :PropertyAxisLocCoord(object, _("y"), a){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt loc = m_a->Location(); loc.SetY(value); m_a->SetLocation(loc); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Direction().Y(); }
	Property* MakeACopy()const{ return new PropertyAxisLocY(*this); }
};

class PropertyAxisLocZ :public PropertyAxisLocCoord
{
public:
	PropertyAxisLocZ(HeeksObj* object, gp_Ax1 *a) :PropertyAxisLocCoord(object, _("z"), a){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt loc = m_a->Location(); loc.SetZ(value); m_a->SetLocation(loc); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return m_a->Location().Z(); }
	Property* MakeACopy()const{ return new PropertyAxisLocZ(*this); }
};

PropertyList* PropertyAxisLoc(HeeksObj* object, const wxChar* title, gp_Ax1* a)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyAxisLocX(object, a));
	p->m_list.push_back(new PropertyAxisLocY(object, a));
	p->m_list.push_back(new PropertyAxisLocZ(object, a));
	return p;
}






class PropertyTrsfBase :public Property{
protected:
	gp_Trsf* m_trsf;
public:
	PropertyTrsfBase(HeeksObj* object, const wxChar* title, gp_Trsf *trsf) :Property(object, title), m_trsf(trsf){ }
};

class PropertyLengthTrsfPosX :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfPosX(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("x"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_XYZ t = m_trsf->TranslationPart(); t.SetX(t.X() + value); m_trsf->SetTranslationPart(t); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return  m_trsf->TranslationPart().X(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfPosX(*this); }
};

class PropertyLengthTrsfPosY :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfPosY(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("y"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_XYZ t = m_trsf->TranslationPart(); t.SetY(t.Y() + value); m_trsf->SetTranslationPart(t); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return  m_trsf->TranslationPart().Y(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfPosY(*this); }
};

class PropertyLengthTrsfPosZ :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfPosZ(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("z"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_XYZ t = m_trsf->TranslationPart(); t.SetZ(t.Z() + value); m_trsf->SetTranslationPart(t); m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return  m_trsf->TranslationPart().Z(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfPosZ(*this); }
};

class PropertyLengthTrsfXDirX :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfXDirX(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("x"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); x.SetX(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); return x.X(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfXDirX(*this); }
};

class PropertyLengthTrsfXDirY :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfXDirY(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("y"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); x.SetY(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); return x.Y(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfXDirY(*this); }
};

class PropertyLengthTrsfXDirZ :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfXDirZ(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("z"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); x.SetZ(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); return x.Z(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfXDirZ(*this); }
};

class PropertyLengthTrsfYDirX :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfYDirX(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("x"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); y.SetX(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir y(0,1,0);  y.Transform(*m_trsf); return y.X(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfYDirX(*this); }
};

class PropertyLengthTrsfYDirY :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfYDirY(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("y"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); y.SetY(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir y(0, 1, 0);  y.Transform(*m_trsf); return y.Y(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfYDirY(*this); }
};

class PropertyLengthTrsfYDirZ :public PropertyTrsfBase
{
public:
	PropertyLengthTrsfYDirZ(HeeksObj* object, gp_Trsf *trsf) :PropertyTrsfBase(object, _("z"), trsf){ }
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	void Set(double value){ gp_Pnt d; d.Transform(*m_trsf); gp_Dir x(1, 0, 0);  x.Transform(*m_trsf); gp_Dir y(1, 0, 0); y.Transform(*m_trsf); y.SetZ(value); *m_trsf = make_matrix(d, x, y);  m_object->OnApplyProperties(); }
	double GetDouble(void)const{ gp_Dir y(0, 1, 0);  y.Transform(*m_trsf); return y.Z(); }
	Property* MakeACopy()const{ return new PropertyLengthTrsfYDirZ(*this); }
};

static const wxString angle_titles[3] = { _("vertical angle"), _("horizontal angle"), _("twist angle") };

class PropertyDoubleTrsfAngle :public PropertyTrsfBase
{
	int m_type;
public:
	PropertyDoubleTrsfAngle(HeeksObj* object, gp_Trsf* trsf, int type) :PropertyTrsfBase(object, angle_titles[type], trsf), m_type(type){}
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	Property* MakeACopy()const{ return new PropertyDoubleTrsfAngle(*this); }
	void Set(double value){
		double vertical_angle, horizontal_angle, twist_angle;
		gp_Dir x(1, 0, 0);
		gp_Dir y(0, 1, 0);
		gp_Pnt d(0, 0, 0);
		x.Transform(*m_trsf);
		y.Transform(*m_trsf);
		d.Transform(*m_trsf);
		CoordinateSystem::AxesToAngles(x, y, vertical_angle, horizontal_angle, twist_angle);
		switch (m_type)
		{
		case 0:
			vertical_angle = value * M_PI / 180;
			break;
		case 1:
			horizontal_angle = value * M_PI / 180;
			break;
		default:
			twist_angle = value * M_PI / 180;
			break;
		}
		gp_Dir dx, dy;
		CoordinateSystem::AnglesToAxes(vertical_angle, horizontal_angle, twist_angle, dx, dy);
		*m_trsf = make_matrix(d, dx, dy);
		m_object->OnApplyProperties();
	}
	double GetDouble(void)const{
		double vertical_angle, horizontal_angle, twist_angle;
		gp_Dir x(1, 0, 0);
		gp_Dir y(0, 1, 0);
		x.Transform(*m_trsf);
		y.Transform(*m_trsf);
		CoordinateSystem::AxesToAngles(x, y, vertical_angle, horizontal_angle, twist_angle);
		switch (m_type)
		{
		case 0:
			return vertical_angle / M_PI * 180;
		case 1:
			return horizontal_angle / M_PI * 180;
		default:
			return twist_angle / M_PI * 180;
		}
	}
};


PropertyList* PropertyTrsfPnt(HeeksObj* object, const wxChar* title, gp_Trsf* trsf)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyLengthTrsfPosX(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfPosY(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfPosZ(object, trsf));
	return p;
}

PropertyList* PropertyTrsfXDir(HeeksObj* object, const wxChar* title, gp_Trsf* trsf)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyLengthTrsfXDirX(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfXDirY(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfXDirZ(object, trsf));
	return p;
}

PropertyList* PropertyTrsfYDir(HeeksObj* object, const wxChar* title, gp_Trsf* trsf)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(new PropertyLengthTrsfYDirX(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfYDirY(object, trsf));
	p->m_list.push_back(new PropertyLengthTrsfYDirZ(object, trsf));
	return p;
}


PropertyList* PropertyTrsf(HeeksObj* object, const wxChar* title, gp_Trsf* trsf)
{
	PropertyList* p = new PropertyList(title);
	p->m_list.push_back(PropertyTrsfPnt(object, _("position"), trsf));
	p->m_list.push_back(PropertyTrsfXDir(object, _("x axis"), trsf));
	p->m_list.push_back(PropertyTrsfYDir(object, _("y axis"), trsf));
	p->m_list.push_back(new PropertyDoubleTrsfAngle(object, trsf, 0));
	p->m_list.push_back(new PropertyDoubleTrsfAngle(object, trsf, 1));
	p->m_list.push_back(new PropertyDoubleTrsfAngle(object, trsf, 2));
	return p;
}