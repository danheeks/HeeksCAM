import math

class Box3D:
    def __init__(self, xmin = None, ymin = None, zmin = None, xmax = None, ymax = None, zmax = None):
        self.x = [0, 0, 0, 0, 0, 0]
        if xmin != None: self.x[0] = xmin
        if ymin != None: self.x[1] = ymin
        if zmin != None: self.x[2] = zmin
        if xmax != None: self.x[3] = xmax
        if ymax != None: self.x[4] = ymax
        if zmax != None: self.x[5] = zmax

        self.valid = ( xmin != None and ymin != None and zmin != None and xmax != None and ymax != None and zmax != None)

    def __eq__(self, other):
        for i in range(0, 6):
            if self.x[i] != other.x[i]: return False
        if self.valid != other.valid: return False

        return True

    def __ne__(self, other):
        return self.__eq__(other) != True

    def InsertPoint(self, x, y, z):
        if self.valid:
            if x < self.x[0]: self.x[0] = x
            if x > self.x[3]: self.x[3] = x
            if y < self.x[1]: self.x[1] = y
            if y > self.x[4]: self.x[4] = y
            if z < self.x[2]: self.x[2] = z
            if z > self.x[5]: self.x[5] = z
        else:
            self.valid = True
            self.x[0] = x
            self.x[3] = x
            self.x[1] = y
            self.x[4] = y
            self.x[2] = z
            self.x[5] = z

    def InsertBox(self, box):
        if box.valid:
            if self.valid:
                for i in range(0, 3):
                    if box.x[i] < self.x[i]: self.x[i] = box.x[i]
                    if box.x[i+3] > self.x[i+3]: self.x[i+3] = box.x[i+3]
            else:
                self.valid = box.valid
                for i in range(0, 6):
                    self.x[i] = box.x[i]

    def Centre(self):
        return (self.x[0] + self.x[3])/2, (self.x[1] + self.x[4])/2, (self.x[2] + self.x[5])/2

    def Width(self):
        if self.valid: return self.x[3] - self.x[0]
        return 0.0

    def Height(self):
        if self.valid: return self.x[4] - self.x[1]
        return 0.0

    def Depth(self):
        if self.valid: return self.x[5] - self.x[2]
        return 0.0

    def Radius(self):
        return math.sqrt(self.Width() * self.Width() + self.Height() * self.Height() + self.Depth() * self.Depth())

    def MinX(self):
        return self.x[0]

    def MaxX(self):
        return self.x[3]

    def MinY(self):
        return self.x[1]

    def MaxX(self):
        return self.x[4]

    def MinZ(self):
        return self.x[2]

    def MaxZ(self):
        return self.x[5]

    def vert(self, index):
        if index == 0:
            return self.x[0], self.x[1], self.x[2]
        if index == 1:
            return self.x[3], self.x[1], self.x[2]
        if index == 2:
            return self.x[3], self.x[4], self.x[2]
        if index == 3:
            return self.x[0], self.x[4], self.x[2]
        if index == 4:
            return self.x[0], self.x[1], self.x[5]
        if index == 5:
            return self.x[3], self.x[1], self.x[5]
        if index == 6:
            return self.x[3], self.x[4], self.x[5]
        if index == 7:
            return self.x[0], self.x[4], self.x[5]
        return 0.0, 0.0, 0.0
