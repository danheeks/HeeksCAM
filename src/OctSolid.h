// OctSolid.h
#pragma once

class CBox;
#include "geometry.h"
#include "Box.h"

class COctEle;

class COctSolid
{
public:
	COctSolid(){}

	virtual int Inside(const CBox& box)const{ return 0; } // return 2 if completely inside, return 1 if some inside, return 0 if not at all inside
	virtual void SetElementsColor(COctEle& ele)const{}
};