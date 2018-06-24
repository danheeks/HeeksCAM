import geom
import math
import copy

class Line:
    def __init__(self, p, v):
        if isinstance(p, geom.Point3D) == False:
            raise ValueError("Point argument expected")
        if isinstance(v, geom.Point3D) == False:
            raise ValueError("Point argument expected")
        self.p = copy.deepcopy(p)
        self.v = copy.deepcopy(v)
        
    def Dist(self, p):
        vn = copy.deepcopy(self.v)
        vn.Normalize()
        d1 = self.p * vn
        d2 = p * vn
        pn = self.p + vn * ( d2 - d1 )
        return pn.Dist(p)
        
    def IntersectPlane(self, plane):
        return plane.IntofLine(self)