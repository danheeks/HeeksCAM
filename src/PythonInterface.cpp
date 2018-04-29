// HeeksPython.cpp
#include "stdafx.h"

#ifdef WIN32
#include "windows.h"
#endif

#include "PythonInterface.h"
#include "HeeksObj.h"
#include "ToolImage.h"
#include "HeeksConfig.h"
#include "Observer.h"
#include "HCircle.h"
#include "HPoint.h"
#include "NCCode.h"
#include "OutputCanvas.h"
#include "HArc.h"
#include "StlSolid.h"
#include "Property.h"
#include <set>

#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

class Property;

#include <boost/progress.hpp>
#include <boost/timer.hpp>
#include <boost/foreach.hpp>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/wrapper.hpp>
#include <boost/python/call.hpp>


namespace bp = boost::python;



template <class Base = bp::default_call_policies>
struct incref_return_value_policy : Base
{
	static PyObject *postcall(PyObject *args, PyObject *result)
	{
		PyObject *self = PyTuple_GET_ITEM(args, 0);
		Py_INCREF(self);
		return result;
	}
};


enum
{
	OBJECT_TYPE_UNKNOWN = 0,
	OBJECT_TYPE_BODY,
	OBJECT_TYPE_SOLID,
	OBJECT_TYPE_SHEET,
	OBJECT_TYPE_SKETCH,
	OBJECT_TYPE_SKETCH_CONTOUR,
	OBJECT_TYPE_SKETCH_SEG,
	OBJECT_TYPE_SKETCH_LINE,
	OBJECT_TYPE_SKETCH_ARC,
	OBJECT_TYPE_BODY_CHILD,
	OBJECT_TYPE_FACE,
	OBJECT_TYPE_EDGE,
	OBJECT_TYPE_VERTEX,
	OBJECT_TYPE_CIRCLE,
};

wxMenu *currentMenu = NULL;


std::vector<PyObject *> python_menu_callbacks;

PyObject *main_module = NULL;
PyObject *globals = NULL;

void CadMessageBox(std::wstring str)
{
	wxMessageBox(str.c_str());
}

void AddMenu(std::wstring str)
{
	wxMenu *newMenu = new wxMenu;
	currentMenu = newMenu;
	wxGetApp().m_frame->GetMenuBar()->Insert(wxGetApp().m_frame->GetMenuBar()->GetMenuCount() - 1, newMenu, str.c_str());
}

std::map<int, wxWindow*> window_id_map;

void OnWindow(wxCommandEvent& event)
{
	int id = event.GetId();
	std::map<int, wxWindow*>::iterator FindIt = window_id_map.find(id);
	if (FindIt != window_id_map.end())
	{
		wxWindow* window = FindIt->second;
		wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
		wxAuiPaneInfo& pane_info = aui_manager->GetPane(window);
		if (pane_info.IsOk()){
			pane_info.Show(event.IsChecked());
			aui_manager->Update();
		}
	}
}

void OnUpdateWindow(wxUpdateUIEvent& event)
{
	int id = event.GetId();
	std::map<int, wxWindow*>::iterator FindIt = window_id_map.find(id);
	if (FindIt != window_id_map.end())
	{
		wxWindow* window = FindIt->second;
		wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
		event.Check(aui_manager->GetPane(window).IsShown());
	}
}

std::map<HWND, wxWindow*> hwnd_map;

void AddHideableWindow(std::wstring str, int hwnd)
{
	std::map<HWND, wxWindow*>::iterator FindIt = hwnd_map.find((HWND)hwnd);
	if (FindIt == hwnd_map.end())
	{
		wxWindow* window = new wxWindow;
		window->SetHWND((HWND)hwnd);
		window->Reparent(wxGetApp().m_frame);
		wxGetApp().m_frame->m_aui_manager->AddPane(window, wxAuiPaneInfo().Name(str.c_str()).Caption(str.c_str()).Left().Layer(0).BestSize(wxSize(300, 400)));
		wxGetApp().RegisterHideableWindow(window);
		hwnd_map.insert(std::make_pair((HWND)hwnd, window));

		// add tick box on the window menu
		wxMenu* window_menu = wxGetApp().m_frame->m_menuWindow;
		window_menu->AppendSeparator();
		int id = wxGetApp().m_frame->AddMenuItem(window_menu, str, wxBitmap(), OnWindow, OnUpdateWindow, NULL, true);
		window_id_map.insert(std::make_pair(id, window));
		wxGetApp().m_frame->m_aui_manager->Update();
	}
}

void ShowHideableWindow(int hwnd, int show_not_hide)
{
	// show one of the added external windows
	std::map<HWND, wxWindow*>::iterator FindIt = hwnd_map.find((HWND)hwnd);
	if (FindIt != hwnd_map.end())
	{
		wxWindow* window = FindIt->second;
		wxGetApp().m_frame->m_aui_manager->GetPane(window).Show();
		wxGetApp().m_frame->m_aui_manager->Update();
	}
}

