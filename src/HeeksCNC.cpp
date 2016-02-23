// HeeksCNC.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include <errno.h>
#include <wx/stdpaths.h>
#include <wx/dynlib.h>
#include <wx/aui/aui.h>
#include <wx/filename.h>
#include "PropertyString.h"
#include "PropertyCheck.h"
#include "PropertyList.h"
#include "Observer.h"
#include "ToolImage.h"
#include "Program.h"
#include "ProgramCanvas.h"
#include "OutputCanvas.h"
#include "NCCode.h"
#include "Profile.h"
#include "Pocket.h"
#include "Drilling.h"
#include "CTool.h"
#include "Operations.h"
#include "Tools.h"
#include "strconv.h"
#include "Tool.h"
#include "CNCPoint.h"
#include "Tags.h"
#include "Tag.h"
#include "ScriptOp.h"
#include "Simulate.h"
#include "Pattern.h"
#include "Patterns.h"
#include "Surface.h"
#include "Surfaces.h"
#include "Stock.h"
#include "Stocks.h"
#include "HeeksFrame.h"
#include "HeeksConfig.h"
#include "DigitizedPoint.h"
#include "DigitizeMode.h"
#include "wxImageLoader.h"
#include "HArea.h"
#include "HCircle.h"
#include "HArc.h"
#include "HLine.h"

#include <sstream>

extern void ImportToolsFile( const wxChar *file_path );

wxString HeeksCNCType(const int type);


#ifdef HAVE_TOOLBARS
void OnMachiningBar( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(wxGetApp().m_machiningBar);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateMachiningBar( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	event.Check(aui_manager->GetPane(wxGetApp().m_machiningBar).IsShown());
}
#endif

void OnProgramCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(wxGetApp().m_program_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

void OnUpdateProgramCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	event.Check(aui_manager->GetPane(wxGetApp().m_program_canvas).IsShown());
}

static void OnOutputCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(wxGetApp().m_output_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

static void OnPrintCanvas( wxCommandEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	wxAuiPaneInfo& pane_info = aui_manager->GetPane(wxGetApp().m_print_canvas);
	if(pane_info.IsOk()){
		pane_info.Show(event.IsChecked());
		aui_manager->Update();
	}
}

static void OnUpdateOutputCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	event.Check(aui_manager->GetPane(wxGetApp().m_output_canvas).IsShown());
}

static void OnUpdatePrintCanvas( wxUpdateUIEvent& event )
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	event.Check(aui_manager->GetPane(wxGetApp().m_print_canvas).IsShown());
}

static void GetSketches(std::list<int>& sketches, std::list<int> &tools )
{
	// check for at least one sketch selected

	const std::list<HeeksObj*>& list = wxGetApp().m_marked_list->list();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SketchType)
		{
			sketches.push_back(object->m_id);
		} // End if - then

		if ((object != NULL) && (object->GetType() == ToolType))
		{
			tools.push_back( object->m_id );
		} // End if - then
	}
}

static void NewProfileOp()
{
	std::list<int> tools;
	std::list<int> sketches;
	GetSketches(sketches, tools);

	int sketch = 0;
	if(sketches.size() > 0)sketch = sketches.front();

	CProfile *new_object = new CProfile(sketch, (tools.size()>0)?(*tools.begin()):-1);
	new_object->SetID(wxGetApp().GetNextID(ProfileType));
	new_object->AddMissingChildren(); // add the tags container

	if(new_object->Edit())
	{
		wxGetApp().StartHistory();
		wxGetApp().AddUndoably(new_object, wxGetApp().m_program->Operations());

		if(sketches.size() > 1)
		{
			for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
			{
				if(It == sketches.begin())continue;
				CProfile* copy = (CProfile*)(new_object->MakeACopy());
				copy->m_sketch = *It;
				wxGetApp().AddUndoably(copy, wxGetApp().m_program->Operations());
			}
		}
		wxGetApp().EndHistory();
	}
	else
		delete new_object;
}

static void NewProfileOpMenuCallback(wxCommandEvent &event)
{
	NewProfileOp();
}

static void NewPocketOp()
{
	std::list<int> tools;
	std::list<int> sketches;
	GetSketches(sketches, tools);

	int sketch = 0;
	if(sketches.size() > 0)sketch = sketches.front();

	CPocket *new_object = new CPocket(sketch, (tools.size()>0)?(*tools.begin()):-1 );
	new_object->SetID(wxGetApp().GetNextID(PocketType));

	if(new_object->Edit())
	{
		wxGetApp().StartHistory();
		wxGetApp().AddUndoably(new_object, wxGetApp().m_program->Operations());

		if(sketches.size() > 1)
		{
			for(std::list<int>::iterator It = sketches.begin(); It != sketches.end(); It++)
			{
				if(It == sketches.begin())continue;
				CPocket* copy = (CPocket*)(new_object->MakeACopy());
				copy->m_sketch = *It;
				wxGetApp().AddUndoably(copy, wxGetApp().m_program->Operations());
			}
		}
		wxGetApp().EndHistory();
	}
	else
		delete new_object;
}

static void NewPocketOpMenuCallback(wxCommandEvent &event)
{
	NewPocketOp();
}

static void AddNewObjectUndoablyAndMarkIt(HeeksObj* new_object, HeeksObj* parent)
{
	wxGetApp().StartHistory();
	wxGetApp().AddUndoably(new_object, parent);
	wxGetApp().m_marked_list->Clear(true);
	wxGetApp().m_marked_list->Add(new_object, true);
	wxGetApp().EndHistory();
}

static void NewDrillingOp()
{
	std::list<int> points;

	const std::list<HeeksObj*>& list = wxGetApp().m_marked_list->list();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if (object->GetType() == PointType)
		{
            points.push_back( object->m_id );
		} // End if - else
	} // End for

	{
		CDrilling *new_object = new CDrilling( points, 0, -1 );
		new_object->SetID(wxGetApp().GetNextID(DrillingType));
		if(new_object->Edit())
		{
			wxGetApp().StartHistory();
			wxGetApp().AddUndoably(new_object, wxGetApp().m_program->Operations());
			wxGetApp().EndHistory();
		}
		else
			delete new_object;
	}
}

