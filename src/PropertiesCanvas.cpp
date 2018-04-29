// PropertiesCanvas.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.
#include "stdafx.h"
#include "PropertiesCanvas.h"
#include "MarkedObject.h"
#include "Property.h"
#include "PropertyChoice.h"
#include "PropertyCheck.h"
#include "PropertyString.h"
#include "PropertyFile.h"
#include "PropertyDouble.h"
#include "PropertyLength.h"
#include "PropertyColor.h"
#include "PropertyInt.h"
#include "PropertyList.h"
#include "PropertyVertex.h"
#include "PropertyTrsf.h"
#include "propgrid.h"
#include "advprops.h"
#include "MarkedList.h"
#include "HeeksFrame.h"
#include "CoordinateSystem.h"
#include "PropertyChange.h"

BEGIN_EVENT_TABLE(CPropertiesCanvas, wxScrolledWindow)
	EVT_SIZE(CPropertiesCanvas::OnSize)

        // This occurs when a property value changes
        EVT_PG_CHANGED( -1, CPropertiesCanvas::OnPropertyGridChange )
END_EVENT_TABLE()

CPropertiesCanvas::CPropertiesCanvas(wxWindow* parent)
: wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
				   wxHSCROLL | wxVSCROLL | wxNO_FULL_REPAINT_ON_RESIZE)
{
	// Assumes code is in frame/dialog constructor

	// Construct wxPropertyGrid control
	m_pg = new wxPropertyGrid(
		this, // parent
		-1, // id
		wxDefaultPosition, // position
		wxDefaultSize, // size
		// Some specific window styles - for all additional styles,
		// see Modules->PropertyGrid Window Styles
		wxPG_SPLITTER_AUTO_CENTER | // Automatically center splitter until user manually adjusts it
		// Default style
		wxPG_DEFAULT_STYLE | wxBORDER_THEME );

	m_pg->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );  

	m_map = new PropertyMapItem(NULL);

	wxGetApp().RegisterObserver(this);
}

CPropertiesCanvas::~CPropertiesCanvas()
{
	ClearProperties();
	wxGetApp().RemoveObserver(this);
	delete m_pg;
	delete m_map;
}

void CPropertiesCanvas::OnDraw(wxDC& dc)
{
	if(m_refresh_wanted_on_draw)
	{
		RefreshByRemovingAndAddingAll2();
	}
	m_refresh_wanted_on_draw = false;

	wxScrolledWindow::OnDraw(dc);
}

void CPropertiesCanvas::OnSize(wxSizeEvent& event)
{
	wxScrolledWindow::HandleOnSize(event);

	wxSize size = GetClientSize();
	m_pg->SetSize(0, 0, size.x, size.y );

    event.Skip();
}

void CPropertiesCanvas::ClearProperties()
{
	m_pg->Clear();
	pmap.clear();
	delete m_map;
	m_map = new PropertyMapItem(NULL);

	// i think these live in the undo now
#if 0
	for(std::set<Property*>::iterator It = pset.begin(); It != pset.end(); It++)
	{
		Property* p = *It; 
		delete p;
	}
#endif
	pset.clear();
}

PropertyMapItem* PropertyMapItem::OnAddProperty(wxPGProperty* prop, bool& new_item)
{
	const wxString& label = prop->GetLabel();

	std::pair<std::map<wxString, PropertyMapItem>::iterator, bool> insert_return = m_children.insert(std::make_pair(label, PropertyMapItem(prop)));
	new_item = insert_return.second;

	return &(insert_return.first->second);
}

PropertyMapItem* PropertyMapItem::FindItem(const wxString& str)
{
	std::map<wxString, PropertyMapItem>::iterator FindIt = m_children.find(str);
	if(FindIt == m_children.end())
	{
		return &(FindIt->second);
	}

	return NULL;
}

PropertyMapItem* CPropertiesCanvas::FindMapItem(wxPGProperty* p)
{
	std::map<wxPGProperty*, PropertyMapItem* >::iterator FindIt;
	FindIt = pmap.find(p);
	if(FindIt == pmap.end())return NULL;
	return FindIt->second;
}

