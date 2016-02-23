// HeeksCAD.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

/** @mainpage HeeksCAD: Opensource 3D CAD and solid modeling solution.
*   @par Description:
*   - HeeksCAD is a free, open source, CAD application written by Dan Heeks, danheeks@gmail.com
*   @par Features
*   - Import solid models from STEP and IGES files. 
*   - Draw construction geometry and lines and arcs. 
*   - Create new primitive solids, or make solids by extruding a sketch or by making a lofted solid between sketches. 
*   - Modify solids using blending, or boolean operations. 
*   - Save IGES, STEP and STL. Printer plot the 2D geometry or to HPGL. 
*   - Import and export dxf files; lines, arcs, ellipses, splines and polylines are supported. 
*   - HeeksCAD can be translated into any language. There are currently English, German, and Italian translations.
*   - HeeksCAD has been built for Windows, Ubuntu, Debian, and OpenSUSE.
*   - It is possible to make Add-In modules, see HeeksCNC, HeeksArt, and HeeksPython projects.
*
*   @par Dependencies:
*   - Solid modeling is provided by Open CASCADE. 
*   - Graphical user interface is made using wxWidgets.
*
*   @par License 
*   - New BSD License. This means you can take all my work and use it for your own commercial application. Do what you want with it.
*/


#include "stdafx.h"

#include <errno.h>

#include "HeeksCAD.h"
#include "Tool.h"
#include "Material.h"
#include "ToolList.h"
#include "HeeksObj.h"
#include "MarkedObject.h"
#include "PropertyColor.h"
#include "PropertyChoice.h"
#include "PropertyDouble.h"
#include "PropertyLength.h"
#include "PropertyInt.h"
#include "PropertyCheck.h"
#include "PropertyString.h"
#include "PropertyList.h"
#include "HeeksFrame.h"
#include "GraphicsCanvas.h"
#include "OptionsCanvas.h"
#include "InputModeCanvas.h"
#include "TreeCanvas.h"
#include "ObjPropsCanvas.h"
#include "SelectMode.h"
#include "MagDragWindow.h"
#include "ViewRotating.h"
#include "ViewZooming.h"
#include "ViewPanning.h"
#include "DigitizeMode.h"
#include "Shape.h"
#include "Face.h"
#include "ViewPoint.h"
#include "MarkedList.h"
#include "Observer.h"
#include "TransformTool.h"
#include "Grid.h"
#include "Ruler.h"
#include "StretchTool.h"
#include "HLine.h"
#include "HArc.h"
#include "HILine.h"
#include "HCircle.h"
#include "HImage.h"
#include "HPoint.h"
#include "HEllipse.h"
#include "HSpline.h"
#include "HText.h"
#include "HDimension.h"
#include "HXml.h"
#include "Sketch.h"
#include "BezierCurve.h"
#include "StlSolid.h"
#include "HDxf.h"
#include "svg.h"
#include "CoordinateSystem.h"
#include "RegularShapesDrawing.h"
#include "HeeksPrintout.h"
#include "HeeksConfig.h"
#include "Group.h"
#include "AutoSave.h"
#include <wx/progdlg.h>
#include "OrientationModifier.h"
#include "MenuSeparator.h"
#include "HGear.h"
#include "HArea.h"
#include "History.h"
#include "RemoveOrAddTool.h"
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
#include "Picking.h"

#include <sstream>


using namespace std;

#ifdef _DEBUG
#ifdef __WXMSW__
class DestroyedAtClose{
public:
	DestroyedAtClose()
	{}

	~DestroyedAtClose()
	{
_CrtDumpMemoryLeaks();
	}
} destroyed_at_close;

int MyAllocHook( int allocType, void *userData, size_t size, int blockType, long requestNumber, const unsigned char *filename, int lineNumber)
{
	if (requestNumber == 154)
	{
		int here = 0;
		here = 3;
	}
	return TRUE;
}
#endif
#endif

#ifdef PYHEEKSCAD
HeeksCADapp wxGetApp();
HeeksCADapp &wxGetApp(){return wxGetApp();}
#else
IMPLEMENT_APP(HeeksCADapp)
#endif


static unsigned int DecimalPlaces( const double value )
{
	double decimal_places = 1 / value;
	unsigned int required = 0;
	while (decimal_places >= 1.0)
	{
		required++;
		decimal_places /= 10.0;
	}

	return(required + 1);   // Always use 1 more decimal point than our accuracy requires.
}

HeeksCADapp::HeeksCADapp(): ObjList()
{
	m_version_number = wxString(_T(HEEKSCAD_VERSION_MAIN)) + _T(" ") + _T(HEEKSCAD_VERSION_SUB) + _T(" 0");
	m_geom_tol = 0.000001;

	TiXmlBase::SetRequiredDecimalPlaces( DecimalPlaces(m_geom_tol) );	 // Ensure we write XML in enough accuracy to be useful when re-read.

	m_sketch_reorder_tol = 0.01;
	m_view_units = 1.0;
	for(int i = 0; i<NUM_BACKGROUND_COLORS; i++)background_color[i] = HeeksColor(0, 0, 0);
	m_background_mode = BackgroundModeOneColor;
	current_color = HeeksColor(0, 0, 0);
	face_selection_color = HeeksColor(0, 0, 0);
	input_mode_object = NULL;
	cur_mouse_pos.x = 0;
	cur_mouse_pos.y = 0;
	drag_gripper = NULL;
	cursor_gripper = NULL;
	magnification = new MagDragWindow();
	viewrotating = new ViewRotating;
	viewzooming = new ViewZooming;
	viewpanning = new ViewPanning;
	m_select_mode = new CSelectMode();
	m_digitizing = new DigitizeMode();
	digitize_end = false;
	digitize_inters = false;
	digitize_centre = false;
	digitize_midpoint = false;
	digitize_nearest = false;
	digitize_tangent = false;
	digitize_coords = true;
	digitize_screen = false;
	digitizing_radius = 5.0;
	draw_to_grid = true;
	digitizing_grid = 1.0;
	grid_mode = 3;
	m_rotate_mode = 1;
	m_antialiasing = false;
	m_light_push_matrix = true;
	m_marked_list = new MarkedList;
	history = new MainHistory;
	m_doing_rollback = false;
	mouse_wheel_forward_away = true;
	m_mouse_move_highlighting = true;
	ctrl_does_rotate = false;
	m_ruler = new HRuler();
	m_show_ruler = false;
	m_show_datum_coords_system = true;
	m_datum_coords_system_solid_arrows = true;
	m_filepath = wxString(_("Untitled")) + _T(".heeks");
	m_untitled = true;
	m_in_OpenFile = false;
	m_transform_gl_list = 0;
	m_current_coordinate_system = NULL;
	m_mark_newly_added_objects = false;
	m_show_grippers_on_drag = true;
	m_extrude_removes_sketches = false;
	m_loft_removes_sketches = false;
	m_font_tex_number[0] = 0;
	m_font_tex_number[1] = 0;
	m_graphics_text_mode = GraphicsTextModeNone;
	m_locale_initialised = false;
	m_printData = NULL;
	m_pageSetupData = NULL;
	m_file_open_or_import_type = FileOpenOrImportTypeOther;
	m_inPaste = false;
	m_file_open_matrix = NULL;
	m_min_correlation_factor = 0.75;
	m_max_scale_threshold = 1.5;
	m_number_of_sample_points = 10;
	m_property_grid_validation = false;
	m_solid_view_mode = SolidViewFacesAndEdges;
	m_input_uses_modal_dialog = false;
	m_dragging_moves_objects = false;
	m_no_creation_mode = false;
	m_stl_solid_random_colors = false;

#ifndef WIN32
	m_font_paths = _T("/usr/share/qcad/fonts");
	m_word_space_percentage = 100.0;
	m_character_space_percentage = 25.0;
	m_pVectorFont = NULL;	// Default to internal (OpenGL) font.
#endif
	m_stl_facet_tolerance = 0.1;
	m_icon_texture_number = 0;
	m_extrude_to_solid = true;
	m_revolve_angle = 360.0;
	m_window = NULL;
	m_stl_save_as_binary = true;
	m_mouse_move_highlighting = true;
	m_highlight_color = HeeksColor(128, 255, 0);

    {
        std::list<wxString> extensions;
        extensions.push_back(_T("svg"));
        RegisterFileOpenHandler( extensions, OpenSVGFile );
    }
    {
        std::list<wxString> extensions;
        extensions.push_back(_T("stl"));
        RegisterFileOpenHandler( extensions, OpenSTLFile );
    }

    RegisterHeeksTypesConverter(HeeksCADType);

	m_settings_restored = false;

	// cnc constructor code
	m_draw_cutter_radius = true;
	m_program = NULL;
	m_run_program_on_new_line = false;
#ifdef HAVE_TOOLBARS
	m_machiningBar = NULL;
#endif
	m_icon_texture_number = 0;
	m_machining_hidden = false;
	m_settings_restored = false;

}

HeeksCADapp::~HeeksCADapp()
{
	delete m_marked_list;
	m_marked_list = NULL;
	observers.clear();
	delete history;
	delete magnification;
	delete m_select_mode;
	delete m_digitizing;
	delete viewrotating;
	delete viewzooming;
	delete viewpanning;
	m_ruler->m_index = 0;
	delete m_ruler;
	if(m_printData)delete m_printData;
	if(m_pageSetupData)delete m_pageSetupData;

#ifndef WIN32
	m_pVectorFont = NULL;	// Don't free this here.  This memory will be released via ~CxfFonts() instead.
	if (m_pVectorFonts.get() != NULL) delete m_pVectorFonts.release();
#endif
}

bool HeeksCADapp::OnInit()
{
	m_gl_font_initialized = false;
	
	wxInitAllImageHandlers();

	InitialiseLocale();

#ifdef __WXMSW__
#ifdef _DEBUG
	wxCrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
#endif
#endif

#ifdef PYHEEKSCAD
    m_printData = NULL;
    m_pageSetupData = NULL;
#else
    m_printData = new wxPrintData;
    m_pageSetupData = new wxPageSetupDialogData;
    // copy over initial paper size from print record
    (*m_pageSetupData) = *m_printData;
    // Set some initial page margins in mm.
    m_pageSetupData->SetMarginTopLeft(wxPoint(15, 15));
    m_pageSetupData->SetMarginBottomRight(wxPoint(15, 15));
#endif

	int width = 996;
	int height = 691;
	int posx = 200;
	int posy = 200;
	HeeksConfig config;
	config.Read(_T("MainFrameWidth"), &width);
	config.Read(_T("MainFrameHeight"), &height);
	config.Read(_T("MainFramePosX"), &posx);
	config.Read(_T("MainFramePosY"), &posy);
	if(posx < 0)posx = 0;
	if(posy < 0)posy = 0;
	config.Read(_T("DrawEnd"), &digitize_end, false);
	config.Read(_T("DrawInters"), &digitize_inters, false);
	config.Read(_T("DrawCentre"), &digitize_centre, false);
	config.Read(_T("DrawMidpoint"), &digitize_midpoint, false);
	config.Read(_T("DrawNearest"), &digitize_nearest, false);
	config.Read(_T("DrawTangent"), &digitize_tangent, false);
	config.Read(_T("DrawCoords"), &digitize_coords, true);
	config.Read(_T("DrawScreen"), &digitize_screen, false);
	config.Read(_T("DrawToGrid"), &draw_to_grid, true);
	config.Read(_T("UseOldFuse"), &useOldFuse, false);
	config.Read(_T("DrawGrid"), &digitizing_grid);
	config.Read(_T("DrawRadius"), &digitizing_radius);
	{
		long default_color[NUM_BACKGROUND_COLORS] = {
			HeeksColor(187, 233, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(247, 198, 243).COLORREF_color(),
			HeeksColor(193, 235, 236).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color(),
			HeeksColor(255, 255, 255).COLORREF_color()
		};

		for(int i = 0; i<NUM_BACKGROUND_COLORS; i++)
		{
			wxString key = wxString::Format(_T("BackgroundColor%d"), i);
			int color = default_color[i];
			config.Read(key, &color);
			background_color[i] = HeeksColor((long)color);
		}
		int mode = (int)BackgroundModeTwoColors;
		config.Read(_T("BackgroundMode"), &mode);
		m_background_mode = (BackgroundMode)mode;
	}

	{
		wxString str;
		config.Read(_T("CurrentColor"), &str, _T("0 0 0"));
		int r = 0, g = 0, b = 0;
#if wxUSE_UNICODE
		swscanf(str, _T("%d %d %d"), &r, &g, &b);
#else
		sscanf(str, _T("%d %d %d"), &r, &g, &b);
#endif
		current_color = HeeksColor((unsigned char)r, (unsigned char)g, (unsigned char)b);
	}
	{
		int color = HeeksColor(0, 255, 0).COLORREF_color();
		config.Read(_T("FaceSelectionColor"), &color);
		face_selection_color = HeeksColor((long)color);
	}
	config.Read(_T("RotateMode"), &m_rotate_mode);
	config.Read(_T("Antialiasing"), &m_antialiasing);
	config.Read(_T("GridMode"), &grid_mode);
	config.Read(_T("m_light_push_matrix"), &m_light_push_matrix);
	config.Read(_T("WheelForwardAway"), &mouse_wheel_forward_away);
	config.Read(_T("ZoomingReversed"), &ViewZooming::m_reversed);
	config.Read(_T("CtrlDoesRotate"), &ctrl_does_rotate);
	config.Read(_T("DrawDatum"), &m_show_datum_coords_system, true);
	config.Read(_T("DrawDatumSolid"), &m_datum_coords_system_solid_arrows, true);
	config.Read(_T("DatumSize"), &CoordinateSystem::size, 30);
	config.Read(_T("DatumSizeIsPixels"), &CoordinateSystem::size_is_pixels, true);
	config.Read(_T("DrawRuler"), &m_show_ruler, false);
	config.Read(_T("RegularShapesMode"), (int*)(&(regular_shapes_drawing.m_mode)));
	config.Read(_T("RegularShapesNSides"), &(regular_shapes_drawing.m_number_of_side_for_polygon));
	config.Read(_T("RegularShapesRectRad"), &(regular_shapes_drawing.m_rect_radius));
	config.Read(_T("RegularShapesObRad"), &(regular_shapes_drawing.m_obround_radius));
	config.Read(_T("ExtrudeRemovesSketches"), &m_extrude_removes_sketches, true);
	config.Read(_T("LoftRemovesSketches"), &m_loft_removes_sketches, true);
	config.Read(_T("GraphicsTextMode"), (int*)(&m_graphics_text_mode), GraphicsTextModeWithHelp);

	config.Read(_T("DxfMakeSketch"), &HeeksDxfRead::m_make_as_sketch, true);
	config.Read(_T("DxfIgnoreErrors"), &HeeksDxfRead::m_ignore_errors, false);

	config.Read(_T("ViewUnits"), &m_view_units);
	config.Read(_T("FaceToSketchDeviation"), &(FaceToSketchTool::deviation));

	config.Read(_T("MinCorrelationFactor"), &m_min_correlation_factor);
	config.Read(_T("MaxScaleThreshold"), &m_max_scale_threshold);
	config.Read(_T("NumberOfSamplePoints"), &m_number_of_sample_points);
	config.Read(_T("CorrelateByColor"), &m_correlate_by_color);

#ifndef WIN32
	config.Read(_T("FontPaths"), &m_font_paths, _T("/usr/share/qcad/fonts"));
#endif
	config.Read(_T("STLFacetTolerance"), &m_stl_facet_tolerance, 0.1);

	config.Read(_T("AutoSaveInterval"), (int *) &m_auto_save_interval, 0);
	if (m_auto_save_interval > 0)
	{
		m_pAutoSave = std::auto_ptr<CAutoSave>(new CAutoSave(m_auto_save_interval));
	} // End if - then
	config.Read(_T("ExtrudeToSolid"), &m_extrude_to_solid);
	config.Read(_T("RevolveAngle"), &m_revolve_angle);
	config.Read(_T("SolidViewMode"), (int*)(&m_solid_view_mode), 0);

	config.Read(_T("InputUsesModalDialog"), &m_input_uses_modal_dialog, true);
	config.Read(_T("DraggingMovesObjects"), &m_dragging_moves_objects, true);
	config.Read(_T("STLSaveBinary"), &m_stl_save_as_binary, true);
	config.Read(_T("MouseMoveHighlighting"), &m_mouse_move_highlighting, true);
	{
		int color = HeeksColor(128, 255, 0).COLORREF_color();
		config.Read(_T("HighlightColor"), &color);
		m_highlight_color = HeeksColor((long)color);
	}
	config.Read(_T("SketchReorderTolerance"), &m_sketch_reorder_tol, 0.01);
	config.Read(_T("StlSolidRandomColors"), &m_stl_solid_random_colors, false);

	HDimension::ReadFromConfig(config);

	m_ruler->ReadFromConfig(config);

	GetRecentFilesProfileString();

	wxImage::AddHandler(new wxPNGHandler);
	m_current_viewport = NULL;
#ifdef PYHEEKSCAD
	m_frame = NULL;
#else
	m_frame = new CHeeksFrame( wxT( "HeeksCAD free Solid Modelling software based on Open CASCADE" ), wxPoint(posx, posy), wxSize(width, height));

// wxWidgets uses .xpm as icons, except under Windows where .ico are used
#ifndef __WXMSW__
	// So, we include xpm...
	// Note: wxWidgets wxICON() macro assumes that a HeeksCAD_xpm char* is in scope
	#include "../icons/HeeksCAD.xpm"
#endif

	m_frame->SetIcon(wxICON(HeeksCAD));
#endif

	// NOTE: A side-effect of calling the SetInputMode() method is
	// that the GetOptions() method is called.  To that end, all
	// configuration settings should be read BEFORE this point.
	SetInputMode(m_select_mode);
	if(m_frame)
	{
		m_frame->Show(TRUE);
		SetTopWindow(m_frame);
	}

	OnNewOrOpen(false,wxNO);
	ClearHistory();
	SetLikeNewFile();
	SetFrameTitle();

#ifndef PYHEEKSCAD
	if ((m_pAutoSave.get() != NULL) && (m_pAutoSave->AutoRecoverRequested()))
	{
		m_pAutoSave->Recover();
	}
	else
	{
		// Open the file passed in the command line argument
		wxCmdLineEntryDesc cmdLineDesc[2];
		cmdLineDesc[0].kind = wxCMD_LINE_PARAM;
		cmdLineDesc[0].shortName = NULL;
		cmdLineDesc[0].longName = NULL;
#if wxCHECK_VERSION(3, 0, 0)
		cmdLineDesc[0].description = "input files";
#else
		cmdLineDesc[0].description = _T("input files");
#endif
		cmdLineDesc[0].type = wxCMD_LINE_VAL_STRING;
		cmdLineDesc[0].flags = wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE;

		cmdLineDesc[1].kind = wxCMD_LINE_NONE;

		//gets the passed files from cmd line
		wxCmdLineParser parser (cmdLineDesc, argc, argv);

		// get filenames from the commandline
		if (parser.Parse() == 0)
		{
			for(unsigned int i = 0; i<parser.GetParamCount(); i++)
			{
				wxString param = parser.GetParam(i);
#ifdef WIN32
				if(!(param.Lower().EndsWith(_T(".dll")) || param.Lower().EndsWith(_T(".pyd"))))
#else
				if(!(param.Lower().EndsWith(_T(".so"))))
#endif
				{
					OnBeforeNewOrOpen(true, wxOK);
					OpenFile(parser.GetParam(i));
					OnNewOrOpen(true, wxOK);
				}
			}
		}
	}
	//#define USE_DEBUG_WXPATH  
	#ifdef USE_DEBUG_WXPATH
		// this next bit is just to help debug the icons problem
		// the wxStandardPaths class might be useful for this
		// look here for docs:
		// http://docs.wxwidgets.org/trunk/classwx_standard_paths.html
#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& sp = wxStandardPaths::Get();
#else
		wxStandardPaths sp;
#endif
		//sp.SetInstallPrefix(_T("/myPrefixDirectory")); //set the prefix directory here, if needed
		wprintf(_T("system configs directory: ") + sp.GetConfigDir()  + _T("\n"));
		wprintf(_T("applications global data directory: ") + sp.GetDataDir()  + _T("\n"));
		wprintf(_T("documents directory: ") + sp.GetDocumentsDir()  + _T("\n"));
		wprintf(_T("executable path: ") + sp.GetExecutablePath()  + _T("\n"));
		wprintf(_T("install prefix: ") + sp.GetInstallPrefix()  + _T("\n"));
		wprintf(_T("Local data directory: ") + sp.GetLocalDataDir()  + _T("\n"));
		wprintf(_T("plugins directory: ") + sp.GetPluginsDir()  + _T("\n"));
		wprintf(_T("resource directory: ") + sp.GetResourcesDir()  + _T("\n"));
		wprintf(_T("temp directory: ") + sp.GetTempDir()  + _T("\n"));
		wprintf(_T("user config directory: ") + sp.GetUserConfigDir()  + _T("\n"));
		wprintf(_T("user-dependent application data directory: ") + sp.GetUserDataDir()  + _T("\n"));
		wprintf(_T("user local data directory: ") + sp.GetUserLocalDataDir()  + _T("\n"));
	#endif
#endif
	
	return TRUE;
}



void HeeksCADapp::WriteConfig()
{
    HeeksConfig config;
	config.Write(_T("DrawEnd"), digitize_end);
	config.Write(_T("DrawInters"), digitize_inters);
	config.Write(_T("DrawCentre"), digitize_centre);
	config.Write(_T("DrawMidpoint"), digitize_midpoint);
	config.Write(_T("DrawNearest"), digitize_nearest);
	config.Write(_T("DrawTangent"), digitize_tangent);
	config.Write(_T("DrawCoords"), digitize_coords);
	config.Write(_T("DrawScreen"), digitize_screen);
	config.Write(_T("DrawToGrid"), draw_to_grid);
	config.Write(_T("UseOldFuse"), useOldFuse);
	config.Write(_T("DrawGrid"), digitizing_grid);
	config.Write(_T("DrawRadius"), digitizing_radius);
	for(int i = 0; i<NUM_BACKGROUND_COLORS; i++)
	{
		wxString key = wxString::Format(_T("BackgroundColor%d"), i);
		config.Write(key, background_color[i].COLORREF_color());
	}
	config.Write(_T("BackgroundMode"), (int)m_background_mode);
	config.Write(_T("FaceSelectionColor"), face_selection_color.COLORREF_color());
	config.Write(_T("CurrentColor"), wxString::Format( _T("%d %d %d"), current_color.red, current_color.green, current_color.blue));
	config.Write(_T("RotateMode"), m_rotate_mode);
	config.Write(_T("Antialiasing"), m_antialiasing);
	config.Write(_T("GridMode"), grid_mode);
	config.Write(_T("m_light_push_matrix"), m_light_push_matrix);
	config.Write(_T("WheelForwardAway"), mouse_wheel_forward_away);
	config.Write(_T("ZoomingReversed"), ViewZooming::m_reversed);
	config.Write(_T("CtrlDoesRotate"), ctrl_does_rotate);
	config.Write(_T("DrawDatum"), m_show_datum_coords_system);
	config.Write(_T("DrawDatumSolid"), m_show_datum_coords_system);
	config.Write(_T("DatumSize"), CoordinateSystem::size);
	config.Write(_T("DatumSizeIsPixels"), CoordinateSystem::size_is_pixels);
	config.Write(_T("DrawRuler"), m_show_ruler);
	config.Write(_T("RegularShapesMode"), (int)(regular_shapes_drawing.m_mode));
	config.Write(_T("RegularShapesNSides"), regular_shapes_drawing.m_number_of_side_for_polygon);
	config.Write(_T("RegularShapesRectRad"), regular_shapes_drawing.m_rect_radius);
	config.Write(_T("RegularShapesObRad"), regular_shapes_drawing.m_obround_radius);
	config.Write(_T("ExtrudeRemovesSketches"), m_extrude_removes_sketches);
	config.Write(_T("LoftRemovesSketches"), m_loft_removes_sketches);
	config.Write(_T("GraphicsTextMode"), (int)m_graphics_text_mode);
	config.Write(_T("DxfMakeSketch"), HeeksDxfRead::m_make_as_sketch);
	config.Write(_T("DxfIgnoreErrors"), HeeksDxfRead::m_ignore_errors);
	config.Write(_T("FaceToSketchDeviation"), FaceToSketchTool::deviation);

	config.Write(_T("MinCorrelationFactor"), m_min_correlation_factor);
	config.Write(_T("MaxScaleThreshold"), m_max_scale_threshold);
	config.Write(_T("NumberOfSamplePoints"), m_number_of_sample_points);
	config.Write(_T("CorrelateByColor"), m_correlate_by_color);

#ifndef WIN32
	config.Write(_T("FontPaths"), m_font_paths);
#endif
	config.Write(_T("STLFacetTolerance"), m_stl_facet_tolerance);
	config.Write(_T("AutoSaveInterval"), m_auto_save_interval);
	config.Write(_T("ExtrudeToSolid"), m_extrude_to_solid);
	config.Write(_T("RevolveAngle"), m_revolve_angle);
	config.Write(_T("SolidViewMode"), (int)m_solid_view_mode);
	config.Write(_T("STLSaveBinary"), m_stl_save_as_binary);

	config.Write(_T("MouseMoveHighlighting"), m_mouse_move_highlighting);
	config.Write(_T("HighlightColor"), m_highlight_color.COLORREF_color());
	config.Write(_T("StlSolidRandomColors"), m_stl_solid_random_colors);

	HDimension::WriteToConfig(config);

	m_ruler->WriteToConfig(config);

	WriteRecentFilesProfileString(config);

}

int HeeksCADapp::OnExit(){

	if (m_pAutoSave.get() != NULL)
	{
		delete m_pAutoSave.release();
		m_pAutoSave = std::auto_ptr<CAutoSave>(NULL);
	}

    WriteConfig();

	delete history;
	history = NULL;

	for(std::list<Plugin>::iterator It = m_loaded_libraries.begin(); It != m_loaded_libraries.end(); It++){
		wxDynamicLibrary* shared_library = It->dynamic_library;
		delete shared_library;
	}
	m_loaded_libraries.clear();

	int result = wxApp::OnExit();
	return result;
}

void HeeksCADapp::SetInputMode(CInputMode *new_mode){
	if(!new_mode)return;
	if(m_frame)m_current_viewport->EndDrawFront();
	if(new_mode->OnModeChange()){
		input_mode_object = new_mode;
	}
	else{
		input_mode_object = m_select_mode;
	}
	if(m_frame && m_frame->m_input_canvas)m_frame->RefreshInputCanvas();
	if(m_frame && m_frame->m_options)m_frame->RefreshOptions();
	if(m_graphics_text_mode != GraphicsTextModeNone)Repaint();
}

void HeeksCADapp::FindMarkedObject(const wxPoint &point, MarkedObject* marked_object){
	m_current_viewport->FindMarkedObject(point, marked_object);
}

void HeeksCADapp::CreateLights(void)
{
	GLfloat amb[4] =  {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat dif[4] =  {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat spec[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat pos[4] = {0.5f, 0.5f, 0.5f, 0.0f};
    GLfloat lmodel_amb[] = { 0.2f, 0.2f, 0.2f, 1.0 };
    GLfloat local_viewer[] = { 0.0 };
	if(m_light_push_matrix){
		glPushMatrix();
		glLoadIdentity();
	}
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_amb);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_viewer);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,pos);
	if(m_light_push_matrix){
		glPopMatrix();
	}
    glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHT3);
	glDisable(GL_LIGHT4);
	glDisable(GL_LIGHT5);
	glDisable(GL_LIGHT6);
	glDisable(GL_LIGHT7);
}