static void NewDrillingOpMenuCallback(wxCommandEvent &event)
{
	NewDrillingOp();
}

static void NewScriptOpMenuCallback(wxCommandEvent &event)
{
	CScriptOp *new_object = new CScriptOp();
	new_object->SetID(wxGetApp().GetNextID(ScriptOpType));
	if(new_object->Edit())
	{
		wxGetApp().StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Operations());
		wxGetApp().EndHistory();
	}
	else
		delete new_object;
}

static void NewPatternMenuCallback(wxCommandEvent &event)
{
	CPattern *new_object = new CPattern();

	if(new_object->Edit())
	{
		wxGetApp().StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Patterns());
		wxGetApp().EndHistory();
	}
	else
		delete new_object;
}

static void NewSurfaceMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = wxGetApp().m_marked_list->list();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = wxGetApp().GetFirstChild();object; object = wxGetApp().GetNextChild())
		{
			if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
		}
	}

	{
		CSurface *new_object = new CSurface();
		new_object->m_solids = solids;
		if(new_object->Edit())
		{
			wxGetApp().StartHistory();
			AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Surfaces());
			wxGetApp().EndHistory();
		}
		else
			delete new_object;
	}
}

static void NewStockMenuCallback(wxCommandEvent &event)
{
	// check for at least one solid selected
	std::list<int> solids;

	const std::list<HeeksObj*>& list = wxGetApp().m_marked_list->list();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
	}

	// if no selected solids,
	if(solids.size() == 0)
	{
		// use all the solids in the drawing
		for(HeeksObj* object = wxGetApp().GetFirstChild();object; object = wxGetApp().GetNextChild())
		{
			if(object->GetIDGroupType() == SolidType)solids.push_back(object->m_id);
		}
	}

	{
		CStock *new_object = new CStock();
		new_object->m_solids = solids;
		if(new_object->Edit())
		{
			wxGetApp().StartHistory();
			AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Stocks());
			wxGetApp().EndHistory();
		}
		else
			delete new_object;
	}
}

static void AddNewTool(CTool::eToolType type)
{
	// find next available tool number
	int max_tool_number = 0;
	for(HeeksObj* object = wxGetApp().m_program->Tools()->GetFirstChild(); object; object = wxGetApp().m_program->Tools()->GetNextChild())
	{
		if(object->GetType() == ToolType)
		{
			int tool_number = ((CTool*)object)->m_tool_number;
			if(tool_number > max_tool_number)max_tool_number = tool_number;
		}
	}

	// Add a new tool.
	CTool *new_object = new CTool(NULL, type, max_tool_number + 1);
	if(new_object->Edit())
		AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Tools());
	else
		delete new_object;
}

static void NewDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eDrill);
}

static void NewCentreDrillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eCentreDrill);
}

static void NewEndmillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eEndmill);
}

static void NewSlotCutterMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eSlotCutter);
}

static void NewBallEndMillMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eBallEndMill);
}

static void NewChamferMenuCallback(wxCommandEvent &event)
{
	AddNewTool(CTool::eChamfer);
}

static void NewProgramMenuCallback(wxCommandEvent &event)
{
	// Add a new program.
	CProgram *new_object = new CProgram;
	if (new_object->Edit())
	{
		new_object->AddMissingChildren();
		AddNewObjectUndoablyAndMarkIt(new_object, NULL);
	}
	else
		delete new_object;
}

static void RunScriptMenuCallback(wxCommandEvent &event)
{
	wxGetApp().RunPythonScript();
}

static void PostProcessMenuCallback(wxCommandEvent &event)
{
	// write the python program
	wxGetApp().m_program->RewritePythonProgram();

	// run it
	wxGetApp().RunPythonScript();
}

static void CancelMenuCallback(wxCommandEvent &event)
{
	// to do
}

#ifdef WIN32
static void SimulateCallback(wxCommandEvent &event)
{
	RunVoxelcutSimulation();
}
#endif

static void OpenNcFileMenuCallback(wxCommandEvent& event)
{
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxFileDialog dialog(wxGetApp().m_output_canvas, _("Open NC file"), wxEmptyString, wxEmptyString, wildcard_string);
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		wxGetApp().BackplotGCode(dialog.GetPath());
	}
}

// create a temporary file for ngc output
// run appropriate command to make 'Machine' read ngc file
// linux/emc/axis: this would typically entail calling axis-remote <filename>

static void SaveNcFileMenuCallback(wxCommandEvent& event)
{
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& sp = wxStandardPaths::Get();
#else
	wxStandardPaths sp;
#endif
    wxString user_docs =sp.GetDocumentsDir();
    wxString ncdir;
    //ncdir =  user_docs + _T("/nc");
    ncdir =  user_docs; //I was getting tired of having to start out at the root directory in linux
	wxString ext_str(_T("*.*")); // to do, use the machine's NC extension
	wxString wildcard_string = wxString(_("NC files")) + _T(" |") + ext_str;
    wxString defaultDir = ncdir;
	wxFileDialog fd(wxGetApp().m_output_canvas, _("Save NC file"), defaultDir, wxEmptyString, wildcard_string, wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
	fd.SetFilterIndex(1);
	if (fd.ShowModal() == wxID_OK)
	{           
		wxString nc_file_str = fd.GetPath().c_str();
		{
			wxFile ofs(nc_file_str.c_str(), wxFile::write);
			if(!ofs.IsOpened())
			{
				wxMessageBox(wxString(_("Couldn't open file")) + _T(" - ") + nc_file_str);
				return;
			}
               

          if(wxGetApp().m_use_DOS_not_Unix == true)   //DF -added to get DOS line endings HeeksCNC running on Unix 
            {
                long line_num= 0;
                bool ok = true;
                int nLines = wxGetApp().m_output_canvas->m_textCtrl->GetNumberOfLines();
            for ( int nLine = 0; ok && nLine < nLines; nLine ++)
                {   
                    ok = ofs.Write(wxGetApp().m_output_canvas->m_textCtrl->GetLineText(line_num) + _T("\r\n") );
                    line_num = line_num+1;
                }
            }

            else
			    ofs.Write(wxGetApp().m_output_canvas->m_textCtrl->GetValue());
		}
		wxGetApp().BackplotGCode(nc_file_str);
	}
}

static void HelpMenuCallback(wxCommandEvent& event)
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help"));
}