void CPropertiesCanvas::Append(wxPGProperty* parent_prop, wxPGProperty* new_prop, Property* property)
{
	PropertyMapItem* new_item = NULL;
	bool newly_inserted = false;

	if(parent_prop){
		new_item = FindMapItem(parent_prop)->OnAddProperty(new_prop, newly_inserted);
		if(newly_inserted)m_pg->AppendIn(parent_prop, new_prop);
	}
	else
	{
		new_item = m_map->OnAddProperty(new_prop, newly_inserted);
		if(newly_inserted)m_pg->Append(new_prop);
	}

	new_item->m_properties.push_back(property);
	pmap.insert(std::pair<wxPGProperty*, PropertyMapItem* >( new_prop, new_item));
	pset.insert(property);
}

void CPropertiesCanvas::AddProperty(Property* p, wxPGProperty* parent_prop)
{
	switch(p->get_property_type()){
	case StringPropertyType:
		{
			wxPGProperty *new_prop = wxStringProperty(p->GetShortString(),wxPG_LABEL, p->GetString());
			if(!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p);
			if(p->m_highlighted)m_pg->SetPropertyBackgroundColour(new_prop->GetId(), wxColour(71, 141, 248));
		}
		break;
	case DoublePropertyType:
		{
			wxPGProperty *new_prop = wxFloatProperty(p->GetShortString(), wxPG_LABEL, p->GetDouble());
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append(parent_prop, new_prop, p);
		}
		break;
	case LengthPropertyType:
		{
			wxPGProperty *new_prop = wxFloatProperty(p->GetShortString(), wxPG_LABEL, p->GetDouble() / wxGetApp().m_view_units);
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p);
		}
		break;
	case IntPropertyType:
		{
			wxPGProperty *new_prop = wxIntProperty(p->GetShortString(),wxPG_LABEL, p->GetInt());
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p);
		}
		break;
	case ColorPropertyType:
		{
			const HeeksColor& col = ((PropertyColor*)p)->GetColor();
			wxColour wcol(col.red, col.green, col.blue);
			wxPGProperty *new_prop = wxColourProperty(p->GetShortString(),wxPG_LABEL, wcol);
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p);
		}
		break;
	case ChoicePropertyType:
		{
			wxArrayString array_string;
			std::list< wxString > choices;
			((PropertyChoice*)p)->GetChoices(choices);
			std::list< wxString >::iterator It;
			for (It = choices.begin(); It != choices.end(); It++){
				array_string.Add(wxString(It->c_str()));
			}
			wxPGProperty *new_prop = wxEnumProperty(p->GetShortString(),wxPG_LABEL,array_string, ((PropertyChoice*)p)->GetInt());
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p );
		}
		break;
	case CheckPropertyType:
		{
			wxPGProperty* new_prop = wxBoolProperty(p->GetShortString(),wxPG_LABEL, p->GetBool());
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p );
			m_pg->SetPropertyAttribute(new_prop, wxPG_BOOL_USE_CHECKBOX, true);
		}
		break;
	case ListOfPropertyType:
		{
			wxPGProperty* new_prop = wxParentProperty(p->GetShortString(),wxPG_LABEL);
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p );
			std::list< Property* > list;
			p->GetList(list);
			std::list< Property* >::iterator It;
			for (It = list.begin(); It != list.end(); It++){
				Property* p2 = *It;
				AddProperty(p2, new_prop);
			}
		}
		break;
	case FilePropertyType:
		{
			wxPGProperty *new_prop = wxFileProperty(p->GetShortString(),wxPG_LABEL, p->GetString());
			if (!p->m_editable)new_prop->SetFlag(wxPG_PROP_READONLY);
			Append( parent_prop, new_prop, p);
			if(p->m_highlighted)m_pg->SetPropertyBackgroundColour(new_prop->GetId(), wxColour(71, 141, 248));
		}
		break;
	}
}

std::list<Property*>* CPropertiesCanvas::GetProperties(wxPGProperty* p)
{
	PropertyMapItem* item = FindMapItem(p);
	if(item == NULL)return NULL;
	return &(item->m_properties);
}