void HeeksCADapp::DestroyLights(void)
{
    glDisable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
    glDisable(GL_AUTO_NORMAL);
    glDisable(GL_NORMALIZE);
}

void HeeksCADapp::Reset(){
	m_marked_list->Clear(true);
	m_marked_list->Reset();
	std::set<Observer*>::iterator It;
	for(It = observers.begin(); It != observers.end(); It++){
		Observer *ov = *It;
		ov->Clear();
	}
	Clear();
	EndHistory();
	delete history;
	history = new MainHistory;
	m_current_coordinate_system = NULL;
	m_doing_rollback = false;
	gp_Vec vy(0, 1, 0), vz(0, 0, 1);
	m_current_viewport->m_view_point.SetView(vy, vz, 6);
	m_filepath = wxString(_("Untitled")) + _T(".heeks");
	m_untitled = true;
	m_hidden_for_drag.clear();
	m_show_grippers_on_drag = true;
	*m_ruler = HRuler();
	SetInputMode(m_select_mode);

	ResetIDs();
}

static bool undoably_for_ReadSTEPFileFromXMLElement = false;
static HeeksObj* paste_into_for_ReadSTEPFileFromXMLElement = NULL;

static HeeksObj* ReadSTEPFileFromXMLElement(TiXmlElement* pElem)
{
	std::map<int, CShapeData> index_map;

	// get the children ( an index map)
	for(TiXmlElement* subElem = TiXmlHandle(pElem).FirstChildElement().Element(); subElem; subElem = subElem->NextSiblingElement())
	{
		std::string subname(subElem->Value());
		if(subname == std::string("index_map"))
		{
			// loop through all the child elements, looking for index_pair items
			for(TiXmlElement* subsubElem = TiXmlHandle(subElem).FirstChildElement().Element(); subsubElem; subsubElem = subsubElem->NextSiblingElement())
			{
				std::string subsubname(subsubElem->Value());
				if(subsubname == std::string("index_pair"))
				{
					int index = -1;
					CShapeData shape_data;

					// get the attributes
					for(TiXmlAttribute* a = subsubElem->FirstAttribute(); a; a = a->Next())
					{
						std::string attr_name(a->Name());
						if(attr_name == std::string("index")){index = a->IntValue();}
						else if(attr_name == std::string("id")){shape_data.m_id = a->IntValue();}
						else if(attr_name == std::string("title")){shape_data.m_title.assign(Ctt(a->Value()));}
						else if(attr_name == std::string("title_from_id")){shape_data.m_title_made_from_id = (a->IntValue() != 0);}
						else if(attr_name == std::string("solid_type")){shape_data.m_solid_type = (SolidTypeEnum)(a->IntValue());}
						else if(attr_name == std::string("vis")){shape_data.m_visible = (a->IntValue() != 0);}
						else shape_data.m_xml_element.SetAttribute(a->Name(), a->Value());
					}

					// get face ids
					for(TiXmlElement* faceElem = TiXmlHandle(subsubElem).FirstChildElement("face").Element(); faceElem; faceElem = faceElem->NextSiblingElement("face"))
					{
						int id = 0;
						faceElem->Attribute("id", &id);
						shape_data.m_face_ids.push_back(id);
					}

					// get edge ids
					for(TiXmlElement* edgeElem = TiXmlHandle(subsubElem).FirstChildElement("edge").Element(); edgeElem; edgeElem = edgeElem->NextSiblingElement("edge"))
					{
						int id = 0;
						edgeElem->Attribute("id", &id);
						shape_data.m_edge_ids.push_back(id);
					}

					// get vertex ids
					for(TiXmlElement* vertexElem = TiXmlHandle(subsubElem).FirstChildElement("vertex").Element(); vertexElem; vertexElem = vertexElem->NextSiblingElement("vertex"))
					{
						int id = 0;
						vertexElem->Attribute("id", &id);
						shape_data.m_vertex_ids.push_back(id);
					}

					if(index != -1)index_map.insert(std::pair<int, CShapeData>(index, shape_data));
				}
			}
		}
		else if(subname == std::string("file_text"))
		{
			const char* file_text = subElem->GetText();
			if(file_text)
			{
#if wxCHECK_VERSION(3, 0, 0)
				wxStandardPaths& sp = wxStandardPaths::Get();
#else
				wxStandardPaths sp;
#endif
				sp.GetTempDir();
				wxFileName temp_file(sp.GetTempDir().c_str(), _T("temp_HeeksCAD_STEP_file.step") );
				{
#if wxUSE_UNICODE
#ifdef __WXMSW__
					wofstream ofs(Ttc(temp_file.GetFullPath().c_str()));
#else
					wofstream ofs(Ttc(temp_file.GetFullPath().c_str()));
#endif
#else
					ofstream ofs(temp_file.GetFullPath());
#endif
					ofs<<file_text;
				}
				CShape::ImportSolidsFile(temp_file.GetFullPath(), undoably_for_ReadSTEPFileFromXMLElement, &index_map, paste_into_for_ReadSTEPFileFromXMLElement);
			}
		}
	}

	// get the attributes
	for(TiXmlAttribute* a = pElem->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(name == "text")
		{
#if wxCHECK_VERSION(3, 0, 0)
			wxStandardPaths& sp = wxStandardPaths::Get();
#else
			wxStandardPaths sp;
#endif
			sp.GetTempDir();
			wxFileName temp_file( sp.GetTempDir().c_str(), _T("temp_HeeksCAD_STEP_file.step") );
			{
				ofstream ofs(Ttc(temp_file.GetFullPath().c_str()));
				ofs<<a->Value();
			}
			CShape::ImportSolidsFile(temp_file.GetFullPath(),undoably_for_ReadSTEPFileFromXMLElement, &index_map, paste_into_for_ReadSTEPFileFromXMLElement);
		}
	}

	return NULL;
}

void HeeksCADapp::InitializeXMLFunctions()
{
	// set up function map
	if(xml_read_fn_map.size() == 0)
	{
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Line", HLine::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Arc", HArc::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "InfiniteLine", HILine::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Circle", HCircle::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Point", HPoint::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Image", HImage::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Sketch", CSketch::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "STEP_file", ReadSTEPFileFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "STLSolid", CStlSolid::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "CoordinateSystem", CoordinateSystem::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Text", HText::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Dimension", HDimension::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Ellipse", HEllipse::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Spline", HSpline::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Group", CGroup::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "OrientationModifier", COrientationModifier::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Gear", HGear::ReadFromXMLElement ) );
		xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( "Area", HArea::ReadFromXMLElement ) );
	}
}

void HeeksCADapp::RegisterReadXMLfunction(const char* type_name, HeeksObj*(*read_xml_function)(TiXmlElement* pElem))
{
	if(xml_read_fn_map.find(type_name) != xml_read_fn_map.end()){
		wxMessageBox(_T("Error - trying to register an XML read function for an existing type"));
		return;
	}
	xml_read_fn_map.insert( std::pair< std::string, HeeksObj*(*)(TiXmlElement* pElem) > ( type_name, read_xml_function ) );
}

HeeksObj* HeeksCADapp::ReadXMLElement(TiXmlElement* pElem)
{
	std::string name(pElem->Value());

	std::map< std::string, HeeksObj*(*)(TiXmlElement* pElem) >::iterator FindIt = xml_read_fn_map.find( name );
	HeeksObj* object = NULL;
	if(FindIt != xml_read_fn_map.end())
	{
		object = (*(FindIt->second))(pElem);
	}
	else
	{
		object = HXml::ReadFromXMLElement(pElem);
	}

	if (object != NULL)
	{
		// Check to see if we already have an object for this type/id pair.  If so, use the existing one instead.
		//
		// NOTE: This would be better if another ObjList pointer was passed in and we checked the objects in that
		// ObjList for pre-existing elements rather than going to the global one.  This would allow imported data
		// (i.e. data read from another file and used to augment the current data) to work as well as the scenario
		// where we are reading into an empty memory block. (new data)

		HeeksObj *existing = NULL;

		existing = GetIDObject(object->GetIDGroupType(), object->m_id);

		if ((existing != NULL) && (existing != object))
		{
			// There was a pre-existing version of this type/id pair.  Don't replace it with another one.
			delete object;
			return(existing);
		}
		else
		{
			// It's new.  Keep this new copy.
			return object;
		}
	}

	return(object);	//  NULL
}

void HeeksCADapp::ObjectWriteBaseXML(HeeksObj *object, TiXmlElement *element)
{
	if(object->UsesID())element->SetAttribute("id", object->m_id);
	if(!object->m_visible)element->SetAttribute("vis", 0);
}

void HeeksCADapp::ObjectReadBaseXML(HeeksObj *object, TiXmlElement* element)
{
	// get the attributes
	for(TiXmlAttribute* a = element->FirstAttribute(); a; a = a->Next())
	{
		std::string name(a->Name());
		if(!this->m_inPaste && object->UsesID() && name == "id"){object->SetID(a->IntValue());}
		if(name == "vis"){object->m_visible = (a->IntValue() != 0);}
	}
}

void HeeksCADapp::OpenXMLFile(const wxChar *filepath, HeeksObj* paste_into, HeeksObj* paste_before, bool undoably, bool show_error)
{
	TiXmlDocument doc(Ttc(filepath));
	if (!doc.LoadFile())
	{
		if(show_error && doc.Error())
		{
			wxString msg(filepath);
			msg << wxT(": ") << Ctt(doc.ErrorDesc());
			wxMessageBox(msg);
		}
		return;
	}

	undoably_for_ReadSTEPFileFromXMLElement = undoably;
	paste_into_for_ReadSTEPFileFromXMLElement = paste_into;

	TiXmlHandle hDoc(&doc);
	TiXmlElement* pElem;
	TiXmlNode* root = &doc;

	pElem=hDoc.FirstChildElement().Element();
	if (!pElem) return;
	std::string name(pElem->Value());
	if(name == "HeeksCAD_Document")
	{
		root = pElem;
	}

	ObjectReferences_t unique_set;

	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	std::list<HeeksObj*> objects;
	for(pElem = root->FirstChildElement(); pElem;	pElem = pElem->NextSiblingElement())
	{
		HeeksObj* object = ReadXMLElement(pElem);
		if(object)
		{
			objects.push_back(object);
		}
	}

	if(objects.size() > 0)
	{
		HeeksObj* add_to = this;
		if(paste_into)add_to = paste_into;
		for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
		{
			HeeksObj* object = *It;
			object->ReloadPointers();

			while(1)
			{
				if(add_to->CanAdd(object) && object->CanAddTo(add_to))
				{
					if(object->OneOfAKind())
					{
						bool one_found = false;
						for(HeeksObj* child = add_to->GetFirstChild(); child; child = add_to->GetNextChild())
						{
							if(child->GetType() == object->GetType())
							{
								child->CopyFrom(object);
								one_found = true;
								break;
							}
						}
						if(!one_found)
						{
							add_to->Add(object, paste_before);
							if(m_inPaste)WasAdded(object);
						}
					}
					else
					{
						add_to->Add(object, paste_before);
						if(m_inPaste)WasAdded(object);
					}
					break;
				}
				else if(paste_into == NULL)
				{
					// can't add normally, look for preferred paste target
					add_to = object->PreferredPasteTarget();
					if(add_to == NULL || add_to == this)// already tried
						break;
				}
				else break;
			}
		}
	}
	setlocale(LC_NUMERIC, oldlocale);

	CGroup::MoveSolidsToGroupsById(this);
}

/* static */ void HeeksCADapp::OpenSVGFile(const wxChar *filepath)
{
	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	HeeksSvgRead svgread(filepath,true);
	setlocale(LC_NUMERIC, oldlocale);
}

/* static */ void HeeksCADapp::OpenSTLFile(const wxChar *filepath)
{
	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	HeeksColor c(128, 128, 128);
	CStlSolid* new_object = new CStlSolid(filepath, &c);
	wxGetApp().AddUndoably(new_object, NULL, NULL);
	setlocale(LC_NUMERIC, oldlocale);
}

/* static */ void HeeksCADapp::OpenDXFFile(const wxChar *filepath )
{
	char oldlocale[1000];
	strcpy(oldlocale, setlocale(LC_NUMERIC, "C"));

	bool try_again = false;
	try {
		HeeksDxfRead dxf_file(filepath, true);
		dxf_file.DoRead(HeeksDxfRead::m_ignore_errors);
	} catch(Standard_Failure)
	{
		int response = wxMessageBox(_("OpenCascade failures occured during DXF read processing.  Would you like to import again and ignore the errors?"), _("DXF Read"), wxYES_NO);
		if (response == wxYES)
		{
			try_again = true;
		}
	}

	if (try_again)
	{
		try {
			HeeksDxfRead dxf_file(filepath, true);
			dxf_file.DoRead(true);
		} catch(...)
		{
			wxMessageBox(_("OpenCascade failure occured during DXF read processing"));
		}
	}
	setlocale(LC_NUMERIC, oldlocale);
}

bool HeeksCADapp::OpenImageFile(const wxChar *filepath)
{
	wxString wf(filepath);
	wf.LowerCase();

	wxList handlers = wxImage::GetHandlers();
	for(wxList::iterator It = handlers.begin(); It != handlers.end(); It++)
	{
		wxImageHandler* handler = (wxImageHandler*)(*It);
		wxString ext = _T(".") + handler->GetExtension();
		if(wf.EndsWith(ext))
		{
			HImage* new_object = new HImage(filepath);
			Add(new_object, NULL);
			Repaint();
			return true;
		}
	}

	return false;
}

void HeeksCADapp::OnNewButton()
{
	int res = CheckForModifiedDoc();
	if(res != wxCANCEL)
	{
		OnBeforeNewOrOpen(false, res);
		Reset();
		OnNewOrOpen(false, res);
		ClearHistory();
		SetLikeNewFile();
		SetFrameTitle();
		Repaint();
	}
}

void HeeksCADapp::OnOpenButton()
{
	wxString default_directory = wxGetCwd();

	if (m_recent_files.size() > 0)
	{
		#ifdef WIN32
			wxString delimiter(_T("\\"));
		#else
			wxString delimiter(_T("/"));
		#endif // WIN32

		default_directory = *(m_recent_files.begin());
		int last_directory_delimiter = default_directory.Find(delimiter[0],true);
		if (last_directory_delimiter > 0)
		{
			default_directory.Remove(last_directory_delimiter);
		}
	}

    wxFileDialog dialog(m_frame, _("Open file"), default_directory, wxEmptyString, GetKnownFilesWildCardString(true, false));
    dialog.CentreOnParent();

    if (dialog.ShowModal() == wxID_OK)
    {
		int res = CheckForModifiedDoc();
		if(res != wxCANCEL)
		{
			OnBeforeNewOrOpen(true, res);
			Reset();
			if(!OpenFile(dialog.GetPath().c_str()))
			{
				wxString str = wxString(_("Invalid file type chosen")) + _T("  ") + _("expecting") + _T(" ") + GetKnownFilesCommaSeparatedList(true, false);
				wxMessageBox(str);
			}
			else
			{
				m_frame->m_graphics->OnMagExtents(true, true, 25);
				OnNewOrOpen(true, res);
				ClearHistory();
				SetLikeNewFile();
			}
		}
    }
}

