
class CLengthCtrl;
class HDialog;

class XYZBoxes : public wxStaticBoxSizer
{
public:
	CLengthCtrl *m_lgthX;
	CLengthCtrl *m_lgthY;
	CLengthCtrl *m_lgthZ;
	wxStaticText *m_sttcX;
	wxStaticText *m_sttcY;
	wxStaticText *m_sttcZ;

	XYZBoxes(HDialog *dlg, const wxString& label, const wxString &xstr, const wxString &ystr, const wxString &zstr);
};
