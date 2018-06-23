import geom

class Circle:
    def __init__(self, p, r):
        self.p = p
        self.r = r
        
    def Intof(self, c1):
        # intersection with another circle
        # returns a list of intersections
        v = geom.Point(self.p, c1.p)
        d = v.Normalize()
        if d < geom.tolerance:
            return []
            
        sum = math.fabs(self.r) + math.fabs(c1.r)
        diff = math.fabs(math.fabs(self.r) - math.fabs(c1.r))
        if d > sum + geom.tolerance or d < diff - geom.tolerance:
            return []
            
        # dist from centre of this circle to mid intersection
        d0 = 0.5 * ( d + (self.r + c1.r) * ( self.r - c1.r ) / d )
        if d0 - self.r > geom.tolerance:
            return [] # circles don't intersect
        
        intersections = []
        
        h = (self.r - d0) * (self.r + d0)  # half distance between intersects squared
        if h < 0: d0 = c0.radius           # tangent
        intersections.append(v * d0 + self.p)
        if h < geom.TOLERANCE_SQ:
            return intersections
        
        h = math.sqrt(h)
        
        v = ~v
        intersections.append(v * h + intersections[0])
        v = -v
        intersections[0] = v * h + intersections[0]
        return intersections