bool HeeksCADapp::OpenFile(const wxChar *filepath, bool import_not_open, HeeksObj* paste_into, HeeksObj* paste_before, bool retain_filename /* = true */ )
{
	bool history_started = false;
	if(import_not_open && paste_into == NULL)
	{
		StartHistory();
		history_started = true;
	}

	m_in_OpenFile = true;
	m_file_open_or_import_type = FileOpenOrImportTypeOther;
	double file_open_matrix[16];
	if(import_not_open && m_current_coordinate_system)
	{
		extract(m_current_coordinate_system->GetMatrix(), file_open_matrix);
		m_file_open_matrix = file_open_matrix;
	}

	// returns true if file open was successful
	wxString wf(filepath);
	wf.LowerCase();

	wxString extension(filepath);
	int offset = extension.Find('.',true);
	if (offset > 0) extension.Remove(0, offset+1);
	extension.LowerCase();

	bool open_succeeded = true;

	if(wf.EndsWith(_T(".heeks")))
	{
		m_file_open_or_import_type = FileOpenTypeHeeks;
		if(import_not_open)
			m_file_open_or_import_type = FileImportTypeHeeks;
		OpenXMLFile(filepath, paste_into, paste_before, history_started);
	}
	else if(m_fileopen_handlers.find(extension) != m_fileopen_handlers.end())
	{
		(m_fileopen_handlers[extension])(filepath);
	}
	else if(wf.EndsWith(_T(".dxf")))
	{
		m_file_open_or_import_type = FileOpenOrImportTypeDxf;
		OpenDXFFile(filepath);
	}
	// check for images
	else if(OpenImageFile(filepath))
	{
	}

	// check for solid files
	else if(CShape::ImportSolidsFile(filepath, false))
	{
	}
#ifdef WIN32
	else if(wf.EndsWith(_T(".dll")) || wf.EndsWith(_T(".pyd")))
#else
	else if(wf.EndsWith(_T(".so")))
#endif
	{
		// add a plugin
	}
	else
	{
		// error
		wxString str = wxString(_("Invalid file type chosen")) + _T("  ") + _("expecting") + _T(" ") + GetKnownFilesCommaSeparatedList(true, import_not_open);
		wxMessageBox(str);
		open_succeeded = false;
	}

	if(open_succeeded && !import_not_open)
	{
	    InsertRecentFileItem(filepath);

		if(retain_filename)
		{
			m_filepath.assign(filepath);
			m_untitled = false;
			SetFrameTitle();
		}
		SetLikeNewFile();
	}

	m_file_open_matrix = NULL;
	m_in_OpenFile = false;

	if(history_started)EndHistory();

	return open_succeeded;
}

static void WriteDXFEntity(HeeksObj* object, CDxfWrite& dxf_file, const wxString parent_layer_name)
{
    wxString layer_name;

    if (parent_layer_name.Len() == 0)
    {
        layer_name << object->m_id;
    }
    else
    {
        layer_name = parent_layer_name;
    }

	switch(object->GetType())
	{
	case LineType:
		{
			HLine* l = (HLine*)object;
			double s[3], e[3];
			extract(l->A, s);
			extract(l->B, e);
			dxf_file.WriteLine(s, e, Ttc(layer_name.c_str()), l->m_thickness, l->m_extrusion_vector);
		}
		break;
	case PointType:
		{
			HPoint* p = (HPoint*)object;
			double s[3];
			extract(p->m_p, s);
			dxf_file.WritePoint(s, Ttc(layer_name.c_str()));
		}
		break;
	case ArcType:
		{
			HArc* a = (HArc*)object;
			double s[3], e[3], c[3];
			extract(a->A, s);
			extract(a->B, e);
			extract(a->C, c);
			bool dir = a->m_axis.Direction().Z() > 0;
			dxf_file.WriteArc(s, e, c, dir, Ttc(layer_name.c_str()), a->m_thickness, a->m_extrusion_vector);
		}
		break;
      case EllipseType:
                {
			HEllipse* e = (HEllipse*)object;
			double c[3];
			extract(e->C, c);
			bool dir = e->m_zdir.Z() > 0;
			double maj_r = e->m_majr;
			double min_r = e->m_minr;
			double rot = e->GetRotation();
			dxf_file.WriteEllipse(c, maj_r, min_r, rot, 0, 2 * M_PI, dir, Ttc(layer_name.c_str()), 0.0);
                }
		break;
        case CircleType:
                {
			HCircle* cir = (HCircle*)object;
			double c[3];
			extract(cir->m_axis.Location(), c);
			double radius = cir->m_radius;
			dxf_file.WriteCircle(c, radius, Ttc(layer_name.c_str()), cir->m_thickness, cir->m_extrusion_vector);
                }
		break;
	default:
		{
		    if (parent_layer_name.Len() == 0)
		    {
		        layer_name.Clear();
                if ((object->GetShortString() != NULL) && (wxString(object->GetTypeString()) != wxString(object->GetShortString())))
                {
                    layer_name << object->GetShortString();
                }
                else
                {
                    layer_name << object->m_id;   // Use the ID as a layer name so that it's unique.
                }
		    }
		    else
		    {
		        layer_name = parent_layer_name;
		    }

			for(HeeksObj* child = object->GetFirstChild(); child; child = object->GetNextChild())
			{

				// recursive
				WriteDXFEntity(child, dxf_file, layer_name);
			}
		}
	}
}

void HeeksCADapp::SaveDXFFile(const wxChar *filepath)
{
	CDxfWrite dxf_file(Ttc(filepath));
	if(dxf_file.Failed())
	{
		wxString str = wxString(_("couldn't open file")) + filepath;
		wxMessageBox(str);
		return;
	}

	// write all the objects
	for(std::list<HeeksObj*>::iterator It = m_objects.begin(); It != m_objects.end(); It++)
	{
		HeeksObj* object = *It;
		// At this level, don't assign each element to its own layer.  We only want sketch objects
		// to be located on their own layer.  This will be done from within the WriteDXFEntity() method.
		WriteDXFEntity(object, dxf_file, _T(""));
	}

	// when dxf_file goes out of scope it writes the file, see ~CDxfWrite
}

static ofstream* ofs_for_write_stl_triangle = NULL;
static double* scale_for_write_triangle = NULL;

static void write_stl_triangle(const double* x, const double* n)
{
	(*ofs_for_write_stl_triangle) << " facet normal " << n[0] << " " << n[1] << " " << n[2] << endl;
	(*ofs_for_write_stl_triangle) << "   outer loop" << endl;
	if(scale_for_write_triangle)
	{
		(*ofs_for_write_stl_triangle) << "     vertex " << x[0]*(*scale_for_write_triangle) << " " << x[1]*(*scale_for_write_triangle) << " " << x[2]*(*scale_for_write_triangle) << endl;
		(*ofs_for_write_stl_triangle) << "     vertex " << x[3]*(*scale_for_write_triangle) << " " << x[4]*(*scale_for_write_triangle) << " " << x[5]*(*scale_for_write_triangle) << endl;
		(*ofs_for_write_stl_triangle) << "     vertex " << x[6]*(*scale_for_write_triangle) << " " << x[7]*(*scale_for_write_triangle) << " " << x[8]*(*scale_for_write_triangle) << endl;
		(*ofs_for_write_stl_triangle) << "   endloop" << endl;
		(*ofs_for_write_stl_triangle) << " endfacet" << endl;
	}
	else
	{
		(*ofs_for_write_stl_triangle) << "     vertex " << x[0] << " " << x[1] << " " << x[2] << endl;
		(*ofs_for_write_stl_triangle) << "     vertex " << x[3] << " " << x[4] << " " << x[5] << endl;
		(*ofs_for_write_stl_triangle) << "     vertex " << x[6] << " " << x[7] << " " << x[8] << endl;
		(*ofs_for_write_stl_triangle) << "   endloop" << endl;
		(*ofs_for_write_stl_triangle) << " endfacet" << endl;
	}
}

static void write_py_triangle(const double* x, const double* n)
{
	(*ofs_for_write_stl_triangle) << "s.addTriangle(ocl.Triangle(ocl.Point(" << x[0] << ", " << x[1] << ", " << x[2] << "), ocl.Point(" << x[3] << ", " << x[4] << ", " << x[5] << "), ocl.Point(" << x[6] << ", " << x[7] << ", " << x[8] << ")))" << endl;
}

static void write_cpp_triangle(const double* x, const double* n)
{
	for(int i = 0; i<3; i++)
	{
		(*ofs_for_write_stl_triangle) << "glNormal3d(" << n[i*3 + 0] << ", " << n[i*3 + 1] << ", " << n[i*3 + 2] << ");" << endl;
		(*ofs_for_write_stl_triangle) << "glVertex3d(" << x[i*3 + 0] << ", " << x[i*3 + 1] << ", " << x[i*3 + 2] << ");" << endl;
	}
}

class NineFloatsThreeFloats
{
public:
	float x[9];
	float n[3];
};
static std::list<NineFloatsThreeFloats> binary_triangles;

static void write_binary_triangle(const double* x, const double* n)
{
	NineFloatsThreeFloats t;
	for(int i = 0; i<9; i++)t.x[i] = (float)(x[i]);
	for(int i = 0; i<3; i++)t.n[i] = (float)(n[i]);
	binary_triangles.push_back(t);
}

void HeeksCADapp::SaveSTLFileBinary(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance, double* scale)
{
#ifdef __WXMSW__
	ofstream ofs(filepath, ios::binary);
#else
	ofstream ofs(Ttc(filepath), ios::binary);
#endif

	// write 80 characters ( could be anything )
	char header[80] = "Binary STL file made with HeeksCAD                                     ";
	ofs.write(header, 80);

	// get all the triangles
	for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->GetTriangles(write_binary_triangle, facet_tolerance < 0 ? m_stl_facet_tolerance : facet_tolerance);
	}

	// write the number of facets
	unsigned int num_facets = binary_triangles.size();
	ofs.write((char*)(&num_facets), 4);

	for(std::list<NineFloatsThreeFloats>::iterator It = binary_triangles.begin(); It != binary_triangles.end(); It++)
	{
		NineFloatsThreeFloats t = *It;

		gp_Pnt p0(t.x[0], t.x[1], t.x[2]);
		gp_Pnt p1(t.x[3], t.x[4], t.x[5]);
		gp_Pnt p2(t.x[6], t.x[7], t.x[8]);
		gp_Vec v1(p0, p1);
		gp_Vec v2(p0, p2);
		float n[3] = {0.0f, 0.0f, 1.0f};
		try
		{
			gp_Vec norm = (v1 ^ v2).Normalized();
			n[0] = (float)(norm.X());
			n[1] = (float)(norm.Y());
			n[2] = (float)(norm.Z());
		}
		catch(...)
		{
		}

		ofs.write((char*)(n), 12);
		ofs.write((char*)(t.x), 36);
		short attr = 0;
		ofs.write((char*)(&attr), 2);
	}

	binary_triangles.clear();
}

void HeeksCADapp::SaveSTLFileAscii(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance, double* scale)
{
#ifdef __WXMSW__
	ofstream ofs(filepath);
#else
	ofstream ofs(Ttc(filepath));
#endif
	if(!ofs)
	{
		wxString str = wxString(_("couldn't open file")) + _T(" - ") + filepath;
		wxMessageBox(str);
		return;
	}
	ofs.imbue(std::locale("C"));

	ofs<<"solid"<<endl;

	// write all the objects
	ofs_for_write_stl_triangle = &ofs;
	scale_for_write_triangle = scale;
	for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->GetTriangles(write_stl_triangle, facet_tolerance < 0 ? m_stl_facet_tolerance : facet_tolerance);
	}
	scale_for_write_triangle = NULL;

	ofs<<"endsolid"<<endl;
}

class ObjFileVertex
{
public:
	int m_index;
	gp_Pnt m_p;
	ObjFileVertex(int index, const gp_Pnt& p) :m_index(index), m_p(p){}
};

class ObjFileTriangle
{
public:
	int m_v[3];
	ObjFileTriangle(int v0, int v1, int v2){ m_v[0] = v0; m_v[1] = v1; m_v[2] = v2; }
};

class ObjFileVertexManager
{
	std::map<double, std::list<ObjFileVertex*> > m_map;
	int m_next_index;
	std::list<ObjFileTriangle> m_tris;
	std::list<ObjFileVertex*> m_verts;

public:
	ObjFileVertexManager() :m_next_index(1){}
	~ObjFileVertexManager()
	{
		for (std::list<ObjFileVertex*>::iterator It = m_verts.begin(); It != m_verts.end(); It++)
		{
			ObjFileVertex* v = *It;
			delete v;
		}
	}

	int InsertVertex(const gp_Pnt& p)
	{
		ObjFileVertex* v = NULL;

		std::map<double, std::list<ObjFileVertex*> >::iterator FindIt = m_map.find(p.X());
		if (FindIt == m_map.end())
		{
			v = new ObjFileVertex(m_next_index, p);
			std::list<ObjFileVertex*> list;
			list.push_back(v);
			m_verts.push_back(v);
			m_map.insert(std::make_pair(p.X(), list));
			m_next_index++;
		}
		else
		{
			std::list<ObjFileVertex*> &list = FindIt->second;
			for (std::list<ObjFileVertex*>::iterator It = list.begin(); It != list.end(); It++)
			{
				ObjFileVertex* v = *It;
				if (v->m_p.IsEqual(p, 0.001))
					return v->m_index;
			}

			v = new ObjFileVertex(m_next_index, p);
			list.push_back(v);
			m_verts.push_back(v);
			m_next_index++;
		}
		return v->m_index;
	}

	void InsertTriangle(const double* t)
	{
		int v0 = InsertVertex(gp_Pnt(t[0], t[1], t[2]));
		int v1 = InsertVertex(gp_Pnt(t[3], t[4], t[5]));
		int v2 = InsertVertex(gp_Pnt(t[6], t[7], t[8]));
		m_tris.push_back(ObjFileTriangle(v0, v1, v2));
	}

	int GetIndex(const gp_Pnt& p)
	{
		std::map<double, std::list<ObjFileVertex*> >::iterator FindIt = m_map.find(p.X());
		if (FindIt != m_map.end())
		{
			std::list<ObjFileVertex*> &list = FindIt->second;
			for (std::list<ObjFileVertex*>::iterator It = list.begin(); It != list.end(); It++)
			{
				ObjFileVertex* v = *It;
				if (v->m_p.IsEqual(p, 0.001))
					return v->m_index;
			}
		}
		return 0;
	}

	void WriteObjFile(const std::wstring& filepath)
	{
		ofstream ofs(filepath);
		if (!ofs)
		{
			wxString str = wxString(_("couldn't open file")) + _T(" - ") + filepath;
			wxMessageBox(str);
			return;
		}
		ofs.imbue(std::locale("C"));

		// write the vertices
		for (std::list<ObjFileVertex*>::iterator It = m_verts.begin(); It != m_verts.end(); It++)
		{
			ObjFileVertex* v = *It;
			ofs << "v " << v->m_p.X() << " " << v->m_p.Y() << " " << v->m_p.Z() << endl;
		}

		// write the triangles
		for (std::list<ObjFileTriangle>::iterator It = m_tris.begin(); It != m_tris.end(); It++)
		{
			ObjFileTriangle& tri = *It;
			ofs << "f " << tri.m_v[0] << " " << tri.m_v[1] << " " << tri.m_v[2] << endl;
		}
	}
};

ObjFileVertexManager* vertex_manager_for_write_triangle = NULL;


static void add_obj_triangles(const double* x, const double* n)
{
	vertex_manager_for_write_triangle->InsertTriangle(x);
}

void HeeksCADapp::SaveOBJFileAscii(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance, double* scale)
{
	ObjFileVertexManager vertex_manager;
	vertex_manager_for_write_triangle = &vertex_manager;

	for (std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->GetTriangles(add_obj_triangles, facet_tolerance < 0 ? m_stl_facet_tolerance : facet_tolerance);
	}

	vertex_manager.WriteObjFile(filepath);
}

void HeeksCADapp::SaveSTLFile(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance, double* scale, bool binary)
{
	if(binary)SaveSTLFileBinary(objects, filepath, facet_tolerance, scale);
	else SaveSTLFileAscii(objects, filepath, facet_tolerance, scale);
}

void HeeksCADapp::SaveCPPFile(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance)
{
#ifdef __WXMSW__
	ofstream ofs(filepath);
#else
	ofstream ofs(Ttc(filepath));
#endif
	if(!ofs)
	{
		wxString str = wxString(_("couldn't open file")) + _T(" - ") + filepath;
		wxMessageBox(str);
		return;
	}
	ofs.imbue(std::locale("C"));

	ofs<<"glBegin(GL_TRIANGLES);"<<endl;

	// write all the objects
	ofs_for_write_stl_triangle = &ofs;
	for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->GetTriangles(write_cpp_triangle, facet_tolerance < 0 ? m_stl_facet_tolerance : facet_tolerance, false);
	}

	ofs<<"glEnd();"<<endl;
}

void HeeksCADapp::SavePyFile(const std::list<HeeksObj*>& objects, const wxChar *filepath, double facet_tolerance)
{
#ifdef __WXMSW__
	ofstream ofs(filepath);
#else
	ofstream ofs(Ttc(filepath));
#endif
	if(!ofs)
	{
		wxString str = wxString(_("couldn't open file")) + _T(" - ") + filepath;
		wxMessageBox(str);
		return;
	}
	ofs.imbue(std::locale("C"));

	ofs<<"s = ocl.STLSurf()"<<endl;

	// write all the objects
	ofs_for_write_stl_triangle = &ofs;
	for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->GetTriangles(write_py_triangle, facet_tolerance < 0 ? m_stl_facet_tolerance : facet_tolerance, false);
	}
}

void HeeksCADapp::SaveXMLFile(const std::list<HeeksObj*>& objects, const wxChar *filepath, bool for_clipboard)
{
	// write an xml file
	TiXmlDocument doc;
	const char *l_pszVersion = "1.0";
	const char *l_pszEncoding = "UTF-8";
	const char *l_pszStandalone = "";

	TiXmlDeclaration* decl = new TiXmlDeclaration( l_pszVersion, l_pszEncoding, l_pszStandalone);
	doc.LinkEndChild( decl );

	TiXmlNode* root = &doc;
	if(!for_clipboard)
	{
		root = new TiXmlElement( "HeeksCAD_Document" );
		doc.LinkEndChild( root );
	}

	// loop through all the objects writing them
	CShape::m_solids_found = false;
	for(std::list<HeeksObj*>::const_iterator It = objects.begin(); It != objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->WriteXML(root);
	}

	// write a step file for all the solids
	if(CShape::m_solids_found){
#if wxCHECK_VERSION(3, 0, 0)
		wxStandardPaths& sp = wxStandardPaths::Get();
#else
		wxStandardPaths sp;
#endif
		sp.GetTempDir();
		wxFileName temp_file( sp.GetTempDir().c_str(), _T("temp_HeeksCAD_STEP_file.step") );
		std::map<int, CShapeData> index_map;
		CShape::ExportSolidsFile(objects, temp_file.GetFullPath(), &index_map);

		TiXmlElement *step_file_element = new TiXmlElement( "STEP_file" );
		root->LinkEndChild( step_file_element );

		// write the index map as a child of step_file
		{
			TiXmlElement *index_map_element = new TiXmlElement( "index_map" );
			step_file_element->LinkEndChild( index_map_element );
			for(std::map<int, CShapeData>::iterator It = index_map.begin(); It != index_map.end(); It++)
			{
				TiXmlElement *index_pair_element = new TiXmlElement( "index_pair" );
				index_map_element->LinkEndChild( index_pair_element );
				int index = It->first;
				CShapeData& shape_data = It->second;
				index_pair_element->SetAttribute("index", index);
				index_pair_element->SetAttribute("id", shape_data.m_id);
				index_pair_element->SetAttribute("title", Ttc(shape_data.m_title));
				index_pair_element->SetAttribute("title_from_id", (shape_data.m_title_made_from_id?1:0));
				index_pair_element->SetAttribute("vis", shape_data.m_visible ? 1:0);
				if(shape_data.m_solid_type != SOLID_TYPE_UNKNOWN)index_pair_element->SetAttribute("solid_type", shape_data.m_solid_type);
				// get the CShapeData attributes
				for(TiXmlAttribute* a = shape_data.m_xml_element.FirstAttribute(); a; a = a->Next())
				{
					index_pair_element->SetAttribute(a->Name(), a->Value());
				}

				// write the face ids
				for(std::list<int>::iterator It = shape_data.m_face_ids.begin(); It != shape_data.m_face_ids.end(); It++)
				{
					int id = *It;
					TiXmlElement *face_id_element = new TiXmlElement( "face" );
					index_pair_element->LinkEndChild( face_id_element );
					face_id_element->SetAttribute("id", id);
				}

				// write the edge ids
				for(std::list<int>::iterator It = shape_data.m_edge_ids.begin(); It != shape_data.m_edge_ids.end(); It++)
				{
					int id = *It;
					TiXmlElement *edge_id_element = new TiXmlElement( "edge" );
					index_pair_element->LinkEndChild( edge_id_element );
					edge_id_element->SetAttribute("id", id);
				}

				// write the vertex ids
				for(std::list<int>::iterator It = shape_data.m_vertex_ids.begin(); It != shape_data.m_vertex_ids.end(); It++)
				{
					int id = *It;
					TiXmlElement *vertex_id_element = new TiXmlElement( "vertex" );
					index_pair_element->LinkEndChild( vertex_id_element );
					vertex_id_element->SetAttribute("id", id);
				}
			}
		}

		// write the step file as a string attribute of step_file
		ifstream ifs(Ttc(temp_file.GetFullPath().c_str()));

		if(!(!ifs)){
			std::string fstr;
			char str[1024];
			while(!(ifs.eof())){
				ifs.getline(str, 1022);
				strcat(str, "\n");
				fstr.append(str);
				if(!ifs)break;
			}

			TiXmlElement *file_text_element = new TiXmlElement( "file_text" );
			step_file_element->LinkEndChild( file_text_element );
			TiXmlText *text = new TiXmlText(fstr.c_str());
			text->SetCDATA(true);
			file_text_element->LinkEndChild( text );
		}
	}

	doc.SaveFile( Ttc(filepath) );
}

