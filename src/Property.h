// Property.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

// Base class for all Properties

#if !defined Property_HEADER
#define Property_HEADER

enum{
	InvalidPropertyType,
	StringPropertyType,
	DoublePropertyType,
	LengthPropertyType,
	IntPropertyType,
	ChoicePropertyType,
	ColorPropertyType,
	CheckPropertyType,
	ListOfPropertyType,
	FilePropertyType
};

class Property{
public:
	std::wstring m_title;
	bool m_editable;
	bool m_highlighted;
	HeeksObj* m_object;

	Property(void) :m_editable(false), m_highlighted(false), m_object(NULL), m_title(_("Unknown")){} // default constructor for python
	Property(HeeksObj* object, const wxChar* title) :m_editable(true), m_highlighted(false), m_object(object), m_title(title){}
	Property(const Property& ho);
	virtual ~Property(){}

	virtual const Property& operator=(const Property &ho);

	virtual int get_property_type(){return InvalidPropertyType;}
	virtual Property *MakeACopy(void)const{ return NULL; }
	virtual const wxChar* GetShortString(void)const{ return m_title.c_str(); }
	virtual void Set(bool value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const HeeksColor& value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(double value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const wxChar*){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(int value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const gp_Trsf& value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual bool GetBool()const{ return false; }
	virtual void GetChoices(std::list< wxString > &choices){}
	virtual const HeeksColor &GetColor()const{ return *((const HeeksColor*)NULL); }
	virtual double GetDouble()const{ return 0.0; }
	virtual const wxChar* GetString()const{ return NULL; }
	virtual int GetInt()const{return 0;}
	virtual const gp_Trsf &GetTrsf()const{ return *((const gp_Trsf*)NULL); }
	virtual void GetList(std::list< Property* > &list)const{}
};

class PropertyCheck :public Property{
protected:
	bool* m_pvar;
public:
	PropertyCheck(HeeksObj* object) :Property(object, NULL), m_pvar(NULL){}
	PropertyCheck(HeeksObj* object, const wxChar* title, bool* pvar) :Property(object, title), m_pvar(pvar){ }
	PropertyCheck(HeeksObj* object, const wxChar* title, const bool* pvar) :Property(object, title), m_pvar((bool*)pvar){ m_editable = false; }
	// Property's virtual functions
	int get_property_type(){ return CheckPropertyType; }
	Property *MakeACopy(void)const;
	void Set(bool value){ *m_pvar = value; if(m_object)m_object->OnApplyProperties(); }
	bool GetBool(void)const{ return *m_pvar; }
};

class PropertyCheckWithConfig : public PropertyCheck
{
	const wxChar* m_config_name;
public:
	PropertyCheckWithConfig(HeeksObj* object, const wxChar* title, bool* pvar, const wxChar* config_name) :PropertyCheck(object, title, pvar), m_config_name(config_name){}
	void Set(bool value);
	Property *MakeACopy(void)const;
};

class PropertyChoice :public Property{
protected:
	int* m_pvar;
	std::list< wxString > m_choices;
public:
	PropertyChoice(HeeksObj* object, const wxChar* title, const std::list< wxString > &choices, int* pvar) :Property(object, title), m_choices(choices), m_pvar(pvar){}
	// Property's virtual functions
	int get_property_type(){ return ChoicePropertyType; }
	Property *MakeACopy(void)const;
	void Set(int value){ *m_pvar = value; }
	int GetInt()const{ return *m_pvar; }
	void GetChoices(std::list< wxString > &choices){ choices = m_choices; }
};

class PropertyColor :public Property{
protected:
	HeeksColor* m_pvar;
public:
	PropertyColor(HeeksObj* object) :Property(object, NULL){}
	PropertyColor(HeeksObj* object, const wxChar* title, HeeksColor* pvar) :Property(object, title), m_pvar(pvar){ }
	// Property's virtual functions
	int get_property_type(){ return ColorPropertyType; }
	Property *MakeACopy(void)const;
	void Set(const HeeksColor& value){ *m_pvar = value; if (m_object)m_object->OnApplyProperties(); }
	const HeeksColor &GetColor()const{ return *m_pvar; }
};

class PropertyDouble :public Property{
protected:
	double* m_pvar;
public:
	PropertyDouble(HeeksObj* object) :Property(object, NULL), m_pvar(NULL){}
	PropertyDouble(HeeksObj* object, const wxChar* title, double* pvar) :Property(object, title), m_pvar(pvar){ }
	PropertyDouble(HeeksObj* object, const wxChar* title, const double* pvar) :Property(object, title), m_pvar((double*)pvar){ m_editable = false; }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	Property *MakeACopy(void)const;
	void Set(double value){ *m_pvar = value; if (m_object)m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return *m_pvar; }
};

class PropertyDoubleScaled :public Property{
	double* m_pvar;
	double m_scale;
public:
	PropertyDoubleScaled(HeeksObj* object, const wxChar* title, double* pvar, double scale) :Property(object, title), m_pvar(pvar), m_scale(scale){ }
	PropertyDoubleScaled(HeeksObj* object, const wxChar* title, const double* pvar, double scale) :Property(object, title), m_pvar((double*)pvar), m_scale(scale){ m_editable = false; }
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	Property *MakeACopy(void)const{ return new PropertyDoubleScaled(*this); }
	void Set(double value){ *m_pvar = value / m_scale; if (m_object)m_object->OnApplyProperties(); }
	double GetDouble(void)const{ return *m_pvar * m_scale; }
};

class PropertyLengthScaled :public PropertyDoubleScaled{
public:
	PropertyLengthScaled(HeeksObj* object, const wxChar* title, double* pvar, double scale) :PropertyDoubleScaled(object, title, pvar, scale){}
	PropertyLengthScaled(HeeksObj* object, const wxChar* title, const double* pvar, double scale) :PropertyDoubleScaled(object, title, pvar, scale){}
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	Property *MakeACopy(void)const{	return new PropertyLengthScaled(*this);	}
};

class PropertyDoubleLimited :public PropertyDouble{
	bool m_l, m_u;
	double m_upper, m_lower;
public:
	PropertyDoubleLimited(HeeksObj* object, const wxChar* title, double* pvar, bool l, double lower, bool u, double upper) :PropertyDouble(object, title, pvar), m_l(l), m_u(u), m_lower(lower), m_upper(upper){}
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	Property *MakeACopy(void)const;
	void Set(double value){ if (m_l && value < m_lower)value = m_lower; if (m_u && value > m_upper)value = m_upper; PropertyDouble::Set(value); }
};

class PropertyString :public Property{
	wxString *m_pvar;
public:
	PropertyString(HeeksObj* object, const wxChar* title, wxString* pvar) :Property(object, title), m_pvar(pvar){}
	// Property's virtual functions
	int get_property_type(){ return StringPropertyType; }
	Property *MakeACopy(void)const;
	void Set(const wxChar* value){ *m_pvar = value; }
	const wxChar* GetString()const{ return m_pvar->c_str(); }
};

class PropertyStringReadOnly :public Property{
	wxString m_value;
public:
	PropertyStringReadOnly(const wxChar* title, const wxChar* value) :Property(NULL, title), m_value(value){ m_editable = false; }
	// Property's virtual functions
	int get_property_type(){ return StringPropertyType; }
	Property *MakeACopy(void)const;
	const wxChar* GetString()const{ return m_value.c_str(); }
};

class PropertyStringWithConfig : public PropertyString
{
	const wxChar* m_config_name;
public:
	PropertyStringWithConfig(HeeksObj* object, const wxChar* title, wxString* pvar, const wxChar* config_name) :PropertyString(object, title, pvar), m_config_name(config_name){}
	void Set(const wxChar* value);
	Property *MakeACopy(void)const;
};

class PropertyFile :public PropertyString{
public:
	PropertyFile(HeeksObj* object, const wxChar* title, wxString* pvar) :PropertyString(object, title, pvar){}
	// Property's virtual functions
	int get_property_type(){ return FilePropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyInt :public Property{
protected:
	int* m_pvar;
public:
	PropertyInt(HeeksObj* object) :Property(object, NULL){}
	PropertyInt(HeeksObj* object, const wxChar* title, int* pvar) :Property(object, title), m_pvar(pvar){ }
	PropertyInt(HeeksObj* object, const wxChar* title, const int* pvar) :Property(object, title), m_pvar((int*)pvar){ m_editable = false; }
	// Property's virtual functions
	int get_property_type(){ return IntPropertyType; }
	Property *MakeACopy(void)const;
	void Set(int value){ *m_pvar = value; if (m_object)m_object->OnApplyProperties(); }
	int GetInt(void)const{ return *m_pvar; }
};

class PropertyLength :public PropertyDouble{
public:
	PropertyLength(HeeksObj* object) :PropertyDouble(object){}

	PropertyLength(HeeksObj* object, const wxChar* title, double* pvar) :PropertyDouble(object, title, pvar){}
	PropertyLength(HeeksObj* object, const wxChar* title, const double* pvar) :PropertyDouble(object, title, pvar){}
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyLengthWithConfig : public PropertyLength
{
	const wxChar* m_config_name;
public:
	PropertyLengthWithConfig(HeeksObj* object, const wxChar* title, double* pvar, const wxChar* config_name) :PropertyLength(object, title, pvar), m_config_name(config_name){}
	void Set(double value);
	Property *MakeACopy(void)const;
};


class PropertyLengthWithKillGLLists : public PropertyLength
{
public:
	PropertyLengthWithKillGLLists(HeeksObj* object, const wxChar* title, double* pvar) :PropertyLength(object, title, pvar){}
	void Set(double value){ PropertyLength::Set(value); m_object->KillGLLists(); }
	Property *MakeACopy(void)const{ return new PropertyLengthWithKillGLLists(*this); }
};

class PropertyList :public Property{
public:
	std::list< Property* > m_list;
	PropertyList(const wxChar* title) :Property(NULL, title){}
	// Property's virtual functions
	int get_property_type(){ return ListOfPropertyType; }
	Property *MakeACopy(void)const;
	void GetList(std::list< Property* > &list)const{ list = m_list; }
};


PropertyList* PropertyVertex(HeeksObj* object, const wxChar* title, double* p);
PropertyList* PropertyVertex(HeeksObj* object, const wxChar* title, const double* p);
PropertyList* PropertyPnt(HeeksObj* object, const wxChar* title, gp_Pnt* pnt);
void PropertyPnt(std::list<Property *> *list, HeeksObj* object, gp_Pnt* pnt);
PropertyList* PropertyDir(HeeksObj* object, const wxChar* title, gp_Dir* dir);
void PropertyDir(std::list<Property *> *list, HeeksObj* object, gp_Pnt* pnt);
PropertyList* PropertyAxisLoc(HeeksObj* object, const wxChar* title, gp_Ax1* a);
PropertyList* PropertyAxisDir(HeeksObj* object, const wxChar* title, gp_Ax1* a);
PropertyList* PropertyTrsf(HeeksObj* object, const wxChar* title, gp_Trsf* trsf);


#endif
