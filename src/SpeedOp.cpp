// SpeedOp.cpp
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "stdafx.h"
#include "SpeedOp.h"
#include "HeeksConfig.h"
#include "ProgramCanvas.h"
#include "Program.h"
#include "PropertyInt.h"
#include "PropertyDouble.h"
#include "PropertyLength.h"
#include "PropertyList.h"
#include "PropertyCheck.h"
#include "tinyxml.h"
#include "Tool.h"
#include "CTool.h"

static void on_set_horizontal_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_horizontal_feed_rate = value;
	((CSpeedOp*)object)->WriteDefaultValues();
}

static void on_set_vertical_feed_rate(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_vertical_feed_rate = value;
	((CSpeedOp*)object)->WriteDefaultValues();
}

static void on_set_spindle_speed(double value, HeeksObj* object)
{
	((CSpeedOp*)object)->m_spindle_speed = value;
	((CSpeedOp*)object)->WriteDefaultValues();
}

void CSpeedOp::WriteXMLAttributes(TiXmlNode* pElem)
{
	TiXmlElement * element = new TiXmlElement( "speedop" );
	wxGetApp().LinkXMLEndChild( pElem,  element );
	element->SetDoubleAttribute( "hfeed", m_horizontal_feed_rate);
	element->SetDoubleAttribute( "vfeed", m_vertical_feed_rate);
	element->SetDoubleAttribute( "spin", m_spindle_speed);
}

void CSpeedOp::ReadFromXMLElement(TiXmlElement* pElem)
{
	TiXmlElement* speedop = wxGetApp().FirstNamedXMLChildElement(pElem, "speedop");
	if(speedop)
	{
		speedop->Attribute("hfeed", &m_horizontal_feed_rate);
		speedop->Attribute("vfeed", &m_vertical_feed_rate);
		speedop->Attribute("spin", &m_spindle_speed);

		wxGetApp().RemoveXMLChild(pElem, speedop);
	}
}

void CSpeedOp::glCommands(bool select, bool marked, bool no_color)
{
	COp::glCommands(select, marked, no_color);
}


CSpeedOp & CSpeedOp::operator= ( const CSpeedOp & rhs )
{
	if (this != &rhs)
	{
		COp::operator=(rhs);
		m_horizontal_feed_rate = rhs.m_horizontal_feed_rate;
		m_vertical_feed_rate = rhs.m_vertical_feed_rate;
		m_spindle_speed = rhs.m_spindle_speed;
	}

	return(*this);
}

CSpeedOp::CSpeedOp( const CSpeedOp & rhs ) : COp(rhs)
{
	m_horizontal_feed_rate = rhs.m_horizontal_feed_rate;
	m_vertical_feed_rate = rhs.m_vertical_feed_rate;
	m_spindle_speed = rhs.m_spindle_speed;
}

void CSpeedOp::WriteBaseXML(TiXmlElement *element)
{
	WriteXMLAttributes(element);
	COp::WriteBaseXML(element);
}

void CSpeedOp::ReadBaseXML(TiXmlElement* element)
{
	ReadFromXMLElement(element);
	COp::ReadBaseXML(element);
}

void CSpeedOp::WriteDefaultValues()
{
	COp::WriteDefaultValues();

	HeeksConfig config;
	config.Write(_T("SpeedOpHorizFeed"), m_horizontal_feed_rate);
	config.Write(_T("SpeedOpVertFeed"), m_vertical_feed_rate);
	config.Write(_T("SpeedOpSpindleSpeed"), m_spindle_speed);
}

void CSpeedOp::ReadDefaultValues()
{
	COp::ReadDefaultValues();

	HeeksConfig config;
	config.Read(_T("SpeedOpHorizFeed"), &m_horizontal_feed_rate, 100.0);
	config.Read(_T("SpeedOpVertFeed"), &m_vertical_feed_rate, 100.0);
	config.Read(_T("SpeedOpSpindleSpeed"), &m_spindle_speed, 7000);
}

void CSpeedOp::GetProperties(std::list<Property *> *list)
{
	if (CTool::IsMillingToolType(CTool::FindToolType(this->m_tool_number)))
	{
		list->push_back(new PropertyLength(_("horizontal feed rate"), m_horizontal_feed_rate, this, on_set_horizontal_feed_rate));
		list->push_back(new PropertyLength(_("vertical feed rate"), m_vertical_feed_rate, this, on_set_vertical_feed_rate));
		list->push_back(new PropertyDouble(_("spindle speed"), m_spindle_speed, this, on_set_spindle_speed));
	}
	COp::GetProperties(list);
}

Python CSpeedOp::AppendTextToProgram()
{
	Python python;

	python << COp::AppendTextToProgram();

	if (m_spindle_speed != 0)
	{
		python << _T("spindle(") << m_spindle_speed << _T(")\n");
	} // End if - then

	python << _T("feedrate_hv(") << m_horizontal_feed_rate / wxGetApp().m_program->m_units << _T(", ");
    python << m_vertical_feed_rate / wxGetApp().m_program->m_units << _T(")\n");
    python << _T("flush_nc()\n");

    return(python);
}

// static
void CSpeedOp::GetOptions(std::list<Property *> *list)
{
}

// static
void CSpeedOp::ReadFromConfig()
{
	HeeksConfig config;
}

// static
void CSpeedOp::WriteToConfig()
{
	HeeksConfig config;
}

void CSpeedOp::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
    COp::GetTools(t_list, p);
}

bool CSpeedOp::operator==(const CSpeedOp & rhs) const
{
	if (m_horizontal_feed_rate != rhs.m_horizontal_feed_rate) return(false);
	if (m_vertical_feed_rate != rhs.m_vertical_feed_rate) return(false);
	if (m_spindle_speed != rhs.m_spindle_speed) return(false);

	return(COp::operator==(rhs));
}