bool HeeksCADapp::SaveFile(const wxChar *filepath, bool use_dialog, bool update_recent_file_list, bool set_app_caption)
{
	if(use_dialog){
		wxFileDialog fd(m_frame, _("Save graphical data file"), wxEmptyString, filepath, GetKnownFilesWildCardString(false, false), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		fd.SetFilterIndex(1);
		if (fd.ShowModal() == wxID_CANCEL)return false;
		return SaveFile( fd.GetPath().c_str(), false, update_recent_file_list );
	}

	wxString wf(filepath);
	wf.LowerCase();

	if(wf.EndsWith(_T(".heeks")))
	{
		// call external OnSave functions
		for(std::list< void(*)(bool) >::iterator It = m_on_save_callbacks.begin(); It != m_on_save_callbacks.end(); It++)
		{
			void(*callbackfunc)(bool) = *It;
			(*callbackfunc)(false);
		}

		SaveXMLFile(filepath);
	}
	else if(wf.EndsWith(_T(".dxf")))
	{
		SaveDXFFile(filepath);
	}
	else if(wf.EndsWith(_T(".stl")))
	{
		SaveSTLFile(m_objects, filepath, -1.0, NULL, m_stl_save_as_binary);
	}
	else if(wf.EndsWith(_T(".cpp")))
	{
		SaveCPPFile(m_objects, filepath);
	}
	else if (wf.EndsWith(_T(".obj")))
	{
		SaveOBJFileAscii(m_objects, filepath);
	}
	else if(wf.EndsWith(_T(".py")))
	{
		SavePyFile(m_objects, filepath);
	}
	else if(CShape::ExportSolidsFile(m_objects, filepath))
	{
	}
	else
	{
		wxString str = wxString(_("Invalid file type chosen")) + _T("  ") + _("expecting") + _T(" ") + GetKnownFilesCommaSeparatedList(false, false);
		wxMessageBox(str);
		return false;
	}

	m_filepath.assign(filepath);
	m_untitled = false;

	if(update_recent_file_list)InsertRecentFileItem(filepath);
	if(set_app_caption)SetFrameTitle();
	SetLikeNewFile();

	return true;
}


void HeeksCADapp::Repaint(bool soon)
{
#ifdef PYHEEKSCAD
	if(m_current_viewport)
	{
		m_current_viewport->m_need_refresh = true;
		if(soon)m_current_viewport->m_need_update = true;
	}
#else
	if(soon)m_frame->m_graphics->RefreshSoon();
	else m_frame->m_graphics->Refresh();
#endif
}

void HeeksCADapp::RecalculateGLLists()
{
	for(HeeksObj* object = GetFirstChild(); object; object = GetNextChild()){
		object->KillGLLists();
	}
}

void HeeksCADapp::RenderDatumOrCurrentCoordSys()
{
	if(m_show_datum_coords_system || m_current_coordinate_system)
	{
		bool bright_datum = (m_current_coordinate_system == NULL);
		if(m_datum_coords_system_solid_arrows)
		{
			// make the datum appear at the front of everything, by clearing the depth buffer
			if(m_show_datum_coords_system)
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				CoordinateSystem::RenderDatum(bright_datum, true);
			}
			if(m_current_coordinate_system)
			{
				glClear(GL_DEPTH_BUFFER_BIT);
				CoordinateSystem::rendering_current = true;
				m_current_coordinate_system->glCommands(false, m_marked_list->ObjectMarked(m_current_coordinate_system), false);
				CoordinateSystem::rendering_current = false;
			}
		}
		else
		{
			// make the datum appear at the front of everything, by setting the depth range
			GLfloat save_depth_range[2];
			glGetFloatv(GL_DEPTH_RANGE, save_depth_range);
			glDepthRange(0, 0);

			if(m_current_coordinate_system)
			{
				CoordinateSystem::rendering_current = true;
				m_current_coordinate_system->glCommands(false, m_marked_list->ObjectMarked(m_current_coordinate_system), false);
				CoordinateSystem::rendering_current = false;
			}
			if(m_show_datum_coords_system)
			{
				CoordinateSystem::RenderDatum(bright_datum, false);
			}

			// restore the depth range
			glDepthRange(save_depth_range[0], save_depth_range[1]);
		}
	}
}

void HeeksCADapp::glCommandsAll(const CViewPoint &view_point)
{

	CreateLights();
	glDisable(GL_LIGHTING);
	Material().glMaterial(1.0);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);
	glLineWidth(1);
	glDepthMask(1);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glShadeModel(GL_FLAT);
	view_point.SetPolygonOffset();

	std::list<HeeksObj*> after_others_objects;

	for(std::list<HeeksObj*>::iterator It=m_objects.begin(); It!=m_objects.end() ;It++)
	{
		HeeksObj* object = *It;
		if(object->OnVisibleLayer() && object->m_visible)
		{
			if(object->DrawAfterOthers())after_others_objects.push_back(object);
			else
			{
				object->glCommands(false, m_marked_list->ObjectMarked(object), false);
			}
		}
	}

	// draw any last_objects
	for(std::list<HeeksObj*>::iterator It = after_others_objects.begin(); It != after_others_objects.end(); It++)
	{
		HeeksObj* object = *It;
		object->glCommands(false, m_marked_list->ObjectMarked(object), false);
	}

	glDisable(GL_POLYGON_OFFSET_FILL);
	for(std::list< void(*)() >::iterator It = m_on_glCommands_list.begin(); It != m_on_glCommands_list.end(); It++)
	{
		void(*callbackfunc)() = *It;
		(*callbackfunc)();
	}
	glEnable(GL_POLYGON_OFFSET_FILL);

	input_mode_object->OnRender();
	if(m_transform_gl_list)
	{
        glPushMatrix();
		double m[16];
		extract_transposed(m_drag_matrix, m);
		glMultMatrixd(m);
		glCallList(m_transform_gl_list);
		glPopMatrix();
	}

	// draw the ruler
	if(m_show_ruler && m_ruler->m_visible)
	{
		m_ruler->glCommands(false, false, false);
	}

	// draw the grid
	glDepthFunc(GL_LESS);
	RenderGrid(&view_point);
	glDepthFunc(GL_LEQUAL);

	// draw the datum
	RenderDatumOrCurrentCoordSys();

	DestroyLights();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL );
	if(m_hidden_for_drag.size() == 0 || !m_show_grippers_on_drag)m_marked_list->GrippersGLCommands(false, false);

	// draw the input mode text on the top
	if(m_graphics_text_mode != GraphicsTextModeNone)
	{
		wxString screen_text1, screen_text2;

		if(input_mode_object && input_mode_object->GetTitle())
		{
			screen_text1.Append(input_mode_object->GetTitle());
			screen_text1.Append(_T("\n"));
		}
		if(m_graphics_text_mode == GraphicsTextModeWithHelp && input_mode_object)
		{
			const wxChar* help_str = input_mode_object->GetHelpText();
			if(help_str)
			{
				screen_text2.Append(help_str);
			}
		}
		render_screen_text(screen_text1, screen_text2, false);
	}
}

void HeeksCADapp::OnInputModeTitleChanged()
{
	if(m_graphics_text_mode != GraphicsTextModeNone)
	{
		Repaint();
	}
}

void HeeksCADapp::OnInputModeHelpTextChanged()
{
	if(m_graphics_text_mode == GraphicsTextModeWithHelp)
	{
		Repaint();
	}
}

void HeeksCADapp::glCommands(bool select, bool marked, bool no_color)
{
	// this is called when select is true
	ObjList::glCommands(select, marked, no_color);

	// draw the ruler
	if(m_show_ruler)
	{
		if (select)SetPickingColor(m_ruler->GetIndex());
		m_ruler->glCommands(select, false, no_color);
	}
}

void HeeksCADapp::GetBox(CBox &box){
	CBox temp_box;
	ObjList::GetBox(temp_box);
	if(temp_box.m_valid && temp_box.Radius() > 0.000001)
		box.Insert(temp_box);
}

double HeeksCADapp::GetPixelScale(void){
	return m_current_viewport->m_view_point.m_pixel_scale;
}

bool HeeksCADapp::IsModified(void){
	if(history->IsModified())return true;

	for(std::list< bool(*)() >::iterator It = m_is_modified_callbacks.begin(); It != m_is_modified_callbacks.end(); It++)
	{
		bool(*callbackfunc)() = *It;
		bool is_modified = (*callbackfunc)();
		if(is_modified)
		{
			return true;
		}
	}

	return false;
}

void HeeksCADapp::SetAsModified(){
	history->SetAsModified();
}

void HeeksCADapp::SetLikeNewFile(void){
	history->SetLikeNewFile();
}

void HeeksCADapp::ClearHistory(void){
	history->ClearFromFront();
	history->SetLikeNewFile();
}

static void AddToolListWithSeparator(std::list<Tool*> &l, std::list<Tool*> &temp_l)
{
	if(temp_l.size()>0)
	{
		if(l.size()>0)l.push_back(new MenuSeparator);
		std::list<Tool*>::iterator FIt;
		for(FIt = temp_l.begin(); FIt != temp_l.end(); FIt++)
			l.push_back(*FIt);
	}
}

static std::vector<ToolIndex> tool_index_list;

class CFullScreenTool : public Tool
{
public:
	// Tool's virtual functions
	const wxChar* GetTitle(){
		if (wxGetApp().m_frame->IsFullScreen()) return _("Exit Full Screen Mode");
		else return _("Show Full Screen");
	}
	void Run(){
		wxGetApp().m_frame->ShowFullScreen(!wxGetApp().m_frame->IsFullScreen());
	}
	wxString BitmapPath(){return _T("fullscreen");}
};

class MoveOrCopyTool: public Tool
{
	bool m_move_not_copy;
public:
	static HeeksObj* paste_into;
	static HeeksObj* paste_before;

	MoveOrCopyTool(bool move_not_copy):m_move_not_copy(move_not_copy){}
	void Run(){
		if(m_move_not_copy)
		{
			// cut the objects
			wxGetApp().m_marked_list->CutSelectedItems();
		}
		else
		{
			// cpyut the objects
			wxGetApp().m_marked_list->CopySelectedItems();
		}

		// paste the objects
		wxGetApp().Paste(paste_into, paste_before);
	}
	const wxChar* GetTitle(){return m_move_not_copy ? _("Move here") : _("Copy here");}
	wxString BitmapPath(){return m_move_not_copy ? _T("move") : _T("copy");}
};

static MoveOrCopyTool move_tool(true);
static MoveOrCopyTool copy_tool(false);
HeeksObj* MoveOrCopyTool::paste_into(NULL);
HeeksObj* MoveOrCopyTool::paste_before(NULL);

void HeeksCADapp::DoMoveOrCopyDropDownMenu(wxWindow *wnd, const wxPoint &point, MarkedObject* marked_object, HeeksObj* paste_into, HeeksObj* paste_before)
{
	tool_index_list.clear();
	wxPoint new_point = point;
	wxMenu menu;
	std::list<Tool*> f_list;

	MoveOrCopyTool::paste_into = paste_into;
	MoveOrCopyTool::paste_before = paste_before;
	f_list.push_back(&move_tool);
	f_list.push_back(&copy_tool);

	for (std::list<Tool*>::iterator FIt = f_list.begin(); FIt != f_list.end(); FIt++)
		m_frame->AddToolToListAndMenu(*FIt, tool_index_list, &menu);
	wnd->PopupMenu(&menu, point);
}

class UnsetCoordSystemActive:public Tool{
	// set world coordinate system active again
public:
	void Run(){
		wxGetApp().m_current_coordinate_system = NULL;
		wxGetApp().m_frame->RefreshProperties();
		wxGetApp().Repaint();
	}
	const wxChar* GetTitle(){return _("Set world coordinate system active");}
	wxString BitmapPath(){return _T("unsetcoordsys");}
};

static UnsetCoordSystemActive coord_system_unset;

std::list<Tool *> dynamic_tool_menu_options;


class AddPointsAtIntersections : public Tool{

public:
	void Run(){
		// Intersect each selected object with all other selected objects.  Where they intersect,
		// add points objects.

		std::list<HeeksObj *> marked_list = wxGetApp().m_marked_list->list();

		for (std::list<HeeksObj *>::const_iterator itLhs = marked_list.begin(); itLhs != marked_list.end(); itLhs++)
		{
			std::list<HeeksObj *>::const_iterator itNext = itLhs;
			itNext++;
			for (std::list<HeeksObj *>::const_iterator itRhs = itNext; itRhs != marked_list.end(); itRhs++)
			{
				std::list< double > intersections;
				if ((*itLhs)->Intersects(*itRhs, &intersections) > 0)
				{
					gp_Pnt location(0.0, 0.0, 0.0);
					for (std::list<double>::iterator itValue = intersections.begin(); itValue != intersections.end(); /* increment within loop */ )
					{
						wxString verbose;

						location.SetX( *itValue );
						verbose << *itValue;

						itValue++; if (itValue == intersections.end()) break;
						location.SetY( *itValue );
						verbose << _T(",") << *itValue;

						itValue++; if (itValue == intersections.end()) break;
						location.SetZ( *itValue );
						verbose << _T(",") << *itValue;
						itValue++;

						HPoint *new_point = new HPoint( location, &(wxGetApp().current_color) );
						wxGetApp().Add(new_point, NULL);
					}

				}
			}
		}
	}
	const wxChar* GetTitle(){return(_("Add points at intersections"));}
	wxString BitmapPath(){return _T("add_intersection_points");}

private:
	wxString	m_title;
	gp_Pnt		m_location;
};

AddPointsAtIntersections add_points_at_intersections;


void HeeksCADapp::GenerateIntersectionMenuOptions( std::list<Tool*> &f_list )
{
	if (m_marked_list->list().size() > 1)
	{
		f_list.push_back(&add_points_at_intersections);
	}
}


void HeeksCADapp::GetDropDownTools(std::list<Tool*> &f_list, const wxPoint &point, MarkedObject* marked_object, bool dont_use_point_for_functions, bool control_pressed)
{
	tool_index_list.clear();
	wxPoint new_point = point;
	if(dont_use_point_for_functions){
		new_point.x = -1;
		new_point.y = -1;
	}
	std::list<Tool*> temp_f_list;

	GetTools(marked_object, f_list, new_point, control_pressed);

	m_marked_list->GetTools(marked_object, f_list, &new_point, true);

	if(!wxGetApp().m_no_creation_mode)GenerateIntersectionMenuOptions( f_list );

	temp_f_list.clear();
	if(input_mode_object)input_mode_object->GetTools(&temp_f_list, &new_point);
	AddToolListWithSeparator(f_list, temp_f_list);
	temp_f_list.clear();

	if(m_current_coordinate_system)f_list.push_back(&coord_system_unset);

#ifndef PYHEEKSCAD
	// exit full screen
	if(m_frame->IsFullScreen() && point.x>=0 && point.y>=0)temp_f_list.push_back(new CFullScreenTool);
#endif

	AddToolListWithSeparator(f_list, temp_f_list);
}

void HeeksCADapp::DoDropDownMenu(wxWindow *wnd, const wxPoint &point, MarkedObject* marked_object, bool dont_use_point_for_functions, bool control_pressed)
{
	std::list<Tool*> f_list;
	GetDropDownTools(f_list, point, marked_object, dont_use_point_for_functions, control_pressed);
	std::list<Tool*>::iterator FIt;
	wxMenu menu;
	for (FIt = f_list.begin(); FIt != f_list.end(); FIt++)
		m_frame->AddToolToListAndMenu(*FIt, tool_index_list, &menu);
	wnd->PopupMenu(&menu, point);
}

void HeeksCADapp::on_menu_event(wxCommandEvent& event)
{
	int id = event.GetId();
	if(id){
		Tool *t = tool_index_list[id - ID_FIRST_POP_UP_MENU_TOOL].m_tool;
		StartHistory();
		t->Run();
		EndHistory();
	}
}

void HeeksCADapp::DoUndoable(Undoable *u)
{
	history->DoUndoable(u);
}

bool HeeksCADapp::RollBack(void)
{
	m_doing_rollback = true;
	bool result = history->InternalRollBack();
	m_doing_rollback = false;
	return result;
}

bool HeeksCADapp::CanUndo(void)
{
	return history->CanUndo();
}

bool HeeksCADapp::CanRedo(void)
{
	return history->CanRedo();
}

bool HeeksCADapp::RollForward(void)
{
	m_doing_rollback = true;
	bool result = history->InternalRollForward();
	m_doing_rollback = false;
	return result;
}

void HeeksCADapp::StartHistory()
{
	history->StartHistory();
}

void HeeksCADapp::EndHistory(void)
{
	history->EndHistory();
}

void HeeksCADapp::ClearRollingForward(void)
{
	history->ClearFromCurPos();
}

void HeeksCADapp::RegisterObserver(Observer* observer)
{
	if (observer==NULL) return;
	observers.insert(observer);
	observer->OnChanged(&m_objects, NULL, NULL);
}

void HeeksCADapp::RemoveObserver(Observer* observer){
	observers.erase(observer);
}

void HeeksCADapp::ObserversOnChange(const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed, const std::list<HeeksObj*>* modified){
	std::set<Observer*>::iterator It;
	for(It = observers.begin(); It != observers.end(); It++){
		Observer *ov = *It;
		ov->OnChanged(added, removed, modified);
	}
}

void HeeksCADapp::ObserversMarkedListChanged(bool selection_cleared, const std::list<HeeksObj*>* added, const std::list<HeeksObj*>* removed){
	std::set<Observer*>::iterator It;
	for(It = observers.begin(); It != observers.end(); It++){
		Observer *ov = *It;
		ov->WhenMarkedListChanges(selection_cleared, added, removed);
	}
}

void HeeksCADapp::ObserversFreeze()
{
	std::set<Observer*>::iterator It;
	for(It = observers.begin(); It != observers.end(); It++){
		Observer *ov = *It;
		ov->Freeze();
	}
}

void HeeksCADapp::ObserversThaw()
{
	std::set<Observer*>::iterator It;
	for(It = observers.begin(); It != observers.end(); It++){
		Observer *ov = *It;
		ov->Thaw();
	}
}

bool HeeksCADapp::Add(HeeksObj *object, HeeksObj* prev_object)
{
	if (!ObjList::Add(object, prev_object)) return false;

	if(object->GetType() == CoordinateSystemType && (!m_in_OpenFile || (m_file_open_or_import_type !=FileOpenTypeHeeks && m_file_open_or_import_type  != FileImportTypeHeeks)))
	{
		m_current_coordinate_system = (CoordinateSystem*)object;
	}

	if(m_mark_newly_added_objects)
	{
		m_marked_list->Add(object, true);
	}

	return true;
}

void HeeksCADapp::Remove(HeeksObj* object)
{
#ifdef MULTIPLE_OWNERS
	HeeksObj* owner = object->GetFirstOwner();
	while(owner)
	{
		if(owner != this)
		{
			owner->Remove(object);
			owner->ReloadPointers();
		}
		else
			ObjList::Remove(object);
		owner = object->GetNextOwner();
	}
#else
	if(object->m_owner)
	{
		if(object->m_owner != this)
		{
			object->m_owner->Remove(object);
			object->m_owner->ReloadPointers();
		}
		else
			ObjList::Remove(object);
	}
#endif
	if(object == m_current_coordinate_system)m_current_coordinate_system = NULL;
}

void HeeksCADapp::Remove(std::list<HeeksObj*> objects)
{
	ObjList::Remove(objects);
}

void HeeksCADapp::AddUndoably(HeeksObj *object, HeeksObj* owner, HeeksObj* prev_object)
{
	if(object == NULL)return;
	if(owner == NULL)owner = this;
	AddObjectTool *undoable = new AddObjectTool(object, owner, prev_object);
	DoUndoable(undoable);
}

void HeeksCADapp::AddUndoably(const std::list<HeeksObj*> &list, HeeksObj* owner)
{
	if(list.size() == 0)return;
	if(owner == NULL)owner = this;
	AddObjectsTool *undoable = new AddObjectsTool(list, owner);
	DoUndoable(undoable);
}

void HeeksCADapp::DeleteUndoably(HeeksObj *object){
	if(object == NULL)return;
	if(!object->CanBeRemoved())return;
	RemoveObjectTool *undoable = new RemoveObjectTool(object);
	DoUndoable(undoable);
}

