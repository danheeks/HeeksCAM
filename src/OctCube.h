// Cube.h
#include "OctSolid.h"
#include "geometry.h"
#include "Box.h"

class COctCube : public COctSolid
{
public:
	CBox m_box;

	COctCube(const CBox& box);

	// CSolid's virtual functions
	int Inside(const CBox& box)const;
	void SetElementsColor(COctEle& ele)const;
};