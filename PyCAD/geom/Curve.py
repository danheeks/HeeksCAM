import copy
import geom
import math

class Curve:
    def __init__(self):
        self.vertices = []
        
    def Append(self, v):
        if isinstance(v, geom.Point):
            self.vertices.append(geom.Vertex(v))
        elif isinstance(v, geom.Vertex):
            self.vertices.append(v)
        else:
            raise ValueError("Vertex argument expected")
            
    def __str__(self):
        s = ('number of vertices = ' + str(len(self.vertices)))
        
        i = 0
        for vertex in self.vertices:
            s += '\n'
            s += str(vertex)
            i += 1
        s += '\n'
        return s
            
    def NearestPointToPoint(self, p):
        best_point = None
        prev_p = None
        first_span = True
        for vertex in self.vertices:
            if prev_p:
                near_point = geom.Span(prev_p, vertex, first_span).NearestPointToPoint(p)
                first_span = False
                dist = near_point.Dist(p)
                if(best_point == None or dist < best_dist):
                    best_dist = dist
                    best_point = near_point
            prev_p = vertex.p
        return best_point
        
    def NearestPointToSpan(self, span):
        best_point = None
        prev_p = None
        first_span = True
        for vertex in self.vertices:
            if prev_p:
                near_point, dist = Span(prev_p, vertex, first_span).NearestPointToSpan(span)
                first_span = False
                dist = near_point.Dist(p)
                if(best_point == None or dist < best_dist):
                    best_dist = dist
                    best_point = near_point
            prev_p = vertex.p
        return best_point, best_dist

    def NearestPointToCurve(self, c):
        best_point = None
        prev_p = None
        first_span = True
        for vertex in c.vertices:
            if prev_p:
                near_point = Span(prev_p, vertex, first_span).NearestPointToPoint(p)
                first_span = False
                dist = near_point.Dist(p)
                if(best_point == None or dist < best_dist):
                    best_dist = dist
                    best_point = near_point
            prev_p = vertex.p
        return best_point

    def Reverse(self):
        new_vertices = []
        prev_v = None
        for v in reversed(self.vertices):
            type = 0
            c = Point(0,0)
            if prev_p:
                type = -prev_v.type
                c = prev_v.c
            new_v = Vertex(type, v.p, c)
            new_vertices.append(new_v)
            prev_v = v
        vertices = new_vertices
        
    def NumVertices(self):
        return len(self.vertices)
    
    def FirstVertex(self):
        return self.vertices[0]
    
    def LastVertex(self):
        return self.vertices[-1]
        
    def GetArea(self):
        area = 0.0
        prev_p = None
        for vertex in self.vertices:
            if prev_p:
                area += Span(prev_p, vertex).GetArea()
            prev_p = vertex.p
        return area
    
    def IsClockwise(self):
        return self.GetArea()>0.0
    
    def IsClosed(self):
        if len(self.vertices) == 0: return False
        return self.vertices[0].p == self.vertices[-1].p
    
    def ChangeStart(self, p):
        new_curve = Curve()
        started = False
        finished = False
        closed = self.IsClosed()
        range_end = 1
        if self.IsClosed():
            range_end = 2
            
        for i in range(0, range_end):
            prev_p = None
            span_index = 0
            for vertex in self.vertices:
                if prev_p:
                    span = Span(prev_p, vertex)
                    if span.On(p):
                        if started:
                            if p == prev_p or span_index != start_span:
                                new_curve.vertices.append(vertex)
                            else:
                                if p == vertex.p:
                                    new_curve.vertices.append(vertex)
                                else:
                                    v = copy.copy(vertex)
                                    v.p = p
                                    new_curve.vertices.append(v)
                                finished = True
                        else:
                            new_curve.vertices.append(Vertex(p))
                            started = True
                            start_span = span_index
                            if p != vertex.p:
                                new_curve.vertices.append(vertex)
                    else:
                        if started:
                            new_curve.vertices.append(vertex)
                    span_index += 1
                prev_p = vertex.p
        
        if started:
            self.vertices = new_curve.vertices
            
    def ChangeEnd(self, p):
        # changes the end position of the curve, doesn't keep close curves closed
        new_curve = Curve()
        
        prev_p = None
        for vertex in self.vertices:
            if prev_p:
                span = Span(prev_p, vertex)
                if span.On(p):
                    v = copy.copy(vertex)
                    v.p = p
                    new_curve.vertices.append(v)
                    break
                else:
                    if p != vertex.p:
                        new_curve.vertices.append(vertex)
            else:
                new_curve.vertices.append(vertex)
            prev_p = vertex.p

        self.vertices = new_curve.vertices
        
    def _Offset(self, leftwards_value):
        """ offset the curve using Geoff's method """
        if (math.fabs(leftwards_value) < geom.tolerance) or len(self.vertices) < 2:
            return True
        
        RollDir = l if leftwards_value < 0 else -1
        
        kOffset = Curve()
        kOffset_started = False
        
        spans = self.GetSpans()
        
        bClosed = self.IsClosed()
        nspans = len(spans)
        if bClosed:
            curSpan = spans[-1]
            prevSpanOff = copy.copy(curSpan)
            prevSpanOff.Offset(leftwards_value)
            nspans += 1
            
        for spannumber in range(0, nspans):
            if spannumber == nspans:
                curSpan = spans[0] # closed curve - read first span again
            else:
                curSpan = spans[spannumber]
                
            if curSpan.IsNullSpan() == False:
                intersections = []
                curSpanOff = copy.copy(curSpan)
                
                curSpanOff.Offset(leftwards_value)
                
                if kOffset_started == False:
                    kOffset.Append(geom.Vertex(curSpanOff.p))
                    kOffset_started = True
                    
                if spannumber > 0:
                    # see if tangent
                    d = curSpanOff.p.Dist(prevSpanOff.v.p)
                    if (d>geom.tolerance) and (curSpanOff.IsNullSpan() == False) and (prevSpanOff.IsNullSpan() == False):
                        # see if offset spans intersect
                        
                        cp = prevSpanOff.GetVector(1.0) ^ curSpanOff.GetVector(0.0)
                        inters = ( cp > 0.0 and leftwards_value > 0) or ( cp < 0.0 and leftwards_value < 0)
                        
                        if inters:
                            intersections = prevSpanOff.Intersect(curSpanOff)
                            
                        if len(intersections) == 1:
                            # intersection - modify previous endpoint
                            kOffset.vertices[-1].p = intersections[0]
                        else:
                            # 0 or 2 intersections, add roll around (remove -ve loops in elimination function)
                            kOffset.vertices.append(geom.Vertex(RollDir, curSpanOff.p, curSpan.p))
                
                if spannumber < len(spans):
                    if kOffset_started == False:
                        kOffset.vertices.append(geom.Vertex(curSpanOff.p))
                        kOffset_started = True
                    kOffset.vertices.append(curSpanOff.v)
                elif len(intersections) == 1:
                    kOffset.vertices[0].p = intersections[0]
            if curSpanOff.IsNullSpan() == False:prevSpanOff = curSpanOff

        if kOffset._EliminateLoops(leftwards_value) == True and bClosed:
            # check for inverted offsets of closed kurves
            if kOffset.IsClosed():
                a = self.GetArea()
                dir = a < 0
                ao = kOffset.GetArea()
                dirOffset = ao < 0
                
                if dir != dirOffset:
                    return False
                else:
                    # check area change compatible with offset direction - catastrophic failure
                    bigger = (a > 0 and leftward_value > 0) or (a < 0 and leftward_value < 0)
                    if bigger and math.fabs(ao) < math.fabs(a):
                        return False
            else:
                return False # started closed but now open??
            
        self.vertices = kOffset.vertices
            
        return True
            
        
    def _EliminateLoops(self, leftwards_value):
        new_vertices = []
        kinVertex = 0
        
        spans = self.GetSpans()
        sp0 = geom.Span(geom.Point(0,0), geom.Point(0,0))
        sp1 = geom.Span(geom.Point(0,0), geom.Point(0,0))
        
        while(kinVertex < len(self.vertices)):
            clipped = False
            sp0.p = self.vertices[kinVertex].p
            kinVertex += 1

            if kinVertex == 1:
                new_vertices.append(geom.Vertex(sp0.p)) # start point mustn't dissappear for this simple method
            if kinVertex < len(self.vertices):
                ksaveVertex = kinVertex
                sp0.v = self.vertices[kinVertex]
                kinVertex += 1
                
                ksaveVertex1 = kinVertex
                if kinVertex < len(self.vertices):
                    sp1.p = self.vertices[kinVertex].p
                    kinVertex += 1
                    ksaveVertex2 = kinVertex
                    fwdCount = 0
                    while(kinVertex < len(self.vertices)):
                        sp1.v = self.vertices[kinVertex]
                        kinVertex += 1
                        intersections = sp0.Intersect(sp1)
                        if (len(intersections) > 0) and (sp0.p.Dist(intersections[0]) < geom.tolerance): intersections = []
                        if len(intersections) > 0:
                            if len(intersections) == 2:
                                # choose first intercept on sp0
                                intersections.pop()
                            ksaveVertex = ksaveVertex1
                            clipped = True  # in a clipped section	
                            if self._DoesIntersInterfere(intersection[0], self, leftwards_value):
                                sp0.v.p = intersections[0]  # ok so truncate this span to the intersection
                                clipped = False  # end of clipped section
                                break
                            # no valid intersection found so carry on
                        sp1.p = sp1.v.p  # next
                        ksaveVertex1 = ksaveVertex2
                        ksaveVertex2 = kinVertex
                        
                        fwdCount += 1
                        if ((kinVertex > len(self.vertices) + 1) or fwdCount > 25) and clipped == False:
                            break
                    
                if clipped:
                    # still in a clipped section - error
                    return False
                
                print('adding ' + str(sp0.v))
                new_vertices.append(sp0.v)
                kinVertex = ksaveVertex
                
        # no more spans - seems ok
        self.vertices = new_vertices
        return True
        
    def GetSpans(self):
        spans = []
        prev_p = None
        for vertex in self.vertices:
            if prev_p:
                spans.append(geom.Span(prev_p, vertex))
            prev_p = vertex.p
        return spans
        
    def Offset(self, leftwards_value):
        save_curve = copy.deepcopy(self)
        
        # try offsetting with Geoff's method
        if self._Offset(leftwards_value):
            return True
        
        if self.IsClosed() == False:
            return False
        
        # use clipper to offset the closed curve
        inwards_offset = leftwards_value
        cw = False
        if self.IsClockwise():
            inwards_offset = -inwards_offset
            cw = True
        a = geom.Area()
        a.Append(self)
        a.Offset(inwards_value)
        if len(a.curves) == 1:
            start_span = None
            if len(self.vertices) > 1:
                start_span = geom.Span(self.vertices[0].p, self.vertices[1], True)
            self.vertices = copy.deepcopy(a.curves[0].vertices)
            if self.IsClockwise() != cw:
                self.Reverse()
            if start_span:
                forward = start_span.GetVector(0.0)
                left = ~forward
                offset_start = start_span.p + left*leftwards_value
                self.ChangeStart(self.NearestPointToPoint(offset_start))
                return True
        return False
    
    def OffsetForward(self, forwards_value, refit_arcs):
        # for drag-knife compensation
        
        # to do
        pass
        
    def GetFirstSpan(self):
        if len(self.vertices) < 2:
            return None
        return geom.Span(self.vertices[0].p, self.vertices[1].p)

    def GetLastSpan(self):
        if len(self.vertices) < 2:
            return None
        return geom.Span(self.vertices[-2].p, self.vertices[-1].p)
        
    def Break(self, p):
        # inserts a point, if it lies on the curve
        prev_p = None
        i = 0
        for vertex in self.vertices:
            if p == vertex.p:
                break # point is already on a vertex
            if prev_p:
                span = Span(prev_p, vertex)
                if span.On(p):
                    v = copy.copy(vertex)
                    v.p = p
                    self.vertices.insert(i, v)
                    break
            prev_p = vertex.p
            i += 1
            
    def Perim(self):
        prev_p = None
        perim = 0.0
        for vertex in self.vertices:
            if prev_p:
                span = Span(prev_p, vertex)
                perim += span.Length()
            prev_p = vertex.p
        return perim
    
    def PerimToPoint(self, perim):
        if len(self.vertices) == 0:
            return geom.Point(0.0, 0.0)
        prev_p = None
        kperim = 0.0
        for vertex in self.vertices:
            if prev_p:
                span = Span(prev_p, vertex)
                length = span.Length()
                if perim < kperim + length:
                    p = span.MidPerim(perim - kperim)
                    return p
                kperim += length
            prev_p = vertex.p
        return copy.deepcopy(self.vertices[-1].p)
        
    def PointToPerim(self, p):
        best_dist = None
        prev_p = None
        perim = 0.0
        first_span = True
        for vertex in self.vertices:
            if prev_p:
                span = Span(prev_p, vertex, first_span)
                near_point = span.NearestPointToPoint(p)
                first_span = False
                dist = near_point.Dist(p)
                if(best_dist == None or dist < best_dist):
                    best_dist = dist
                    span_to_point(prev_p, geom.Vertex(span.v.type, near_point, span.v.c))
                    perim_at_best_dist = perim + span_to_point.Length()
                perim += span.Length()
            prev_p = vertex.p
        if best_dist == None:
            return None
        return perim_at_best_dist
    
    def FitArcs(self):
        new_vertices = []
        might_be_an_arc = []
        arc = None
        arc_added = False
        i = 0
        for vt in self.vertices:
            if vt.type != 0 or i == 0:
                if i != 0:
                    pass
                
    def UnFitArcs(self):
        pass
        # to do
        
    def Intersections(self):
        pass
        # to do
        
    def GetMaxCutterRadius(self):
        pass
        # to do
        
    def GetBox(self):
        pass
        # to do
    
                    
