// DepthOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "DepthOp.h"
#include "HeeksConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "PropertyInt.h"
#include "PropertyDouble.h"
#include "PropertyLength.h"
#include "PropertyString.h"
#include "tinyxml.h"
#include "Tool.h"
#include "CTool.h"


CDepthOp & CDepthOp::operator= ( const CDepthOp & rhs )
{
	if (this != &rhs)
	{
		CSpeedOp::operator=( rhs );
		m_clearance_height = rhs.m_clearance_height;
		m_start_depth = rhs.m_start_depth;
		m_step_down = rhs.m_step_down;
		m_z_finish_depth = rhs.m_z_finish_depth;
		m_z_thru_depth = rhs.m_z_thru_depth;
		m_final_depth = rhs.m_final_depth;
		m_rapid_safety_space = rhs.m_rapid_safety_space;

	}

	return(*this);
}

CDepthOp::CDepthOp( const CDepthOp & rhs ) : CSpeedOp(rhs)
{
	m_clearance_height = rhs.m_clearance_height;
	m_start_depth = rhs.m_start_depth;
	m_step_down = rhs.m_step_down;
	m_z_finish_depth = rhs.m_z_finish_depth;
	m_z_thru_depth = rhs.m_z_thru_depth;
	m_final_depth = rhs.m_final_depth;
	m_rapid_safety_space = rhs.m_rapid_safety_space;
}

void CDepthOp::ReloadPointers()
{
	CSpeedOp::ReloadPointers();
}


/**
	Set the starting depth to match the Z values on the sketches.

	If we've selected a chamfering bit then set the final depth such
	that a 1 mm chamfer is applied.  These are only starting points but
	we should make them as convenient as possible.
 */
static void on_set_clearance_height(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_clearance_height = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_step_down(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_step_down = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_z_finish_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_z_finish_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_z_thru_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_z_thru_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_user_depths(const wxChar* value, HeeksObj* object)
{
	((CDepthOp*)object)->m_user_depths.assign(value);
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_start_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_start_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_final_depth(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_final_depth = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

static void on_set_rapid_safety_space(double value, HeeksObj* object)
{
	((CDepthOp*)object)->m_rapid_safety_space = value;
	((CDepthOp*)object)->WriteDefaultValues();
}

void CDepthOp::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = new TiXmlElement( "depthop" );
	wxGetApp().LinkXMLEndChild( pElem,  element );
	element->SetDoubleAttribute( "clear", m_clearance_height);
	element->SetDoubleAttribute( "down", m_step_down);
	if(m_z_finish_depth > 0.0000001)element->SetDoubleAttribute( "zfinish", m_z_finish_depth);
	if(m_z_thru_depth > 0.0000001)element->SetDoubleAttribute( "zthru", m_z_thru_depth);
	element->SetAttribute("userdepths", wxString(m_user_depths.c_str()).utf8_str());
	element->SetDoubleAttribute( "startdepth", m_start_depth);
	element->SetDoubleAttribute( "depth", m_final_depth);
	element->SetDoubleAttribute( "r", m_rapid_safety_space);
}

void CDepthOp::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* element = wxGetApp().FirstNamedXMLChildElement(pElem, "depthop");
	if(element)
	{
		element->Attribute("clear", &m_clearance_height);
		element->Attribute("down", &m_step_down);
		element->Attribute("zfinish", &m_z_finish_depth);
		element->Attribute("zthru", &m_z_thru_depth);
		m_user_depths.assign(Ctt(element->Attribute("userdepths")));
		element->Attribute("startdepth", &m_start_depth);
		element->Attribute("depth", &m_final_depth);
		element->Attribute("r", &m_rapid_safety_space);
		wxGetApp().RemoveXMLChild(pElem, element);	// We don't want to interpret this again when
										// the ObjList::ReadBaseXML() method gets to it.
	}
}

void CDepthOp::glCommands(bool select, bool marked, bool no_color)
{
	CSpeedOp::glCommands(select, marked, no_color);
}

void CDepthOp::WriteBaseXML(TiXmlElement *element)
{
	WriteXMLAttributes(element);
	CSpeedOp::WriteBaseXML(element);
}

void CDepthOp::ReadBaseXML(TiXmlElement* element)
{
	ReadFromXMLElement(element);
	CSpeedOp::ReadBaseXML(element);
}

void CDepthOp::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyLength(_("clearance height"), m_clearance_height, this, on_set_clearance_height));

	if (CTool::IsMillingToolType(CTool::FindToolType(this->m_tool_number)))
	{
		list->push_back(new PropertyLength(_("rapid safety space"), m_rapid_safety_space, this, on_set_rapid_safety_space));
		list->push_back(new PropertyLength(_("start depth"), m_start_depth, this, on_set_start_depth));
		list->push_back(new PropertyLength(_("final depth"), m_final_depth, this, on_set_final_depth));
		list->push_back(new PropertyLength(_("max step down"), m_step_down, this, on_set_step_down));
		list->push_back(new PropertyLength(_("z finish depth"), m_z_finish_depth, this, on_set_z_finish_depth));
		list->push_back(new PropertyLength(_("z thru depth"), m_z_thru_depth, this, on_set_z_thru_depth));
		list->push_back(new PropertyString(_("user depths"), m_user_depths.c_str(), this, on_set_user_depths));
	}
	CSpeedOp::GetProperties(list);
}

