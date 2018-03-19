// PropertyChange.h

class PropertyChangeString: public Undoable
{
	PropertyString* m_property;
public:
	wxString m_value;
	wxString m_old;
	HeeksObj* m_object;
	void(*m_callbackfunc)(const wxChar*, HeeksObj*);

	PropertyChangeString(const wxString& value, PropertyString* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change String");}
};

class PropertyDouble;

class PropertyChangeDouble: public Undoable
{
	PropertyDouble* m_property;
public:
	double m_value;
	double m_old;
	HeeksObj* m_object;
	void(*m_callbackfunc)(double, HeeksObj*);

	PropertyChangeDouble(const double& value, PropertyDouble* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Double");}
};

class PropertyLength;

class PropertyChangeLength: public Undoable
{
	PropertyLength* m_property;
public:
	double m_value;
	double m_old;
	HeeksObj* m_object;
	void(*m_callbackfunc)(double, HeeksObj*);

	PropertyChangeLength(const double& value, PropertyLength* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Length");}
};

class PropertyInt;

class PropertyChangeInt: public Undoable
{
	PropertyInt* m_property;
public:
	int m_value;
	int m_old;

	PropertyChangeInt(const int& value, PropertyInt* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Int");}
};

class PropertyColor;

class PropertyChangeColor: public Undoable
{
	PropertyColor* m_property;
public:
	HeeksColor m_value;
	HeeksColor m_old;

	PropertyChangeColor(const HeeksColor& value, PropertyColor* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Color");}
};

#if 0
class PropertyVertex;

class PropertyChangeVertex: public Undoable
{
	PropertyVertex* m_property;
public:
	double m_value[3];
	double m_old[3];

	PropertyChangeVertex(double x, double y, double z, PropertyVertex* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Vertex");}
};
#endif

class PropertyTrsf;

class PropertyChangeTrsf: public Undoable
{
	PropertyTrsf* m_property;
public:
	gp_Trsf m_value;
	gp_Trsf m_old;

	PropertyChangeTrsf(const gp_Trsf &value, PropertyTrsf* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Trsf");}
};

class PropertyChoice;

class PropertyChangeChoice: public Undoable
{
	PropertyChoice* m_property;
public:
	int m_value;
	int m_old;

	PropertyChangeChoice(const int& value, PropertyChoice* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Choice");}
};

class PropertyCheck;

class PropertyChangeCheck: public Undoable
{
	PropertyCheck* m_property;
public:
	bool m_value;
	bool m_old;

	PropertyChangeCheck(const bool& value, PropertyCheck* property);

	void Run(bool redo);
	void RollBack();
	const wxChar* GetTitle(){return _("Property Change Check");}
};