void HeeksCADapp::DeleteUndoably(const std::list<HeeksObj*>& list)
{
	if(list.size() == 0)return;
	std::list<HeeksObj*> list2;
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		HeeksObj* object = *It;
		if(object->CanBeRemoved())list2.push_back(object);
	}
	if(list2.size() == 0)return;
	RemoveObjectsTool *undoable = new RemoveObjectsTool(list2, list2.front()->m_owner);
	DoUndoable(undoable);
}

void HeeksCADapp::CopyUndoably(HeeksObj* object, HeeksObj* copy_with_new_data)
{
	DoUndoable(new CopyObjectUndoable(object, copy_with_new_data));
}

void HeeksCADapp::TransformUndoably(HeeksObj *object, double *m)
{
	if(!object)return;
	gp_Trsf mat = make_matrix(m);
	gp_Trsf im = mat;
	im.Invert();
	TransformTool *undoable = new TransformTool(object, mat, im);
	DoUndoable(undoable);
}

void HeeksCADapp::TransformUndoably(const std::list<HeeksObj*> &list, double *m)
{
	if(list.size() == 0)return;
	gp_Trsf mat = make_matrix(m);
	gp_Trsf im = mat;
	im.Invert();
	TransformObjectsTool *undoable = new TransformObjectsTool(list, mat, im);
	DoUndoable(undoable);
}

class ReverseUndoable : public Undoable{
	HeeksObj* m_object;

public:
	ReverseUndoable(HeeksObj* object):m_object(object){}
	void Run(bool redo){CSketch::ReverseObject(m_object);}
	const wxChar* GetTitle(){return _("Reverse Object");}
	void RollBack(){CSketch::ReverseObject(m_object);}
};

void HeeksCADapp::ReverseUndoably(HeeksObj *object)
{
	DoUndoable(new ReverseUndoable(object));
}

void HeeksCADapp::EditUndoably(HeeksObj *object)
{
	HeeksObj* copy_object = object->MakeACopyWithID();
	if(copy_object->Edit())
	{
		wxGetApp().CopyUndoably(object, copy_object);
	}
	else
		delete copy_object;
}

void HeeksCADapp::Transform(std::list<HeeksObj*> objects,double *m)
{
	std::list<HeeksObj*>::iterator it;
	for(it = objects.begin(); it!= objects.end(); ++it)
	{
		(*it)->ModifyByMatrix(m);
	}
}

void HeeksCADapp::WasModified(HeeksObj *object)
{
	std::list<HeeksObj*> list;
	list.push_back(object);
	WereModified(list);
}

void HeeksCADapp::WasAdded(HeeksObj *object)
{
	std::list<HeeksObj*> list;
	list.push_back(object);
	WereAdded(list);
}

void HeeksCADapp::WasRemoved(HeeksObj *object)
{
	std::list<HeeksObj*> list;
	list.push_back(object);
	WereRemoved(list);
}

void HeeksCADapp::WereModified(const std::list<HeeksObj*>& list)
{
	if (list.size() == 0) return;
	HeeksObj* object = *(list.begin());
	if (object == NULL) return;
	ObserversOnChange(NULL, NULL, &list);
	SetAsModified();
}

void HeeksCADapp::WereAdded(const std::list<HeeksObj*>& list)
{
	if (list.size() == 0) return;
	HeeksObj* object = *(list.begin());
	if (object == NULL) return;
	ObserversOnChange(&list, NULL, NULL);
	SetAsModified();
}

void HeeksCADapp::WereRemoved(const std::list<HeeksObj*>& list)
{
	if (list.size() == 0) return;
	HeeksObj* object = *(list.begin());
	if (object == NULL) return;

	std::list<HeeksObj*> marked_remove;
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++)
	{
		object = *It;
		if(m_marked_list->ObjectMarked(object))marked_remove.push_back(object);
	}
	if(marked_remove.size() > 0)m_marked_list->Remove(marked_remove, false);

	ObserversOnChange(NULL, &list, NULL);
	SetAsModified();
}

gp_Trsf HeeksCADapp::GetDrawMatrix(bool get_the_appropriate_orthogonal)
{
	if(get_the_appropriate_orthogonal){
		// choose from the three orthoganal possibilities, the one where it's z-axis closest to the camera direction
		gp_Vec vx, vy;
		m_current_viewport->m_view_point.GetTwoAxes(vx, vy, false, 0);
		{
			gp_Pnt o(0, 0, 0);
			if(m_current_coordinate_system)o.Transform(m_current_coordinate_system->GetMatrix());
			return make_matrix(o, vx, vy);
		}
	}

	gp_Trsf mat;
	if(m_current_coordinate_system)mat = m_current_coordinate_system->GetMatrix();
	return mat;
}

void on_set_background_color0(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[0] = value;
	wxGetApp().Repaint();
}

void on_set_background_color1(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[1] = value;
	wxGetApp().Repaint();
}

void on_set_background_color2(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[2] = value;
	wxGetApp().Repaint();
}

void on_set_background_color3(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[3] = value;
	wxGetApp().Repaint();
}

void on_set_background_color4(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[4] = value;
	wxGetApp().Repaint();
}

void on_set_background_color5(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[5] = value;
	wxGetApp().Repaint();
}

void on_set_background_color6(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[6] = value;
	wxGetApp().Repaint();
}

void on_set_background_color7(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[7] = value;
	wxGetApp().Repaint();
}

void on_set_background_color8(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[8] = value;
	wxGetApp().Repaint();
}

void on_set_background_color9(HeeksColor value, HeeksObj* object)
{
	wxGetApp().background_color[9] = value;
	wxGetApp().Repaint();
}

void on_set_background_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().m_background_mode = (BackgroundMode)value;
	wxGetApp().Repaint();
	wxGetApp().m_frame->RefreshOptions();
}

void on_set_face_color(HeeksColor value, HeeksObj* object)
{
	wxGetApp().face_selection_color = value;
	wxGetApp().Repaint();
}

void on_set_current_color(HeeksColor value, HeeksObj* object)
{
	wxGetApp().current_color = value;
}

void on_set_grid_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().grid_mode = value;
	wxGetApp().Repaint();
}

void on_set_perspective(bool value, HeeksObj* object)
{
	wxGetApp().m_current_viewport->m_view_point.SetPerspective(value);
	wxGetApp().Repaint();
}

void on_set_tool_icon_size(int value, HeeksObj* object, bool from_undo_redo)
{
	int size = 16;
	switch(value)
	{
	case 0:
		size = 16;
		break;
	case 1:
		size = 20;
		break;
	case 2:
		size = 24;
		break;
	case 3:
		size = 28;
		break;
	case 4:
		size = 32;
		break;
	case 5:
		size = 48;
		break;
	case 6:
		size = 64;
		break;
	case 7:
		size = 96;
		break;

	}

	ToolImage::SetBitmapSize(size);

	wxGetApp().m_frame->OnChangeBitmapSize();
	wxGetApp().m_frame->m_aui_manager->Update();
}

void on_grid(bool onoff, HeeksObj* object)
{
	wxGetApp().draw_to_grid = onoff;
#ifndef USING_RIBBON
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->m_snap_button->m_bitmap = wxBitmap(ToolImage(wxGetApp().draw_to_grid ? _T("snap") : _T("snapgray")));
#endif
#endif
	wxGetApp().Repaint();
}

void on_useOldFuse(bool onoff, HeeksObj* object)
{
	wxGetApp().useOldFuse = onoff;
}

static void on_extrude_to_solid(bool onoff, HeeksObj* object)
{
	wxGetApp().m_extrude_to_solid = onoff;
}

static void on_revolve_angle(double value, HeeksObj* object)
{
	wxGetApp().m_revolve_angle = value;
}

void on_set_min_correlation_factor(double value, HeeksObj* object)
{
	wxGetApp().m_min_correlation_factor = value;
	wxGetApp().Repaint();
}

void on_set_max_scale_threshold(double value, HeeksObj* object)
{
	wxGetApp().m_max_scale_threshold = value;
	wxGetApp().Repaint();
}

void on_set_number_of_sample_points(int value, HeeksObj* object)
{
	wxGetApp().m_number_of_sample_points = value;
	wxGetApp().Repaint();
}

void on_set_correlate_by_color(bool value, HeeksObj* object)
{
	wxGetApp().m_correlate_by_color = value;
	wxGetApp().Repaint();
}


void on_grid_edit(double grid_value, HeeksObj* object)
{
	wxGetApp().digitizing_grid = grid_value;
	wxGetApp().Repaint();
}




void on_set_geom_tol(double value, HeeksObj* object)
{
	wxGetApp().m_geom_tol = value;
	wxGetApp().Repaint();

	TiXmlBase::SetRequiredDecimalPlaces( DecimalPlaces(value) );
}

void on_set_sketch_reorder_tol(double value, HeeksObj* object)
{
	wxGetApp().m_sketch_reorder_tol = value;
	HeeksConfig config;
	config.Write(_T("SketchReorderTolerance"), wxGetApp().m_sketch_reorder_tol);
	wxGetApp().Repaint();
}

void on_set_face_to_sketch_deviation(double value, HeeksObj* object)
{
	FaceToSketchTool::deviation = value;
}

void on_set_show_datum(bool onoff, HeeksObj* object)
{
	wxGetApp().m_show_datum_coords_system = onoff;
	wxGetApp().Repaint();
}

void on_set_solid_datum(bool onoff, HeeksObj* object)
{
	wxGetApp().m_datum_coords_system_solid_arrows = onoff;
	wxGetApp().Repaint();
}

void on_set_show_ruler(bool onoff, HeeksObj* object)
{
	wxGetApp().m_show_ruler = onoff;
	wxGetApp().Repaint();
}

void on_set_rotate_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().m_rotate_mode = value;
	if(!wxGetApp().m_rotate_mode)
	{
		gp_Vec vy(0, 1, 0), vz(0, 0, 1);
		wxGetApp().m_current_viewport->m_view_point.SetView(vy, vz, 6);
		wxGetApp().m_current_viewport->StoreViewPoint();
		wxGetApp().Repaint();
	}
}

void on_set_screen_text(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().m_graphics_text_mode = (GraphicsTextMode)value;
	wxGetApp().Repaint();
}

void on_set_antialiasing(bool value, HeeksObj* object)
{
	wxGetApp().m_antialiasing = value;
	wxGetApp().Repaint();
}

void on_set_light_push_matrix(bool value, HeeksObj* object)
{
	wxGetApp().m_light_push_matrix = value;
	wxGetApp().Repaint();
}

void on_set_reverse_mouse_wheel(bool value, HeeksObj* object)
{
	wxGetApp().mouse_wheel_forward_away = !value;
}

void on_set_stl_save_binary(bool value, HeeksObj* object)
{
	wxGetApp().m_stl_save_as_binary = value;
}

void on_set_reverse_zooming(bool value, HeeksObj* object)
{
	ViewZooming::m_reversed = value;
	wxGetApp().OnInputModeHelpTextChanged();
}

void on_set_ctrl_does_rotate(bool value, HeeksObj* object)
{
	wxGetApp().ctrl_does_rotate = value;
	wxGetApp().OnInputModeHelpTextChanged();
}

void on_intersection(bool onoff, HeeksObj* object){
	wxGetApp().digitize_inters = onoff;
#ifndef USING_RIBBON
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->m_inters_button->m_bitmap = wxBitmap(ToolImage(wxGetApp().digitize_inters ? _T("inters") : _T("intersgray")));
#endif
#endif
}

void on_centre(bool onoff, HeeksObj* object){
	wxGetApp().digitize_centre = onoff;
#ifndef USING_RIBBON
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->m_centre_button->m_bitmap = wxBitmap(ToolImage(wxGetApp().digitize_centre ? _T("centre") : _T("centregray")));
#endif
#endif
}

void on_end_of(bool onoff, HeeksObj* object){
	wxGetApp().digitize_end = onoff;
#ifndef USING_RIBBON
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->m_endof_button->m_bitmap = wxBitmap(ToolImage(wxGetApp().digitize_end ? _T("endof") : _T("endofgray")));
#endif
#endif
}

void on_mid_point(bool onoff, HeeksObj* object){
	wxGetApp().digitize_midpoint = onoff;
#ifndef USING_RIBBON
#ifdef HAVE_TOOLBARS
	wxGetApp().m_frame->m_midpoint_button->m_bitmap = wxBitmap(ToolImage(wxGetApp().digitize_midpoint ? _T("midpoint") : _T("midpointgray")));
#endif
#endif
}

void on_nearest(bool onoff, HeeksObj* object){
	wxGetApp().digitize_nearest = onoff;
}

void on_tangent(bool onoff, HeeksObj* object){
	wxGetApp().digitize_tangent = onoff;
}

void on_radius(double value, HeeksObj* object){
	wxGetApp().digitizing_radius = value;
}

void on_coords(bool onoff, HeeksObj* object){
	wxGetApp().digitize_coords = onoff;
}

void on_relative(bool onoff, HeeksObj* object){
	wxGetApp().digitize_screen = onoff;
}

static std::list<Property *> *list_for_GetOptions = NULL;

static void AddPropertyCallBack(Property* p)
{
	list_for_GetOptions->push_back(p);
}

void on_set_datum_size(double value, HeeksObj* object){
	CoordinateSystem::size = value;
	wxGetApp().Repaint();
}

void on_set_size_is_pixels(bool value, HeeksObj* object){
	CoordinateSystem::size_is_pixels = value;
	wxGetApp().Repaint();
}

void on_sel_filter_line(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_LINE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_LINE;
}

void on_sel_filter_arc(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_ARC;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_ARC;
}

void on_sel_filter_iline(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_ILINE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_ILINE;
}

void on_sel_filter_circle(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_CIRCLE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_CIRCLE;
}

void on_sel_filter_point(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_POINT;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_POINT;
}

void on_sel_filter_solid(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_SOLID;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_SOLID;
}

void on_sel_filter_stl_solid(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_STL_SOLID;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_STL_SOLID;
}

void on_sel_filter_wire(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_WIRE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_WIRE;
}

void on_sel_filter_face(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_FACE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_FACE;
}

void on_sel_filter_vertex(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_VERTEX;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_VERTEX;
}

void on_sel_filter_edge(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_EDGE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_EDGE;
}

void on_sel_filter_sketch(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_SKETCH;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_SKETCH;
}

void on_sel_filter_image(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_IMAGE;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_IMAGE;
}

void on_sel_filter_coordinate_sys(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_COORDINATE_SYSTEM;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_COORDINATE_SYSTEM;
}

void on_sel_filter_text(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_TEXT;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_TEXT;
}

void on_sel_filter_dimension(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_DIMENSION;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_DIMENSION;
}

void on_sel_filter_ruler(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_RULER;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_RULER;
}
void on_sel_filter_pad(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_PAD;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_PAD;
}

void on_sel_filter_part(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_PART;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_PART;
}

void on_sel_filter_pocket(bool value, HeeksObj* object){
	if(value)wxGetApp().m_marked_list->m_filter |= MARKING_FILTER_POCKETSOLID;
	else wxGetApp().m_marked_list->m_filter &= ~MARKING_FILTER_POCKETSOLID;
}

void on_dxf_make_sketch(bool value, HeeksObj* object){
	HeeksDxfRead::m_make_as_sketch = value;
	HeeksConfig config;
	config.Write(_T("ImportDxfAsSketches"), HeeksDxfRead::m_make_as_sketch);
}

void on_sel_dxf_read_errors(bool value, HeeksObj* object){
	HeeksDxfRead::m_ignore_errors = value;

	HeeksConfig config;
	config.Write(_T("IgnoreDxfReadErrors"), HeeksDxfRead::m_ignore_errors);
}

void on_dxf_read_points(bool value, HeeksObj* object){
	HeeksDxfRead::m_read_points = value;

	HeeksConfig config;
	config.Write(_T("DxfReadPoints"), HeeksDxfRead::m_read_points);
}

void on_edit_layer_name_suffixes_to_discard(const wxChar* value, HeeksObj* object)
{
	HeeksDxfRead::m_layer_name_suffixes_to_discard = value;

	HeeksConfig config;
	config.Write(_T("LayerNameSuffixesToDiscard"), HeeksDxfRead::m_layer_name_suffixes_to_discard);
}

void on_add_uninstanced_blocks(bool value, HeeksObj* object)
{
	HeeksDxfRead::m_add_uninstanced_blocks = value;

	HeeksConfig config;
	config.Write(_T("DxfAddUninstancedBlocks"), HeeksDxfRead::m_add_uninstanced_blocks);
}

void on_stl_facet_tolerance(double value, HeeksObj* object){
	wxGetApp().m_stl_facet_tolerance = value;
}

void on_set_auto_save_interval(int value, HeeksObj* object){
	wxGetApp().m_auto_save_interval = value;

	if (wxGetApp().m_auto_save_interval > 0)
	{
		if (wxGetApp().m_pAutoSave.get() == NULL)
		{
			wxGetApp().m_pAutoSave = std::auto_ptr<CAutoSave>(new CAutoSave( wxGetApp().m_auto_save_interval, true ));
		}
		else
		{
			wxGetApp().m_pAutoSave->Start( wxGetApp().m_auto_save_interval * 60 * 1000, false );
		}
	}
	else
	{
		if (wxGetApp().m_pAutoSave.get() != NULL)
		{
			delete wxGetApp().m_pAutoSave.release();
			wxGetApp().m_pAutoSave = std::auto_ptr<CAutoSave>(NULL);
		}
	}
	HeeksConfig config;
	config.Write(_T("AutoSaveInterval"), wxGetApp().m_auto_save_interval);
}

static void on_set_units(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().m_view_units = (value == 0) ? 1.0:25.4;
	wxGetApp().OnChangeViewUnits(wxGetApp().m_view_units);

	// Notify any registered handler routines.
	for (HeeksCADapp::UnitsChangedHandlers_t::iterator itHandler = wxGetApp().m_units_changed_handlers.begin();
            itHandler != wxGetApp().m_units_changed_handlers.end(); itHandler++)
    {
            (*(*itHandler))(wxGetApp().m_view_units);
    }

	HeeksConfig config;
	config.Write(_T("ViewUnits"), wxGetApp().m_view_units);
	wxGetApp().m_frame->RefreshProperties();
	wxGetApp().m_frame->RefreshInputCanvas();
	wxGetApp().m_ruler->KillGLLists();
	wxGetApp().Repaint();
}

static void on_dimension_draw_flat(bool value, HeeksObj* object)
{
	HDimension::DrawFlat = value;
	wxGetApp().Repaint();
}

static void on_input_uses_modal_dialog(bool value, HeeksObj* object)
{
	wxGetApp().m_input_uses_modal_dialog = value;
	wxGetApp().Repaint();
}

static void on_dragging_moves_objects(bool value, HeeksObj* object)
{
	wxGetApp().m_dragging_moves_objects = value;
	wxGetApp().Repaint();
}

#ifndef WIN32
static void on_edit_font_paths(const wxChar* value, HeeksObj* object)
{
	wxGetApp().m_font_paths.assign(value);
	if (wxGetApp().m_pVectorFonts.get()) delete wxGetApp().m_pVectorFonts.release();
	wxGetApp().GetAvailableFonts(true);

	HeeksConfig config;
	config.Write(_T("FontPaths"), wxGetApp().m_font_paths);
}

static void on_edit_word_space_percentage(const double value, HeeksObj* object)
{
	wxGetApp().m_word_space_percentage = value;

	HeeksConfig config;
	config.Write(_T("FontWordSpacePercentage"), wxGetApp().m_word_space_percentage);

	if (wxGetApp().m_pVectorFonts.get())
	{
	    wxGetApp().m_pVectorFonts->SetWordSpacePercentage(value);
	}
}

static void on_edit_character_space_percentage(const double value, HeeksObj* object)
{
	wxGetApp().m_character_space_percentage = value;

	HeeksConfig config;
	config.Write(_T("FontCharacterSpacePercentage"), wxGetApp().m_character_space_percentage);

	if (wxGetApp().m_pVectorFonts.get())
	{
	    wxGetApp().m_pVectorFonts->SetCharacterSpacePercentage(value);
	}
}
#endif

void on_set_mouse_move_highlighting(bool value, HeeksObj* object)
{
	wxGetApp().m_mouse_move_highlighting = value;
	wxGetApp().Repaint();
}

void on_set_highlight_color(HeeksColor value, HeeksObj* object)
{
	wxGetApp().m_highlight_color = value;
	wxGetApp().Repaint();
}

void on_stl_solid_random_colors(bool value, HeeksObj* object)
{
	wxGetApp().m_stl_solid_random_colors = value;
	wxGetApp().KillGLLists();
	wxGetApp().Repaint();
}

void on_set_solid_view_mode(int value, HeeksObj* object, bool from_undo_redo)
{
	wxGetApp().m_solid_view_mode = (SolidViewMode)value;
}

void on_set_use_clipper(bool value, HeeksObj* object)
{
	wxGetApp().m_use_Clipper_not_Boolean = value;
}

void on_set_use_DOS(bool value, HeeksObj* object)
{
	wxGetApp().m_use_DOS_not_Unix = value;

}