void CDepthOp::WriteDefaultValues()
{
	CSpeedOp::WriteDefaultValues();

	HeeksConfig config;
	config.Write(_T("ClearanceHeight"), m_clearance_height);
	config.Write(_T("StartDepth"), m_start_depth);
	config.Write(_T("StepDown"), m_step_down);
	config.Write(_T("ZFinish"), m_z_finish_depth);
	config.Write(_T("ZThru"), m_z_thru_depth);
	config.Write(_T("UserDepths"), m_user_depths.c_str());
	config.Write(_T("FinalDepth"), m_final_depth);
	config.Write(_T("RapidDown"), m_rapid_safety_space);
}

void CDepthOp::ReadDefaultValues()
{
	CSpeedOp::ReadDefaultValues();

	HeeksConfig config;

	config.Read(_T("ClearanceHeight"), &m_clearance_height, 5.0);
	config.Read(_T("StartDepth"), &m_start_depth, 0.0);
	config.Read(_T("StepDown"), &m_step_down, 1.0);
	config.Read(_T("ZFinish"), &m_z_finish_depth, 0.0);
	config.Read(_T("ZThru"), &m_z_thru_depth, 0.0);
	wxString s;
	config.Read(_T("UserDepths"), &s, _T(""));
	m_user_depths = s;
	config.Read(_T("FinalDepth"), &m_final_depth, -1.0);
	config.Read(_T("RapidDown"), &m_rapid_safety_space, 2.0);
}

Python CDepthOp::AppendTextToProgram()
{
	Python python;

    python << CSpeedOp::AppendTextToProgram();

	python << _T("depthparams = depth_params(");
	python << _T("float(") << m_clearance_height / wxGetApp().m_program->m_units << _T(")");
	python << _T(", float(") << m_rapid_safety_space / wxGetApp().m_program->m_units << _T(")");
    python << _T(", float(") << m_start_depth / wxGetApp().m_program->m_units << _T(")");
    python << _T(", float(") << m_step_down / wxGetApp().m_program->m_units << _T(")");
    python << _T(", float(") << m_z_finish_depth / wxGetApp().m_program->m_units << _T(")");
    python << _T(", float(") << m_z_thru_depth / wxGetApp().m_program->m_units << _T(")");
    python << _T(", float(") << m_final_depth / wxGetApp().m_program->m_units << _T(")");
	if (m_user_depths.length() == 0) python << _T(", None");
	else python << _T(", [") << m_user_depths.c_str() << _T("]");
	python << _T(")\n");

	CTool *pTool = CTool::Find( m_tool_number );
	if (pTool != NULL)
	{
		python << _T("tool_diameter = float(") << (pTool->CuttingRadius(true) * 2.0) << _T(")\n");
		python << _T("cutting_edge_angle = float(") << pTool->m_cutting_edge_angle<< _T(")\n");

	} // End if - then

	return(python);
}

void CDepthOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    CSpeedOp::GetTools( t_list, p );
}

bool CDepthOp::operator== ( const CDepthOp & rhs ) const
{
	if (m_clearance_height != rhs.m_clearance_height) return(false);
	if (m_start_depth != rhs.m_start_depth) return(false);
	if (m_step_down != rhs.m_step_down) return(false);
	if (m_z_finish_depth != rhs.m_z_finish_depth) return(false);
	if (m_z_thru_depth != rhs.m_z_thru_depth) return(false);
	if (m_final_depth != rhs.m_final_depth) return(false);
	if (m_rapid_safety_space != rhs.m_rapid_safety_space) return(false);

	return(CSpeedOp::operator==(rhs));
}
