import geom
import copy

class Box:
    def __init__(self, minxy = None, maxxy = None):
        if minxy == None:
            self.minxy = None
        elif isinstance(minxy, geom.Point):
            self.minxy = copy.deepcopy(minxy)
        else:
            raise ValueError("Point argument expected")

        if maxxy == None:
            self.maxxy = None
        elif isinstance(maxxy, geom.Point):
            self.maxxy = copy.deepcopy(maxxy)
        else:
            raise ValueError("Point argument expected")
            
        self.valid = (self.minxy != None) and (self.maxxy != None)
        
    def __eq__(self, other):
        if self.minxy != other.minxy: return False
        if self.maxxy != other.maxxy: return False
        if self.valid != other.valid: return False
        return True
    
    def __ne__(self, other):
        return (self == other) == False
        
    def InsertPoint(self, p):
        if self.valid:
            if p.x < self.minxy.x: self.minxy.x = p.x
            if p.y < self.minxy.y: self.minxy.y = p.y
            if p.x > self.maxxy.x: self.maxxy.x = p.x
            if p.y > self.maxxy.y: self.maxxy.y = p.y
        else:
            self.valid = True
            self.minxy = copy.deepcopy(p)
            self.maxxy = copy.deepcopy(p)
            
    def InsertBox(self, b):
        if b.valid:
            if self.valid:
                if b.minxy.x < self.minxy.x: self.minxy.x = b.minxy.x
                if b.minxy.y < self.minxy.y: self.minxy.y = b.minxy.y
                if b.maxxy.x > self.maxxy.x: self.maxxy.x = b.maxxy.x
                if b.maxxy.y > self.maxxy.y: self.maxxy.y = b.maxxy.y
            else:
                self.valid = True
                self.minxy = copy.deepcopy(b.minxy)
                self.maxxy = copy.deepcopy(b.maxxy)
        
    def Centre(self):
        return (self.minxy + self.maxxy) * 0.5
    
    def Width(self):
        if self.valid:
            return self.maxxy.x - self.minxy.x
        else:
            return 0.0

    def Height(self):
        if self.valid:
            return self.maxxy.y - self.minxy.y
        else:
            return 0.0
        
    def Radius(self):
        return math.sqrt(self.Width() * self.Width() + self.Height() * self.Height()) * 0.5
    
    def MinX(self):
        return self.minxy.x
    
    def MaxX(self):
        return self.maxxy.x
    
    def MinY(self):
        return self.minxy.y
    
    def MaxY(self):
        return self.maxxy.y
    
    def Contains(self, b):
        if b.MinX() < self.MinX(): return False
        if b.MinY() < self.MinY(): return False
        if b.MaxX() > self.MaxX(): return False
        if b.MaxY() > self.MaxY(): return False
        return True
    
    def Intersects(self, b):
        if b.MinX() > self.MaxX(): return False
        if b.MinY() > self.MaxY(): return False
        if b.MaxX() < self.MinX(): return False
        if b.MaxY() < self.MaxY(): return False
        return True
        
    
        
    
    
    