void HeeksCADapp::GetOptions(std::list<Property *> *list)
{
	PropertyList* view_options = new PropertyList(_("view options"));

	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("stay upright") ) );
		choices.push_back ( wxString ( _("free") ) );
		view_options->m_list.push_back ( new PropertyChoice ( _("rotate mode"),  choices, m_rotate_mode, NULL, on_set_rotate_mode ) );
	}
	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("none") ) );
		choices.push_back ( wxString ( _("input mode title") ) );
		choices.push_back ( wxString ( _("full help") ) );
		view_options->m_list.push_back ( new PropertyChoice ( _("screen text"),  choices, m_graphics_text_mode, NULL, on_set_screen_text ) );
	}
	view_options->m_list.push_back( new PropertyCheck(_("antialiasing"), m_antialiasing, NULL, on_set_antialiasing));
	view_options->m_list.push_back( new PropertyCheck(_("reverse mouse wheel"), !(mouse_wheel_forward_away), NULL, on_set_reverse_mouse_wheel));
	view_options->m_list.push_back( new PropertyCheck(_("reverse zooming mode"), ViewZooming::m_reversed, NULL, on_set_reverse_zooming));
	view_options->m_list.push_back( new PropertyCheck(_("Ctrl key does rotate"), ctrl_does_rotate, NULL, on_set_ctrl_does_rotate));
	view_options->m_list.push_back(new PropertyCheck(_("show datum"), m_show_datum_coords_system, NULL, on_set_show_datum));
	view_options->m_list.push_back(new PropertyCheck(_("datum is solid"), m_datum_coords_system_solid_arrows, NULL, on_set_solid_datum));
	view_options->m_list.push_back(new PropertyDouble(_("datum size"), CoordinateSystem::size, NULL, on_set_datum_size));
	view_options->m_list.push_back(new PropertyCheck(_("datum size is pixels not mm"), CoordinateSystem::size_is_pixels, NULL, on_set_size_is_pixels));
	view_options->m_list.push_back(new PropertyCheck(_("show ruler"), m_show_ruler, NULL, on_set_show_ruler));
	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("single color") ) );
		choices.push_back ( wxString ( _("top color and bottom color") ) );
		choices.push_back ( wxString ( _("left color and right color") ) );
		choices.push_back ( wxString ( _("four corner colors") ) );
		choices.push_back ( wxString ( _("sky and ground") ) );
		view_options->m_list.push_back ( new PropertyChoice ( _("background mode"),  choices, (int)m_background_mode, NULL, on_set_background_mode ) );
	}
	switch(m_background_mode)
	{
	case BackgroundModeOneColor:
		view_options->m_list.push_back ( new PropertyColor ( _("background color"),  background_color[0], NULL, on_set_background_color0 ) );
		break;

	case BackgroundModeTwoColors:
		view_options->m_list.push_back ( new PropertyColor ( _("top background color"),  background_color[0], NULL, on_set_background_color0 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("bottom background color"),  background_color[1], NULL, on_set_background_color1 ) );
		break;

	case BackgroundModeTwoColorsLeftToRight:
		view_options->m_list.push_back ( new PropertyColor ( _("left background color"),  background_color[0], NULL, on_set_background_color0 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("right background color"),  background_color[2], NULL, on_set_background_color2 ) );
		break;

	case BackgroundModeFourColors:
		view_options->m_list.push_back ( new PropertyColor ( _("top left background color"),  background_color[0], NULL, on_set_background_color0 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("bottom left background color"),  background_color[1], NULL, on_set_background_color1 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("top right background color"),  background_color[2], NULL, on_set_background_color2 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("bottom right background color"),  background_color[3], NULL, on_set_background_color3 ) );
		break;

	case BackgroundModeSkyDome:
		view_options->m_list.push_back ( new PropertyColor ( _("sky top color"),  background_color[4], NULL, on_set_background_color4 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("sky middle color"),  background_color[5], NULL, on_set_background_color5 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("sky bottom color"),  background_color[6], NULL, on_set_background_color6 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("ground top color"),  background_color[7], NULL, on_set_background_color7 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("ground middle color"),  background_color[8], NULL, on_set_background_color8 ) );
		view_options->m_list.push_back ( new PropertyColor ( _("ground bottom color"),  background_color[9], NULL, on_set_background_color9 ) );
		break;

	}
	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("no grid") ) );
		choices.push_back ( wxString ( _("faint color") ) );
		choices.push_back ( wxString ( _("alpha blending") ) );
		choices.push_back ( wxString ( _("colored alpha blending") ) );
		view_options->m_list.push_back ( new PropertyChoice ( _("grid mode"),  choices, grid_mode, NULL, on_set_grid_mode ) );
	}
	view_options->m_list.push_back ( new PropertyColor ( _("face selection color"), face_selection_color, NULL, on_set_face_color ) );
	view_options->m_list.push_back( new PropertyCheck(_("perspective"), m_current_viewport->m_view_point.GetPerspective(), NULL, on_set_perspective));

	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _T("16") ) );
		choices.push_back ( wxString ( _T("20") ) );
		choices.push_back ( wxString ( _T("24") ) );
		choices.push_back ( wxString ( _T("28") ) );
		choices.push_back ( wxString ( _T("32") ) );
		choices.push_back ( wxString ( _T("48") ) );
		choices.push_back ( wxString ( _T("64") ) );
		choices.push_back ( wxString ( _T("96") ) );
		int s = ToolImage::GetBitmapSize();
		int choice = 0;
		if(s > 70)choice = 5;
		else if(s > 60)choice = 4;
		else if(s > 40)choice = 3;
		else if(s > 30)choice = 2;
		else if(s > 20)choice = 1;
		view_options->m_list.push_back ( new PropertyChoice ( _("tool icon size"),  choices, choice, NULL, on_set_tool_icon_size ) );
	}

	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("mm") ) );
		choices.push_back ( wxString ( _("inch") ) );
		int choice = 0;
		if(m_view_units > 25.0)choice = 1;
		view_options->m_list.push_back ( new PropertyChoice ( _("units"),  choices, choice, this, on_set_units ) );
	}
	view_options->m_list.push_back(new PropertyCheck(_("flat on screen dimension text"), HDimension::DrawFlat, NULL, on_dimension_draw_flat));
	{
		std::list< wxString > choices;
		choices.push_back ( wxString ( _("faces and edges") ) );
		choices.push_back ( wxString ( _("edges only") ) );
		choices.push_back ( wxString ( _("faces only") ) );
		view_options->m_list.push_back ( new PropertyChoice ( _("solid view mode"),  choices, m_solid_view_mode, this, on_set_solid_view_mode ) );
	}
	view_options->m_list.push_back(new PropertyCheck(_("input uses modal dialog"), m_input_uses_modal_dialog, NULL, on_input_uses_modal_dialog));
	view_options->m_list.push_back(new PropertyCheck(_("dragging moves objects"), m_dragging_moves_objects, NULL, on_dragging_moves_objects));
	view_options->m_list.push_back(new PropertyCheck(_("highlight items under mouse"), m_mouse_move_highlighting, NULL, on_set_mouse_move_highlighting));
	view_options->m_list.push_back(new PropertyColor(_("highlight color"), m_highlight_color, NULL, on_set_highlight_color));
	view_options->m_list.push_back(new PropertyCheck(_("stl solid random colors"), m_stl_solid_random_colors, NULL, on_stl_solid_random_colors));

	list->push_back(view_options);

	PropertyList* digitizing = new PropertyList(_("digitizing"));
	digitizing->m_list.push_back(new PropertyCheck(_("end"), digitize_end, NULL, on_end_of));
	digitizing->m_list.push_back(new PropertyCheck(_("intersection"), digitize_inters, NULL, on_intersection));
	digitizing->m_list.push_back(new PropertyCheck(_("centre"), digitize_centre, NULL, on_centre));
	digitizing->m_list.push_back(new PropertyCheck(_("midpoint"), digitize_midpoint, NULL, on_mid_point));
	digitizing->m_list.push_back(new PropertyCheck(_("nearest"), digitize_nearest, NULL, on_nearest));
	digitizing->m_list.push_back(new PropertyCheck(_("tangent"), digitize_tangent, NULL, on_tangent));
	digitizing->m_list.push_back(new PropertyCheck(_("coordinates"), digitize_coords, NULL, on_coords));
	digitizing->m_list.push_back(new PropertyCheck(_("screen"), digitize_screen, NULL, on_relative));
	digitizing->m_list.push_back(new PropertyLength(_("grid size"), digitizing_grid, NULL, on_grid_edit));
	digitizing->m_list.push_back(new PropertyCheck(_("snap to grid"), draw_to_grid, NULL, on_grid));
	list->push_back(digitizing);

	PropertyList* correlation_properties = new PropertyList(_("correlation"));
	correlation_properties->m_list.push_back(new PropertyDouble(_("Minimum correlation factor (0.0 (nothing like it) -> 1.0 (perfect match))"), m_min_correlation_factor, NULL, on_set_min_correlation_factor));
	correlation_properties->m_list.push_back(new PropertyDouble(_("Maximum scale threshold (1.0 - must be same size, 1.5 (can be half as big again or 2/3 size)"), m_max_scale_threshold, NULL, on_set_max_scale_threshold));
	correlation_properties->m_list.push_back(new PropertyInt(_("Number of sample points"), m_number_of_sample_points, NULL, on_set_number_of_sample_points));
	correlation_properties->m_list.push_back(new PropertyCheck( _("Correlate by color"), m_correlate_by_color, NULL, on_set_correlate_by_color));
	list->push_back(correlation_properties);

	PropertyList* drawing = new PropertyList(_("drawing"));
	drawing->m_list.push_back ( new PropertyColor ( _("current color"),  current_color, NULL, on_set_current_color ) );
	drawing->m_list.push_back(new PropertyLength(_("geometry tolerance"), m_geom_tol, NULL, on_set_geom_tol));
	drawing->m_list.push_back(new PropertyLength(_("sketch reorder tolerance"), m_sketch_reorder_tol, NULL, on_set_sketch_reorder_tol));
	drawing->m_list.push_back(new PropertyLength(_("face to sketch deviaton"), FaceToSketchTool::deviation, NULL, on_set_face_to_sketch_deviation));
	drawing->m_list.push_back(new PropertyCheck(_("Use old solid fuse ( to prevent coplanar faces )"), useOldFuse, NULL, on_useOldFuse));
	drawing->m_list.push_back(new PropertyCheck(_("Extrude makes a solid"), m_extrude_to_solid, NULL, on_extrude_to_solid));
	drawing->m_list.push_back(new PropertyDouble(_("Solid revolution angle"), m_revolve_angle, NULL, on_revolve_angle));
	list->push_back(drawing);

	for(std::list<Plugin>::iterator It = m_loaded_libraries.begin(); It != m_loaded_libraries.end(); It++){
		wxDynamicLibrary* shared_library = It->dynamic_library;
		list_for_GetOptions = list;
		bool success;
		void(*GetOptions)(void(*)(Property*)) = (void(*)(void(*)(Property*)))(shared_library->GetSymbol(_T("GetOptions"), &success));
		if(GetOptions)(*GetOptions)(AddPropertyCallBack);
	}

	PropertyList* selection_filter = new PropertyList(_("selection filter"));
	selection_filter->m_list.push_back(new PropertyCheck(_("point"), (m_marked_list->m_filter & MARKING_FILTER_POINT) != 0, NULL, on_sel_filter_point));
	selection_filter->m_list.push_back(new PropertyCheck(_("line"), (m_marked_list->m_filter & MARKING_FILTER_LINE) != 0, NULL, on_sel_filter_line));
	selection_filter->m_list.push_back(new PropertyCheck(_("arc"), (m_marked_list->m_filter & MARKING_FILTER_ARC) != 0, NULL, on_sel_filter_arc));
	selection_filter->m_list.push_back(new PropertyCheck(_("infinite line"), (m_marked_list->m_filter & MARKING_FILTER_ILINE) != 0, NULL, on_sel_filter_iline));
	selection_filter->m_list.push_back(new PropertyCheck(_("circle"), (m_marked_list->m_filter & MARKING_FILTER_CIRCLE) != 0, NULL, on_sel_filter_circle));
	selection_filter->m_list.push_back(new PropertyCheck(_("edge"), (m_marked_list->m_filter & MARKING_FILTER_EDGE) != 0, NULL, on_sel_filter_edge));
	selection_filter->m_list.push_back(new PropertyCheck(_("face"), (m_marked_list->m_filter & MARKING_FILTER_FACE) != 0, NULL, on_sel_filter_face));
	selection_filter->m_list.push_back(new PropertyCheck(_("vertex"), (m_marked_list->m_filter & MARKING_FILTER_VERTEX) != 0, NULL, on_sel_filter_vertex));
	selection_filter->m_list.push_back(new PropertyCheck(_("solid"), (m_marked_list->m_filter & MARKING_FILTER_SOLID) != 0, NULL, on_sel_filter_solid));
	selection_filter->m_list.push_back(new PropertyCheck(_("stl_solid"), (m_marked_list->m_filter & MARKING_FILTER_STL_SOLID) != 0, NULL, on_sel_filter_stl_solid));
	selection_filter->m_list.push_back(new PropertyCheck(_("wire"), (m_marked_list->m_filter & MARKING_FILTER_WIRE) != 0, NULL, on_sel_filter_wire));
	selection_filter->m_list.push_back(new PropertyCheck(_("sketch"), (m_marked_list->m_filter & MARKING_FILTER_SKETCH) != 0, NULL, on_sel_filter_sketch));
	selection_filter->m_list.push_back(new PropertyCheck(_("image"), (m_marked_list->m_filter & MARKING_FILTER_IMAGE) != 0, NULL, on_sel_filter_image));
	selection_filter->m_list.push_back(new PropertyCheck(_("coordinate system"), (m_marked_list->m_filter & MARKING_FILTER_COORDINATE_SYSTEM) != 0, NULL, on_sel_filter_coordinate_sys));
	selection_filter->m_list.push_back(new PropertyCheck(_("text"), (m_marked_list->m_filter & MARKING_FILTER_TEXT) != 0, NULL, on_sel_filter_text));
	selection_filter->m_list.push_back(new PropertyCheck(_("dimension"), (m_marked_list->m_filter & MARKING_FILTER_DIMENSION) != 0, NULL, on_sel_filter_dimension));
	selection_filter->m_list.push_back(new PropertyCheck(_("ruler"), (m_marked_list->m_filter & MARKING_FILTER_RULER) != 0, NULL, on_sel_filter_ruler));
	selection_filter->m_list.push_back(new PropertyCheck(_("pad"), (m_marked_list->m_filter & MARKING_FILTER_PAD) != 0, NULL, on_sel_filter_pad));
	selection_filter->m_list.push_back(new PropertyCheck(_("part"), (m_marked_list->m_filter & MARKING_FILTER_PART) != 0, NULL, on_sel_filter_part));
	selection_filter->m_list.push_back(new PropertyCheck(_("pocket"), (m_marked_list->m_filter & MARKING_FILTER_POCKETSOLID) != 0, NULL, on_sel_filter_pocket));
	list->push_back(selection_filter);

	PropertyList* file_options = new PropertyList(_("file options"));
	PropertyList* dxf_options = new PropertyList(_("DXF"));
	dxf_options->m_list.push_back(new PropertyCheck(_("make sketch"), HeeksDxfRead::m_make_as_sketch, NULL, on_dxf_make_sketch));
	dxf_options->m_list.push_back(new PropertyCheck(_("ignore errors where possible"), HeeksDxfRead::m_ignore_errors, NULL, on_sel_dxf_read_errors));
	dxf_options->m_list.push_back(new PropertyCheck(_("read points"), HeeksDxfRead::m_read_points, NULL, on_dxf_read_points));
	dxf_options->m_list.push_back( new PropertyString(_("Layer Name Suffixes To Discard"), HeeksDxfRead::m_layer_name_suffixes_to_discard, this, on_edit_layer_name_suffixes_to_discard));
	dxf_options->m_list.push_back(new PropertyCheck(_("add uninstanced blocks"), HeeksDxfRead::m_add_uninstanced_blocks, NULL, on_add_uninstanced_blocks));

	file_options->m_list.push_back(dxf_options);
	PropertyList* stl_options = new PropertyList(_("STL"));
	stl_options->m_list.push_back(new PropertyDouble(_("stl save facet tolerance"), m_stl_facet_tolerance, NULL, on_stl_facet_tolerance));
	stl_options->m_list.push_back( new PropertyCheck(_("STL save binary"), m_stl_save_as_binary, NULL, on_set_stl_save_binary));
	file_options->m_list.push_back(stl_options);
	file_options->m_list.push_back(new PropertyInt(_("auto save interval (in minutes)"), m_auto_save_interval, NULL, on_set_auto_save_interval));
	list->push_back(file_options);

#ifndef WIN32
	// Font options
	PropertyList* font_options = new PropertyList(_("font options"));
	font_options->m_list.push_back( new PropertyString(_("Paths (semicolon delimited)"), m_font_paths, this, on_edit_font_paths));
	font_options->m_list.push_back( new PropertyDouble(_("Word Space Percentage"), m_word_space_percentage, this, on_edit_word_space_percentage));
	font_options->m_list.push_back( new PropertyDouble(_("Character Space Percentage"), m_character_space_percentage, this, on_edit_character_space_percentage));
	list->push_back(font_options);
#endif

	PropertyList* machining_options = new PropertyList(_("machining options"));
	CNCCode::GetOptions(&(machining_options->m_list));
	CSpeedOp::GetOptions(&(machining_options->m_list));
	CProfile::GetOptions(&(machining_options->m_list));
	CPocket::GetOptions(&(machining_options->m_list));
	machining_options->m_list.push_back(new PropertyCheck(_("Use Clipper not Boolean"), m_use_Clipper_not_Boolean, NULL, on_set_use_clipper));
	machining_options->m_list.push_back(new PropertyCheck(_("Use DOS Line Endings"), m_use_DOS_not_Unix, NULL, on_set_use_DOS));

	list->push_back(machining_options);
}

void HeeksCADapp::DeleteMarkedItems()
{
	std::list<HeeksObj *> list;
	for(std::list<HeeksObj*>::iterator It = m_marked_list->list().begin(); It != m_marked_list->list().end(); It++)
	{
		HeeksObj* object = *It;
		if(object->CanBeRemoved())list.push_back(object);
	}

	// clear first, so properties cancel happens first
	m_marked_list->Clear(false);

	if(list.size() == 1){
		DeleteUndoably(*(list.begin()));
	}
	else if(list.size()>1){
		DeleteUndoably(list);
	}
	Repaint(0);
}

void HeeksCADapp::glColorEnsuringContrast(const HeeksColor &c)
{
	if(c == HeeksColor(0, 0, 0) || c == HeeksColor(255,255,255))background_color[0].best_black_or_white().glColor();
	else c.glColor();
}

static wxString known_file_ext;

const wxChar* HeeksCADapp::GetKnownFilesWildCardString(bool open, bool import_export)const
{
	if(!import_export)
	{
		known_file_ext = wxString(_("Heeks files")) + _T(" |*.heeks;*.HEEKS");
		return known_file_ext.c_str();
	}

	if(open){
		if(m_alternative_open_wild_card_string.Len() > 0)
			return m_alternative_open_wild_card_string.c_str();

		wxList handlers = wxImage::GetHandlers();
		wxString imageExtStr;
		wxString imageExtStr2;
		for(wxList::iterator It = handlers.begin(); It != handlers.end(); It++)
		{
			wxImageHandler* handler = (wxImageHandler*)(*It);
			wxString ext = handler->GetExtension();
			if(It != handlers.begin())imageExtStr.Append(_T(";"));
			imageExtStr.Append(_T("*."));
			imageExtStr.Append(ext);
			if(It != handlers.begin())imageExtStr2.Append(_T(" "));
			imageExtStr2.Append(_T("*."));
			imageExtStr2.Append(ext);
		}

		wxString registeredExtensions;
		for (FileOpenHandlers_t::const_iterator itHandler = m_fileopen_handlers.begin(); itHandler != m_fileopen_handlers.end(); itHandler++)
		{
		    registeredExtensions << _T(";*.") << itHandler->first;
		}

		known_file_ext = wxString(_("Known Files")) + _T(" |*.heeks;*.HEEKS;*.igs;*.IGS;*.iges;*.IGES;*.stp;*.STP;*.step;*.STEP;*.dxf;*.DXF") + imageExtStr + registeredExtensions + _T("|") + _("Heeks files") + _T(" (*.heeks)|*.heeks;*.HEEKS|") + _("IGES files") + _T(" (*.igs *.iges)|*.igs;*.IGS;*.iges;*.IGES|") + _("STEP files") + _T(" (*.stp *.step)|*.stp;*.STP;*.step;*.STEP|") + _("STL files") + _T(" (*.stl)|*.stl;*.STL|") + _("Scalar Vector Graphics files") + _T(" (*.svg)|*.svg;*.SVG|") + _("DXF files") + _T(" (*.dxf)|*.dxf;*.DXF|") + _("RS274X/Gerber files") + _T(" (*.gbr,*.rs274x)|*.gbr;*.GBR;*.rs274x;*.RS274X;*.pho;*.PHO|") + _("Picture files") + _T(" (") + imageExtStr2 + _T(")|") + imageExtStr;
		return known_file_ext.c_str();
	}
	else{
		// file save
		known_file_ext = wxString(_("Known Files")) + _T(" |*.heeks;*.igs;*.iges;*.stp;*.step;*.stl;*.dxf;*.cpp;*.py;*.obj|") + _("Heeks files") + _T(" (*.heeks)|*.heeks|") + _("IGES files") + _T(" (*.igs *.iges)|*.igs;*.iges|") + _("STEP files") + _T(" (*.stp *.step)|*.stp;*.step|") + _("STL files") + _T(" (*.stl)|*.stl|") + _("DXF files") + _T(" (*.dxf)|*.dxf|") + _("CPP files") + _T(" (*.cpp)|*.cpp|") + _("OpenCAMLib python files") + _T(" (*.py)|*.py|") + _("Wavefront .obj files") + _T(" (*.obj)|*.obj");
		return known_file_ext.c_str();
	}
}

