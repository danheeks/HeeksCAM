import geom
import copy
import sys
import math

class Point3D:
    def __init__(self, x, y, z):
        """ make a 3D point ( or vector ) from x, y, z coordinates or as a vector between two points """
        if isinstance(x, self.__class__) and isinstance(y, self.__class__):
            p0 = x
            p1 = y
            self.x = p1.x - p0.x
            self.y = p1.y - p0.y
            self.z = p1.z - p0.z
        else:
            self.x = float(x)
            self.y = float(y)
            self.z = float(z)
        
    def Transform(self, m):
        if m.IsUnit():
            return
        
        new_x = self.x * m.m[0] + self.y * m.m[1] + self.z * m.m[2] + m.m[3]
        new_y = self.x * m.m[4] + self.y * m.m[5] + self.z * m.m[6] + m.m[7]
        new_z = self.x * m.m[8] + self.y * m.m[9] + self.z * m.m[10] + m.m[11]

    def Transformed(self, m):
        p = copy.deepcopy(self)
        p.Transform(m)
        return p
        
    def Dist(self, p):
        return Point3D(self, p).Length()
        
    def DistSq(self, p):
        return (self.x - p.x) * (self.x - p.x) + (self.y - p.y) * (self.y - p.y) + (self.z - p.z) * (self.z - p.z)

    def Mid(self, p, factor):
        return Point3D(self, p) * factor + self
    
    def __str__(self):
        return 'Point3D x = ' + str(self.x) + ', y = ' + str(self.y) + ', z = ' + str(self.z)

    def __add__(self, other):
        if isinstance(other, self.__class__):
            return Point3D(self.x + other.x, self.y + other.y, self.z + other.z)
        else:
            return NotImplemented
        
    def __sub__(self, other):
        if isinstance(other, self.__class__):
            return Point3D(self.x - other.x, self.y - other.y, self.z - other.z)
        else:
            return NotImplemented
                
    def __mul__(self, other):
        if isinstance(other, self.__class__):
            return self.x * other.x + self.y * other.y + self.z * other.z
        elif isinstance(other, float) or isinstance(other, int):
            return Point3D(self.x * other, self.y * other, self.z * other)
        else:
            return NotImplemented
        
    if (sys.version_info > (3, 0)):
        def __truediv__(self, other):
            if isinstance(other, float) or isinstance(other, int):
                return Point3D(self.x / other, self.y / other, self.z / other)
            else:
                return NotImplemented        
    else:
        def __div__(self, other):
            if isinstance(other, float) or isinstance(other, int):
                return Point3D(self.x / other, self.y / other, self.z / other)
            else:
                return NotImplemented        
        
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.x == other.x and self.y == other.y and self.z == other.z
        else:
            return NotImplemented
        
    def __ne__(self, other):
        if isinstance(other, self.__class__):
            return self.x != other.x and self.y != other.y and self.z != other.z
        else:
            return NotImplemented
        
    def __neg__(self):
        return Point3D(-self.x, -self.y, -self.z)
        
    def __xor__(self, other):
        return Point3D(self.y * other.z - self.z * other.y, self.z * other.x - self.x * other.z, self.x * other.y - self.y * other.x)
            
    def Normalize(self):
        m = self.magnitude()
        if m < 1.0e-09:
            self.x = 0.0
            self.y = 0.0
            self.z = 0.0
            return 0.0
        self.x /= m
        self.y /= m
        self.z /= m
        return m

    def Normalized(self):
        m = self.magnitude()
        if m < 1.0e-09:
            return Point3D(0.0, 0.0, 0.0)
        return Point3D(self.x / m, self.y / m, self.z / m)
        
    def magnitude(self):
        return math.sqrt(self.x * self.x + self.y * self.y + self.z * self.z) # magnitude
    
    def magnitudeSq(self):
        return self.x * self.x + self.y * self.y + self.z * self.z # magnitude squared
