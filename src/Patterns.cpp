// Patterns.cpp
// Copyright (c) 2013, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Patterns.h"
#include "Program.h"
#include "PropertyChoice.h"
#include "Pattern.h"
#include "HeeksConfig.h"
#include "tinyxml.h"
#include <wx/stdpaths.h>

bool CPatterns::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == PatternType));
}


HeeksObj *CPatterns::MakeACopy(void) const
{
    return(new CPatterns(*this));  // Call the copy constructor.
}


CPatterns::CPatterns()
{
    HeeksConfig config;
}


CPatterns::CPatterns( const CPatterns & rhs ) : ObjList(rhs)
{
}

CPatterns & CPatterns::operator= ( const CPatterns & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
    }
    return(*this);
}


const wxBitmap &CPatterns::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(wxGetApp().GetResFolder() + _T("/icons/patterns.png")));
	return *icon;
}

void CPatterns::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Patterns" );
	wxGetApp().LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CPatterns::ReadFromXMLElement(TiXmlElement* pElem)
{
	CPatterns* new_object = new CPatterns;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

void CPatterns::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	HeeksCADapp::GetNewPatternTools(t_list);

	ObjList::GetTools(t_list, p);
}
