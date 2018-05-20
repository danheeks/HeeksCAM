// svg.h
// Copyright (c) 2009, Dan Heeks
// This program is released under the BSD license. See the file COPYING for details.

struct TwoPoints
{
	gp_Pnt ppnt;
	gp_Pnt pcpnt;
};

class CArea;

// derive a class from this and implement it's virtual functions
class CSvgRead{
private:
	std::list<gp_Trsf> m_transform_stack;
	bool m_fail;
	double m_stroke_width;
	double m_fill_opacity;
	CArea* m_current_area;
	bool m_usehspline;
	std::list<HeeksObj*> m_sketches_to_add;
	CSketch* m_sketch;
	bool m_unite;

	std::string RemoveCommas(std::string input);
	void ReadSVGElement(TiXmlElement* pElem);
	void ReadStyle(TiXmlAttribute* a);
	void ReadG(TiXmlElement* pElem);
	void ReadTransform(TiXmlElement* pElem);
	void ReadPath(TiXmlElement* pElem);
	void ReadRect(TiXmlElement* pElem);
	void ReadCircle(TiXmlElement* pElem);
	void ReadEllipse(TiXmlElement* pElem);
	void ReadLine(TiXmlElement* pElem);
	void ReadPolyline(TiXmlElement* pElem, bool close);
	gp_Pnt ReadStart(const char *text,gp_Pnt ppnt,bool isupper);
	void ReadClose(gp_Pnt ppnt, gp_Pnt spnt);
	gp_Pnt ReadLine(const char *text,gp_Pnt ppnt,bool isupper);
	gp_Pnt ReadHorizontal(const char *text,gp_Pnt ppnt,bool isupper);
	gp_Pnt ReadVertical(const char *text,gp_Pnt ppnt,bool isupper);
	struct TwoPoints ReadCubic(const char *text,gp_Pnt ppnt,bool isupper);
	struct TwoPoints ReadCubic(const char *text,gp_Pnt ppnt,gp_Pnt pcpnt, bool isupper);
	struct TwoPoints ReadQuadratic(const char *text,gp_Pnt ppnt,bool isupper);
	struct TwoPoints ReadQuadratic(const char *text,gp_Pnt ppnt,gp_Pnt pcpnt,bool isupper);
	gp_Pnt ReadEllipse(const char *text,gp_Pnt ppnt,bool isupper);
	int JumpValues(const char *text, int number);
	void ProcessArea();
public:
	CSvgRead(const wxChar* filepath, bool usehspline, bool unite); // this opens the file
	~CSvgRead(); // this closes the file

	gp_Trsf m_transform;

	void Read(const wxChar* filepath);

	void AddSketchIfNeeded();
	void ModifyByMatrix(HeeksObj* object);
	void OnReadStart();
	void OnReadLine(gp_Pnt p1, gp_Pnt p2);
	void OnReadCubic(gp_Pnt s, gp_Pnt c1, gp_Pnt c2, gp_Pnt e);
	void OnReadQuadratic(gp_Pnt s, gp_Pnt c, gp_Pnt e);
	void OnReadEllipse(gp_Pnt c, double maj_r, double min_r, double rot, double start, double end);
	void OnReadCircle(gp_Pnt c, double r);

	bool Failed(){return m_fail;}
};

class CSketch;
