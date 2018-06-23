import geom
import math
import sys

class Point:
    """ a 2D point or vector with x and y float values """
    def __init__(self, x, y):
        """ create Point from x, y or from two Points as a vector from one point to the other """
        if isinstance(x, self.__class__) and isinstance(y, self.__class__):
            # constructor from two points, the vector from p1 to p2
            self.x = y.x - x.x
            self.y = y.y - x.y
        else:
            self.x = float(x)
            self.y = float(y)
            
    def __str__(self):
        return 'Point x = ' + str(self.x) + ', y = ' + str(self.y)

    def __add__(self, other):
        if isinstance(other, self.__class__):
            return Point(self.x + other.x, self.y + other.y)
        else:
            return NotImplemented
        
    def __sub__(self, other):
        if isinstance(other, self.__class__):
            return Point(self.x - other.x, self.y - other.y)
        else:
            return NotImplemented
                
    def __mul__(self, other):
        if isinstance(other, self.__class__):
            return self.x * other.x + self.y * other.y
        elif isinstance(other, float) or isinstance(other, int):
            return Point(self.x * other, self.y * other)
        else:
            return NotImplemented
        
    if (sys.version_info > (3, 0)):
        def __truediv__(self, other):
            if isinstance(other, float) or isinstance(other, int):
                return Point(self.x / other, self.y / other)
            else:
                return NotImplemented        
    else:
        def __div__(self, other):
            if isinstance(other, float) or isinstance(other, int):
                return Point(self.x / other, self.y / other)
            else:
                return NotImplemented        
        
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.x == other.x and self.y == other.y
        else:
            return NotImplemented
        
    def __ne__(self, other):
        if isinstance(other, self.__class__):
            return self.x != other.x and self.y != other.y
        else:
            return NotImplemented
        
    def __neg__(self):
        return Point(-self.x, -self.y)
        
    def __invert__(self):
        """ return a vector 90 degrees to the left """
        return Point(-self.y, self.x)
        
    def __xor__(self, other):
        return self.x * other.y - self.y * other.x
        
    def Dist(self, other):
        """ returns the distance to another point """
        dx = other.x - self.x
        dy = other.y - self.y
        return math.sqrt(dx*dx + dy*dy)
        
    def Length(self):
        return math.sqrt(self.x*self.x + self.y*self.y)
        
    def Normalize(self):
        """ makes the vector into a unit vector
        this will leave a (0, 0) vector as it is
        returns the length before operation """
        length = self.Length()
        if math.fabs(length)> 0.000000000000001:
            self.x /= length
            self.y /= length
        return len
    
    def Rotate(self, a, b = None):
        """ rotate vector around 0,0 given angle, or by cosine and sine of the angle if two parameters given """
        if b == None:
            # just one parameter, rotate by angle
            self.Rotate(math.cos(a), math.sin(a))
        else:
            # two parameters, rotate by cos and sin
            temp = -self.y * b + self.x * a
            self.y = self.x * b + a * self.y
            self.x = temp
            
    def Transform(self, m):
        """ transforms this point by the matrix """
        new_x = self.x * m.m[0] + self.y * m.m[1] + m.m[3]
        new_y = self.x * m.m[4] + self.y * m.m[5] + m.m[7]
        self.x = new_x
        self.y = new_y
    