import geom
import math
import copy

class Span:
    def __init__(self, p, v, start_span = False):
        """ create a Span from a point and a vertex """
        if isinstance(p, geom.Point) == False:
            raise ValueError("Point argument expected")
        self.p = copy.deepcopy(p)
        if isinstance(v, geom.Point):
            self.v = geom.Vertex(p)
        elif isinstance(v, geom.Vertex):
            self.v = copy.deepcopy(v)
        else:
            raise ValueError("Vertex argument expected")
            self.start_span = start_span
            
    def __str__(self):
        return 'Span p = ' + str(self.p) + ', v = ' + str(self.v)
            
    def NearestPointNotOnSpan(self, p):
        if self.v.type == 0:
            vs = self.v.p - self.p
            vs.Normalize()
            dp = (p - self.p) * vs
            return (vs * dp) + self.p
        else:
            radius = self.p.Dist(self.v.c)
            r = p.Dist(self.v.c)
            if r < geom.tolerance:
                return p
            vc = (self.v.c - p)
            return p + vc * ((r - radius) / r)
        
    def NearestPointToPoint(self, p):
        if isinstance(p, geom.Point) == False:
            return NotImplemented
        np = self.NearestPointNotOnSpan(p)
        t = self.Parameter(np)
        if t >= 0.0 and t <= 1.0:
            return np
        
        d1 = p.Dist(self.p)
        d2 = p.Dist(self.v.p)
        
        if d1 < d2: return self.p
        else: return self.v.p
        
    def MidParam(self, param):
        # returns a point which is 0-1 along span
        if math.fabs(param) < 0.00000000000001: return self.p
        if math.fabs(param - 1.0) < 0.00000000000001: return self.v.p

        if self.v.type == 0:
            vs = self.v.p - self.p
            p = vs * param + self.p
        else:
            v = self.p - self.v.c
            v.Rotate(param * IncludedAngle())
            p = v + self.v.c
        return p
    
    def _NearestPointToSpan(self, span):
        midpoint = self.MidParam(0.5)
        np = span.NearestPoint(self.p)
        best_point = self.p
        dist = np.Dist(self.p)
        if span.start_span:dist -= area_accuracy * 2 # give start of curve most priority
        npm = span.NearestPointToPoint(midpoint)
        dm = npm.Dist(midpoint) - area_accuracy # lie about midpoint distance to give midpoints priority
        if dm < dist:
            dist = dm
            best_point = midpoint
        np2 = p.NearestPointToPoint(self.v.p)
        dp2 = np2.Dist(self.v.p)
        if dp2 < dist:
            dist = dp2
            best_point = self.v.p
        return best_point, d

    def NearestPointToSpan(self, span):
        best_point, best_dist = self._NearestPointToSpan(span)

        # try the other way round too
        best_point2, best_dist2 = span._NearestPointToSpan(self)
        if best_dist2 < best_dist:
            best_point = self.NearestPointToPoint(best_point2)
            best_dist = best_dist2

	return best_point, best_dist

    def GetBox(self, box):
        """ adds the box of this span to the given box reference """
        print('in GetBox, span = ' + str(self))
        box.InsertPoint(self.p)
        box.InsertPoint(self.v.p)
        print('in GetBox, span = ' + str(self))
        
        if self.v.type:
            # arc, add quadrant points
            vs = self.p - self.v.c
            ve = self.v.p - self.v.c
            qs = GetQuadrant(vs)
            qe = GetQuadrant(ve)
            if self.v.type == -1:
                # swap qs and qe
                t=qs
                qs = qe
                qe = t
                if qe<qs:qe = qe + 4
                rad = self.v.p.Dist(self.v.c)
                for i in range(qs, qe):
                    box.Insert(self.v.c + QuadrantEndPoint(i) * rad)
        
    def IncludedAngle(self):
        if self.v.type:
            vs = ~(self.p - self.v.c)
            ve = ~(self.v.p - self.v.c)
            if self.v.type == -1:
                vs = ~vs
                ve = ~ve
            vs.Normalize()
            ve.Normalize()
            
            return IncludedAngle(vs, ve, self.v.type)
        return 0.0
    
    def GetArea(self):
        if self.v.type:
            angle = self.IncludedAngle()
            radius = self.p.Dist(self.v.c)
            return 0.5 * ((self.v.c.x - self.p.x) * (self.v.c.y + self.p.y) - (self.v.c.x - self.v.p.x) * (self.v.c.y + self.v.p.y) - angle * radius * radius)
        return 0.5 * (self.v.p.x - self.p.x) * (self.p.y + self.v.p.y)
        
    def Parameter(self, p):
        if self.v.type == 0:
            v0 = p - self.p
            vs = self.v.p - self.p
            length = vs.Length()
            vs.Normalize()
            t = vs * v0
            t = t / length
        else:
            vs = ~(self.p - self.v.c)
            v = ~(p - self.v.c)
            vs.Normalize()
            v.Normalize()
            if v.tpye == -1:
                vs = -vs
                v = -v
            ang = IncludedAngle(vs, v, self.v.type)
            angle = self.IncludedAngle()
            t = ang/ angle
        return t
    
    def On(self, p):
        return p == self.NearestPoint(p)
        
    def Length(self):
        if self.v.type != 0:
            radius = self.p.Dist(self.v.c)
            return math.fabs(self.IncludedAngle()) * radius
        return self.p.Dist(self.v.p)
        
    def GetVector(self, fraction):
        """ returns the direction vector at point which is 0-1 along span """
        if self.v.type == 0:
            v = geom.Point(self.p, self.v.p)
            v.Normalize()
            return v

        p= self.MidParam(fraction)
        v = geom.Point(self.v.c, p)
        v.Normalize()
        if self.v.type == 1:
            return geom.Point(-v.y, v.x)
        else:
            return geom.Point(v.y, -v.x)
            
    def LineLineIntof(self, span):
        v0 = geom.Point(self.p, self.v.p)
        v1 = geom.Point(span.p, span.v.p)
        v2 = geom.Point(self.p, span.p)
        
        cp = v1 ^ v0
        
        if math.fabs(cp) < geom.UNIT_VECTOR_TOLERANCE:
            return []
            
        t = (v1 ^ v2) / cp
        intersections = []
        intersections.append( v0 * t + self.p )
        return intersections
    
    def LineArcIntof(self, arc_span):
        v0 = geom.Point(arc_span.v.c, self.p)
        v1 = geom.Point(self.p, self.v.p)
        s = v1.x * v1.x + v1.y * v1.y
        s0 = v0.x * v0.x + v0.y * v0.y
        roots = quadratic(s, 2 * (v0 * v1), s0 - arc_span.Radius() * arc_span.Radius())
        intersections = []
        if len(roots) > 0:
            toler = geom.tolerance / math.sqrt(s)
            for root in roots:
                if root > -toler and root < 1.0 + toler:
                    p = v1 * root + self.p
                    if arc_span.On(p):
                        intersections.append(p)
        return intersections
    
    def ArcArcIntof(self, arc_span):
        pts = Circle(self.v.c, self.Radius()).Intof(Circle(arc_span.v.c, arc_span.Radius()))
        
        if len(pts) == 0:
            return []
            
        intersections = []
        
        for p in pts:
            if self.On(p) and arc_span.On(p):
                intersections.append(p)
        return intersections
        
        
    def quadratic(a, b, c):
        roots = []
        epsilon = 1.0e-06
        epsilonsq = epsilon * epsilon
        if math.fabs(a) < epsilon:
            if math.fabs(b) >= epsilon:
                roots.append(-c/b)
        else:
            b /= a
            c /= a
            s = b * b - 4 * c
            if s >= -epsilon:
                roots.append( -0.5 * b)
                if s > epsilonsq:
                    s = 0.5 * math.sqrt(s)
                    roots.append(roots[0] - s)
                    roots[0] += s
        return roots                
        
    def Intersect(self, span):
        """ finds all the intersection points between two spans and returns them in a list """
        box0 = geom.Box()
        self.GetBox(box0)
        box1 = geom.Box()
        span.GetBox(box1)
        if box0.Intersects(box1) == False: return []
        if self.v.type == 0:
            if span.v.type == 0:
                return self.LineLineIntof(span)
            else:
                return self.LineArcIntof(span)
        else:
            if span.v.type == 0:
                return span.LineArcIntof(self)
            else:
                return self.ArcArcIntof(span)
    
    def Reverse(self):
        t = self.p
        self.p = self.v.p
        self.v.p = t
        self.v.type = -self.v.type
        
    def GetRadius(self):
        if self.v.type == 0:
            return 9.9
        else:
            return self.p.Dist(self.v.c)
            
    def Offset(self, leftwards_value):
        if self.v.type == 0:
            v = geom.Point(self.p, self.v.p)
            v.Normalize()
            vp = ~v
            self.p = self.p + vp * leftwards_value
            self.v.p = self.v.p + vp * leftwards_value
        else:
            vs = self.GetVector(0.0)
            ve = self.GetVector(1.0)
            vsp = ~vs
            vse = ~ve
            self.p = self.p + vsp * leftwards_value
            self.v.p = self.v.p + vse * leftwards_value
            
    def IsNullSpan(self):
        return self.p == self.v.p
            

def InlcudedAngle(v0, v1, dir):
    inc_ang = v0 * v1
    if inc_ang > 1.0 - 1.0e-10: return 0
    if inc_ang < -1.0 + 10e-10:
        inc_ang = math.pi
    else:
        # dot product,   v1 . v2  =  cos ang
        if inc_ang > 1.0: inc_ang = 1.0
        inc_ang = math.acos(inc_ang)  # 0 to pi radians
        if dir * (v0 ^ v1) < 0: inc_ang = 2 * math.pi - inc_ang
    return dir * inc_ang

def GetQuadrant(v):
    # 0 = [+,+], 1 = [-,+], 2 = [-,-], 3 = [+,-]
    if v.x > 0:
        if v.y > 0:
            return 0
        return 3
    if v.y > 0:
        return 1
    return 2

def QuadrantEndPoint(i):
    if i>3: i-= 4
    if i == 0:
        return Point(0.0, 1.0)
    elif i == 1:
        return Point(-1.0, 0.0)
    elif i == 2:
        return Point(0.0, -1.0)
    else:
        return Point(1.0, 0.0)
        
