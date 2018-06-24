import geom
import math

class Plane:
    def __init__(self, p0, p1, p2 = None):
        """ construct Plane from three points, from distance and normal ( Point3D ), or a point ( Point3D ) and a normal vector ( Point3D ) """
        if p2 == None:
            if isinstance(p0, geom.Point3D):
                self.normal = p1
                self.normal.Normalize()
                self.d = -(self.normal * p0)
            elif isinstance(p0, int) or isinatance(p0, float):
                self.normal = p1
                mag = self.normal.Normalize()
                self.ok = self.normal != geom.Point3D(0.0, 0.0, 0.0)
                if self.ok: self.d = float(p0) / mag;
            else:
                raise ValueError("Point3D or float for first argument expected")
        else:
            # construct plane from 3 points
            self.normal = geom.Point3D(p0, p1) ^ geom.Point3D(p0, p2)
            self.normal.Normalize()
            self.ok = self.normal != geom.Point3D(0.0, 0.0, 0.0)
            self.d = -(self.normal * p0)

    def Dist(self, p):
        # returns signed distance to plane from point p
        return (self.normal * p) + self.d

    def Near(self, p):
        # returns near point to p on the plane
        return -(self.normal * self.Dist(p)) + p

    def IntofLine(self, l):
        # intersection between plane and line
        # input this plane, line
        # output intersection point or None
        den = l.v * self.normal
        
        if math.fabs(den) < geom.UNIT_VECTOR_TOLERANCE:
            return None # line is parallel to the plane, return false, even if the line lies on the plane

        t = -(self.normal * l.p + self.d) / den
        return l.v * t + l.p

    def IntofPlane(self, plane):
        # intersection of 2 planes
        v = self.normal ^ plane.normal;
        v.Normalize()
        
        if v == geom.Point3D(0.0, 0.0, 0.0):
            return None	# parallel planes

	dot = self.normal * plane.normal

        den = dot * dot - 1.0
        a = (self.d - plane.d * dot) / den
        b = (plane.d - self.d * dot) / den
        return geom.Line3D( a * self.normal + b * self.normal, v )

    def IntofPlanePlane(self, plane0, plane1):
        # intersection of 3 planes
        line = self.IntofPlane(plane0)
        if line == None:
            return None
        return plane1.IntofLine(line)