// a class to re-use existing "OnButton" functions in a Tool class
class CCallbackTool: public Tool{
public:
	wxString m_title;
	wxString m_bitmap_path;
	void(*m_callback)(wxCommandEvent&);

	CCallbackTool(const wxString& title, const wxString& bitmap_path, void(*callback)(wxCommandEvent&)): m_title(title), m_bitmap_path(bitmap_path), m_callback(callback){}

	// Tool's virtual functions
	const wxChar* GetTitle(){return m_title;}
	void Run(){
		wxCommandEvent dummy_evt;
		(*m_callback)(dummy_evt);
	}
	wxString BitmapPath(){ return m_bitmap_path;}
};

static CCallbackTool new_drill_tool(_("New Drill..."), _T("drill"), NewDrillMenuCallback);
static CCallbackTool new_centre_drill_tool(_("New Centre Drill..."), _T("centredrill"), NewCentreDrillMenuCallback);
static CCallbackTool new_endmill_tool(_("New End Mill..."), _T("endmill"), NewEndmillMenuCallback);
static CCallbackTool new_slotdrill_tool(_("New Slot Drill..."), _T("slotdrill"), NewSlotCutterMenuCallback);
static CCallbackTool new_ball_end_mill_tool(_("New Ball End Mill..."), _T("ballmill"), NewBallEndMillMenuCallback);
static CCallbackTool new_chamfer_mill_tool(_("New Chamfer Mill..."), _T("chamfmill"), NewChamferMenuCallback);

void HeeksCADapp::GetNewToolTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_drill_tool);
	t_list->push_back(&new_centre_drill_tool);
	t_list->push_back(&new_endmill_tool);
	t_list->push_back(&new_slotdrill_tool);
	t_list->push_back(&new_ball_end_mill_tool);
	t_list->push_back(&new_chamfer_mill_tool);
}

static CCallbackTool new_pattern_tool(_("New Pattern..."), _T("pattern"), NewPatternMenuCallback);

void HeeksCADapp::GetNewPatternTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_pattern_tool);
}

static CCallbackTool new_surface_tool(_("New Surface..."), _T("surface"), NewSurfaceMenuCallback);

void HeeksCADapp::GetNewSurfaceTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_surface_tool);
}

static CCallbackTool new_stock_tool(_("New Stock..."), _T("stock"), NewStockMenuCallback);

void HeeksCADapp::GetNewStockTools(std::list<Tool*>* t_list)
{
	t_list->push_back(&new_stock_tool);
}

#define MAX_XML_SCRIPT_OPS 10

std::vector< CXmlScriptOp > script_ops;
int script_op_flyout_index = 0;

static void NewXmlScriptOp(int i)
{
	CScriptOp *new_object = new CScriptOp();
	new_object->m_title_made_from_id = false;
	new_object->m_title = script_ops[i].m_name;
	new_object->m_str = script_ops[i].m_script;
	new_object->m_user_icon = true;
	new_object->m_user_icon_name = script_ops[i].m_icon;

	if(new_object->Edit())
	{
		wxGetApp().StartHistory();
		AddNewObjectUndoablyAndMarkIt(new_object, wxGetApp().m_program->Operations());
		wxGetApp().EndHistory();
	}
	else
		delete new_object;
}

static void NewXmlScriptOpCallback0(wxCommandEvent &event)
{
	NewXmlScriptOp(0);
}

static void NewXmlScriptOpCallback1(wxCommandEvent &event)
{
	NewXmlScriptOp(1);
}

static void NewXmlScriptOpCallback2(wxCommandEvent &event)
{
	NewXmlScriptOp(2);
}

static void NewXmlScriptOpCallback3(wxCommandEvent &event)
{
	NewXmlScriptOp(3);
}

static void NewXmlScriptOpCallback4(wxCommandEvent &event)
{
	NewXmlScriptOp(4);
}

static void NewXmlScriptOpCallback5(wxCommandEvent &event)
{
	NewXmlScriptOp(5);
}

static void NewXmlScriptOpCallback6(wxCommandEvent &event)
{
	NewXmlScriptOp(6);
}

static void NewXmlScriptOpCallback7(wxCommandEvent &event)
{
	NewXmlScriptOp(7);
}

static void NewXmlScriptOpCallback8(wxCommandEvent &event)
{
	NewXmlScriptOp(8);
}

static void NewXmlScriptOpCallback9(wxCommandEvent &event)
{
	NewXmlScriptOp(9);
}

static void AddXmlScriptOpMenuItems(wxMenu *menu = NULL)
{
	script_ops.clear();
	CProgram::GetScriptOps(script_ops);

	int i = 0;
	for(std::vector< CXmlScriptOp >::iterator It = script_ops.begin(); It != script_ops.end(); It++, i++)
	{
		CXmlScriptOp &s = *It;
		if(i >= MAX_XML_SCRIPT_OPS)break;
		void(*onButtonFunction)(wxCommandEvent&) = NULL;
		switch(i)
		{
		case 0:
			onButtonFunction = NewXmlScriptOpCallback0;
			break;
		case 1:
			onButtonFunction = NewXmlScriptOpCallback1;
			break;
		case 2:
			onButtonFunction = NewXmlScriptOpCallback2;
			break;
		case 3:
			onButtonFunction = NewXmlScriptOpCallback3;
			break;
		case 4:
			onButtonFunction = NewXmlScriptOpCallback4;
			break;
		case 5:
			onButtonFunction = NewXmlScriptOpCallback5;
			break;
		case 6:
			onButtonFunction = NewXmlScriptOpCallback6;
			break;
		case 7:
			onButtonFunction = NewXmlScriptOpCallback7;
			break;
		case 8:
			onButtonFunction = NewXmlScriptOpCallback8;
			break;
		case 9:
			onButtonFunction = NewXmlScriptOpCallback9;
			break;
		}

		if(menu)
			wxGetApp().m_frame->AddMenuItem(menu, s.m_name, ToolImage(s.m_bitmap), onButtonFunction);
#ifdef HAVE_TOOLBARS
		else
			wxGetApp().AddFlyoutButton(s.m_name, ToolImage(s.m_bitmap), s.m_name, onButtonFunction);
#endif
	}
}	

