// HDialogs.cpp
// Copyright (c) 2010, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include <stdafx.h>
#include "HDialogs.h"

wxSizerItem* HControl::AddToSizer(wxSizer* s)
{
	if(m_w != NULL)return s->Add( m_w, 0, m_add_flag, HDialog::control_border );
	else return s->Add( m_s, 0, m_add_flag, HDialog::control_border );
}

const int HDialog::control_border = 3;

HDialog::HDialog(wxWindow *parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	:wxDialog(parent, id, title, pos, size, style, name), m_ignore_event_functions(false)
{
}

wxStaticText *HDialog::AddLabelAndControl(wxBoxSizer* sizer, const wxString& label, wxWindow* control, wxWindow* control2)
{
	wxStaticText *static_label;
	HControl c = MakeLabelAndControl(label, control, control2, &static_label);
	if(c.m_w)sizer->Add( c.m_w, 0, wxEXPAND | wxALL, control_border );
	else sizer->Add( c.m_s, 0, wxEXPAND | wxALL, control_border );
	return static_label;
}

HControl HDialog::MakeLabelAndControl(const wxString& label, wxWindow* control, wxStaticText** static_text)
{
	return MakeLabelAndControl(label, control, NULL, static_text);
}

HControl HDialog::MakeLabelAndControl(const wxString& label, wxWindow* control1, wxWindow* control2, wxStaticText** static_text)
{
    wxBoxSizer *sizer_horizontal = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *static_label = new wxStaticText(this, wxID_ANY, label);
	sizer_horizontal->Add( static_label, 0, wxRIGHT | wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL, control_border );
	sizer_horizontal->Add(control1, 1, wxLEFT | (control2 ? wxRIGHT:0) | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, control_border);
	if (control2)sizer_horizontal->Add(control2, 0, wxLEFT | wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL, control_border);
	if(static_text)*static_text = static_label;

	return HControl(sizer_horizontal, wxEXPAND | wxALL);
}

HControl HDialog::MakeOkAndCancel(int orient, bool help)
{
    wxBoxSizer *sizerOKCancel = new wxBoxSizer(orient);
    wxButton* buttonOK = new wxButton(this, wxID_OK, _("OK"));
	sizerOKCancel->Add( buttonOK, 0, wxALL, control_border );
    wxButton* buttonCancel = new wxButton(this, wxID_CANCEL, _("Cancel"));
	sizerOKCancel->Add( buttonCancel, 0, wxALL, control_border );
	if (help)
	{
		wxButton* buttonHelp = new wxButton(this, wxID_HELP, _("Help"));
		sizerOKCancel->Add( buttonHelp, 0, wxALL, control_border );
	}
    buttonOK->SetDefault();
	return HControl(sizerOKCancel, wxALL | wxALIGN_RIGHT | wxALIGN_BOTTOM);
}
