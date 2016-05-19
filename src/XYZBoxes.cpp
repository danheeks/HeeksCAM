#include <stdafx.h>
#include "XYZBoxes.h"
#include "HDialogs.h"
#include "NiceTextCtrl.h"

XYZBoxes::XYZBoxes(HDialog *dlg, const wxString& label, const wxString &xstr, const wxString &ystr, const wxString &zstr) :wxStaticBoxSizer(wxVERTICAL, dlg, label)
{
	m_sttcX = dlg->AddLabelAndControl(this, xstr, m_lgthX = new CLengthCtrl(dlg));
	m_sttcY = dlg->AddLabelAndControl(this, ystr, m_lgthY = new CLengthCtrl(dlg));
	m_sttcZ = dlg->AddLabelAndControl(this, zstr, m_lgthZ = new CLengthCtrl(dlg));
}