#ifdef HAVE_TOOLBARS
static void AddToolBars()
{
	if(!wxGetApp().m_machining_hidden)
	{
		wxFrame* frame = wxGetApp().m_frame;
		if(wxGetApp().m_machiningBar)delete wxGetApp().m_machiningBar;
		wxGetApp().m_machiningBar = new wxToolBar(frame, -1, wxDefaultPosition, wxDefaultSize, wxTB_NODIVIDER | wxTB_FLAT);
		wxGetApp().m_machiningBar->SetToolBitmapSize(wxSize(ToolImage::GetBitmapSize(), ToolImage::GetBitmapSize()));

		wxGetApp().StartToolBarFlyout(_("Milling operations"));
		wxGetApp().AddFlyoutButton(_T("Profile"), ToolImage(_T("opprofile")), _("New Profile Operation..."), NewProfileOpMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Pocket"), ToolImage(_T("pocket")), _("New Pocket Operation..."), NewPocketOpMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Drill"), ToolImage(_T("drilling")), _("New Drill Cycle Operation..."), NewDrillingOpMenuCallback);
		wxGetApp().EndToolBarFlyout((wxToolBar*)(wxGetApp().m_machiningBar));

		wxGetApp().StartToolBarFlyout(_("Other operations"));
		wxGetApp().AddFlyoutButton(_T("ScriptOp"), ToolImage(_T("scriptop")), _("New Script Operation..."), NewScriptOpMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Pattern"), ToolImage(_T("pattern")), _("New Pattern..."), NewPatternMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Surface"), ToolImage(_T("surface")), _("New Surface..."), NewSurfaceMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Stock"), ToolImage(_T("stock")), _("New Stock..."), NewStockMenuCallback);
		AddXmlScriptOpMenuItems();

		wxGetApp().EndToolBarFlyout((wxToolBar*)(wxGetApp().m_machiningBar));

		wxGetApp().StartToolBarFlyout(_("Tools"));
		wxGetApp().AddFlyoutButton(_T("drill"), ToolImage(_T("drill")), _("Drill..."), NewDrillMenuCallback);
		wxGetApp().AddFlyoutButton(_T("centredrill"), ToolImage(_T("centredrill")), _("Centre Drill..."), NewCentreDrillMenuCallback);
		wxGetApp().AddFlyoutButton(_T("endmill"), ToolImage(_T("endmill")), _("End Mill..."), NewEndmillMenuCallback);
		wxGetApp().AddFlyoutButton(_T("slotdrill"), ToolImage(_T("slotdrill")), _("Slot Drill..."), NewSlotCutterMenuCallback);
		wxGetApp().AddFlyoutButton(_T("ballmill"), ToolImage(_T("ballmill")), _("Ball End Mill..."), NewBallEndMillMenuCallback);
		wxGetApp().AddFlyoutButton(_T("chamfmill"), ToolImage(_T("chamfmill")), _("Chamfer Mill..."), NewChamferMenuCallback);
		wxGetApp().EndToolBarFlyout((wxToolBar*)(wxGetApp().m_machiningBar));

		wxGetApp().StartToolBarFlyout(_("Post Processing"));
		wxGetApp().AddFlyoutButton(_T("PostProcess"), ToolImage(_T("postprocess")), _("Post-Process"), PostProcessMenuCallback);
		wxGetApp().AddFlyoutButton(_T("Run Python Script"), ToolImage(_T("runpython")), _("Run Python Script"), RunScriptMenuCallback);
		wxGetApp().AddFlyoutButton(_T("OpenNC"), ToolImage(_T("opennc")), _("Open NC File"), OpenNcFileMenuCallback);
		wxGetApp().AddFlyoutButton(_T("SaveNC"), ToolImage(_T("savenc")), _("Save NC File"), SaveNcFileMenuCallback);
#ifndef WIN32
		wxGetApp().AddFlyoutButton(_T("Send to Machine"), ToolImage(_T("tomachine")), _("Send to Machine"), SendToMachineMenuCallback);
#endif
		wxGetApp().AddFlyoutButton(_T("Cancel"), ToolImage(_T("cancel")), _("Cancel Python Script"), CancelMenuCallback);
#ifdef WIN32
		wxGetApp().AddFlyoutButton(_T("Simulate"), ToolImage(_T("simulate")), _("Simulate"), SimulateCallback);
#endif
		wxGetApp().EndToolBarFlyout((wxToolBar*)(wxGetApp().m_machiningBar));

		wxGetApp().m_machiningBar->Realize();
		wxGetApp().m_frame->AddToolBar(wxGetApp().m_machiningBar, _T("MachiningBar"), _("Machining tools"));
		wxGetApp().m_external_toolbars.push_back(wxGetApp().m_machiningBar);


	}
}
#endif

void OnBuildTexture()
{
	wxString filepath = wxGetApp().GetResFolder() + _T("/icons/iconimage.png");
	int width, height, textureWidth, textureHeight;
	unsigned int* t = loadImage(filepath.c_str(), &width, &height, &textureWidth, &textureHeight);
	wxGetApp().m_icon_texture_number = 0;
	if (t)wxGetApp().m_icon_texture_number = *t;
}

static void UnitsChangedHandler( const double units )
{
    // The view units have changed.  See if the user wants the NC output units to change
    // as well.

    if (units != wxGetApp().m_program->m_units)
    {
        int response;
        response = wxMessageBox( _("Would you like to change the NC code generation units too?"), _("Change Units"), wxYES_NO );
        if (response == wxYES)
        {
            wxGetApp().m_program->m_units = units;
        }
    }
}

