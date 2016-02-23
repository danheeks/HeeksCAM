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
#include "Drilling.h"
#include "Pocket.h"
#include "Profile.h"
#include "Operations.h"
#include "Program.h"
#include "HPoint.h"
#include "CTool.h"
#include "Tools.h"
#include "Surfaces.h"
#include "Stocks.h"
#include "Patterns.h"
#include "NCCode.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "HArc.h"
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

void OnWindow(wxCommandEvent& event)
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(wxGetApp().m_window);
	if (pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateWindow(wxUpdateUIEvent& event)
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	event.Check(aui_manager->GetPane(wxGetApp().m_window).IsShown());
}

void AddWindow(std::wstring str, int hwnd)
{
	if (wxGetApp().m_window == NULL)
	{
		wxGetApp().m_window = new wxWindow;
		wxGetApp().m_window->SetHWND((HWND)hwnd);
		wxGetApp().m_window->Reparent(wxGetApp().m_frame);
		//wxGetApp().m_window = new wxWindow(wxID_ANY);
		wxGetApp().m_frame->m_aui_manager->AddPane(wxGetApp().m_window, wxAuiPaneInfo().Name(str.c_str()).Caption(str.c_str()).Left().Layer(0).BestSize(wxSize(300, 400)));
		wxGetApp().RegisterHideableWindow(wxGetApp().m_window);

		// add tick box on the window menu
		wxMenu* window_menu = wxGetApp().m_frame->m_menuWindow;
		window_menu->AppendSeparator();
		wxGetApp().m_frame->AddMenuItem(window_menu, str, wxBitmap(), OnWindow, OnUpdateWindow, NULL, true);
		wxGetApp().m_frame->m_aui_manager->Update();
	}

//	return (int)(wxGetApp().m_window->GetHWND());
}

void AttachWindowToWindow(int hwnd)
{
	::SetParent((HWND)hwnd, wxGetApp().m_window->GetHWND());
}

int GetWindowHandle()
{
	return (int)(wxGetApp().m_window->GetHWND());
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
	*main_module = PyImport_ImportModule("__main__");
	*globals = PyModule_GetDict(*main_module);

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
		PyObject *main_module, *globals;
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
		PyObject *main_module, *globals;
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
	case CircleType:
		list.append(boost::python::pointer_wrapper<HCircle*>((HCircle*)object));
		break;
	}
}

boost::python::list GetSelectedObjects() {
	boost::python::list slist;
	for (std::list<HeeksObj *>::iterator It = wxGetApp().m_marked_list->list().begin(); It != wxGetApp().m_marked_list->list().end(); It++)
	{
		HeeksObj* object = *It;
		//slist.append(object);
		AddObjectToPythonList(object, slist);
	}
	return slist;
}

boost::python::list GetObjects() {
	boost::python::list olist;
	for (HeeksObj *object = wxGetApp().GetFirstChild(); object; object = wxGetApp().GetNextChild())
	{
		olist.append(object);
		AddObjectToPythonList(object, olist);
	}
	return olist;
}