void AttachWindowToWindow(int hwnd)
{
	::SetParent((HWND)hwnd, wxGetApp().m_frame->GetHWND());
}

int GetWindowHandle()
{
	return (int)(wxGetApp().m_frame->GetHWND());
}

void CadImport(std::wstring fp)
{
	wxGetApp().OpenFile(fp.c_str(), true);
}

void AppendAboutString(std::wstring str)
{
	wxGetApp().m_frame->m_extra_about_box_str.Append(str);
}

static std::list<PyObject*> new_or_open_callbacks;

void RegisterNewOrOpen(PyObject *callback)
{
	new_or_open_callbacks.push_back(callback);
}

void MessageBoxPythonError()
{
	std::wstring error_string;

	PyObject *errtype, *errvalue, *traceback;
	PyErr_Fetch(&errtype, &errvalue, &traceback);
	PyErr_NormalizeException(&errtype, &errvalue, &traceback);
	if (errtype != NULL && 0)/* error type not as useful, less is more */ {
		PyObject *s = PyObject_Str(errtype);
		PyObject* pStrObj = PyUnicode_AsUTF8String(s);
		char* c = PyBytes_AsString(pStrObj);
		if (c)
		{
			wchar_t wstr[1024];
			mbstowcs(wstr, c, 1024);
			error_string.append(wstr);
			error_string.append(L"\n\n");
		}
		Py_DECREF(s);
		Py_DECREF(pStrObj);
	}
	if (errvalue != NULL) {
		PyObject *s = PyObject_Str(errvalue);
		PyObject* pStrObj = PyUnicode_AsUTF8String(s);
		char* c = PyBytes_AsString(pStrObj);
		if (c)
		{
			wchar_t wstr[1024];
			mbstowcs(wstr, c, 1024);
			error_string.append(wstr);
			error_string.append(L"\n\n\n");
		}
		Py_DECREF(s);
		Py_DECREF(pStrObj);
	}

	PyObject *pModule = PyImport_ImportModule("traceback");

	if (traceback != NULL && pModule != NULL)
	{
		PyObject* pDict = PyModule_GetDict(pModule);
		PyObject* pFunc = PyDict_GetItemString(pDict, "format_tb");
		if (pFunc && PyCallable_Check(pFunc))
		{
			PyObject* pArgs = PyTuple_New(1);
			pArgs = PyTuple_New(1);
			PyTuple_SetItem(pArgs, 0, traceback);
			PyObject* pValue = PyObject_CallObject(pFunc, pArgs);
			if (pValue != NULL)
			{
				int len = PyList_Size(pValue);
				if (len > 0) {
					PyObject *t, *tt;
					int i;
					char *buffer;
					for (i = 0; i < len; i++) {
						tt = PyList_GetItem(pValue, i);
						t = Py_BuildValue("(O)", tt);
						if (!PyArg_ParseTuple(t, "s", &buffer)){
							return;
						}

						wchar_t wstr[1024];
						mbstowcs(wstr, buffer, 1024);
						error_string.append(wstr);
						error_string.append(L"\n");
					}
				}
			}
			Py_DECREF(pValue);
			Py_DECREF(pArgs);
		}
	}
	Py_DECREF(pModule);

	Py_XDECREF(errvalue);
	Py_XDECREF(errtype);
	Py_XDECREF(traceback);

	wxMessageBox(error_string.c_str());
}

std::map<int, PyObject*> menu_item_map;

static void BeforePythonCall(PyObject **main_module, PyObject **globals)
{
	if (*main_module == NULL)
	{
		*main_module = PyImport_ImportModule("__main__");
		*globals = PyModule_GetDict(*main_module);
	}

	wxString stdOutErr(_T("import sys\nclass CatchOutErr:\n  def __init__(self):\n    self.value = ''\n  def write(self, txt):\n    self.value += txt\ncatchOutErr = CatchOutErr()\nsys.stdout = catchOutErr")); //this is python code to redirect stdouts/stderr
	PyRun_String(stdOutErr.utf8_str(), Py_file_input, *globals, *globals); //invoke code to redirect

	if (PyErr_Occurred())
		MessageBoxPythonError();
}

static void AfterPythonCall(PyObject *main_module)
{
	PyObject *catcher = PyObject_GetAttrString(main_module, "catchOutErr"); //get our catchOutErr created above
	PyObject *output = PyObject_GetAttrString(catcher, "value"); //get the stdout and stderr from our catchOutErr object
	wxString s(PyUnicode_AsUTF8(output), wxConvUTF8);
	if (s.Length() > 0) {
		wxGetApp().m_print_canvas->m_textCtrl->AppendText(s);
	}

	if (PyErr_Occurred())
		MessageBoxPythonError();
}