class SketchBox{
public:
	CBox m_box;
	gp_Vec m_latest_shift;

	SketchBox(const CBox &box);

	SketchBox(const SketchBox &s)
	{
		m_box = s.m_box;
		m_latest_shift = s.m_latest_shift;
	}

	void UpdateBoxAndSetShift(const CBox &new_box)
	{
		// use Centre
		double old_centre[3], new_centre[3];
		m_box.Centre(old_centre);
		new_box.Centre(new_centre);
		m_latest_shift = gp_Vec(new_centre[0] - old_centre[0], new_centre[1] - old_centre[1], 0.0);
		m_box = new_box;
	}
};
SketchBox::SketchBox(const CBox &box)
	{
		m_box = box;
		m_latest_shift = gp_Vec(0, 0, 0);
	}

class HeeksCADObserver: public Observer
{
public:
	std::map<int, SketchBox> m_box_map;

	void OnChanged(const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed, const std::list<HeeksObj*>* modified)
	{
		if(added)
		{
			for(std::list<HeeksObj*>::const_iterator It = added->begin(); It != added->end(); It++)
			{
				HeeksObj* object = *It;
				if(object->GetType() == SketchType)
				{
					CBox box;
					object->GetBox(box);
					m_box_map.insert(std::make_pair(object->GetID(), SketchBox(box)));
				}
			}
		}

		if(modified)
		{
			for(std::list<HeeksObj*>::const_iterator It = modified->begin(); It != modified->end(); It++)
			{
				HeeksObj* object = *It;
				if(object->GetType() == SketchType)
				{
					CBox new_box;
					object->GetBox(new_box);
					std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
					if(FindIt != m_box_map.end())
					{
						SketchBox &sketch_box = FindIt->second;
						sketch_box.UpdateBoxAndSetShift(new_box);
					}
				}
			}

			// check all the profile operations, so we can move the tags
			if (wxGetApp().m_program)
			{
				for (HeeksObj* object = wxGetApp().m_program->Operations()->GetFirstChild(); object; object = wxGetApp().m_program->Operations()->GetNextChild())
				{
					if (object->GetType() == ProfileType)
					{
						CProfile* profile = (CProfile*)object;
						std::map<int, SketchBox>::iterator FindIt = m_box_map.find(object->GetID());
						if (FindIt != m_box_map.end())
						{
							SketchBox &sketch_box = FindIt->second;
							for (HeeksObj* tag = profile->Tags()->GetFirstChild(); tag; tag = profile->Tags()->GetNextChild())
							{
								((CTag*)tag)->m_pos[0] += sketch_box.m_latest_shift.X();
								((CTag*)tag)->m_pos[1] += sketch_box.m_latest_shift.Y();
							}

							profile->m_start_x += sketch_box.m_latest_shift.X();
							profile->m_start_y += sketch_box.m_latest_shift.Y();
							profile->m_start_z += sketch_box.m_latest_shift.Z();

							profile->m_end_x += sketch_box.m_latest_shift.X();
							profile->m_end_y += sketch_box.m_latest_shift.Y();
							profile->m_end_z += sketch_box.m_latest_shift.Z();

							profile->m_roll_on_point_x += sketch_box.m_latest_shift.X();
							profile->m_roll_on_point_y += sketch_box.m_latest_shift.Y();
							profile->m_roll_on_point_z += sketch_box.m_latest_shift.Z();

							profile->m_roll_off_point_x += sketch_box.m_latest_shift.X();
							profile->m_roll_off_point_y += sketch_box.m_latest_shift.Y();
							profile->m_roll_off_point_z += sketch_box.m_latest_shift.Z();
						}
					}
				}
			}

			for(std::map<int, SketchBox>::iterator It = m_box_map.begin(); It != m_box_map.end(); It++)
			{
				SketchBox &sketch_box = It->second;
				sketch_box.m_latest_shift = gp_Vec(0, 0, 0);
			}
		}
	}

	void Clear()
	{
		m_box_map.clear();
	}
}heekscad_observer;

class NewProfileOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Profile Operation");}
	void Run(){
		NewProfileOp();
	}
	wxString BitmapPath(){ return _T("opprofile");}
};

class NewPocketOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Pocket Operation");}
	void Run(){
		NewPocketOp();
	}
	wxString BitmapPath(){ return _T("pocket");}
};

class NewDrillingOpTool:public Tool
{
	// Tool's virtual functions
	const wxChar* GetTitle(){return _("New Drilling Operation");}
	void Run(){
		NewDrillingOp();
	}
	wxString BitmapPath(){ return _T("drilling");}
};

static void GetMarkedListTools(std::list<Tool*>& t_list)
{
	std::set<int> group_types;

	const std::list<HeeksObj*>& list = wxGetApp().m_marked_list->list();
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		group_types.insert(object->GetIDGroupType());
	}

	for(std::set<int>::iterator It = group_types.begin(); It != group_types.end(); It++)
	{
		switch(*It)
		{
		case SketchType:
			t_list.push_back(new NewProfileOpTool);
			t_list.push_back(new NewPocketOpTool);
			break;
		case PointType:
			t_list.push_back(new NewDrillingOpTool);
			break;
		}
	}
}

