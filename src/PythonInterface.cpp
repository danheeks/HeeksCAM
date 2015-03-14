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
};

wxMenu *currentMenu = NULL;


std::vector<PyObject *> python_menu_callbacks;

void CadMessageBox(std::wstring str)
{
	wxMessageBox(str.c_str());
}

void AddMenu(std::wstring str)
{
	wxMenu *newMenu = new wxMenu;
	currentMenu = newMenu;
	wxGetApp().m_frame->GetMenuBar()->Append(newMenu, str.c_str());
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

void OnMenuItem(wxCommandEvent &event)
{
	std::map<int, PyObject*>::iterator FindIt = menu_item_map.find(event.GetId());
	if (FindIt != menu_item_map.end())
	{
		// Execute the python function
		PyObject* python_callback = FindIt->second;
		PyObject* result = PyObject_CallFunction(python_callback, 0);

		if (PyErr_Occurred())
			MessageBoxPythonError();

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
		PyObject_CallFunction(OnSelectionChanged, 0);
		if (PyErr_Occurred())
			MessageBoxPythonError();
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

class PythonCadObject
{
protected:
	HeeksObj* m_object;

public:
	PythonCadObject() :m_object(NULL){}
	PythonCadObject(HeeksObj* object) :m_object(object){}

	HeeksObj* GetObject(){return m_object;}

	virtual int GetType()const{ return OBJECT_TYPE_UNKNOWN; }
};

class PythonCadFace : public PythonCadObject
{
public:
	PythonCadFace(){}
	PythonCadFace(HeeksObj* object) : PythonCadObject(object){}

	// Object's virtual functions
	int GetType()const{ return OBJECT_TYPE_FACE; }
};

class PythonCadEdge : public PythonCadObject
{
public:
	PythonCadEdge(){}
	PythonCadEdge(HeeksObj* object) : PythonCadObject(object){}

	// Object's virtual functions
	int GetType()const{ return OBJECT_TYPE_EDGE; }
};

class PythonCadSketch : public PythonCadObject
{
public:
	PythonCadSketch(){}
	PythonCadSketch(HeeksObj* object) : PythonCadObject(object){}

	// Object's virtual functions
	int GetType()const{ return OBJECT_TYPE_SKETCH; }
};

bp::tuple SketchGetStartPoint(PythonCadSketch &sketch)
{
	double s[3] = { 0.0, 0.0, 0.0 };

	CSketch* sketch_obj = (CSketch*)sketch.GetObject();

	HeeksObj* last_child = NULL;
	HeeksObj* child = sketch_obj->GetFirstChild();
	child->GetStartPoint(s);
	return bp::make_tuple(s[0], s[1], s[2]);
}

bp::tuple SketchGetEndPoint(PythonCadSketch &sketch)
{
	double e[3] = { 0.0, 0.0, 0.0 };

	CSketch* sketch_obj = (CSketch*)sketch.GetObject();

	HeeksObj* last_child = NULL;
	for (HeeksObj* child = sketch_obj->GetFirstChild(); child; child = sketch_obj->GetNextChild())
	{
		child->GetEndPoint(e);
	}
	return bp::make_tuple(e[0], e[1], e[2]);
}

class PythonCadSolid : public PythonCadObject
{
public:
	PythonCadSolid(){}
	PythonCadSolid(HeeksObj* object) : PythonCadObject(object){}

	// Object's virtual functions
	int GetType()const{ return OBJECT_TYPE_SOLID; }

	void WriteSTL(double tolerance, std::wstring filepath)
	{
		std::list<HeeksObj*> list;
		list.push_back(m_object);
		wxGetApp().SaveSTLFileAscii(list, filepath.c_str(), tolerance);
	}
};

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
		return OBJECT_TYPE_SOLID;
	case SketchType:
		return OBJECT_TYPE_SKETCH;
	default:
		return OBJECT_TYPE_UNKNOWN;
	}
}

boost::python::list GetSelectedObjects() {
	boost::python::list slist;
	for (std::list<HeeksObj *>::iterator It = wxGetApp().m_marked_list->list().begin(); It != wxGetApp().m_marked_list->list().end(); It++)
	{
		HeeksObj* object = *It;
		switch (object->GetType())
		{
		case SolidType:
			slist.append(PythonCadSolid(object));
			break;
		case FaceType:
			slist.append(PythonCadFace(object));
			break;
		case EdgeType:
			slist.append(PythonCadEdge(object));
			break;
		case SketchType:
			slist.append(PythonCadSketch(object));
			break;
		default:
			slist.append(PythonCadObject(object));
			break;
		}
	}
	return slist;
}

BOOST_PYTHON_MODULE(cad) {
	/// class Object
	/// interface to a solid
	bp::class_<PythonCadObject>("Object")
		.def(bp::init<PythonCadObject>())
		.def("GetType", &PythonCadObject::GetType) ///function GetType///return Object///returns the object's type
		;

	bp::class_<PythonCadFace, bp::bases<PythonCadObject>>("Face")
		.def(bp::init<PythonCadFace>())
		;
	bp::class_<PythonCadEdge, bp::bases<PythonCadObject>>("Edge")
		.def(bp::init<PythonCadEdge>())
		;
	bp::class_<PythonCadSketch, bp::bases<PythonCadObject>>("Sketch")
		.def(bp::init<PythonCadSketch>())
		.def("GetStartPoint", &SketchGetStartPoint)
		.def("GetEndPoint", &SketchGetEndPoint)
		;
	bp::class_<PythonCadSolid, bp::bases<PythonCadObject>>("Solid")
		.def(bp::init<PythonCadSolid>())
		.def("WriteSTL", &PythonCadSolid::WriteSTL) ///function WriteSTL///params float tolerance, string filepath///writes an STL file for the body to the given tolerance
		;

	bp::def("AddMenu", AddMenu);///function AddMenu///params str title///adds a menu to the CAD software
	bp::def("AddMenuItem", AddMenuItem);///function AddMenuItem///params str title, function callback///adds a menu item to the last added menu
	bp::def("MessageBox", CadMessageBox);
	bp::def("RegisterOnSelectionChanged", RegisterOnSelectionChanged);
	bp::def("GetSelectedObjects", GetSelectedObjects);

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
	Py_Initialize();
	PyEval_InitThreads();

	PyObject *main_module = PyImport_ImportModule("__main__");
	PyObject *globals = PyModule_GetDict(main_module);
	PyRun_String("import OnStart", Py_file_input, globals, globals);

	if (PyErr_Occurred())
		MessageBoxPythonError();
}