void CPropertiesCanvas::OnPropertyGridChange( wxPropertyGridEvent& event ) {
	wxPGProperty* p = event.GetPropertyPtr();

	std::list<Property*>* properties = GetProperties(p);
	if(properties == NULL)return;

	std::list<Undoable*> changers;

	for(std::list<Property*>::iterator It = properties->begin(); It != properties->end(); It++)
	{
		Property* property = *It;

	switch(property->get_property_type()){
	case StringPropertyType:
		{
			PropertyChangeString* changer = new PropertyChangeString(event.GetPropertyValue().GetString(), (PropertyString*)property);
			changers.push_back(changer);
		}
		break;
	case DoublePropertyType:
		{
			PropertyChangeDouble* changer = new PropertyChangeDouble(event.GetPropertyValue().GetDouble(), (PropertyDouble*)property);
			changers.push_back(changer);
		}
		break;
	case LengthPropertyType:
		{
			PropertyChangeLength* changer = new PropertyChangeLength(event.GetPropertyValue().GetDouble() * wxGetApp().m_view_units, (PropertyLength*)property);
			changers.push_back(changer);
		}
		break;
	case IntPropertyType:
		{
			PropertyChangeInt* changer = new PropertyChangeInt(event.GetPropertyValue().GetLong(), (PropertyInt*)property);
			changers.push_back(changer);
		}
		break;
	case ColorPropertyType:
		{
			wxVariant var = event.GetPropertyValue();
			const wxColour* wcol = wxGetVariantCast(var,wxColour);
			HeeksColor col(wcol->Red(), wcol->Green(), wcol->Blue());
			PropertyChangeColor* changer = new PropertyChangeColor(col, (PropertyColor*)property);
			changers.push_back(changer);
		}
		break;
	case ChoicePropertyType:
		{
			PropertyChangeChoice* changer = new PropertyChangeChoice(event.GetPropertyValue().GetLong(), (PropertyChoice*)property);
			changers.push_back(changer);
			//(*(((PropertyChoice*)property)->m_callbackfunc))(event.GetPropertyValue().GetLong(), ((PropertyChoice*)property)->m_object);
		}
		break;
	case CheckPropertyType:
		{
			PropertyChangeCheck* changer = new PropertyChangeCheck(event.GetPropertyValue().GetBool(), (PropertyCheck*)property);
			changers.push_back(changer);
			//(*(((PropertyCheck*)property)->m_callbackfunc))(event.GetPropertyValue().GetBool(), ((PropertyCheck*)property)->m_object);
		}
		break;
	case ListOfPropertyType:
		{
		}
		break;
	case FilePropertyType:
		{
			PropertyChangeString* changer = new PropertyChangeString(event.GetPropertyValue().GetString(), (PropertyString*)property);
			changers.push_back(changer);
		}
		break;
	}
	}

	wxGetApp().StartHistory();
	for(std::list<Undoable*>::iterator It = changers.begin(); It != changers.end(); It++)
	{
		Undoable* changer = *It;
		wxGetApp().DoUndoable(changer);
	}
	wxGetApp().EndHistory();
}

void CPropertiesCanvas::OnPropertyGridSelect( wxPropertyGridEvent& event ) {
#if 0
	wxPGProperty* p = event.GetPropertyPtr();

	std::list<Property*>* properties = GetProperties(p);
	if(properties == NULL)return;

	// to do, use a PropertyChange undoable command
	for(std::list<Property*>::iterator It = properties->begin(); It != properties->end(); It++)
	{
		Property* property = *It;
		if(property->m_selectcallback)
			(*(property->m_selectcallback))(((PropertyChoice*)property)->m_object);
	}
#endif
}

void CPropertiesCanvas::DeselectProperties()
{
	m_pg->DoSelectProperty(NULL);
}

void CPropertiesCanvas::RefreshByRemovingAndAddingAll()
{
#if 0
	m_refresh_wanted_on_draw = true;
#else
	RefreshByRemovingAndAddingAll2();
#endif
	Refresh();
}

void CPropertiesCanvas::Freeze()
{
	wxScrolledWindow::Freeze();
}

void CPropertiesCanvas::Thaw()
{
	wxScrolledWindow::Thaw();
}

void PropertyChange::Run(bool redo)
{
}

void PropertyChange::RollBack()
{
}