void OnMenuItem(wxCommandEvent &event)
{
	std::map<int, PyObject*>::iterator FindIt = menu_item_map.find(event.GetId());
	if (FindIt != menu_item_map.end())
	{
		//PyObject *main_module, *globals;
		BeforePythonCall(&main_module, &globals);

		// Execute the python function
		PyObject* python_callback = FindIt->second;
		PyObject* result = PyObject_CallFunction(python_callback, 0);

		AfterPythonCall(main_module);

		// Release the python objects we still have
		if (result)Py_DECREF(result);
		else PyErr_Print();
	}
}

void AddMenuItem(std::wstring str, PyObject *callback)
{
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return;
	}

	if (currentMenu)
	{
		int id = wxGetApp().m_frame->AddMenuItem(currentMenu, str.c_str(), wxBitmap(), OnMenuItem);
		menu_item_map.insert(std::make_pair(id, callback));
	}
}


PyObject* OnSelectionChanged = NULL;

void CallOnSelectionChanged()
{
	if (OnSelectionChanged)
	{
		//PyObject *main_module, *globals;
		BeforePythonCall(&main_module, &globals);

		PyObject_CallFunction(OnSelectionChanged, 0);

		AfterPythonCall(main_module);
	}
}

void RegisterOnSelectionChanged(PyObject *callback)
{
	if (!PyCallable_Check(callback)) {
		PyErr_SetString(PyExc_TypeError, "parameter must be callable");
		return;
	}

	OnSelectionChanged = callback;
}

bp::tuple SketchGetStartPoint(CSketch &sketch)
{
	double s[3] = { 0.0, 0.0, 0.0 };

	HeeksObj* last_child = NULL;
	HeeksObj* child = sketch.GetFirstChild();
	child->GetStartPoint(s);
	return bp::make_tuple(s[0], s[1], s[2]);
}

bp::tuple SketchGetEndPoint(CSketch &sketch)
{
	double e[3] = { 0.0, 0.0, 0.0 };

	HeeksObj* last_child = NULL;
	for (HeeksObj* child = sketch.GetFirstChild(); child; child = sketch.GetNextChild())
	{
		child->GetEndPoint(e);
	}
	return bp::make_tuple(e[0], e[1], e[2]);
}

void SolidWriteSTL(CSolid& solid, double tolerance, std::wstring filepath)
{
	std::list<HeeksObj*> list;
	list.push_back(&solid);
	wxGetApp().SaveSTLFileAscii(list, filepath.c_str(), tolerance);
}

void StlSolidWriteSTL(CStlSolid& solid, double tolerance, std::wstring filepath)
{
	std::list<HeeksObj*> list;
	list.push_back(&solid);
	wxGetApp().SaveSTLFileAscii(list, filepath.c_str(), tolerance);
}

int HeeksTypeToObjectType(long type)
{
	switch (type)
	{
	case EdgeType:
		return OBJECT_TYPE_EDGE;
	case FaceType:
		return OBJECT_TYPE_FACE;
	case VertexType:
		return OBJECT_TYPE_VERTEX;
	case SolidType:
	case StlSolidType:
		return OBJECT_TYPE_SOLID;
	case SketchType:
		return OBJECT_TYPE_SKETCH;
	default:
		return OBJECT_TYPE_UNKNOWN;
	}
}

void AddObjectToPythonList(HeeksObj* object, boost::python::list& list)
{
	switch (object->GetType())
	{
	case SketchType:
		list.append(boost::python::pointer_wrapper<CSketch*>((CSketch*)object));
		break;
	case SolidType:
		list.append(boost::python::pointer_wrapper<CSolid*>((CSolid*)object));
		break;
	case StlSolidType:
		list.append(boost::python::pointer_wrapper<CStlSolid*>((CStlSolid*)object));
		break;
	case CircleType:
		list.append(boost::python::pointer_wrapper<HCircle*>((HCircle*)object));
		break;
	default:
		list.append(boost::python::pointer_wrapper<HeeksObj*>((HeeksObj*)object));
		break;
	}
}

boost::python::list GetSelectedObjects() {
	boost::python::list slist;
	for (std::list<HeeksObj *>::iterator It = wxGetApp().m_marked_list->list().begin(); It != wxGetApp().m_marked_list->list().end(); It++)
	{
		HeeksObj* object = *It;
		//slist.append(object);
		//slist.append(boost::python::pointer_wrapper<HeeksObj*>((HeeksObj*)object));
		AddObjectToPythonList(object, slist);
	}
	return slist;
}

