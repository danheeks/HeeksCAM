// Profile.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

#include "HeeksCNCTypes.h"
#include "SketchOp.h"
#include "Drilling.h"
#include "CNCPoint.h"

#include <vector>

class CProfile;
class CTags;

class CProfile: public CSketchOp{
private:
	CTags* m_tags;				// Access via Tags() method

public:
	typedef enum {
		eRightOrInside = -1,
		eOn = 0,
		eLeftOrOutside = +1
	}eSide;


	typedef enum {
		eConventional,
		eClimb
	}eCutMode;

	// these are only used when m_sketches.size() == 1
	int m_tool_on_side;
	int m_cut_mode;
	bool m_auto_roll_on;
	bool m_auto_roll_off;
	double m_auto_roll_radius;
	double m_lead_in_line_len;
	double m_lead_out_line_len;
	double m_roll_on_point_x;
	double m_roll_on_point_y;
	double m_roll_on_point_z;
	double m_roll_off_point_x;
	double m_roll_off_point_y;
	double m_roll_off_point_z;
	bool m_start_given;
	bool m_end_given;
	double m_start_x;
	double m_start_y;
	double m_start_z;
	double m_end_x;
	double m_end_y;
	double m_end_z;
	double m_extend_at_start;
	double m_extend_at_end;
	bool m_end_beyond_full_profile;
	int m_sort_sketches;
	double m_offset_extra; // in mm
	bool m_do_finishing_pass;
	bool m_only_finishing_pass; // don't do roughing pass
	double m_finishing_h_feed_rate;
	int m_finishing_cut_mode;
	double m_finishing_step_down;

	static double max_deviation_for_spline_to_arc;

	CProfile();
	CProfile(int sketch, const int tool_number );

	CProfile( const CProfile & rhs );
	CProfile & operator= ( const CProfile & rhs );

	bool operator==( const CProfile & rhs ) const;
	bool operator!=( const CProfile & rhs ) const { return(! (*this == rhs)); }

	bool IsDifferent( HeeksObj *other ) { return(*this != (*(CProfile *)other)); }

	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadParamsFromXMLElement(TiXmlElement* pElem);

	// HeeksObj's virtual functions
	int GetType()const{return ProfileType;}
	const wxChar* GetTypeString(void) const { return _("Profile"); }
	void glCommands(bool select, bool marked, bool no_color);
	const wxBitmap &GetIcon();
	void GetProperties(std::list<Property *> *list);
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	HeeksObj *MakeACopy(void)const;
	void CopyFrom(const HeeksObj* object);
	void WriteXML(TiXmlNode *root);
	bool Add(HeeksObj* object, HeeksObj* prev_object);
	void Remove(HeeksObj* object);
	bool CanAdd(HeeksObj* object);
	bool CanAddTo(HeeksObj* owner);
	void GetOnEdit(bool(**callback)(HeeksObj*));
	void WriteDefaultValues();
	void ReadDefaultValues();
	void Clear();

	// Data access methods.
	CTags* Tags(){return m_tags;}

	Python WriteSketchDefn(HeeksObj* sketch, bool reversed );
	Python AppendTextForSketch(HeeksObj* object, int cut_mode);

	// COp's virtual functions
	Python AppendTextToProgram();

	static HeeksObj* ReadFromXMLElement(TiXmlElement* pElem);
	void AddMissingChildren();
	Python AppendTextToProgram(bool finishing_pass);

	static void GetOptions(std::list<Property *> *list);
	static void ReadFromConfig();
	static void WriteToConfig();
};