const wxChar* HeeksCADapp::GetKnownFilesCommaSeparatedList(bool open, bool import_export)const
{
	if(!import_export)
		return _T("heeks");

	if(open){
		wxList handlers = wxImage::GetHandlers();
		wxString known_ext_str = _T("heeks, igs, iges, stp, step, dxf");
		for(wxList::iterator It = handlers.begin(); It != handlers.end(); It++)
		{
			wxImageHandler* handler = (wxImageHandler*)(*It);
			wxString ext = handler->GetExtension();
			known_ext_str.Append(_T(", "));
			known_ext_str.Append(ext);
		}

		for (FileOpenHandlers_t::const_iterator itHandler = m_fileopen_handlers.begin(); itHandler != m_fileopen_handlers.end(); itHandler++)
		{
		    known_ext_str << _T(", ") << itHandler->first;
		}


		return known_ext_str;
	}
	else{
		// file save
		return _T("heeks, igs, iges, stp, step, stl, dxf");
	}
}

class MarkObjectTool:public Tool{
public:
	MarkedObject *m_marked_object;
	wxPoint m_point;
	bool m_xor_marked_list;

	MarkObjectTool(MarkedObject *marked_object, const wxPoint& point, bool xor_marked_list):m_marked_object(marked_object), m_point(point), m_xor_marked_list(xor_marked_list){}

	// Tool's virtual functions
	const wxChar* GetTitle(){
		if(m_xor_marked_list){
			if(wxGetApp().m_marked_list->ObjectMarked(m_marked_object->GetObject())){
				return _("Deselect");
			}
			else{
				if(wxGetApp().m_marked_list->size() == 0)return _("Select");
				return _("Add to selection");
			}
		}
		return _("Select");
	}

	wxString BitmapPath(){ return _T("select");}

	void Run(){
		if(m_marked_object == NULL)return;
		if(m_marked_object->GetObject() == NULL)return;
		if(m_xor_marked_list){
			if(wxGetApp().m_marked_list->ObjectMarked(m_marked_object->GetObject())){
				wxGetApp().m_marked_list->Remove(m_marked_object->GetObject(), true);
			}
			else{
				wxGetApp().m_marked_list->Add(m_marked_object->GetObject(), true);
			}
		}
		else{
			wxGetApp().m_marked_list->Clear(true);
			wxGetApp().m_marked_list->Add(m_marked_object->GetObject(), true);
#ifndef PYHEEKSCAD
			if(wxGetApp().m_frame->m_properties && !(wxGetApp().m_frame->m_properties->IsShown())){
				wxGetApp().m_frame->m_aui_manager->GetPane(wxGetApp().m_frame->m_properties).Show();
				wxGetApp().m_frame->m_aui_manager->Update();
			}
#endif
		}
		if(m_point.x >= 0){
			gp_Lin ray = wxGetApp().m_current_viewport->m_view_point.SightLine(m_point);
			double ray_start[3], ray_direction[3];
			extract(ray.Location(), ray_start);
			extract(ray.Direction(), ray_direction);
			m_marked_object->GetObject()->SetClickMarkPoint(m_marked_object, ray_start, ray_direction);
		}
		wxGetApp().Repaint();
	}
};

class EditObjectTool:public Tool{
public:
	MarkedObject *m_marked_object;

	EditObjectTool(MarkedObject *marked_object):m_marked_object(marked_object){}

	// Tool's virtual functions
	const wxChar* GetTitle(){return _("Edit");}

	void Run(){
		if(m_marked_object == NULL)return;
		HeeksObj* object = m_marked_object->GetObject();
		if(object)wxGetApp().EditUndoably(object);
	}
};

void HeeksCADapp::GetTools(MarkedObject* marked_object, std::list<Tool*>& t_list, const wxPoint& point, bool control_pressed)
{
	std::list<MarkedObject*> stack;
	stack.push_back(marked_object);
	std::map<int, MarkedObject*> types;

	while(stack.size() > 0)
	{
		MarkedObject* m = stack.front();
		stack.pop_front();

		for(std::map<HeeksObj*, MarkedObject*>::iterator It = m->m_map.begin(); It != m->m_map.end(); It++)
		{
			stack.push_back(It->second);
		}

		for(std::map<int, MarkedObject*>::iterator It = m->m_types.begin(); It != m->m_types.end(); It++)
		{
			int type = It->first;
			MarkedObject* object = It->second;
			std::map<int, MarkedObject*>::iterator FindIt = types.find(type);

			if(FindIt == types.end())
			{
				types.insert(std::make_pair(type, object));
			}
			else
			{
				MarkedObject* existing_object = FindIt->second;
				if(object->GetDepth() < existing_object->GetDepth())
				{
					types.erase(type);
					types.insert(std::make_pair(type, object));
				}
			}
		}
	}

	if(types.size() > 0)
	{
	for(std::map<int, MarkedObject*>::iterator It = types.begin(); It != types.end(); It++)
	{
		MarkedObject* object = It->second;
		GetTools2(object, t_list, point, control_pressed, types.size() > 1);
	}
	}
	else
	{
		GetTools2(marked_object, t_list, point, control_pressed, false);
	}
}

void HeeksCADapp::GetTools2(MarkedObject* marked_object, std::list<Tool*>& t_list, const wxPoint& point, bool control_pressed, bool make_tool_list_container)
{
	std::list<Tool*> tools;

	if(marked_object->GetObject())
	{
		marked_object->GetObject()->GetTools(&tools, &point);
		if(tools.size() > 0)tools.push_back(new MenuSeparator);

		tools.push_back(new MarkObjectTool(marked_object, point, control_pressed));

		bool(*callback)(HeeksObj*) = NULL;
		marked_object->GetObject()->GetOnEdit(&callback);
		if(callback)tools.push_back(new EditObjectTool(marked_object));

		if (tools.size()>0)
		{
			if(make_tool_list_container)
			{
				const wxChar* str = marked_object->GetObject()->GetShortStringOrTypeString();

				if (wxString(str).CmpNoCase(_("Unknown")) == 0)
				{
					// delete the unused tools, if no valid name
					for (std::list<Tool*>::iterator It = tools.begin(); It != tools.end(); It++)
					{
						delete *It;
					}
				}
				else
				{
					ToolList *function_list = new ToolList(marked_object->GetObject()->GetShortStringOrTypeString());
					function_list->Add(tools);
					t_list.push_back(function_list);
				}
			}
			else
			{
				for(std::list<Tool*>::iterator It = tools.begin(); It != tools.end(); It++)
				{
					Tool* tool = *It;
					t_list.push_back(tool);
				}
			}
		}
	}
}

wxString HeeksCADapp::GetExeFolder()const
{
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& sp = wxStandardPaths::Get();
#else
	wxStandardPaths sp;
#endif
	wxString exepath = sp.GetExecutablePath();
	int last_fs = exepath.Find('/', true);
	int last_bs = exepath.Find('\\', true);
	wxString exedir;
	if(last_fs > last_bs){
		exedir = exepath.Truncate(last_fs);
	}
	else{
		exedir = exepath.Truncate(last_bs);
	}

	return exedir;
}

wxString HeeksCADapp::GetResFolder()const
{
#ifdef WIN32
	return GetExeFolder();
#else
#ifdef CODEBLOCKS
	return (GetExeFolder() + _T("/../.."));
#else
#ifdef RUNINPLACE
	return GetExeFolder();
#else
	return (GetExeFolder() + _T("/../share/heekscad"));
#endif
#endif
#endif
}

// do your own glBegin and glEnd
void HeeksCADapp::get_2d_arc_segments(double xs, double ys, double xe, double ye, double xc, double yc, bool dir, bool want_start, double pixels_per_mm, void(*callbackfunc)(const double* xy)){
	double ax = xs - xc;
	double ay = ys - yc;
	double bx = xe - xc;
	double by = ye - yc;

	double start_angle = atan2(ay, ax);
	double end_angle = atan2(by, bx);

	if(dir){
		if(start_angle > end_angle)end_angle += 6.28318530717958;
	}
	else{
		if(end_angle > start_angle)start_angle += 6.28318530717958;
	}

	double dxc = xs - xc;
	double dyc = ys - yc;
	double radius = sqrt(dxc*dxc + dyc*dyc);
	double d_angle = end_angle - start_angle;
	int segments = (int)(pixels_per_mm * radius * fabs(d_angle) / 6.28318530717958 + 1);

    double theta = d_angle / (double)segments;
	while(theta>1.0){segments*=2;theta = d_angle / (double)segments;}
    double tangetial_factor = tan(theta);
    double radial_factor = 1 - cos(theta);

    double x = radius * cos(start_angle);
    double y = radius * sin(start_angle);

    for(int i = 0; i < segments + 1; i++)
    {
		if(i != 0 || want_start){
			double xy[2] = {xc + x, yc + y};
			(*callbackfunc)(xy);
		}

        double tx = -y;
        double ty = x;

        x += tx * tangetial_factor;
        y += ty * tangetial_factor;

        double rx = - x;
        double ry = - y;

        x += rx * radial_factor;
        y += ry * radial_factor;
    }
}

static long save_filter_for_StartPickObjects = 0;
static bool save_just_one_for_EndPickObjects = false;
static CInputMode* save_mode_for_EndPickObjects = NULL;

void HeeksCADapp::StartPickObjects(const wxChar* str, long marking_filter, bool just_one)
{
	save_mode_for_EndPickObjects = input_mode_object;
	m_select_mode->m_prompt_when_doing_a_main_loop.assign(str);
	m_select_mode->m_doing_a_main_loop = true;
	save_just_one_for_EndPickObjects = m_select_mode->m_just_one;
	m_select_mode->m_just_one = just_one;
	SetInputMode(m_select_mode);

	// set marking filter
	save_filter_for_StartPickObjects = m_marked_list->m_filter;
	m_marked_list->m_filter = marking_filter;
}

int HeeksCADapp::EndPickObjects()
{
	// restore marking filter
	m_marked_list->m_filter = save_filter_for_StartPickObjects;

	m_select_mode->m_just_one = save_just_one_for_EndPickObjects;
	m_select_mode->m_doing_a_main_loop = false;
	SetInputMode(save_mode_for_EndPickObjects); // update tool bar

	return 1;
}

int HeeksCADapp::PickObjects(const wxChar* str, long marking_filter, bool just_one)
{
	StartPickObjects(str, marking_filter, just_one);

	// stay in an input loop until finished picking
	OnRun();

	return EndPickObjects();
}

int HeeksCADapp::OnRun()
{
	try
	{
		return wxApp::OnRun();
	}
	catch(...)
	{
		return 0;
	}
}

bool HeeksCADapp::OnExceptionInMainLoop()
{
	throw;
}

bool HeeksCADapp::PickPosition(const wxChar* str, double* pos, void(*callback)(const double*))
{
	CInputMode* save_mode = input_mode_object;
	m_digitizing->m_prompt_when_doing_a_main_loop.assign(str);
	m_digitizing->m_doing_a_main_loop = true;
	m_digitizing->m_callback = callback;
	SetInputMode(m_digitizing);

	OnRun();

	bool return_found = false;
	if(m_digitizing->digitized_point.m_type != DigitizeNoItemType){
		extract(m_digitizing->digitized_point.m_point, pos);
		return_found = true;
	}

	m_digitizing->m_doing_a_main_loop = false;
	m_digitizing->m_callback = NULL;
	SetInputMode(save_mode);
	return return_found;
}

static int sphere_display_list = 0;

void HeeksCADapp::glSphere(double radius, const double* pos)
{
	glPushMatrix();

	if(pos)
	{
		glTranslated(pos[0], pos[1], pos[2]);
	}

	glScaled(radius, radius, radius);

	if(sphere_display_list)
	{
		glCallList(sphere_display_list);
	}
	else{
		sphere_display_list = glGenLists(1);
		glNewList(sphere_display_list, GL_COMPILE_AND_EXECUTE);

		glBegin(GL_QUADS);

		double v_ang = 0.0;
		double pcosv = 0.0;
		double psinv = 0.0;

		for(int i = 0; i<11; i++, v_ang += 0.15707963267948966)
		{
			double cosv = cos(v_ang);
			double sinv = sin(v_ang);

			if(i > 0)
			{
				double h_ang = 0.0;
				double pcosh = 0.0;
				double psinh = 0.0;

				for(int j = 0; j<21; j++, h_ang += 0.314159265358979323)
				{
					double cosh = cos(h_ang);
					double sinh = sin(h_ang);

					if(j > 0)
					{
						// top quad
						glNormal3d(pcosh * pcosv, psinh * pcosv, psinv);
						glVertex3d(pcosh * pcosv, psinh * pcosv, psinv);
						glNormal3d(cosh * pcosv, sinh * pcosv, psinv);
						glVertex3d(cosh * pcosv, sinh * pcosv, psinv);
						glNormal3d(cosh * cosv, sinh * cosv, sinv);
						glVertex3d(cosh * cosv, sinh * cosv, sinv);
						glNormal3d(pcosh * cosv, psinh * cosv, sinv);
						glVertex3d(pcosh * cosv, psinh * cosv, sinv);

						// bottom quad
						glNormal3d(pcosh * pcosv, psinh * pcosv, -psinv);
						glVertex3d(pcosh * pcosv, psinh * pcosv, -psinv);
						glNormal3d(pcosh * cosv, psinh * cosv, -sinv);
						glVertex3d(pcosh * cosv, psinh * cosv, -sinv);
						glNormal3d(cosh * cosv, sinh * cosv, -sinv);
						glVertex3d(cosh * cosv, sinh * cosv, -sinv);
						glNormal3d(cosh * pcosv, sinh * pcosv, -psinv);
						glVertex3d(cosh * pcosv, sinh * pcosv, -psinv);
					}
					pcosh = cosh;
					psinh = sinh;
				}
			}
			pcosv = cosv;
			psinv = sinv;
		}

		glEnd();

		glEndList();
	}

	glPopMatrix();
}

void HeeksCADapp::OnNewOrOpen(bool open, int res)
{
	// cnc onneworopen
	OnCNCNewOrOpen(open, res);

	ObserversOnChange(&m_objects, NULL, NULL);
}

void HeeksCADapp::OnBeforeNewOrOpen(bool open, int res)
{
	for(std::list< void(*)(int, int) >::iterator It = m_beforeneworopen_callbacks.begin(); It != m_beforeneworopen_callbacks.end(); It++)
	{
		void(*callbackfunc)(int, int) = *It;
		(*callbackfunc)(open ? 1:0, res);
	}
}

void HeeksCADapp::OnBeforeFrameDelete(void)
{
	for(std::list< void(*)() >::iterator It = m_beforeframedelete_callbacks.begin(); It != m_beforeframedelete_callbacks.end(); It++)
	{
		void(*callbackfunc)() = *It;
		(*callbackfunc)();
	}
}

void HeeksCADapp::RegisterHideableWindow(wxWindow* w)
{
	m_hideable_windows.push_back(w);
}

void HeeksCADapp::RemoveHideableWindow(wxWindow* w)
{
	m_hideable_windows.remove(w);
}

void HeeksCADapp::GetRecentFilesProfileString()
{
	HeeksConfig config;
	for(int i = 0; i < MAX_RECENT_FILES; i++)
	{
		wxString key_name = wxString::Format(_T("RecentFilePath%d"), i);
		wxString filepath = config.Read(key_name);
		if(filepath.IsEmpty())break;
		m_recent_files.push_back(filepath);
	}
}

void HeeksCADapp::WriteRecentFilesProfileString(wxConfigBase &config)
{
	std::list< wxString >::iterator It = m_recent_files.begin();
	for(int i = 0; i < MAX_RECENT_FILES; i++)
	{
		wxString key_name = wxString::Format(_T("RecentFilePath%d"), i);
		wxString filepath;
		if(It != m_recent_files.end())
		{
			filepath = *It;
			It++;
		}
		config.Write(key_name, filepath);
	}
}

void HeeksCADapp::InsertRecentFileItem(const wxChar* filepath)
{
	// add to recent files list
	m_recent_files.remove(filepath);
	m_recent_files.push_front( wxString( filepath ) );
	if(m_recent_files.size() > MAX_RECENT_FILES)m_recent_files.pop_back();
}

int HeeksCADapp::CheckForModifiedDoc()
{
	// returns wxCANCEL if not OK to continue opening file
	if(!m_no_creation_mode && IsModified())
	{
		wxString str = wxString(_("Save changes to file")) + _T(" ") + m_filepath;
		int res = wxMessageBox(str, wxMessageBoxCaptionStr, wxCANCEL|wxYES_NO|wxCENTRE);
		if(res == wxCANCEL || res == wxNO) return res;
		if(res == wxYES)
		{
			return (int) SaveFile(m_filepath.c_str(), true);
		}
	}
	return wxOK;
}

void HeeksCADapp::SetFrameTitle()
{
	if(m_frame == NULL)return;
	wxString str;
#ifdef FREE_VERSION
	str = _T("TRIAL VERSION of ");
#endif
	str += GetAppName().c_str();
	if((!m_untitled) || (!m_no_creation_mode))str += wxString(_T(" - ")) + m_filepath;
	m_frame->SetTitle(str);
}


HeeksObj* HeeksCADapp::GetIDObject(int type, int id)
{
	UsedIds_t::iterator FindIt1 = used_ids.find(type);
	if (FindIt1 == used_ids.end()) return(NULL);

	IdsToObjects_t &ids = FindIt1->second;
	IdsToObjects_t::iterator FindIt2 = ids.find(id);

	if (FindIt2 == ids.end())return NULL;

	std::list<HeeksObj*> &list = FindIt2->second;
	return list.back();
}

std::list<HeeksObj*> HeeksCADapp::GetIDObjects(int type, int id)
{
    std::list<HeeksObj *> results;

	UsedIds_t::iterator FindIt1 = used_ids.find(type);
	if (FindIt1 == used_ids.end()) return results;

	IdsToObjects_t &ids = FindIt1->second;
	IdsToObjects_t::iterator FindIt2 = ids.find(id);

	if (FindIt2 == ids.end()) return results;
	return FindIt2->second;
}

void HeeksCADapp::SetObjectID(HeeksObj* object, int id)
{
	if(object->UsesID())
	{
		object->m_id = id;
		GroupId_t id_group_type = object->GetIDGroupType();

		UsedIds_t::iterator FindIt1 = used_ids.find(id_group_type);
		if(FindIt1 == used_ids.end())
		{
			// add a new map
			std::list<HeeksObj*> empty_list;
			empty_list.push_back(object);
			IdsToObjects_t empty_map;
			empty_map.insert( std::make_pair( id, empty_list ) );
			FindIt1 = used_ids.insert( std::make_pair( id_group_type, empty_map )).first;
		}
		else
		{

			IdsToObjects_t &map = FindIt1->second;
			IdsToObjects_t::iterator FindIt2 = map.find(id);
			if(FindIt2 == map.end())
			{
				std::list<HeeksObj*> empty_list;
				empty_list.push_back(object);
				map.insert( std::make_pair(id, empty_list) );
			}
			else
			{
				std::list<HeeksObj*> &list = FindIt2->second;
				list.push_back(object);
			}
		}
	}
}

int HeeksCADapp::GetNextID(int id_group_type)
{
	UsedIds_t::iterator FindIt1 = used_ids.find(id_group_type);
	if(FindIt1 == used_ids.end())return 1;

	std::map< int, int >::iterator FindIt2 = next_id_map.find(id_group_type);

	IdsToObjects_t &map = FindIt1->second;

	if(FindIt2 == next_id_map.end())
	{
		// add a new int
		int next_id = map.begin()->first + 1;
		FindIt2 = next_id_map.insert( std::make_pair(id_group_type, next_id) ).first;
	}

	int &next_id = FindIt2->second;

	while(map.find(next_id) != map.end())next_id++;
	return next_id;
}

void HeeksCADapp::RemoveID(HeeksObj* object)
{
	int id_group_type = object->GetIDGroupType();

	UsedIds_t::iterator FindIt1 = used_ids.find(id_group_type);
	if(FindIt1 == used_ids.end())return;
	std::map< int, int >::iterator FindIt2 = next_id_map.find(id_group_type);

	IdsToObjects_t &map = FindIt1->second;
	if (FindIt2 != next_id_map.end())
	{
		IdsToObjects_t::iterator FindIt3 = map.find(object->m_id);
		if(FindIt3 != map.end())
		{
			std::list<HeeksObj*> &list = FindIt3->second;
			list.remove(object);
			if(list.size() == 0)map.erase(object->m_id);
		}
	} // End if - then
}

void HeeksCADapp::ResetIDs()
{
	used_ids.clear();
	next_id_map.clear();
}

void HeeksCADapp::RegisterOnGLCommands( void(*callbackfunc)() )
{
	m_on_glCommands_list.push_back(callbackfunc);
}

void HeeksCADapp::RemoveOnGLCommands( void(*callbackfunc)() )
{
	m_on_glCommands_list.remove(callbackfunc);
}

void HeeksCADapp::RegisterOnGraphicsSize( void(*callbackfunc)(wxSizeEvent& evt) )
{
	m_on_graphics_size_list.push_back(callbackfunc);
}

