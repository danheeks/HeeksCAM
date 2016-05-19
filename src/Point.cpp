#include "Point.h"


const Point operator*(const double &d, const Point &p){ return p * d; }
double Point::tolerance = 0.001;

double Point::length()const
{
	return sqrt(x*x + y*y);
}

double Point::normalize()
{
	double len = length();
	if (fabs(len)> 0.000000000000001)
		*this = (*this) / len;
	return len;
}