boost::python::list GetObjects() {
	boost::python::list olist;
	for (HeeksObj *object = wxGetApp().GetFirstChild(); object; object = wxGetApp().GetNextChild())
	{
		olist.append(boost::python::pointer_wrapper<HeeksObj*>(object));
		AddObjectToPythonList(object, olist);
	}
	return olist;
}

int GetTypeFromHeeksObj(const HeeksObj* object)
{
	switch (object->GetType())
	{
	case SketchType:
		return (int)OBJECT_TYPE_SKETCH;
	case CircleType:
		return (int)OBJECT_TYPE_CIRCLE;
	case SolidType:
	case StlSolidType:
		return (int)OBJECT_TYPE_SOLID;
	default:
		return object->GetType();
	}
}

std::wstring GetTitleFromHeeksObj(const HeeksObj* object)
{
	return std::wstring(object->GetShortString());
}

boost::python::list SketchSplit(CSketch& sketch) {
	boost::python::list olist;
	std::list<HeeksObj*> new_separate_sketches;
	sketch.ExtractSeparateSketches(new_separate_sketches, false);
	for (std::list<HeeksObj*>::iterator It = new_separate_sketches.begin(); It != new_separate_sketches.end(); It++)
	{
		HeeksObj* object = *It;
		AddObjectToPythonList(object, olist);
	}
	return olist;
}

double SketchGetCircleDiameter(CSketch& sketch)
{
	HeeksObj* span = sketch.GetFirstChild();
	if (span == NULL)
		return 0.0;

	if (span->GetType() == ArcType)
	{
		HArc* arc = (HArc*)span;
		return arc->m_radius * 2;
	}
	else if (span->GetType() == CircleType)
	{
		HCircle* circle = (HCircle*)span;
		return circle->m_radius * 2;
	}
	return 0.0;
}

bp::tuple SketchGetCircleCentre(CSketch& sketch)
{
	HeeksObj* span = sketch.GetFirstChild();
	if (span == NULL)
		return bp::make_tuple(NULL);

	if (span->GetType() == ArcType)
	{
		HArc* arc = (HArc*)span;
		gp_Pnt& C = arc->C;
		return bp::make_tuple(C.X(), C.Y(), C.Z());
	}
	else if (span->GetType() == CircleType)
	{
		HCircle* circle = (HCircle*)span;
		const gp_Pnt& C = circle->m_axis.Location();
		return bp::make_tuple(C.X(), C.Y(), C.Z());
	}

	return bp::make_tuple(NULL);
}

void SketchWriteDXF(CSketch& sketch, std::wstring filepath)
{
	std::list<HeeksObj*> objects;
	objects.push_back(&sketch);
	wxGetApp().SaveDXFFile(objects, filepath.c_str());
}

void ObjectAddObject(HeeksObj* parent, HeeksObj* object)
{
	wxGetApp().AddUndoably(object, parent, NULL);
}

void CadAddObject(HeeksObj* object)
{
	wxGetApp().AddUndoably(object, NULL, NULL);
}

void PyIncRef(PyObject* object)
{
	Py_INCREF(object);
}

HeeksObj* NewPoint(double x, double y, double z)
{
	HeeksObj* new_object = new HPoint(gp_Pnt(x, y, z), &wxGetApp().current_color);
	return new_object;
}

bp::tuple CircleGetCentre(HCircle &circle)
{
	double c[3] = { 0.0, 0.0, 0.0 };
	if (circle.GetCentrePoint(c))
	{
		return bp::make_tuple(c[0], c[1], c[2]);
	}
	else
	{
		return bp::make_tuple(NULL);
	}
}

int GetId(HeeksObj* object)
{
	return object->GetID();
}

class HBitmap : public wxBitmap
{
public:
	HBitmap() :wxBitmap(0, 0){}
	HBitmap(const wxBitmap& b) :wxBitmap(b){}
	HBitmap(const std::wstring &str) :wxBitmap(wxImage(wxGetApp().GetResFolder() + str.c_str())){}
};

std::wstring str_for_base_object;
HBitmap hbitmap_for_base_object;
HeeksColor color_for_base_object;

std::list<Property *> *property_list = NULL;




/*
This RAII structure ensures that threads created on the native C side
adhere to the laws of Python and ensure they grab the GIL lock when
calling into python
*/
struct PyLockGIL
{

	PyLockGIL()
		: gstate(PyGILState_Ensure())
	{
	}

	~PyLockGIL()
	{
		PyGILState_Release(gstate);
	}

	PyLockGIL(const PyLockGIL&) = delete;
	PyLockGIL& operator=(const PyLockGIL&) = delete;

	PyGILState_STATE gstate;
};




bp::detail::method_result Call_Override(bp::override &f)
{
	//PyObject *main_module, *globals;
	BeforePythonCall(&main_module, &globals);

	// Execute the python function
	PyLockGIL lock;
	try
	{
		return f();
	}
	catch (const bp::error_already_set&)
	{
		AfterPythonCall(main_module);
	}
}


