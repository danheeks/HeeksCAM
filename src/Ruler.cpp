// Ruler.cpp
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

#include "stdafx.h"
#include "Ruler.h"
#include "Gripper.h"
#include "Tool.h"
#include "PropertyCheck.h"
#include "PropertyLength.h"
#include "PropertyChoice.h"
#include "HeeksFrame.h"
#include "ObjPropsCanvas.h"
#include "HeeksConfig.h"

void RulerMark::glCommands(double units)
{
	glDisable(GL_POLYGON_OFFSET_FILL);
	double half_width = width/2;
	double dpos = (double)pos;
	if(units > 25.0)
	{
		dpos *= 2.54; // position of the tenth of an inch, in mm
	}

	if(wxGetApp().GetPixelScale() < 10)
	{
		// draw a line
		glBegin(GL_LINES);
		glVertex2d(dpos, 0.0);
		glVertex2d(dpos, -length);
		glEnd();
	}
	else
	{
		// draw triangles
		double p[5][3] = {
			{dpos, 0.0, 0.0},
			{dpos - half_width, -half_width, 0.0},
			{dpos - half_width, -length, 0.0},
			{dpos + half_width, -length, 0.0},
			{dpos + half_width, -half_width, 0.0}
		};

		glBegin(GL_TRIANGLES);
		glVertex3dv(p[0]);
		glVertex3dv(p[1]);
		glVertex3dv(p[4]);
		glVertex3dv(p[1]);
		glVertex3dv(p[2]);
		glVertex3dv(p[4]);
		glVertex3dv(p[2]);
		glVertex3dv(p[3]);
		glVertex3dv(p[4]);
		glEnd();
	}

	// draw text
	if(pos == 0)
	{
		wxString str = _T("cm");
		if(units > 25.0)str = _T("inches");
		glPushMatrix();
		glTranslated(dpos + half_width, -length + 2.05, 0.0);
		glColor4ub(0, 0, 0, 255);
		wxGetApp().render_text(str, false);
		glPopMatrix();
	}
	else if(pos % 10 == 0)
	{
		float text_width, text_height;
		wxString str = wxString::Format(_T("%d"), pos/10);
		if(!wxGetApp().get_text_size(str, &text_width, &text_height))return;
		glPushMatrix();
		glTranslated(dpos - half_width - text_width, -length + 2.05, 0.0);
		wxGetApp().render_text(str, false);
		glPopMatrix();
	}
	glEnable(GL_POLYGON_OFFSET_FILL);
}

HRuler::HRuler() : m_gl_list(0), m_select_gl_list(0)
{
	m_use_view_units = true;
	m_units = 1.0;
	m_width = 25;
	m_length = 312; // long enough for 12 inches
	m_empty_length = 3;
}

void HRuler::GetFourCorners(gp_Pnt *point)
{
	point[0] = gp_Pnt(-m_empty_length, -m_width, 0);
	point[1] = gp_Pnt(m_length - m_empty_length, -m_width, 0);
	point[2] = gp_Pnt(m_length - m_empty_length, 0, 0);
	point[3] = gp_Pnt(-m_empty_length, 0, 0);
}

double HRuler::GetUnits()
{
	if(m_use_view_units)return wxGetApp().m_view_units;
	return m_units;
}

void HRuler::CalculateMarks(std::list<RulerMark> &marks)
{
	if(GetUnits() > 25.0)
	{
		// inches
		int num_tenths = (int)(m_length / 2.54 - 2 * m_empty_length / 2.54 + 0.0001);

		for(int i = 0; i<= num_tenths; i++)
		{
			RulerMark mark;
			if(i % 10 == 0)
			{
				// big mark
				mark.length = 3.0; 
				mark.width = 0.1;
			}
			else if(i % 5 == 0)
			{
				// medium mark
				mark.length = 2.0; 
				mark.width = 0.1;
			}
			else
			{
				// small mark
				mark.length = 1.0; 
				mark.width = 0.1;
			}

			mark.pos = i;

			marks.push_back(mark);
		}
	}
	else
	{
		int num_mm = (int)(m_length - 2 * m_empty_length + 0.0001);

		for(int i = 0; i<= num_mm; i++)
		{
			RulerMark mark;
			if(i % 10 == 0)
			{
				// big mark
				mark.length = 3.0; 
				mark.width = 0.1;
			}
			else if(i % 5 == 0)
			{
				// medium mark
				mark.length = 2.0; 
				mark.width = 0.1;
			}
			else
			{
				// small mark
				mark.length = 1.0; 
				mark.width = 0.1;
			}

			mark.pos = i;

			marks.push_back(mark);
		}
	}
}

