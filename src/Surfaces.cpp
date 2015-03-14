// Surfaces.cpp
// Copyright (c) 2013, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Surfaces.h"
#include "Program.h"
#include "PropertyChoice.h"
#include "Surface.h"
#include "HeeksConfig.h"
#include "tinyxml.h"
#include <wx/stdpaths.h>

bool CSurfaces::CanAdd(HeeksObj* object)
{
	return 	((object != NULL) && (object->GetType() == SurfaceType));
}


HeeksObj *CSurfaces::MakeACopy(void) const
{
    return(new CSurfaces(*this));  // Call the copy constructor.
}


CSurfaces::CSurfaces()
{
    HeeksConfig config;
}


CSurfaces::CSurfaces( const CSurfaces & rhs ) : ObjList(rhs)
{
}

CSurfaces & CSurfaces::operator= ( const CSurfaces & rhs )
{
    if (this != &rhs)
    {
        ObjList::operator=( rhs );
    }
    return(*this);
}


const wxBitmap &CSurfaces::GetIcon()
{
	static wxBitmap* icon = NULL;
	if(icon == NULL)icon = new wxBitmap(wxImage(wxGetApp().GetResFolder() + _T("/icons/surfaces.png")));
	return *icon;
}

void CSurfaces::WriteXML(TiXmlNode *root)
{
	TiXmlElement * element;
	element = new TiXmlElement( "Surfaces" );
	wxGetApp().LinkXMLEndChild( root,  element );
	WriteBaseXML(element);
}

//static
HeeksObj* CSurfaces::ReadFromXMLElement(TiXmlElement* pElem)
{
	CSurfaces* new_object = new CSurfaces;
	new_object->ReadBaseXML(pElem);
	return new_object;
}

void CSurfaces::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	HeeksCADapp::GetNewSurfaceTools(t_list);

	ObjList::GetTools(t_list, p);
}