bp::detail::method_result Call_Override(bp::override &f, int value)
{
	//PyObject *main_module, *globals;
	BeforePythonCall(&main_module, &globals);

	// Execute the python function
	PyLockGIL lock;
	try
	{
		return f(value);
	}
	catch (const bp::error_already_set&)
	{
		AfterPythonCall(main_module);
	}

}


bp::detail::method_result Call_Override(bp::override &f, double value)
{
	//PyObject *main_module, *globals;
	BeforePythonCall(&main_module, &globals);

	// Execute the python function
	PyLockGIL lock;
	try
	{
		return f(value);
	}
	catch (const bp::error_already_set&)
	{
		AfterPythonCall(main_module);
	}

}




class BaseObject : public HeeksObj, public bp::wrapper<HeeksObj>
{
public:
	bool m_uses_display_list;
	int m_display_list;

	BaseObject() :HeeksObj(), m_uses_display_list(false), m_display_list(0){}
	int GetType()const override
	{
		if (bp::override f = this->get_override("GetType"))
		{
			int t = f();
			return t;
		}
		return HeeksObj::GetType();
	}

	const wxBitmap &GetIcon() override
	{
		if (bp::override f = this->get_override("GetIcon"))
		{
			hbitmap_for_base_object = HBitmap(f());
			return hbitmap_for_base_object;
		}
		return HeeksObj::GetIcon();
	}

	const wxChar* GetShortString()const override
	{
		if (bp::override f = this->get_override("GetTitle"))
		{
			std::string s = f();
			str_for_base_object = Ctt(s.c_str());
			return str_for_base_object.c_str();
		}
		return HeeksObj::GetShortString();
	}

	const wxChar* GetTypeString()const override
	{
		if (bp::override f = this->get_override("GetTypeString"))
		{
			std::string s = f();
			str_for_base_object = Ctt(s.c_str());
			return str_for_base_object.c_str();
		}
		return HeeksObj::GetTypeString();
	}

	const HeeksColor* GetColor()const override
	{
		if (bp::override f = this->get_override("GetColor"))
		{
			color_for_base_object = f();
			return &color_for_base_object;
		}
		return HeeksObj::GetColor();
	}

	static bool in_glCommands;
	static bool triangles_begun;
	static bool lines_begun;

	void glCommands(bool select, bool marked, bool no_color) override
	{
		if (in_glCommands)
			return; // shouldn't be needed

		if (!select)
		{
			glEnable(GL_LIGHTING);
			if (!no_color)Material(*this->GetColor()).glMaterial(1.0);
		}

		bool display_list_started = false;
		bool do_render_commands = true;
		if (m_uses_display_list)
		{
			if (m_display_list)
			{
				glCallList(m_display_list);
				do_render_commands = false;
			}
			else{
				m_display_list = glGenLists(1);
				glNewList(m_display_list, GL_COMPILE_AND_EXECUTE);
				display_list_started = true;
			}
		}

		if (do_render_commands)
		{
			if (bp::override f = this->get_override("OnRenderTriangles"))
			{
				in_glCommands = true;

				Call_Override(f);

				if (triangles_begun)
				{
					glEnd();
					triangles_begun = false;
				}

				if (lines_begun)
				{
					glEnd();
					lines_begun = false;
				}
				in_glCommands = false;
			}
		}

		if (display_list_started)
		{
			glEndList();
		}

		if(!select)glDisable(GL_LIGHTING);

	}

	void GetProperties(std::list<Property *> *list) override
	{
		if (bp::override f = this->get_override("GetProperties"))
		{
			property_list = list;
			Property* p = Call_Override(f);
		}
	}

	void GetBox(CBox &box) override
	{
		if (bp::override f = this->get_override("GetBox"))
		{
			bp::tuple tuple = Call_Override(f);

			if (bp::len(tuple) == 6)
			{
				double xmin = bp::extract<double>(tuple[0]);
				double ymin = bp::extract<double>(tuple[1]);
				double zmin = bp::extract<double>(tuple[2]);
				double xmax = bp::extract<double>(tuple[3]);
				double ymax = bp::extract<double>(tuple[4]);
				double zmax = bp::extract<double>(tuple[5]);
				box.Insert(CBox(xmin, ymin, zmin, xmax, ymax, zmax));
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "GetBox takes exactly 6 parameters!");
			}
		}
	}

	void KillGLLists() override
	{
		if (m_uses_display_list && m_display_list)
		{
			glDeleteLists(m_display_list, 1);
			m_display_list = 0;
		}
	}
};

// static definitions
bool BaseObject::in_glCommands = false;
bool BaseObject::triangles_begun = false;
bool BaseObject::lines_begun = false;