void HRuler::glCommands(bool select, bool marked, bool no_color)
{
	double m[16];
	extract_transposed(m_trsf, m);
	glPushMatrix();
	glMultMatrixd(m);

	int* plist = select ? &m_select_gl_list : &m_gl_list;
	if (*plist)
	{
		glCallList(*plist);
	}
	else{
		*plist = glGenLists(1);
		glNewList(*plist, GL_COMPILE_AND_EXECUTE);

		// draw a filled white rectangle
		glDisable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0, 0.0);
		if(!no_color)glColor4ub(255, 255, 255, 120); // white
		if (!select)wxGetApp().EnableBlend();
		glDepthMask(0);
		gp_Pnt point[4];
		GetFourCorners(point);
		glBegin(GL_TRIANGLES);
		glVertex3d(point[0].X(), point[0].Y(), point[0].Z());
		glVertex3d(point[1].X(), point[1].Y(), point[1].Z());
		glVertex3d(point[2].X(), point[2].Y(), point[2].Z());
		glVertex3d(point[0].X(), point[0].Y(), point[0].Z());
		glVertex3d(point[2].X(), point[2].Y(), point[2].Z());
		glVertex3d(point[3].X(), point[3].Y(), point[3].Z());
		glEnd();
		if(select)wxGetApp().DisableBlend();
		glDepthMask(1);

		if (!no_color)
		{
			// draw a black rectangle border
			glColor4ub(0, 0, 0, 255); // black

			glBegin(GL_LINE_STRIP);
			glVertex3d(point[0].X(), point[0].Y(), point[0].Z());
			glVertex3d(point[1].X(), point[1].Y(), point[1].Z());
			glVertex3d(point[2].X(), point[2].Y(), point[2].Z());
			glVertex3d(point[3].X(), point[3].Y(), point[3].Z());
			glVertex3d(point[0].X(), point[0].Y(), point[0].Z());
			glEnd();

			// draw the marks ( with their numbers )
			std::list<RulerMark> marks;
			CalculateMarks(marks);
			for (std::list<RulerMark>::iterator It = marks.begin(); It != marks.end(); It++)
			{
				RulerMark& mark = *It;
				mark.glCommands(GetUnits());
			}
		}
		glEnable(GL_POLYGON_OFFSET_FILL);

		glEndList();
	}

	glPopMatrix();
}

void HRuler::KillGLLists()
{
	if (m_gl_list)
	{
		glDeleteLists(m_gl_list, 1);
		m_gl_list = 0;
	}
	if (m_select_gl_list)
	{
		glDeleteLists(m_select_gl_list, 1);
		m_select_gl_list = 0;
	}
}

void HRuler::GetBox(CBox &box)
{
	gp_Pnt point[4];
	GetFourCorners(point);

	for(int i = 0; i<4; i++)
	{
		point[i].Transform(m_trsf);
		box.Insert(point[i].X(), point[i].Y(), point[i].Z());
	}
}

HeeksObj *HRuler::MakeACopy(void)const{ return new HRuler(*this);}

void HRuler::ModifyByMatrix(const double *mat)
{
	m_trsf = make_matrix(mat) * m_trsf;
}

void HRuler::GetGripperPositions(std::list<GripData> *list, bool just_for_endof)
{
	gp_Pnt point[4];
	GetFourCorners(point);

	for(int i = 0; i<4; i++)point[i].Transform(m_trsf);

	list->push_back(GripData(GripperTypeRotateObject,point[0].X(),point[0].Y(),point[0].Z(),NULL));
	list->push_back(GripData(GripperTypeRotateObject,point[2].X(),point[2].Y(),point[2].Z(),NULL));
}

class ResetRulerTool:public Tool{
	// reset ruler
public:
	void Run(){
		wxGetApp().m_ruler->m_trsf = gp_Trsf();
		wxGetApp().m_frame->RefreshProperties();
		wxGetApp().Repaint();
	}
	const wxChar* GetTitle(){return _("ResetRuler");}
	wxString BitmapPath(){return _T("resetruler");}
	const wxChar* GetToolTip(){return _("Reset the ruler");}
};

static ResetRulerTool reset_ruler_tool;

class PropertyUseViewUnits :public PropertyCheck
{
public:
	PropertyUseViewUnits(HeeksObj* object) :PropertyCheck(object, _("use view units"), &((HRuler*)object)->m_use_view_units){}
	Property *MakeACopy(void)const{ return new PropertyUseViewUnits(*this); }
	void Set(bool value)
	{
		PropertyCheck::Set(value);
		wxGetApp().m_frame->RefreshProperties();
		m_object->KillGLLists();
	}
};

