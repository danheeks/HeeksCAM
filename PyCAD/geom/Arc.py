import geom
import math
import copy

class Arc:
    def __init__(self, s, e, c, dir, user_data):
        self.s = copy.deepcopy(s)
        self.e = copy.deepcopy(e)
        self.c = copy.deepcopy(c)
        self.dir = dir
        self.user_data = user_data
        
    def SetDirWithPoint(self, p):
        angs = math.atan2(self.s.y - self.c.y, self.s.x - self.c.x)
        ange = math.atan2(self.e.y - self.c.y, self.e.x - self.c.x)
        angp = math.atan2(p.y - self.c.y, p.x - self.c.x)
        if ange < angs:ange += 2 * math.pi
        if angp < angs - 0.0000000000001: angp += math.pi
        if angp > ange + 0.0000000000001: self.dir = False
        else: self.dir = True
        
    def IncludedAngle(self):
        angs = math.atan2(self.s.y - self.c.y, self.s.x - self.c.x)
        ange = math.atan2(self.e.y - self.c.y, self.e.x - self.c.x)
        if dir:
            # make sure ange > angs
            if ange < angs:ange += 2 * math.pi
        else:
            if angs < ange:angs += 2 * math.pi
        return math.fabs(ange - angs)
        
    def AlmostALine(self):
        mid_point = self.MidParam(0.5)
        if geom.Line(self.s, self.e - self.s).Dist(mid_point) <= geom.tolerance:
            return True
        
        max_arc_radius = 1.0 / geom.tolerance
        radius = self.c.Dist(self.s)
        if radius > max_arc_radius:
            return True
        return False
    
    def MidParam(self, param):
        # returns a point which is 0-1 along arc
        if math.fabs(param) < 0.00000000000001: return self.s
        if math.fabs(param - 1.0) < 0.00000000000001: return self.e
        
        v = self.s - self.c
        v.Rotate(param * IncludedAngle())
        return v + self.c
    
    def GetSegments(self):
        pass
    # to do