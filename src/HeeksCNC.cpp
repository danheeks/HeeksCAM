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
#include "OutputCanvas.h"
#include "NCCode.h"
#include "strconv.h"
#include "Tool.h"
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

static void HelpMenuCallback(wxCommandEvent& event)
{
	::wxLaunchDefaultBrowser(_T("http://heeks.net/help"));
}

void OnBuildTexture()
{
	wxString filepath = wxGetApp().GetResFolder() + _T("/icons/iconimage.png");
	int width, height, textureWidth, textureHeight;
	unsigned int* t = loadImage(filepath.c_str(), &width, &height, &textureWidth, &textureHeight);
	wxGetApp().m_icon_texture_number = 0;
	if (t)wxGetApp().m_icon_texture_number = *t;
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

	// add the output canvas
	m_output_canvas = new COutputCanvas(frame);
	aui_manager->AddPane(m_output_canvas, wxAuiPaneInfo().Name(_("Output")).Caption(_("Output")).Bottom().BestSize(wxSize(600, 200)));

	// add the print canvas
	m_print_canvas = new CPrintCanvas(frame);
	aui_manager->AddPane(m_print_canvas, wxAuiPaneInfo().Name(_("Print")).Caption(_("Print")).Bottom().BestSize(wxSize(600, 200)));

	bool output_visible;
	bool print_visible;

	config.Read(_T("OutputVisible"), &output_visible);
	config.Read(_T("PrintVisible"), &print_visible);

	// read other settings
	CNCCode::ReadColorsFromConfig();
	aui_manager->GetPane(m_output_canvas).Show(output_visible);
	aui_manager->GetPane(m_print_canvas).Show(print_visible);

	// add tick boxes for them all on the view menu
	wxMenu* window_menu = wxGetApp().m_frame->m_menuWindow;
	window_menu->AppendSeparator();
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Output"), wxBitmap(), OnOutputCanvas, OnUpdateOutputCanvas, NULL, true);
	wxGetApp().m_frame->AddMenuItem(window_menu, _("Print"), wxBitmap(), OnPrintCanvas, OnUpdatePrintCanvas, NULL, true);
	wxGetApp().RegisterHideableWindow(m_output_canvas);
	wxGetApp().RegisterHideableWindow(m_print_canvas);

	// add object reading functions
	wxGetApp().RegisterReadXMLfunction("nccode", CNCCode::ReadFromXMLElement);

	// icons
	wxGetApp().RegisterOnBuildTexture(OnBuildTexture);

	wxGetApp().RegisterHeeksTypesConverter( HeeksCNCType );
}

void HeeksCADapp::OnCNCNewOrOpen(bool open, int res)
{
	if (!open)
	{
		// new
		wxGetApp().m_output_canvas->Clear();
		wxGetApp().m_print_canvas->Clear();
	} // End if - then
}

void HeeksCADapp::OnFrameDelete()
{
	wxAuiManager* aui_manager = wxGetApp().m_frame->m_aui_manager;
	HeeksConfig config;
	config.Write(_T("OutputVisible"), aui_manager->GetPane(m_output_canvas).IsShown());
	config.Write(_T("PrintVisible"), aui_manager->GetPane(m_print_canvas).IsShown());

	CNCCode::WriteColorsToConfig();
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
	case NCCodeBlockType:       return(_("NCCodeBlock"));
	case NCCodeType:       return(_("NCCode"));

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

CMachine::CMachine()
{
}

CMachine::CMachine(const CMachine & rhs)
{
	*this = rhs;	// call the assignment operator
}


CMachine & CMachine::operator= (const CMachine & rhs)
{
	if (this != &rhs)
	{
		post = rhs.post;
		reader = rhs.reader;
		suffix = rhs.suffix;
		description = rhs.description;
		py_params = rhs.py_params;
	} // End if - then

	return(*this);
} // End assignment operator.