void HeeksCADapp::OnCNCStartUp()
{
#if !defined WXUSINGDLL
	wxInitialize();
#endif

	HeeksConfig config;

	// About box, stuff
	wxGetApp().m_frame->m_extra_about_box_str.Append(wxString(_T("\n\n")) + _("HeeksCNC is the free machining add-on to HeeksCAD")
		+ _T("\n") + _T("          http://code.google.com/p/heekscnc/")
		+ _T("\n") + _("Written by Dan Heeks, Hirutso Enni, Perttu Ahola, David Nicholls")
		+ _T("\n") + _("With help from archivist, crotchet1, DanielFalck, fenn, Sliptonic")
		+ _T("\n\n") + _("geometry code, donated by Geoff Hawkesford, Camtek GmbH http://www.peps.de/")
		+ _T("\n") + _("pocketing code from http://code.google.com/p/libarea/ , derived from the kbool library written by Klaas Holwerda http://boolean.klaasholwerda.nl/bool.html")
		+ _T("\n") + _("Zig zag code from opencamlib http://code.google.com/p/opencamlib/")
		+ _T("\n\n") + _("This HeeksCNC software installation is restricted by the GPL license http://www.gnu.org/licenses/gpl-3.0.txt")
		+ _T("\n") + _("  which means it is free and open source, and must stay that way")
		);

	// add menus and toolbars
	wxFrame* frame = wxGetApp().m_frame;
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;

#ifdef HAVE_TOOLBARS
	// tool bars
	wxGetApp().m_AddToolBars_list.push_back(AddToolBars);
	AddToolBars();
#endif

	// Help menu
	wxMenu *menuHelp = wxGetApp().m_frame->m_menuHelp;
	menuHelp->AppendSeparator();
	wxGetApp().m_frame->AddMenuItem(menuHelp, _("Online HeeksCNC Manual"), ToolImage(_T("help")), HelpMenuCallback);

	// Milling Operations menu
	wxMenu *menuMillingOperations = new wxMenu;
	wxGetApp().m_frame->AddMenuItem(menuMillingOperations, _("Profile Operation..."), ToolImage(_T("opprofile")), NewProfileOpMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuMillingOperations, _("Pocket Operation..."), ToolImage(_T("pocket")), NewPocketOpMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuMillingOperations, _("Drilling Operation..."), ToolImage(_T("drilling")), NewDrillingOpMenuCallback);

	// Additive Operations menu
	wxMenu *menuOperations = new wxMenu;
	wxGetApp().m_frame->AddMenuItem(menuOperations, _("Script Operation..."), ToolImage(_T("scriptop")), NewScriptOpMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuOperations, _("Pattern..."), ToolImage(_T("pattern")), NewPatternMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuOperations, _("Surface..."), ToolImage(_T("surface")), NewSurfaceMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuOperations, _("Stock..."), ToolImage(_T("stock")), NewStockMenuCallback);
	AddXmlScriptOpMenuItems(menuOperations);

	// Tools menu
	wxMenu *menuTools = new wxMenu;
	wxGetApp().m_frame->AddMenuItem(menuTools, _("Drill..."), ToolImage(_T("drill")), NewDrillMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuTools, _("Centre Drill..."), ToolImage(_T("centredrill")), NewCentreDrillMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuTools, _("End Mill..."), ToolImage(_T("endmill")), NewEndmillMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuTools, _("Slot Drill..."), ToolImage(_T("slotdrill")), NewSlotCutterMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuTools, _("Ball End Mill..."), ToolImage(_T("ballmill")), NewBallEndMillMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuTools, _("Chamfer Mill..."), ToolImage(_T("chamfmill")), NewChamferMenuCallback);

	// Machining menu
	wxMenu *menuMachining = new wxMenu;
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Add New Program"), ToolImage(_T("program")), NewProgramMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Add New Milling Operation"), ToolImage(_T("ops")), NULL, NULL, menuMillingOperations);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Add Other Operation"), ToolImage(_T("ops")), NULL, NULL, menuOperations);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Add New Tool"), ToolImage(_T("tools")), NULL, NULL, menuTools);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Run Python Script"), ToolImage(_T("runpython")), RunScriptMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Post-Process"), ToolImage(_T("postprocess")), PostProcessMenuCallback);
#ifdef WIN32
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Simulate"), ToolImage(_T("simulate")), SimulateCallback);
#endif
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Open NC File..."), ToolImage(_T("opennc")), OpenNcFileMenuCallback);
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Save NC File as..."), ToolImage(_T("savenc")), SaveNcFileMenuCallback);
#ifndef WIN32
	wxGetApp().m_frame->AddMenuItem(menuMachining, _("Send to Machine"), ToolImage(_T("tomachine")), SendToMachineMenuCallback);
#endif
	frame->GetMenuBar()->Insert( frame->GetMenuBar()->GetMenuCount()-1, menuMachining,  _("&Machining"));

	// add the program canvas
	m_program_canvas = new CProgramCanvas(frame);
	aui_manager->AddPane(m_program_canvas, wxAuiPaneInfo().Name(_("Program")).Caption(_("Program")).Bottom().BestSize(wxSize(600, 200)));

	// add the output canvas
	m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_("Output")).Caption(_("Output")).Bottom().BestSize(wxSize(600, 200)));

	// add the print canvas
	m_print_canvas = new CPrintCanvas(frame);
	aui_manager->AddPane(m_print_canvas, wxAuiPaneInfo().Name(_("Print")).Caption(_("Print")).Bottom().BestSize(wxSize(600, 200)));

	bool program_visible;
	bool output_visible;
	bool print_visible;

	config.Read(_T("ProgramVisible"), &program_visible);
	config.Read(_T("OutputVisible"), &output_visible);
	config.Read(_T("PrintVisible"), &print_visible);

	// read other settings
	CNCCode::ReadColorsFromConfig();
	CProfile::ReadFromConfig();
	CPocket::ReadFromConfig();
	CSpeedOp::ReadFromConfig();
	config.Read(_T("UseClipperNotBoolean"), &m_use_Clipper_not_Boolean, false);
	config.Read(_T("UseDOSNotUnix"), &m_use_DOS_not_Unix, false);
	aui_manager->GetPane(m_program_canvas).Show(program_visible);
	aui_manager->GetPane(m_output_canvas).Show(output_visible);
	aui_manager->GetPane(m_print_canvas).Show(print_visible);

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = wxGetApp().m_frame->m_menuWindow;
	window_menu->AppendSeparator();
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Program"), wxBitmap(), OnProgramCanvas, OnUpdateProgramCanvas, NULL, true);
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Print"), wxBitmap(), OnPrintCanvas, OnUpdatePrintCanvas, NULL, true);
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Machining"), wxBitmap(), OnMachiningBar, OnUpdateMachiningBar, NULL, true);
#endif
	wxGetApp().RegisterHideableWindow(m_program_canvas);
	wxGetApp().RegisterHideableWindow(m_output_canvas);
	wxGetApp().RegisterHideableWindow(m_print_canvas);