def CheckForArc(prev_vt, might_be_an_arc, arc_returned):
    # this examines the vertices in might_be_an_arc
    # if they do fit an arc, set arc to be the arc that they fit and return true
    # returns true, if arc added
    if len(might_be_an_arc) < 2:
        return False
    
    # find middle point
    num = len(might_be_an_arc)
    i = 0
    mid_vt = None
    mid_i = (num - 1)/2
    for vt in might_be_an_arc:
        if i == mid_i:
            mid_vt = vt
            break
        
    # create a circle to test
    p0 = geom.Point(prev_vt.p)
    p1 = geom.Point(mid_vt.p)
    #p2 = 
    
    # to do
                    
def AddArcOrLines(check_for_arc, new_vertices, might_be_an_arc, arc, arc_found, arc_added):
    if check_for_arc and CheckForArc(new_vertices[-1], might_be_an_arc, arc):
        arc_found = True
    else:
        if arc_found:
            if arc.AlmostALine():
                new_vertices.append(geom.Vertex(arc.e, arc.user_data))
            else:
                new_vertices.append(geom.Vertex(1 if arc.dir else -1, arc.e, arc.c, arc.user_data))
                                    
            arc_added = True
            arc_found = False
            back_vt = might_be_an_arc[-1]
            might_be_an_arc = []
            if check_for_arc: might_be_an_arc.append(copy.deepcopy(back_vt))
        else:
            back_vt = might_be_an_arc[-1]
            if check_for_arc: might_be_an_arc.pop_back()
            first = True
            for v in might_be_an_arc:
                if first == False or len(new_vertices) == 0 or new_vertices[-1].p != v.p:
                    new_vertices.append(copy.deepcopy(v))
            might_be_an_arc = []
            if check_for_arc:
                might_be_an_arc.append(copy.deepcopy(back_vt))
            

def _DoesIntersInterfere(pInt, k, leftwards_value):
    # check that intersections don't interfere with the original kurve 
    sp = geom.Span()
    kCheckVertex = 0
    sp.p0 = k.vertices[kCheckVertex].p
    kCheckVertex += 1
    
    offset = fabs(leftwards_value) - geom.tolerance
    while kCheckVertex < len(k.vertices):
        sp.v = k.vertices[kCheckVertex]
        kCheckVertex += 1
        
        # check for interference 
        np = sp.NearestPointToPoint(pInt)
        if np.Dist(pInt) < offset: return True
        sp.p = sp.v.p
    
    return False	# intersection is ok
