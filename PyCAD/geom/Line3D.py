import geom
import math

class Line3D:
    def __init__(self):
        self.ok = False
        
    def MakeFromPointAndVector(p, v, boxed = True):
        #constructor from point & vector
        self.p = p
        self.v = v
        self.length = v.magnitude()
        self.box = None
        if boxed:
            self.box = geom.Box3D()
            self.GetBox(self.box)
        self.ok = length > geom.tolerance
        
    def GetBox(self, box):
        box.InsertPoint(self.p)
        box.InsertPoint(self.p + self.v)
        
    def MakeFromTwoPoints(self, p0, p1):
        self.p = p
        self.v = geom.Point3D(p0, p1)
        self.length = v.magnitude()
        self.box = geom.Box3D()
        self.GetBox(self.box)
        self.ok = length > geom.tolerance
        
    #def Near(self, p):
        # near point to line from point (0 >= t <= 1) in range
        
        