class PropertyRulerUnits : public Property
{
public:
	PropertyRulerUnits(HeeksObj* object) : Property(object, _("units")){}
	Property *MakeACopy(void)const{ return new PropertyRulerUnits(*this); }
	virtual void GetChoices(std::list< wxString > &choices){
		choices.push_back ( wxString ( _("mm") ) );
		choices.push_back ( wxString ( _("inch") ) );
	}
	virtual int get_property_type(){ return ChoicePropertyType; }
	void Set(int value)
	{
		((HRuler*)m_object)->m_units = (value == 0) ? 1.0 : 25.4;
		m_object->KillGLLists();
	}
	int GetInt()const
	{
		if (((HRuler*)m_object)->m_units > 25.0)return 1;
		return 0;
	}
};

void HRuler::GetProperties(std::list<Property *> *list)
{
	list->push_back(new PropertyUseViewUnits(this));
	if(!m_use_view_units){
		list->push_back(new PropertyRulerUnits(this));
	}
	list->push_back(new PropertyLengthWithKillGLLists(this, _("width"), &m_width ));
	list->push_back(new PropertyLengthWithKillGLLists(this, _("length"), &m_length ));
	list->push_back(new PropertyLengthWithKillGLLists(this, _("empty_length"), &m_empty_length ));

	HeeksObj::GetProperties(list);
}

void HRuler::GetTools(std::list<Tool*>* t_list, const wxPoint* p)
{
	t_list->push_back(&reset_ruler_tool);
}

bool HRuler::GetScaleAboutMatrix(double *m)
{
	extract(m_trsf, m);
	return true;
}

void HRuler::WriteToConfig(HeeksConfig& config)
{
	config.Write(_T("RulerTrsf11"), m_trsf.Value(1, 1));
	config.Write(_T("RulerTrsf12"), m_trsf.Value(1, 2));
	config.Write(_T("RulerTrsf13"), m_trsf.Value(1, 3));
	config.Write(_T("RulerTrsf14"), m_trsf.Value(1, 4));
	config.Write(_T("RulerTrsf21"), m_trsf.Value(2, 1));
	config.Write(_T("RulerTrsf22"), m_trsf.Value(2, 2));
	config.Write(_T("RulerTrsf23"), m_trsf.Value(2, 3));
	config.Write(_T("RulerTrsf24"), m_trsf.Value(2, 4));
	config.Write(_T("RulerTrsf31"), m_trsf.Value(3, 1));
	config.Write(_T("RulerTrsf32"), m_trsf.Value(3, 2));
	config.Write(_T("RulerTrsf33"), m_trsf.Value(3, 3));
	config.Write(_T("RulerTrsf34"), m_trsf.Value(3, 4));

	config.Write(_T("RulerUseViewUnits"), m_use_view_units);
	config.Write(_T("RulerUnits"), m_units);
	config.Write(_T("RulerWidth"), m_width);
	config.Write(_T("RulerLength"), m_length);
	config.Write(_T("RulerEmptyLength"), m_empty_length);
}

void HRuler::ReadFromConfig(HeeksConfig& config)
{
	double m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34;
	config.Read(_T("RulerTrsf11"), &m11, 1.0);
	config.Read(_T("RulerTrsf12"), &m12, 0.0);
	config.Read(_T("RulerTrsf13"), &m13, 0.0);
	config.Read(_T("RulerTrsf14"), &m14, 0.0);
	config.Read(_T("RulerTrsf21"), &m21, 0.0);
	config.Read(_T("RulerTrsf22"), &m22, 1.0);
	config.Read(_T("RulerTrsf23"), &m23, 0.0);
	config.Read(_T("RulerTrsf24"), &m24, 0.0);
	config.Read(_T("RulerTrsf31"), &m31, 0.0);
	config.Read(_T("RulerTrsf32"), &m32, 0.0);
	config.Read(_T("RulerTrsf33"), &m33, 1.0);
	config.Read(_T("RulerTrsf34"), &m34, 0.0);
#if OCC_VERSION_HEX >= 0x060900
	m_trsf.SetValues(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34);
#else
	m_trsf.SetValues(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, 0.0001, 0.00000001);
#endif

	config.Read(_T("RulerUseViewUnits"), &m_use_view_units);
	config.Read(_T("RulerUnits"), &m_units);
	config.Read(_T("RulerWidth"), &m_width);
	config.Read(_T("RulerLength"), &m_length);
	config.Read(_T("RulerEmptyLength"), &m_empty_length);
}
