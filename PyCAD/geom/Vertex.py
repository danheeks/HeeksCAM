import geom
import copy

class Vertex:
    def __init__(self, type, p = None, c = None, user_data = None):
        """ create a Vertex from a Point, or from type ( 0, 1, -1 ), point, centre_point and optionally user_data """
        if p == None:
            if isinstance(type, geom.Point) == False:
                raise ValueError("Point argument expected")
            self.type = 0
            self.p = copy.copy(type)
        else:
            if isinstance(type, int) == False:
                raise ValueError("int argument expected for type")
            if type != 0 and type != 1 and type != -1:
                raise ValueError("type should be 0, 1, or -1")
            if isinstance(p, geom.Point) == False:
                raise ValueError("Point argument expected")
            if isinstance(c, geom.Point) == False:
                raise ValueError("Point argument expected for centre point")
            self.type = type
            self.p = copy.copy(p)
        self.c = copy.copy(c)
        self.user_data = user_data
            
    def __str__(self):
        s = 'Vertex p = ' + str(self.p) + ', type = ' + str(self.type)
        if self.type != 0:
            s += ', c = ' + str(self.c)
        return s

    def Transform(self, m):
        self.p.Transform(m)
        if self.c:
            self.c.Transform(m)