int BaseObjectGetType(const HeeksObj& object)
{
	return GetTypeFromHeeksObj(&object);
	//return object.GetType();
}

HBitmap BaseObjectGetIcon(BaseObject& object)
{
	return object.GetIcon();
}

std::wstring BaseObjectGetTitle(const HeeksObj& object)
{
	return GetTitleFromHeeksObj(&object);
	//return object.GetShortString();
}

unsigned int BaseObjectGetID(BaseObject& object)
{
	return object.GetID();
}

void BaseObjectSetUsesGLList(BaseObject& object, bool on)
{
	object.m_uses_display_list = on;
}

HeeksColor BaseObjectGetColor(const BaseObject& object)
{
	return *(object.GetColor());
}

int PropertyGetInt(Property& property)
{
	return property.GetInt();
}

void DrawTriangle(double x0, double x1, double x2, double x3, double x4, double x5, double x6, double x7, double x8)
{
	if (!BaseObject::triangles_begun)
	{
		if (BaseObject::lines_begun)
		{
			glEnd();
			BaseObject::lines_begun = false;
		}
		glBegin(GL_TRIANGLES);
		BaseObject::triangles_begun = true;
	}

	gp_Pnt p0(x0, x1, x2);
	gp_Pnt p1(x3, x4, x5);
	gp_Pnt p2(x6, x7, x8);
	gp_Vec v1(p0, p1);
	gp_Vec v2(p0, p2);
	try
	{
		gp_Vec norm = (v1 ^ v2).Normalized();
		glNormal3d(norm.X(), norm.Y(), norm.Z());
	}
	catch (...)
	{
	}
	glVertex3d(x0, x1, x2);
	glVertex3d(x3, x4, x5);
	glVertex3d(x6, x7, x8);

}

void DrawLine(double x0, double x1, double x2, double x3, double x4, double x5)
{
	if (!BaseObject::lines_begun)
	{
		if (BaseObject::triangles_begun)
		{
			glEnd();
			BaseObject::triangles_begun = false;
		}
		glBegin(GL_LINES);
		BaseObject::lines_begun = true;
	}
	glVertex3d(x0, x1, x2);
	glVertex3d(x3, x4, x5);
}

void AddProperty(Property* property)
{
	property_list->push_back(property);
}


class PropertyWrap : public Property, public bp::wrapper<Property>
{
	int m_type;
public:
	PropertyWrap() :Property(), m_type(InvalidPropertyType){}
	PropertyWrap(int type, const std::wstring& title, HeeksObj* object) :Property(object, title.c_str()), m_type(type){ m_editable = true; }
	int get_property_type(){ return m_type; }
	int GetInt()const override
	{
		if (bp::override f = this->get_override("GetInt"))return Call_Override(f);
		return Property::GetInt();
	}
	double GetDouble()const override
	{
		if (bp::override f = this->get_override("GetFloat"))return Call_Override(f);
		return Property::GetDouble();
	}
	void Set(int value)override
	{
		if (bp::override f = this->get_override("SetInt"))Call_Override(f, value);
	}
	void Set(double value)override
	{
		if (bp::override f = this->get_override("SetFloat"))Call_Override(f, value);
	}
	Property *MakeACopy(void)const{ return new PropertyWrap(*this); }
};


int PropertyWrapGetInt(PropertyWrap& property)
{
	return property.GetInt();
}