#ifdef HAVE_TOOLBARS
	wxGetApp().RegisterHideableWindow(m_machiningBar);
#endif

	// add object reading functions
	wxGetApp().RegisterReadXMLfunction("Program", CProgram::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Operations", COperations::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Tools", CTools::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Profile", CProfile::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Pocket", CPocket::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Drilling", CDrilling::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Tool", CTool::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("CuttingTool", CTool::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Tags", CTags::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Tag", CTag::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("ScriptOp", CScriptOp::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Pattern", CPattern::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Patterns", CPatterns::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Surface", CSurface::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Surfaces", CSurfaces::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Stock", CStock::ReadFromXMLElement);
	wxGetApp().RegisterReadXMLfunction("Stocks", CStocks::ReadFromXMLElement);

	// icons
	wxGetApp().RegisterOnBuildTexture(OnBuildTexture);

	// Import functions.
	{
        std::list<wxString> file_extensions;
        file_extensions.push_back(_T("tool"));
        file_extensions.push_back(_T("tools"));
        file_extensions.push_back(_T("tooltable"));
        if (! wxGetApp().RegisterFileOpenHandler( file_extensions, ImportToolsFile ))
        {
            printf("Failed to register handler for Tool Table files\n");
        }
	}

	wxGetApp().RegisterObserver(&heekscad_observer);

	wxGetApp().RegisterUnitsChangeHandler( UnitsChangedHandler );
	wxGetApp().RegisterHeeksTypesConverter( HeeksCNCType );

	wxGetApp().RegisterMarkeListTools(&GetMarkedListTools);
}

std::list<wxString> HeeksCADapp::GetFileNames( const char *p_szRoot ) const
#ifdef WIN32
{
	std::list<wxString>	results;

	WIN32_FIND_DATA file_data;
	HANDLE hFind;

	std::string pattern = std::string(p_szRoot) + "\\*";
	hFind = FindFirstFile(Ctt(pattern.c_str()), &file_data);

	// Now recurse down until we find document files within 'current' directories.
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) continue;

			results.push_back( file_data.cFileName );
		} while (FindNextFile( hFind, &file_data));

		FindClose(hFind);
	} // End if - then

	return(results);
} // End of GetFileNames() method.
#else
{
	// We're in UNIX land now.

	std::list<wxString>	results;

	DIR *pdir = opendir(p_szRoot);	// Look in the current directory for files
				// whose names begin with "default."
	if (pdir != NULL)
	{
		struct dirent *pent = NULL;
		while ((pent=readdir(pdir)))
		{
			results.push_back(Ctt(pent->d_name));
		} // End while
		closedir(pdir);
	} // End if - then

	return(results);
} // End of GetFileNames() method
#endif



void HeeksCADapp::OnCNCNewOrOpen(bool open, int res)
{
	// check for existance of a program

	bool program_found = false;
	for(HeeksObj* object = wxGetApp().GetFirstChild(); object; object = wxGetApp().GetNextChild())
	{
		if(object->GetType() == ProgramType)
		{
			program_found = true;
			break;
		}
	}

	if(!program_found)
	{
		// add the program
		m_program = new CProgram;

		m_program->AddMissingChildren();
		wxGetApp().Add(m_program, NULL);
		wxGetApp().m_program_canvas->Clear();
		wxGetApp().m_output_canvas->Clear();
		wxGetApp().m_print_canvas->Clear();

		wxGetApp().OpenXMLFile(GetResourceFilename(wxT("default.tooltable")), wxGetApp().m_program->Tools(), NULL, false, false);
	} // End if - then
}

void HeeksCADapp::OnFrameDelete()
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	HeeksConfig config;
	config.Write(_T("ProgramVisible"), aui_manager->GetPane(m_program_canvas).IsShown());
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("PrintVisible"), aui_manager->GetPane(m_print_canvas).IsShown());
#ifdef HAVE_TOOLBARS
	config.Write(_T("MachiningBarVisible"), aui_manager->GetPane(m_machiningBar).IsShown());
#endif

	CNCCode::WriteColorsToConfig();
	CProfile::WriteToConfig();
	CPocket::WriteToConfig();
	CSpeedOp::WriteToConfig();
	config.Write(_T("UseClipperNotBoolean"), m_use_Clipper_not_Boolean);
    config.Write(_T("UseDOSNotUnix"), m_use_DOS_not_Unix);
}

Python HeeksCADapp::SetTool( const int new_tool )
{
	Python python;

	// Select the right tool.
	CTool *pTool = (CTool *) CTool::Find(new_tool);
	if (pTool != NULL)
	{
		if (m_tool_number != new_tool)
		{

			python << _T("tool_change( id=") << new_tool << _T(")\n");
		}

		if(m_attached_to_surface)
		{
			python << _T("nc.creator.set_ocl_cutter(") << pTool->OCLDefinition(m_attached_to_surface) << _T(")\n");
		}
	} // End if - then

	m_tool_number = new_tool;

    return(python);
}

wxString HeeksCADapp::GetDllFolder() const
{
	return GetExeFolder();
}

