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
	VertexPropertyType,
	ChoicePropertyType,
	ColorPropertyType,
	CheckPropertyType,
	ListOfPropertyType,
	TrsfPropertyType,
	FilePropertyType
};

class Property{
public:
	bool m_editable;
	bool m_highlighted;
	HeeksObj* m_object;

	Property(void) :m_editable(false), m_highlighted(false), m_object(NULL){} // default constructor for python
	Property(HeeksObj* object) :m_editable(false), m_highlighted(false), m_object(object){}
	Property(const Property& ho);
	virtual ~Property(){}

	virtual const Property& operator=(const Property &ho);

	virtual int get_property_type(){return InvalidPropertyType;}
	virtual Property *MakeACopy(void)const{ return NULL; }
	virtual const wxChar* GetShortString(void)const{ return _("Unknown"); }
	virtual void Set(bool value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void SetIndex(int value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const HeeksColor& value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(double value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const wxChar*){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(int value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual void Set(const gp_Trsf& value){} // only called by Property Changer, set Property::m_editable to enable this
	virtual bool GetBool()const{ return false; }
	virtual int GetIndex()const{ return 0; }
	virtual void GetChoices(std::list< wxString > &choices){}
	virtual HeeksColor &GetColor()const{ return *((HeeksColor*)NULL); }
	virtual double GetDouble()const{ return 0.0; }
	virtual const wxChar* GetString()const{ return NULL; }
	virtual int GetInt()const{return 0;}
	virtual gp_Trsf GetTrsf()const{ return gp_Trsf(); }
	virtual void GetList(std::list< Property* > &list)const{}
};

class PropertyCheck :public Property{

public:
	PropertyCheck(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return CheckPropertyType; }
	Property *MakeACopy(void)const;

};

class PropertyChoice :public Property{
public:
	PropertyChoice(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return ChoicePropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyColor :public Property{
public:
	PropertyColor(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return ColorPropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyDouble :public Property{
public:
	PropertyDouble(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return DoublePropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyString :public Property{
public:
	PropertyString(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return StringPropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyFile :public PropertyString{
public:
	PropertyFile(HeeksObj* object) :PropertyString(object){}
	// Property's virtual functions
	int get_property_type(){ return FilePropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyInt :public Property{
public:
	PropertyInt(){}
	PropertyInt(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return IntPropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyLength :public PropertyDouble{
public:
	PropertyLength(HeeksObj* object) :PropertyDouble(object){}
	// Property's virtual functions
	int get_property_type(){ return LengthPropertyType; }
	Property *MakeACopy(void)const;
};

class PropertyTrsf :public Property{
public:
	PropertyTrsf(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return TrsfPropertyType; }
	Property *MakeACopy(void)const;
};


class PropertyList :public Property{
public:
	PropertyList(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return ListOfPropertyType; }
	Property *MakeACopy(void)const;
};

#if 0
class PropertyVertex :public Property{
public:
	PropertyVertex(HeeksObj* object) :Property(object){}
	// Property's virtual functions
	int get_property_type(){ return VertexPropertyType; }
	Property *MakeACopy(void)const;

	virtual bool xyOnly(){ return false; }
	virtual bool AffectedByViewUnits(){ return false; }
	virtual double GetX()const{ return 0.0; }
	virtual double GetY()const{ return 0.0; }
	virtual double GetZ()const{ return 0.0; }
	virtual void SetX(double d){ }
	virtual void SetY(double d){ }
	virtual void SetZ(double d){ }
};

class PropertyVector : public PropertyVertex{
	// like a PropertyVertex, but isn't affected by view units
public:
	PropertyVector(HeeksObj* object) :PropertyVertex(object){}
	Property *MakeACopy(void)const;
};
#endif

#endif