BOOST_PYTHON_MODULE(cad) {
	bp::class_<BaseObject, boost::noncopyable >("Object")
		.def("GetType", &BaseObjectGetType)
		.def("GetIcon", &BaseObjectGetIcon)
		.def("GetTitle", &BaseObjectGetTitle)
		.def("GetID", &BaseObjectGetID)
		.def("KillGLLists", &BaseObject::KillGLLists)
		.def("SetUsesGLList", &BaseObjectSetUsesGLList)
		.def("GetColor", &BaseObjectGetColor)
		;

	bp::class_<HeeksColor>("Color")
		.def(bp::init<HeeksColor>())
		.def(bp::init<unsigned char, unsigned char, unsigned char>())
		.def_readwrite("red", &HeeksColor::red)
		.def_readwrite("green", &HeeksColor::green)
		.def_readwrite("blue", &HeeksColor::blue)
		;

	bp::class_<PropertyWrap, boost::noncopyable >("Property")
		.def(bp::init<int, std::wstring, HeeksObj*>())
		.def("GetInt", &PropertyWrapGetInt)
		.def_readwrite("editable", &PropertyWrap::m_editable)
		.def_readwrite("object", &PropertyWrap::m_object)
		;

	bp::class_<HCircle, bp::bases<HeeksObj> >("Circle")
		.def(bp::init<HCircle>())
		.def("GetDiameter", &HCircle::GetDiameter)
		.def("GetCentre", &CircleGetCentre)
		;

	bp::class_<ObjList, bp::bases<HeeksObj>>("ObjList")
		.def(bp::init<ObjList>())
		.def("Clear", &ObjList::ClearUndoably)
		;

	bp::class_<IdNamedObjList, bp::bases<ObjList>>("IdNamedObjList")
		.def(bp::init<IdNamedObjList>())
		;

	bp::class_<CSketch, bp::bases<IdNamedObjList>>("Sketch")
		.def(bp::init<CSketch>())
		.def("GetStartPoint", &SketchGetStartPoint)
		.def("GetEndPoint", &SketchGetEndPoint)
		.def("IsCircle", &CSketch::IsCircle)
		.def("IsClosed", &CSketch::IsClosed)
		.def("HasMultipleSketches", &CSketch::HasMultipleSketches)
		.def("Split", &SketchSplit)
		.def("GetCircleDiameter", &SketchGetCircleDiameter)
		.def("GetCircleCentre", &SketchGetCircleCentre)
		.def("WriteDXF", &SketchWriteDXF)
		;

	bp::class_<CShape, bp::bases<IdNamedObjList>>("Shape")
		.def(bp::init<CShape>())
		;

	bp::class_<CSolid, bp::bases<CShape>>("Solid")
		.def(bp::init<CSolid>())
		.def("WriteSTL", &SolidWriteSTL) ///function WriteSTL///params float tolerance, string filepath///writes an STL file for the body to the given tolerance
		;

	bp::class_<CStlSolid, bp::bases<HeeksObj>>("StlSolid")
		.def(bp::init<CStlSolid>())
		.def("WriteSTL", &StlSolidWriteSTL) ///function WriteSTL///params float tolerance, string filepath///writes an STL file for the body to the given tolerance
		;

	bp::class_<CNCCode, bp::bases<HeeksObj>>("NCCode")
		.def(bp::init<CNCCode>())
		;

	bp::class_<ObjList, bp::bases<HeeksObj>>("Patterns")
		.def(bp::init<ObjList>())
		;

	bp::class_<HBitmap>("Bitmap")
		.def(bp::init<HBitmap>())
		.def(bp::init<std::wstring>())
		;

	bp::def("AddMenu", AddMenu);///function AddMenu///params str title///adds a menu to the CAD software
	bp::def("AddMenuItem", AddMenuItem);///function AddMenuItem///params str title, function callback///adds a menu item to the last added menu
	bp::def("MessageBox", CadMessageBox);
	bp::def("RegisterOnSelectionChanged", RegisterOnSelectionChanged);
	bp::def("GetSelectedObjects", GetSelectedObjects);
	bp::def("GetObjects", GetObjects);
	bp::def("AddObject", CadAddObject);
	bp::def("PyIncRef", PyIncRef);
	bp::def("NewPoint", NewPoint, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("AddHideableWindow", AddHideableWindow, "adds a window to the CAD software");///function AddHideableWindow///params str title///adds a window to the CAD software
	bp::def("ShowHideableWindow", ShowHideableWindow);
	bp::def("AttachWindowToWindow", AttachWindowToWindow);///function AttachWindowToWindow///params int hwnd///attaches given window to the CAD window
	bp::def("GetWindowHandle", GetWindowHandle);
	bp::def("Import", CadImport);
	bp::def("AppendAboutString", AppendAboutString);
	bp::def("RegisterNewOrOpen", RegisterNewOrOpen);
	bp::def("DrawTriangle", &DrawTriangle);
	bp::def("DrawLine", &DrawLine);
	bp::def("AddProperty", AddProperty);


	bp::scope().attr("OBJECT_TYPE_UNKNOWN") = (int)OBJECT_TYPE_UNKNOWN;
	bp::scope().attr("OBJECT_TYPE_BODY") = (int)OBJECT_TYPE_BODY;
	bp::scope().attr("OBJECT_TYPE_SOLID") = (int)OBJECT_TYPE_SOLID;
	bp::scope().attr("OBJECT_TYPE_SHEET") = (int)OBJECT_TYPE_SHEET;
	bp::scope().attr("OBJECT_TYPE_SKETCH") = (int)OBJECT_TYPE_SKETCH;
	bp::scope().attr("OBJECT_TYPE_SKETCH_SEG") = (int)OBJECT_TYPE_SKETCH_SEG;
	bp::scope().attr("OBJECT_TYPE_SKETCH_LINE") = (int)OBJECT_TYPE_SKETCH_LINE;
	bp::scope().attr("OBJECT_TYPE_SKETCH_ARC") = (int)OBJECT_TYPE_SKETCH_ARC;
	bp::scope().attr("OBJECT_TYPE_BODY_CHILD") = (int)OBJECT_TYPE_BODY_CHILD;
	bp::scope().attr("OBJECT_TYPE_FACE") = (int)OBJECT_TYPE_FACE;
	bp::scope().attr("OBJECT_TYPE_EDGE") = (int)OBJECT_TYPE_EDGE;
	bp::scope().attr("OBJECT_TYPE_VERTEX") = (int)OBJECT_TYPE_VERTEX;
	bp::scope().attr("OBJECT_TYPE_CIRCLE") = (int)OBJECT_TYPE_CIRCLE;

	bp::scope().attr("PROPERTY_TYPE_INVALID") = (int)InvalidPropertyType;
	bp::scope().attr("PROPERTY_TYPE_STRING") =	(int)StringPropertyType;
	bp::scope().attr("PROPERTY_TYPE_DOUBLE") =	(int)DoublePropertyType;
	bp::scope().attr("PROPERTY_TYPE_LENGTH") =	(int)LengthPropertyType;
	bp::scope().attr("PROPERTY_TYPE_INT") =		(int)IntPropertyType;
	bp::scope().attr("PROPERTY_TYPE_CHOICE") =	(int)ChoicePropertyType;
	bp::scope().attr("PROPERTY_TYPE_COLOR") =	(int)ColorPropertyType;
	bp::scope().attr("PROPERTY_TYPE_CHECK") =	(int)CheckPropertyType;
	bp::scope().attr("PROPERTY_TYPE_LIST") =	(int)ListOfPropertyType;
	bp::scope().attr("PROPERTY_TYPE_FILE") =	(int)FilePropertyType;
}


class PythonObserver : public Observer
{
public:
	void OnChanged(const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed, const std::list<HeeksObj*>* modified)
	{
	}
	void WhenMarkedListChanges(bool selection_cleared, const std::list<HeeksObj*>* added_list, const std::list<HeeksObj*>* removed_list)
	{
		CallOnSelectionChanged();
	}
}python_observer;

void HeeksCADapp::OnPythonStartUp()
{
	wxGetApp().RegisterObserver(&python_observer);

	// start Python
	PyImport_AppendInittab("cad", &PyInit_cad);
	Py_SetPythonHome(_T("C:\\Python34(32bit)"));
	Py_Initialize();
	PyEval_InitThreads();

	main_module = PyImport_ImportModule("__main__");
	globals = PyModule_GetDict(main_module);
	PyRun_String("import OnStart", Py_file_input, globals, globals);

	if (PyErr_Occurred())
		MessageBoxPythonError();
}

void HeeksCADapp::PythonOnNewOrOpen(bool open, int res)
{
	// loop through call back functions calling them
	for (std::list<PyObject*>::iterator It = new_or_open_callbacks.begin(); It != new_or_open_callbacks.end(); It++)
	{
		PyObject* python_callback = *It;

		//PyObject *main_module, *globals;
		BeforePythonCall(&main_module, &globals);

		// Execute the python function
		PyObject* result = PyObject_CallFunction(python_callback, 0);

		AfterPythonCall(main_module);

		// Release the python objects we still have
		if (result)Py_DECREF(result);
		else PyErr_Print();
	}
}

static void RunPythonCommand(const char* command)
{
	//PyObject *main_module, *globals;
	BeforePythonCall(&main_module, &globals);

	PyRun_String(command, Py_file_input, globals, globals);

	AfterPythonCall(main_module);
}


static wxString GetBackplotFilePath() 
{
	// The xml file is created in the temporary folder
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
#else
	wxStandardPaths standard_paths;
#endif
	wxFileName file_str(standard_paths.GetTempDir().c_str(), _T("backplot.xml"));
	return file_str.GetFullPath();
}

void HeeksCADapp::BackplotGCode(const wxString& output_file)
{
	wxBusyCursor busy_cursor();

	if (m_machine.reader == _T("not found"))
	{
		wxMessageBox(_T("Machine reader name (defined in Program Properties) not found"));
	} // End if - then
	else
	{
		wxString output_filepath = output_file;
		//wxString output_filepath = GetOutputFileName();
		output_filepath.Replace('\\', '/');
		wxString command = wxString::Format(_T("from nc.hxml_writer import HxmlWriter\nfrom nc.%s import Parser as Parser\nparser = Parser(HxmlWriter())\nparser.Parse('%s')\ndel parser"), m_machine.reader.c_str(), output_filepath.c_str());
		RunPythonCommand(command);

		// there should now be an xml file written
		wxString xml_file_str = GetBackplotFilePath();
		{
			wxFile ofs(xml_file_str.c_str());
			if (!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + xml_file_str);
				return;
			}
		}

		xml_file_str.Replace('\\', '/');

		// read the xml file, just like paste, into the top level
		wxGetApp().OpenXMLFile(xml_file_str, &wxGetApp());
		wxGetApp().Repaint();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		wxGetApp().m_frame->Raise();
	} // End if - else
}