wxString HeeksCADapp::GetResourceFilename(const wxString resource, const bool writableOnly) const
{
	wxString filename;

#ifdef WIN32
	// Windows
	filename = GetDllFolder() + wxT("/") + resource;
#else
	// Unix
	// Under Unix, it looks for a user-defined resource first.
	// According to FreeDesktop XDG standards, HeeksCNC user-defineable resources should be placed in XDG_CONFIG_HOME (usually: ~/.config/heekscnc/)
	filename = (wxGetenv(wxT("XDG_CONFIG_HOME"))?wxGetenv(wxT("XDG_CONFIG_HOME")):wxFileName::GetHomeDir() + wxT("/.config")) + wxT("/heekscnc/") + resource;
	
	// Under Unix user can't save its resources in system (permissions denied), so we always return a user-writable file
	if(!writableOnly)
	{
		// If user-defined file exists, the resource is located
		if(!wxFileName::FileExists(filename))
		{
			// Else it fallbacks to system-wide resource file (installed with HeeksCNC)
			filename = GetResFolder() + wxT("/") + resource;
			// Note: it should be a good idea to use wxStandardPaths::GetResourcesDir() but it returns HeeksCAD's resource dir (eg. /usr/share/heekscad)
		}
	}
	else
	{
		// Writable file is wanted, so ressource directories should exists (ie. mkdir -p ~/.config/heekscnc)
		wxFileName fn(filename);
		wxFileName::Mkdir(fn.GetPath(), 0700, wxPATH_MKDIR_FULL);
	}
	
#endif
	wprintf(wxT("Resource: ") + resource + wxT(" found at: ") + filename + wxT("\n"));
	return filename;
}

class MyApp : public wxApp
{

 public:

   virtual bool OnInit(void);

 };

 bool MyApp::OnInit(void)

 {

   return true;

 }


wxString HeeksCNCType( const int type )
{
    switch (type)
    {
    case ProgramType:       return(_("Program"));
	case NCCodeBlockType:       return(_("NCCodeBlock"));
	case NCCodeType:       return(_("NCCode"));
	case OperationsType:       return(_("Operations"));
	case ProfileType:       return(_("Profile"));
	case PocketType:       return(_("Pocket"));
	case DrillingType:       return(_("Drilling"));
	case ToolType:       return(_("Tool"));
	case ToolsType:       return(_("Tools"));
	case TagsType:       return(_("Tags"));
	case TagType:       return(_("Tag"));
	case ScriptOpType:       return(_("ScriptOp"));

	default:
        return(_T("")); // Indicates that this function could not make the conversion.
    } // End switch
} // End HeeksCNCType() routine

void HeeksCADapp::LinkXMLEndChild(TiXmlNode* root, TiXmlElement* pElem)
{
	root->LinkEndChild(pElem);
}

TiXmlElement* HeeksCADapp::FirstNamedXMLChildElement(TiXmlElement* pElem, const char* name)
{
	return TiXmlHandle(pElem).FirstChildElement(name).Element();
}

void HeeksCADapp::RemoveXMLChild(TiXmlNode* pElem, TiXmlElement* child)
{
	pElem->RemoveChild(child);
}

bool HeeksCADapp::Digitize(const wxPoint &point, double* pos)
{
	DigitizedPoint p = wxGetApp().m_digitizing->digitize(point);
	if (p.m_type == DigitizeNoItemType)
		return false;

	extract(p.m_point, pos);
	return true;
}

bool HeeksCADapp::GetLastDigitizePosition(double *pos)
{
	if (wxGetApp().m_digitizing != NULL)
	{
		pos[0] = wxGetApp().m_digitizing->digitized_point.m_point.X();
		pos[1] = wxGetApp().m_digitizing->digitized_point.m_point.Y();
		pos[2] = wxGetApp().m_digitizing->digitized_point.m_point.Z();

		return(true);
	}
	else
	{
		return(false);
	}
}

void HeeksCADapp::ObjectAreaString(HeeksObj* object, wxString &s)
{

	switch (object->GetType())
	{
	case CircleType:
	{
					   double c[2] = { ((HCircle*)object)->m_axis.Location().X(), ((HCircle*)object)->m_axis.Location().Y() };
					   double radius = ((HCircle*)object)->m_radius;
					   s << _T("a = area.Area()\n");
					   s << _T("c = area.Curve()\n");
					   s << _T("c.append(area.Point(") << c[0] + radius << _T(", ") << c[1] << _T("))\n");
					   s << _T("c.append(area.Vertex(1, area.Point(") << c[0] - radius << _T(", ") << c[1] << _T("), area.Point(") << c[0] << _T(", ") << c[1] << _T(")))\n");
					   s << _T("c.append(area.Vertex(1, area.Point(") << c[0] + radius << _T(", ") << c[1] << _T("), area.Point(") << c[0] << _T(", ") << c[1] << _T(")))\n");
					   s << _T("a.append(c)\n");
	}
		break;
	case AreaType:
	{
					 s << _T("a = area.Area()\n");
					 CArea &a = ((HArea*)object)->m_area;
					 for (std::list<CCurve>::iterator It = a.m_curves.begin(); It != a.m_curves.end(); It++)
					 {
						 CCurve &c = *It;
						 s << _T("c = area.Curve()\n");
						 for (std::list<CVertex>::iterator VIt = c.m_vertices.begin(); VIt != c.m_vertices.end(); VIt++)
						 {
							 CVertex &v = *VIt;
							 if (v.m_type == 0)s << _T("c.append(area.Point(") << v.m_p.x << _T(", ") << v.m_p.y << _T("))\n");
							 else s << _T("c.append(area.Vertex(") << v.m_type << _T(", area.Point(") << v.m_p.x << _T(", ") << v.m_p.y << _T("), area.Point(") << v.m_c.x << _T(", ") << v.m_c.y << _T(")))\n");
						 }
						 s << _T("a.append(c)\n");
					 }
	}
		break;
	}
}

const wxChar* HeeksCADapp::GetFileFullPath()
{
	if (wxGetApp().m_untitled)return NULL;
	return wxGetApp().m_filepath;
}

void HeeksCADapp::SetViewUnits(double units, bool write_to_config)
{
	wxGetApp().m_view_units = units;
	wxGetApp().OnChangeViewUnits(wxGetApp().m_view_units);
	if (write_to_config)
	{
		HeeksConfig config;
		config.Write(_T("ViewUnits"), wxGetApp().m_view_units);
	}
}

void HeeksCADapp::Mark(HeeksObj* object)
{
	wxGetApp().m_marked_list->Add(object, true);
}

void HeeksCADapp::Unmark(HeeksObj* object)
{
	wxGetApp().m_marked_list->Remove(object, true);
}