void HeeksCADapp::RemoveOnGraphicsSize( void(*callbackfunc)(wxSizeEvent& evt) )
{
	m_on_graphics_size_list.remove(callbackfunc);
}

void HeeksCADapp::RegisterOnMouseFn( void(*callbackfunc)(wxMouseEvent&) )
{
	m_lbutton_up_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RemoveOnMouseFn( void(*callbackfunc)(wxMouseEvent&) )
{
	m_lbutton_up_callbacks.remove(callbackfunc);
}

void HeeksCADapp::RegisterOnSaveFn( void(*callbackfunc)(bool from_changed_prompt) )
{
	m_on_save_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RegisterIsModifiedFn( bool(*callbackfunc)() )
{
	m_is_modified_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::CreateTransformGLList(const std::list<HeeksObj*>& list, bool show_grippers_on_drag){
	DestroyTransformGLList();
	m_transform_gl_list = glGenLists(1);
	glNewList(m_transform_gl_list, GL_COMPILE);
	std::list<HeeksObj *>::const_iterator It;
	for(It = list.begin(); It != list.end(); It++){
		(*It)->glCommands(false, true, false);
	}
	glDisable(GL_DEPTH_TEST);
	if(show_grippers_on_drag)m_marked_list->GrippersGLCommands(false, false);
	glEnable(GL_DEPTH_TEST);
	glEndList();
}

void HeeksCADapp::DestroyTransformGLList(){
	if (m_transform_gl_list)
	{
		glDeleteLists(m_transform_gl_list, 1);
	}
	m_transform_gl_list = 0;
}

bool HeeksCADapp::IsPasteReady()
{
	wxString fstr;

#ifndef WIN32
	if(wxTheClipboard->IsOpened())return false;
#endif

	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported( wxDF_TEXT ))
		{
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			fstr = data.GetText();
		}
		wxTheClipboard->Close();

		if(fstr.StartsWith(_T("<?xml version=\"1.0\"")))return true;
	}

	return false;
}

void HeeksCADapp::Paste(HeeksObj* paste_into, HeeksObj* paste_before)
{
	m_inPaste = true;

	// assume the text is the contents of a heeks file
	wxString fstr;

	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported( wxDF_TEXT ))
		{
			wxTextDataObject data;
			wxTheClipboard->GetData( data );
			fstr = data.GetText();
		}
		wxTheClipboard->Close();
	}

	// write a temporary file
#if wxCHECK_VERSION(3, 0, 0)
	wxStandardPaths& sp = wxStandardPaths::Get();
#else
	wxStandardPaths sp;
#endif
	sp.GetTempDir();
	wxFileName temp_file( sp.GetTempDir().c_str(), _T("temp_Heeks_clipboard_file.heeks"));

	{
#if wxUSE_UNICODE
		wofstream ofs(Ttc(temp_file.GetFullPath().c_str()));
#else
		ofstream ofs(temp_file.GetFullPath());
#endif
		ofs<<fstr.c_str();
	}

	m_marked_list->Clear(true);
	m_mark_newly_added_objects = true;
	OpenFile(temp_file.GetFullPath(), true, paste_into, paste_before);
	m_mark_newly_added_objects = false;

	m_inPaste = false;
}

bool HeeksCADapp::CheckForNOrMore(const std::list<HeeksObj*> &list, int min_num, int type, const wxString& msg, const wxString& caption)
{
	int num_of_type = 0;
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++){
		HeeksObj* object = *It;
		if(object->GetType() == type)num_of_type++;
	}

	if(num_of_type < min_num)
	{
		wxMessageBox(msg, caption);
		return false;
	}

	return true;
}

bool HeeksCADapp::CheckForNOrMore(const std::list<HeeksObj*> &list, int min_num, int type1, int type2, const wxString& msg, const wxString& caption)
{
	int num_of_type = 0;
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++){
		HeeksObj* object = *It;
		if(object->GetType() == type1 || object->GetType() == type2)num_of_type++;
	}

	if(num_of_type < min_num)
	{
		wxMessageBox(msg, caption);
		return false;
	}

	return true;
}

bool HeeksCADapp::CheckForNOrMore(const std::list<HeeksObj*> &list, int min_num, int type1, int type2, int type3, const wxString& msg, const wxString& caption)
{
	int num_of_type = 0;
	for(std::list<HeeksObj*>::const_iterator It = list.begin(); It != list.end(); It++){
		HeeksObj* object = *It;
		if(object->GetType() == type1 || object->GetType() == type2 || object->GetType() == type3)num_of_type++;
	}

	if(num_of_type < min_num)
	{
		wxMessageBox(msg, caption);
		return false;
	}

	return true;
}

void HeeksCADapp::create_font()
{
	if(m_gl_font_initialized)
		return;
	wxString fstr;
	switch((wxLocale::GetSystemLanguage()))
	{
			case wxLANGUAGE_SLOVAK :
				fstr = GetResFolder() + _T("/bitmaps/slovak.glf");
				break;
			default:
				fstr = GetResFolder() + _T("/bitmaps/font.glf");
				break;
	}
	glGenTextures(2, m_font_tex_number );

	//Create our glFont from verdana.glf, using texture 1
	m_gl_font.Create((char*)Ttc(fstr.c_str()), m_font_tex_number[0], m_font_tex_number[1]);
	m_gl_font_initialized = true;
}

void HeeksCADapp::render_text(const wxChar* str, bool select)
{
	//Needs to be called before text output
	create_font();
	//glColor4ub(0, 0, 0, 255);
	EnableBlend();
	glEnable(GL_TEXTURE_2D);

	glDepthMask(0);
	glDisable(GL_POLYGON_OFFSET_FILL);
	m_gl_font.Begin(select);

	//Draws text with a glFont
	m_gl_font.DrawString(str, 0.08f, 0.0f, 0.0f);

	glDepthMask(1);
	glEnable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_TEXTURE_2D);
	DisableBlend();
}

bool HeeksCADapp::get_text_size(const wxChar* str, float* width, float* height)
{
	create_font();

	std::pair<int, int> size;
	m_gl_font.GetStringSize(str, &size);
	*width = (float)(size.first) * 0.08f;
	*height = (float)(size.second) * 0.08f;

	return true;
}

void HeeksCADapp::render_screen_text2(const wxChar* str, bool select)
{
#if wxUSE_UNICODE
	size_t n = wcslen(str);
#else
	size_t n = strlen(str);
#endif
		wxChar buffer[1024];


	int j = 0;
	const wxChar* newlinestr = _T("\n");
	wxChar newline = newlinestr[0];

	for(size_t i = 0; i<n; i++)
	{
		buffer[j] = str[i];
		j++;
		if(str[i] == newline || i == n-1 || j == 1023){
			buffer[j] = 0;
			render_text(buffer, select);
			if(str[i] == newline)glTranslated(0.0, -2.2, 1.0);
			j = 0;
		}
	}
}

void HeeksCADapp::render_screen_text(const wxChar* str1, const wxChar* str2, bool select)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	m_current_viewport->SetIdentityProjection();
	background_color[0].best_black_or_white().glColor();
	int w, h;
	m_current_viewport->GetViewportSize(&w, &h);
	glTranslated(2.0, h - 1.0, 0.0);

	glScaled(10.0, 10.0, 0);
	render_screen_text2(str1, select);

	glScaled(0.612, 0.612, 0);
	render_screen_text2(str2, select);

//Even though this is in reverse order, the different matrices have different stacks, and we want to exit here in the modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void HeeksCADapp::EnableBlend()
{
	if(!m_antialiasing)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

void HeeksCADapp::DisableBlend()
{
	if(!m_antialiasing)glDisable(GL_BLEND);
}

void HeeksCADapp::render_screen_text_at(const wxChar* str1, double scale, double x, double y, double theta, bool select)
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	m_current_viewport->SetIdentityProjection();
	background_color[0].best_black_or_white().glColor();
	int w, h;
	m_current_viewport->GetViewportSize(&w, &h);
	glTranslated(x,y, 0.0);

	glScaled(scale, scale, 0);
	glRotated(theta,0,0,1);
	render_screen_text2(str1, select);

//Even though this is in reverse order, the different matrices have different stacks, and we want to exit here in the modelview
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

void HeeksCADapp::PlotSetColor(const HeeksColor &c)
{
	m_frame->m_printout->SetColor(c);
}

void HeeksCADapp::PlotLine(const double* s, const double* e)
{
	m_frame->m_printout->DrawLine(s, e);
}

void HeeksCADapp::PlotArc(const double* s, const double* e, const double* c)
{
	m_frame->m_printout->DrawArc(s, e, c);
}

void HeeksCADapp::InitialiseLocale()
{
	if(!m_locale_initialised)
	{
		m_locale_initialised = true;

		int language = wxLANGUAGE_DEFAULT;
		{
			HeeksConfig config;
			config.Read(_T("Language"), &language);
		}

		// Initialize the catalogs we'll be using
#if wxCHECK_VERSION(3, 0, 0)
		if ( !m_locale.Init(language) )
#else
		// don't use wxLOCALE_LOAD_DEFAULT flag so that Init() doesn't return
		// false just because it failed to load wxstd catalog
		if ( !m_locale.Init(language, 0) )
#endif
		{
			wxLogError(_T("This language is not supported by the system."));
			return;
		}
		// This will add standard wx catalog that translates common wx strings.
		// e.g. "Print preview" window will be translated
		m_locale.AddCatalog(wxT("wxstd"));

		wxLocale::AddCatalogLookupPathPrefix(wxT("."));

		m_locale.AddCatalog(m_locale.GetCanonicalName());
		m_locale.AddCatalog(wxT("HeeksCAD"));
	}
}

#ifndef WIN32

std::auto_ptr<VectorFonts>	& HeeksCADapp::GetAvailableFonts(const bool force_read /* = false */ )
{
    static bool already_searched_for_vector_fonts = false;

    if ((already_searched_for_vector_fonts == false) || (force_read == true))
    {
        if (m_pVectorFonts.get() == NULL)
        {
            std::vector<wxString> paths = Tokens( m_font_paths, _T(";") );

            wxProgressDialog progress(	wxString(_T("Reading Vector Font Definition Files")),
							wxString(_T("Reading Vector Font Definition Files")),
							paths.size(),
							NULL,
							wxPD_APP_MODAL | wxPD_ELAPSED_TIME | wxPD_ESTIMATED_TIME | wxPD_REMAINING_TIME | wxPD_AUTO_HIDE );

            int i=0;
            for (std::vector<wxString>::const_iterator l_itPath = paths.begin(); l_itPath != paths.end(); l_itPath++)
            {
                progress.Update(i++);

                if (m_pVectorFonts.get() == NULL)
                {
                    m_pVectorFonts = std::auto_ptr<VectorFonts>(new VectorFonts(*l_itPath, m_word_space_percentage, m_character_space_percentage));
					m_pVectorFonts->SetWordSpacePercentage( m_word_space_percentage );
					m_pVectorFonts->SetCharacterSpacePercentage( m_character_space_percentage );
                } // End if - then
                else
                {
                    m_pVectorFonts->Add( *l_itPath );
                } // End if - else
            } // End for
        } // End if - then

        already_searched_for_vector_fonts = true; // Don't keep looking for something that may not be there
    }

	return(m_pVectorFonts);
} // End GetAvailableFonts() method
#endif

void HeeksCADapp::GetPluginsFromCommandLineParams(std::list<wxString> &plugins)
{
#ifdef __WXMSW__
	// to do, make this compile in Linux
	{
		// Open the file passed in the command line argument
		wxCmdLineEntryDesc cmdLineDesc[2];
		cmdLineDesc[0].kind = wxCMD_LINE_PARAM;
		cmdLineDesc[0].shortName = NULL;
		cmdLineDesc[0].longName = NULL;
		cmdLineDesc[0].description = "input files";
		cmdLineDesc[0].type = wxCMD_LINE_VAL_STRING;
		cmdLineDesc[0].flags = wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE;

		cmdLineDesc[1].kind = wxCMD_LINE_NONE;

		//gets the passed files from cmd line
		wxCmdLineParser parser (cmdLineDesc, argc, argv);

		// get filenames from the commandline
		if (parser.Parse() == 0)
		{
			for(unsigned int i = 0; i<parser.GetParamCount(); i++)
			{
				wxString param = parser.GetParam(i);
#ifdef WIN32
				if(param.Lower().EndsWith(_T(".dll")) || param.Lower().EndsWith(_T(".pyd")))
#else
				if(param.Lower().EndsWith(_T(".so")))
#endif
				{
					plugins.push_back(param);
				}
			}
		}
	}
#endif
}

void HeeksCADapp::RegisterOnBuildTexture(void(*callbackfunc)())
{
	m_on_build_texture_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RegisterOnBeforeNewOrOpen(void(*callbackfunc)(int, int))
{
	m_beforeneworopen_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RegisterOnBeforeFrameDelete(void(*callbackfunc)())
{
	m_beforeframedelete_callbacks.push_back(callbackfunc);
}

bool HeeksCADapp::RegisterFileOpenHandler( const std::list<wxString> file_extensions, FileOpenHandler_t fileopen_handler )
{
    std::set<wxString> valid_extensions;

    // For Linux, where the file system supports case-sensitive file names, we should expand
    // the extensions to include uppercase and lowercase and add them to our set.

    for (std::list<wxString>::const_iterator l_itExtension = file_extensions.begin(); l_itExtension != file_extensions.end(); l_itExtension++)
    {
        wxString extension(*l_itExtension);

        // Make sure the calling routine didn't add the '.'
        if (extension.StartsWith(_T(".")))
        {
            extension.Remove(0,1);
        }

        #ifndef WIN32
            extension.LowerCase();
            valid_extensions.insert( extension );

            extension.UpperCase();
            valid_extensions.insert( extension );
        #else
            extension.LowerCase();
            valid_extensions.insert( extension );
        #endif // WIN32
    } // End for

    for (std::set<wxString>::iterator itExtension = valid_extensions.begin(); itExtension != valid_extensions.end(); itExtension++)
    {
        if (m_fileopen_handlers.find( *itExtension ) != m_fileopen_handlers.end())
        {
            printf("Aborting file-open handler registration for extension %s as it has already been registered\n", Ttc( *itExtension ));
            return(false);
        }
    }

    // We must not have seen these extensions before.  Go ahead and register them.
    for (std::set<wxString>::iterator itExtension = valid_extensions.begin(); itExtension != valid_extensions.end(); itExtension++)
    {
        m_fileopen_handlers.insert( std::make_pair( *itExtension, fileopen_handler ) );
    }

    return(true);
}

bool HeeksCADapp::UnregisterFileOpenHandler( void (*fileopen_handler)(const wxChar *path) )
{
    std::list<FileOpenHandlers_t::iterator> remove;
    for (FileOpenHandlers_t::iterator itHandler = m_fileopen_handlers.begin(); itHandler != m_fileopen_handlers.end(); itHandler++)
    {
        if (itHandler->second == fileopen_handler) remove.push_back(itHandler);
    }

    for (std::list<FileOpenHandlers_t::iterator>::iterator itRemove = remove.begin(); itRemove != remove.end(); itRemove++)
    {
        m_fileopen_handlers.erase( *itRemove );
    }

    return(remove.size() > 0);
}


void HeeksCADapp::RegisterUnitsChangeHandler( void (*units_changed_handler)(const double value) )
{
    m_units_changed_handlers.push_back( units_changed_handler );
}

void HeeksCADapp::UnregisterUnitsChangeHandler( void (*units_changed_handler)(const double value) )
{
    for (UnitsChangedHandlers_t::iterator itHandler = m_units_changed_handlers.begin(); itHandler != m_units_changed_handlers.end(); /* increment within loop */ )
    {
        if (units_changed_handler == *itHandler)
        {
            itHandler = m_units_changed_handlers.erase(itHandler);
        }
        else
        {
            itHandler++;
        }
    }
}


void HeeksCADapp::RegisterHeeksTypesConverter( wxString(*converter)(const int type) )
{
    m_heeks_types_converters.push_back( converter );
}

void HeeksCADapp::UnregisterHeeksTypesConverter( wxString(*converter)(const int type) )
{
    for (HeeksTypesConverters_t::iterator itConverter = m_heeks_types_converters.begin(); itConverter != m_heeks_types_converters.end(); /* increment within loop */ )
    {
        if (converter == *itConverter)
        {
            itConverter = m_heeks_types_converters.erase(itConverter);
        }
        else
        {
            itConverter++;
        }
    }
}

wxString HeeksCADType(const int type)
{
    switch (type)
    {
        case UnknownType:   return(_("Unknown"));
        case DocumentType:   return(_("Document"));
        case PointType:   return(_("Point"));
        case LineType:   return(_("Line"));
        case ArcType:   return(_("Arc"));
        case ILineType:   return(_("ILine"));
        case CircleType:   return(_("Circle"));
        case GripperType:   return(_("Gripper"));
        case VertexType:   return(_("Vertex"));
        case EdgeType:   return(_("Edge"));
        case FaceType:   return(_("Face"));
        case LoopType:   return(_("Loop"));
        case SolidType:   return(_("Solid"));
        case StlSolidType:   return(_("StlSolid"));
        case WireType:   return(_("Wire"));
        case SketchType:   return(_("Sketch"));
        case ImageType:   return(_("Image"));
        case CoordinateSystemType:   return(_("CoordinateSystem"));
        case TextType:   return(_("Text"));
        case DimensionType:   return(_("Dimension"));
        case RulerType:   return(_("Ruler"));
        case XmlType:   return(_("Xml"));
        case EllipseType:   return(_("Ellipse"));
        case SplineType:   return(_("Spline"));
        case GroupType:   return(_("Group"));
        case CorrelationToolType:   return(_("CorrelationTool"));
        case AngularDimensionType:   return(_("AngularDimension"));
        case OrientationModifierType:   return(_("OrientationModifier"));
        case HoleType:   return(_("Hole"));
        case HolePositionsType:   return(_("Positions"));
        case ObjectMaximumType:   return(_("ObjectMaximum"));
        default:    return(_T("")); // Indicate that this routine could not find a conversion.
    } // End switch
} // End HeeksCADType() routine


wxString HeeksCADapp::HeeksType( const int type ) const
{

    for (HeeksTypesConverters_t::const_iterator itConverter = m_heeks_types_converters.begin(); itConverter != m_heeks_types_converters.end(); itConverter++ )
    {
        wxString result = (*itConverter)(type);
        if (result.Length() > 0)
        {
            return(result);
        }
    }

    wxString buf;
    buf << type;
    return(buf);
}


unsigned int HeeksCADapp::GetIndex(HeeksObj *object)
{
    return m_marked_list->GetIndex(object);
}


void HeeksCADapp::ReleaseIndex(unsigned int index)
{
    if(m_marked_list)m_marked_list->ReleaseIndex(index);
}

void HeeksCADapp::GetExternalMarkedListTools(std::list<Tool*>& t_list)
{
	for(std::list< void(*)(std::list<Tool*>&) >::iterator It = m_markedlisttools_callbacks.begin(); It != m_markedlisttools_callbacks.end(); It++)
	{
		void(*callbackfunc)(std::list<Tool*>& t_list) = *It;
		(*callbackfunc)(t_list);
	}
}

void HeeksCADapp::RegisterMarkeListTools(void(*callbackfunc)(std::list<Tool*>& t_list))
{
	m_markedlisttools_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RegisterOnRestoreDefaults(void(*callbackfunc)())
{
	m_on_restore_defaults_callbacks.push_back(callbackfunc);
}

void HeeksCADapp::RestoreDefaults()
{
	HeeksConfig config;
	config.DeleteAll();
	for(std::list< void(*)() >::iterator It = m_on_restore_defaults_callbacks.begin(); It != m_on_restore_defaults_callbacks.end(); It++)
	{
		void(*callbackfunc)() = *It;
		(*callbackfunc)();
	}
	wxGetApp().m_settings_restored = true;
}

#ifdef USING_RIBBON
void HeeksCADapp::AddRibbonPanels(wxRibbonBar* ribbon, wxRibbonPage* main_page)
{
	for (std::list< void(*)(wxRibbonBar*, wxRibbonPage*) >::iterator It = m_AddRibbonPanels_list.begin(); It != m_AddRibbonPanels_list.end(); It++)
	{
		void(*callbackfunc)(wxRibbonBar*, wxRibbonPage*) = *It;
		(*callbackfunc)(ribbon, main_page);
	}
}
#endif


#ifdef HAVE_TOOLBARS


static CFlyOutList* toolbar_flyout = NULL;

void HeeksCADapp::StartToolBarFlyout(const wxString& title_and_bitmap)
{
	if (toolbar_flyout)delete toolbar_flyout;
	toolbar_flyout = new CFlyOutList(title_and_bitmap);
}

void HeeksCADapp::AddFlyoutButton(const wxString& title, const wxBitmap& bitmap, const wxString& tooltip, void(*onButtonFunction)(wxCommandEvent&))
{
	if (toolbar_flyout)
	{
		toolbar_flyout->m_list.push_back(new CFlyOutItem(title, bitmap, tooltip, onButtonFunction));
	}
}

void HeeksCADapp::EndToolBarFlyout(wxToolBar* toolbar)
{
	if (toolbar_flyout)
	{
		wxGetApp().m_frame->AddToolBarFlyout(toolbar, *toolbar_flyout);
		delete toolbar_flyout;
		toolbar_flyout = NULL;
	}
}

#endif