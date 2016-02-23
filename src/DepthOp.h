// DepthOp.h
/*
 * Copyright (c) 2009, Dan Heeks
 * This program is released under the BSD license. See the file COPYING for
 * details.
 */

// base class for machining operations which can have multiple depths

#ifndef DEPTH_OP_HEADER
#define DEPTH_OP_HEADER

#include "SpeedOp.h"
#include <list>

class CDepthOp;

class CDepthOp : public CSpeedOp
{
public:
	double m_clearance_height;
	double m_rapid_safety_space;
	double m_start_depth;
	double m_step_down;
	double m_z_finish_depth;
	double m_z_thru_depth;
	double m_final_depth;
	std::wstring m_user_depths;

	CDepthOp(const int tool_number = -1, const int operation_type = UnknownType )
		: CSpeedOp(tool_number, operation_type)
	{
		m_clearance_height = 0.0;
		m_start_depth = 0.0;
		m_step_down = 0.0;
		m_z_finish_depth = 0.0;
		m_z_thru_depth = 0.0;
		m_final_depth = 0.0;
		m_rapid_safety_space = 0.0;
		ReadDefaultValues();
	}

	CDepthOp & operator= ( const CDepthOp & rhs );
	CDepthOp( const CDepthOp & rhs );

	void set_initial_values(const std::list<int> *sketches = NULL, const int tool_number = -1);
	void write_values_to_config();
	void WriteXMLAttributes(TiXmlNode* pElem);
	void ReadFromXMLElement(TiXmlElement* pElem);

	// HeeksObj's virtual functions
	void GetProperties(std::list<Property *> *list);
	void WriteBaseXML(TiXmlElement *element);
	void ReadBaseXML(TiXmlElement* element);
	void ReloadPointers();
	void WriteDefaultValues();
	void ReadDefaultValues();

	// COp's virtual functions
	Python AppendTextToProgram();
	void GetTools(std::list<Tool*>* t_list, const wxPoint* p);
	void glCommands(bool select, bool marked, bool no_color);

	bool operator== ( const CDepthOp & rhs ) const;
	bool operator!= ( const CDepthOp & rhs ) const { return(! (*this == rhs)); }
	bool IsDifferent(HeeksObj *other) { return(*this != (*((CDepthOp *) other))); }
};

#endif