int GetTypeFromHeeksObj(HeeksObj* object)
{
	switch (object->GetType())
	{
	case SketchType:
		return (int)OBJECT_TYPE_SKETCH;
	case CircleType:
		return (int)OBJECT_TYPE_CIRCLE;
	case SolidType:
		return (int)OBJECT_TYPE_SOLID;
	default:
		return 0;
	}
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

void ObjectAddObject(HeeksObj* parent, HeeksObj* object)
{
	wxGetApp().AddUndoably(object, parent, NULL);
}

void CadAddObject(HeeksObj* object)
{
	wxGetApp().AddUndoably(object, NULL, NULL);
}

HeeksObj* NewPoint(double x, double y, double z)
{
	HeeksObj* new_object = new HPoint(gp_Pnt(x, y, z), &wxGetApp().current_color);
	return new_object;
}

HeeksObj* NewDrilling()
{
	HeeksObj* new_object = new CDrilling();
	return new_object;
}

HeeksObj* NewProfile()
{
	HeeksObj* new_object = new CProfile();
	return new_object;
}

HeeksObj* NewPocket()
{
	HeeksObj* new_object = new CPocket();
	return new_object;
}

HeeksObj* NewTool()
{
	HeeksObj* new_object = new CTool();
	return new_object;
}

CProgram* GetProgram()
{
	return wxGetApp().m_program;
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

void ToolResetTitle(CTool& tool)
{
	tool.ResetTitle();
}

BOOST_PYTHON_MODULE(cad) {
	/// class Object
	/// interface to a solid
	bp::class_<HeeksObj>("Object")
		.def(bp::init<HeeksObj>())
		.def("GetType", &GetTypeFromHeeksObj) ///function GetType///return Object///returns the object's type
		.def("GetId", &GetId)
		.def("AddObject", &ObjectAddObject)
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
		;

	bp::class_<CShape, bp::bases<IdNamedObjList>>("Shape")
		.def(bp::init<CShape>())
		;

	bp::class_<CSolid, bp::bases<CShape>>("Solid")
		.def(bp::init<CSolid>())
		.def("WriteSTL", &SolidWriteSTL) ///function WriteSTL///params float tolerance, string filepath///writes an STL file for the body to the given tolerance
		;

	bp::class_<COp, bp::bases<IdNamedObjList>>("Op")
		.def(bp::init<COp>())
		.def_readwrite("comment", &COp::m_comment)
		.def_readwrite("active", &COp::m_active)
		.def_readwrite("tool_number", &COp::m_tool_number)
		.def_readwrite("pattern", &COp::m_pattern)
		.def_readwrite("surface", &COp::m_surface)
		;

	bp::class_<CProgram, bp::bases<IdNamedObjList>>("Program")
		.def(bp::init<CProgram>())
		.def("GetNCCode", &CProgram::NCCode, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetOperations", &CProgram::Operations, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetTools", &CProgram::Tools, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetPatterns", &CProgram::Patterns, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetSurfaces", &CProgram::Surfaces, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetStocks", &CProgram::Stocks, bp::return_value_policy<bp::reference_existing_object>())
		;

	bp::class_<CNCCode, bp::bases<HeeksObj>>("NCCode")
		.def(bp::init<CNCCode>())
		;

	bp::class_<COperations, bp::bases<ObjList>>("Operations")
		.def(bp::init<COperations>())
		;

	bp::class_<CTools, bp::bases<ObjList>>("Tools")
		.def(bp::init<CTools>())
		;

	bp::class_<ObjList, bp::bases<HeeksObj>>("Patterns")
		.def(bp::init<ObjList>())
		;

	bp::class_<CSurfaces, bp::bases<ObjList>>("Surfaces")
		.def(bp::init<CSurfaces>())
		;

	bp::class_<CStocks, bp::bases<ObjList>>("Stocks")
		.def(bp::init<CStocks>())
		;

	bp::class_<CTool, bp::bases<HeeksObj>>("Tool")
		.def(bp::init<CTool>())
		.def("ResetTitle", &ToolResetTitle)
		.def_readwrite("material", &CTool::m_material)
		.def_readwrite("diameter", &CTool::m_diameter)
		.def_readwrite("tool_length_offset", &CTool::m_tool_length_offset)
		.def_readwrite("corner_radius", &CTool::m_corner_radius)
		.def_readwrite("flat_radius", &CTool::m_flat_radius)
		.def_readwrite("cutting_edge_angle", &CTool::m_cutting_edge_angle)
		.def_readwrite("cutting_edge_height", &CTool::m_cutting_edge_height)
		.def_readwrite("type", &CTool::m_type)
		.def_readwrite("automatically_generate_title", &CTool::m_automatically_generate_title)
		.def_readwrite("title", &CTool::m_title)
		.def_readwrite("tool_number", &CTool::m_tool_number)
		;

	bp::class_<CSpeedOp, bp::bases<COp>>("SpeedOp")
		.def(bp::init<CSpeedOp>())
		.def_readwrite("horizontal_feed_rate", &CSpeedOp::m_horizontal_feed_rate)
		.def_readwrite("vertical_feed_rate", &CSpeedOp::m_vertical_feed_rate)
		.def_readwrite("spindle_speed", &CSpeedOp::m_spindle_speed)
		;

	bp::class_<CDepthOp, bp::bases<CSpeedOp>>("DepthOp")
		.def(bp::init<CDepthOp>())
		.def_readwrite("clearance_height", &CDepthOp::m_clearance_height)
		.def_readwrite("rapid_safety_space", &CDepthOp::m_rapid_safety_space)
		.def_readwrite("start_depth", &CDepthOp::m_start_depth)
		.def_readwrite("step_down", &CDepthOp::m_step_down)
		.def_readwrite("z_finish_depth", &CDepthOp::m_z_finish_depth)
		.def_readwrite("z_thru_depth", &CDepthOp::m_z_thru_depth)
		.def_readwrite("final_depth", &CDepthOp::m_final_depth)
		.def_readwrite("user_depths", &CDepthOp::m_user_depths)
		;

	bp::class_<CDrilling, bp::bases<CDepthOp>>("Drilling")
		.def(bp::init<CDrilling>())
		.def("AddPoint", &CDrilling::AddPoint)
		.def_readwrite("dwell", &CDrilling::m_dwell)
		.def_readwrite("retract_mode", &CDrilling::m_retract_mode)
		.def_readwrite("spindle_mode", &CDrilling::m_spindle_mode)
		.def_readwrite("internal_coolant_on", &CDrilling::m_internal_coolant_on)
		.def_readwrite("rapid_to_clearance", &CDrilling::m_rapid_to_clearance)
		;

	bp::class_<CSketchOp, bp::bases<CDepthOp>>("SketchOp")
		.def(bp::init<CSketchOp>())
		.def_readwrite("sketch", &CSketchOp::m_sketch)
		;

	bp::class_<CPocket, bp::bases<CSketchOp>>("Pocket")
		.def(bp::init<CPocket>())
		.def_readwrite("starting_place", &CPocket::m_starting_place)
		.def_readwrite("material_allowance", &CPocket::m_material_allowance)
		.def_readwrite("step_over", &CPocket::m_step_over)
		.def_readwrite("keep_tool_down_if_poss", &CPocket::m_keep_tool_down_if_poss)
		.def_readwrite("use_zig_zag", &CPocket::m_use_zig_zag)
		.def_readwrite("zig_angle", &CPocket::m_zig_angle)
		.def_readwrite("zig_unidirectional", &CPocket::m_zig_unidirectional)
		.def_readwrite("cut_mode", &CPocket::m_cut_mode)
		;

	bp::class_<CProfile, bp::bases<CSketchOp>>("Profile")
		.def(bp::init<CProfile>())
		.def_readwrite("tool_on_side", &CProfile::m_tool_on_side)
		.def_readwrite("cut_mode", &CProfile::m_cut_mode)
		.def_readwrite("auto_roll_on", &CProfile::m_auto_roll_on)
		.def_readwrite("auto_roll_off", &CProfile::m_auto_roll_off)
		.def_readwrite("auto_roll_radius", &CProfile::m_auto_roll_radius)
		.def_readwrite("lead_in_line_len", &CProfile::m_lead_in_line_len)
		.def_readwrite("lead_out_line_len", &CProfile::m_lead_out_line_len)
		.def_readwrite("roll_on_point_x", &CProfile::m_roll_on_point_x)
		.def_readwrite("roll_on_point_y", &CProfile::m_roll_on_point_y)
		.def_readwrite("roll_on_point_z", &CProfile::m_roll_on_point_z)
		.def_readwrite("roll_off_point_x", &CProfile::m_roll_off_point_x)
		.def_readwrite("roll_off_point_y", &CProfile::m_roll_off_point_y)
		.def_readwrite("roll_off_point_z", &CProfile::m_roll_off_point_z)
		.def_readwrite("start_given", &CProfile::m_start_given)
		.def_readwrite("end_given", &CProfile::m_end_given)
		.def_readwrite("start_x", &CProfile::m_start_x)
		.def_readwrite("start_y", &CProfile::m_start_y)
		.def_readwrite("start_z", &CProfile::m_start_z)
		.def_readwrite("end_x", &CProfile::m_end_x)
		.def_readwrite("end_y", &CProfile::m_end_y)
		.def_readwrite("end_z", &CProfile::m_end_z)
		.def_readwrite("extend_at_start", &CProfile::m_extend_at_start)
		.def_readwrite("extend_at_end", &CProfile::m_extend_at_end)
		.def_readwrite("end_beyond_full_profile", &CProfile::m_end_beyond_full_profile)
		.def_readwrite("sort_sketches", &CProfile::m_sort_sketches)
		.def_readwrite("offset_extra", &CProfile::m_offset_extra)
		.def_readwrite("do_finishing_pass", &CProfile::m_do_finishing_pass)
		.def_readwrite("only_finishing_pass", &CProfile::m_only_finishing_pass)
		.def_readwrite("finishing_h_feed_rate", &CProfile::m_finishing_h_feed_rate)
		.def_readwrite("finishing_cut_mode", &CProfile::m_finishing_cut_mode)
		.def_readwrite("finishing_step_down", &CProfile::m_finishing_step_down)
		;

	bp::def("AddMenu", AddMenu);///function AddMenu///params str title///adds a menu to the CAD software
	bp::def("AddMenuItem", AddMenuItem);///function AddMenuItem///params str title, function callback///adds a menu item to the last added menu
	bp::def("MessageBox", CadMessageBox);
	bp::def("RegisterOnSelectionChanged", RegisterOnSelectionChanged);
	bp::def("GetSelectedObjects", GetSelectedObjects);
	bp::def("GetObjects", GetObjects);
	bp::def("AddObject", CadAddObject);
	bp::def("NewPoint", NewPoint, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("NewDrilling", NewDrilling, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("NewProfile", NewProfile, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("NewPocket", NewPocket, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("NewTool", NewTool, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("GetProgram", GetProgram, bp::return_value_policy<bp::reference_existing_object>());
	bp::def("AddWindow", AddWindow);///function AddWindow///params str title///adds a window to the CAD software
	bp::def("AttachWindowToWindow", AttachWindowToWindow);///function AttachWindowToWindow///params int hwnd///attaches given window to the CAD window
	bp::def("GetWindowHandle", GetWindowHandle);

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

	bp::scope().attr("TOOL_MATERIAL_TYPE_HSS") = (int)CTool::eHighSpeedSteel;
	bp::scope().attr("TOOL_MATERIAL_TYPE_CARBIDE") = (int)CTool::eCarbide;
	bp::scope().attr("TOOL_MATERIAL_TYPE_UNDEFINED") = (int)CTool::eUndefinedMaterialType;

	bp::scope().attr("TOOL_TYPE_DRILL") = (int)CTool::eDrill;
	bp::scope().attr("TOOL_TYPE_CENTRE_DRILL") = (int)CTool::eCentreDrill;
	bp::scope().attr("TOOL_TYPE_ENDMILL") = (int)CTool::eEndmill;
	bp::scope().attr("TOOL_TYPE_SLOT_CUTTER") = (int)CTool::eSlotCutter;
	bp::scope().attr("TOOL_TYPE_BALL_CUTTER") = (int)CTool::eBallEndMill;
	bp::scope().attr("TOOL_TYPE_CHAMFER") = (int)CTool::eChamfer;
	bp::scope().attr("TOOL_TYPE_UNDEFINED") = (int)CTool::eUndefinedToolType;
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

static bool write_python_file(const wxString& python_file_path)
{
	wxFile ofs(python_file_path.c_str(), wxFile::write);
	if (!ofs.IsOpened())return false;

	ofs.Write(wxGetApp().m_program->m_python_program.c_str());

	return true;
}

static void RunPythonCommand(const char* command)
{
	PyObject *main_module, *globals;
	BeforePythonCall(&main_module, &globals);

	PyRun_String(command, Py_file_input, globals, globals);

	AfterPythonCall(main_module);
}

void HeeksCADapp::RunPythonScript()
{
	{
		// clear the output file
		wxFile f(m_program->GetOutputFileName().c_str(), wxFile::write);
		if (f.IsOpened())f.Write(_T("\n"));
	}

	// Check to see if someone has modified the contents of the
	// program canvas manually.  If so, replace the m_python_program
	// with the edited program.  We don't want to do this without
	// this check since the maximum size of m_textCtrl is sometimes
	// a limitation to the size of the python program.  If the first 'n' characters
	// of m_python_program matches the full contents of the m_textCtrl then
	// it's likely that the text control holds as much of the python program
	// as it can hold but more may still exist in m_python_program.
	unsigned int text_control_length = m_program_canvas->m_textCtrl->GetLastPosition();
	if (m_program->m_python_program.substr(0, text_control_length) != m_program_canvas->m_textCtrl->GetValue())
	{
		// copy the contents of the program canvas to the string
		m_program->m_python_program.clear();
		m_program->m_python_program << wxGetApp().m_program_canvas->m_textCtrl->GetValue();
	}

#ifdef FREE_VERSION
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help/buy-heekscnc-1-0"));
#endif
	wxString python_file_made;

	m_output_canvas->m_textCtrl->Clear(); // clear the output window
	m_print_canvas->m_textCtrl->Clear(); // clear the output window

	// write the python file
	wxStandardPaths& standard_paths = wxStandardPaths::Get();
	wxFileName file_str(standard_paths.GetTempDir().c_str(), _T("post.py"));

	if (!write_python_file(file_str.GetFullPath()))
	{
		wxString error;
		error << _T("couldn't write ") << file_str.GetFullPath();
		wxMessageBox(error.c_str());
	}
	else
	{
		python_file_made = file_str.GetFullPath();
		python_file_made.Replace('\\', '/');

		wxString command = wxString::Format(_T("import importlib\nimport sys\nmods = ['kurve_funcs', 'area', 'area_funcs', 'nc.nc', 'nc.iso', 'nc.iso_modal', 'nc.emc2b']\nfor t in mods:\n  if t in sys.modules.keys(): importlib.reload(sys.modules[t])\nwith open('%s') as f:\n  code = compile(f.read(), 'post.py', 'exec')\n  exec(code, None, None)"), python_file_made.c_str());
		const char* char_str = command.utf8_str();
		RunPythonCommand(char_str);
	}

	BackplotGCode(m_program->GetOutputFileName());

}

void HeeksCADapp::BackplotGCode(const wxString& output_file)
{
	wxBusyCursor busy_cursor();

	if (m_program->m_machine.reader == _T("not found"))
	{
		wxMessageBox(_T("Machine reader name (defined in Program Properties) not found"));
	} // End if - then
	else
	{
		wxString output_filepath = output_file;
		//wxString output_filepath = m_program->GetOutputFileName();
		output_filepath.Replace('\\', '/');
		wxString command = wxString::Format(_T("from nc.hxml_writer import HxmlWriter\nfrom nc.%s import Parser as Parser\nparser = Parser(HxmlWriter())\nparser.Parse('%s')\ndel parser"), m_program->m_machine.reader.c_str(), output_filepath.c_str());
		RunPythonCommand(command);

		// there should now be an xml file written
		wxString xml_file_str = wxGetApp().m_program->GetBackplotFilePath();
		{
			wxFile ofs(xml_file_str.c_str());
			if (!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + xml_file_str);
				return;
			}
		}

		xml_file_str.Replace('\\', '/');

		// read the xml file, just like paste, into the program
		wxGetApp().OpenXMLFile(xml_file_str, m_program);
		wxGetApp().Repaint();

		// in Windows, at least, executing the bat file was making HeeksCAD change it's Z order
		wxGetApp().m_frame->Raise();
	} // End if - else
}
