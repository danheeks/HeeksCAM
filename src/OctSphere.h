// OctSphere.h
#include "OctSolid.h"
#include "geometry.h"
#include "Box.h"

class COctSphere : public COctSolid
{
public:
	geoff_geometry::Point3d m_c;
	double m_r;
	CBox m_box;

	COctSphere(const geoff_geometry::Point3d& c, double r);

	// CSolid's virtual functions
	int Inside(const CBox& box)const;
	void SetElementsColor(COctEle& ele)const;

	bool Inside(const geoff_geometry::Point3d& p)const;
};