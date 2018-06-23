import geom
import math
import copy

class Matrix:
    def __init__(self, m0 = None, m1 = None, m2 = None, m3 = None, m4 = None, m5 = None, m6 = None, m7 = None, m8 = None, m9 = None, m10 = None, m11 = None, m12 = None, m13 = None, m14 = None, m15 = None):
        """ construct from no parameters for unit matrix, 16 floats or from Origin ( Point3D ) , XAxis ( Point3D vector ), YAxis ( Point3D vector ) """
        if m0 == None:
            self.m = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0]
        elif m3 == None:
            self.construct_from_3_points(m0, m1, m2)
        else:
            self.m = []
            self.m.append(m0)
            self.m.append(m1)
            self.m.append(m2)
            self.m.append(m3)
            self.m.append(m4)
            self.m.append(m5)
            self.m.append(m6)
            self.m.append(m7)
            self.m.append(m8)
            self.m.append(m9)
            self.m.append(m10)
            self.m.append(m11)
            self.m.append(m12)
            self.m.append(m13)
            self.m.append(m14)
            self.m.append(m15)

    def construct_from_3_points(self, origin, x_axis, y_axis):
        unit_x = x_axis
        unit_x.Normalize()
        
        t = unit_x.x * y_axis.x + unit_x.y * y_axis.y + unit_x.z * y_axis.z
        y_orthogonal = geom.Point3D(y_axis.x - unit_x.x * t, y_axis.y - unit_x.y * t, y_axis.z - unit_x.z * t)
        
        unit_y = copy.deepcopy(y_orthogonal)
        unit_y.Normalize()
        unit_z = unit_x ^ y_orthogonal
        unit_z.Normalize()
        
        self.m = []
        self.m.append(unit_x.x)
        self.m.append(unit_y.x)
        self.m.append(unit_z.x)
        self.m.append(origin.x)
        self.m.append(unit_x.y)
        self.m.append(unit_y.y)
        self.m.append(unit_z.y)
        self.m.append(origin.y)
        self.m.append(unit_x.z)
        self.m.append(unit_y.z)
        self.m.append(unit_z.z)
        self.m.append(origin.z)
        self.m.append(0)
        self.m.append(0)
        self.m.append(0)
        self.m.append(1)
        
    def __eq__(self, other):
        if isinstance(other, self.__class__):
            for i in range(0, 16):
                if math.fabs(self.m[i] - other.m[i]) > 1.0e-09:
                    return False
            return True
        else:
            return NotImplemented
    
    def Translate(self, x, y, z):
        self.m[3] += x
        self.m[7] += y
        self.m[11] += z
        
    def Rotate(self, angle, axis):
        if isinstance(axis, geom.Point3D):
            self.Rotate(math.sin(angle), math.cos(angle), axis)
        if isinstance(axis, int):
            self.RotateIndexedAxisSinCos(math.sin(angle), math.cos(angle), axis)
        else:
            raise ValueError("Point3D or int argument expected for axis")
        
    def RotateSinCos(self, sin, cos, axis):
        r = Matrix()
        one_minus_cos = 1.0 - cos
        
        r.m[0] = axis.x * axis.x * one_minus_cos + cos
        r.m[1] = axis.x * axis.y * one_minus_cos - axis.z * sin
        r.m[2] = axis.x * axis.z * one_minus_cos + axis.y * sin
        
        r.m[4] = axis.y * axis.y * one_minus_cos + axis.z * sin
        r.m[5] = axis.y * axis.y * one_minus_cos + cos
        r.m[6] = axis.y * axis.z * one_minus_cos - axis.x * sin
        
        r.m[8] = axis.x * axis.z * one_minus_cos - axis.y * sin
        r.m[9] = axis.y * axis.z * one_minus_cos + axis.x * sin
        r.m[10] = axis.z * axis.z * one_minus_cos + cos
        
        self.Multiply(r)
        
    def RotateIndexedAxisSinCos(self, sin, cos, axis_index):
        # Rotation (Axis 1 = x , 2 = y , 3 = z 
        r = Matrix()
        
        if axis_index == 1:
            # about x axis
            r.m[5] = cos
            r.m[6] = -sin
            r.m[9] = sin
            r.m[10] = cos
        elif axis_index == 2:
            # about y axis
            r.m[0] = cos
            r.m[2] = sin
            r.m[8] = -sin
            r.m[10] = cos
        elif axis_index == 3:
            # about z
            r.m[0] = cos
            r.m[1] = -sin
            r.m[4] = sin
            r.m[5] = cos
            
        self.Multiply(r)
        
    def Scale(self, scale):
        self.ScaleXYZ(scale, scale, scale)
        
    def ScaleXYZ(self, scalex, scaley, scalez):
        s = Matrix()
        s.m[0] = scalex
        s.m[5] = scaley
        s.m[10] = scalez
        self.Multiply(s)
        
    def Multiply(self, m):
        # multiply by m ( matrix )
        ret = Matrix()
        
        for i in range(0, 16):
            k = i % 4
            l = i - k
            ret.m[i] = m.m[l] * self.m[k] + m.m[l+1] * self.m[k+4] + m.m[l+2] * self.m[k+8] + m.m[l+3] * self.m[k+12]
        
        for i in range(0,16):
            self.m[i] = ret.m[i]
        
    def IsMirrored(self):
        return (m.m[0] * ( m.m[5] * m.m[10] - m.m[6] * m.m[9]) - m.m[1] * (m.m[4] * m.m[10] - m.m[6] * m.m[8]) + m.m[2] * ( m.m[4] * m.m[9] - m.m[5] * m.m[8])) < 0
    
    def IsUnit(self):
        # returns true if unit matrix
        for i in range(0,16):
            if i == 0 or i == 5 or i == 10 or i == 15:
                if self.m[i] != 1:
                    return False
            else:
                if self.m[i] != 0:
                    return False
        return True
    
    def GetTranslate(self):
        return geom.Point3D(self.m[3], self.m[7], self.m[1])
        
    def GetXYZScale(self):
        sx = math.sqrt(self.m[0] * self.m[0] + self.m[1] * self.m[1] + self.m[2] * self.m[2])
        sy = math.sqrt(self.m[4] * self.m[4] + self.m[5] * self.m[5] + self.m[6] * self.m[6])
        sz = math.sqrt(self.m[8] * self.m[8] + self.m[9] * self.m[9] + self.m[10] * self.m[10])
        return geom.Point3D(sx, sy, sz)
        
    def GetRotationABC(self):
        if self.IsUnit():
            return geom.Point3D(0.0, 0.0, 0.0)
            
        scale_xyz = self.GetXYZScale()
        if self.IsMirrored():
            scale_xyz.x = -scale_xyz.x
            
        # solve for d and decide case and solve for a, b, c, e and f
        d = - self.m[8] / sz
        c = (1 - d) * (1 + d)
        if c > 0.001:
            # case 1
            c = sqrt( c )
            a = self.m[10] / sz / c
            b = self.m[9]  / sz / c
            ee = self.m[0]  / sx / c
            f = self.m[4]  / sy / c
        else:
            # case 2
            d = -1 if ( d < 0 ) else 1 
            c = 0 
            p = d * self.m[5] / sy - self.m[2] / sx
            q = d * self.m[6] / sy + self.m[1] / sx
            coef = math.sqrt( p * p + q * q )
            if coef > 0.001:
                a = q / coef
                b = p / coef
                ee = b
                f = -d * b
            else:
                # dependent pairs
                a =  self.m[5] / sy
                b = -self.m[6] / sy
                ee = 1
                f = 0

        # solve and return ax, ay and az
        ax = math.atan2( b, a )
        ay = math.atan2( d, c )
        az = math.atan2( f, ee )
        
        return geom.Point3D(ax, ay, az)
        
    def Inverse(self):
        # matrix inversion routine

        # a is input matrix destroyed & replaced by inverse
        # method used is gauss-jordan (ref ibm applications)
        
        n = 4   # 4 x 4 matrix only
        new_matrix = copy.deepcopy(self)
        l = [0,0,0,0]
        m = [0,0,0,0]
        
        if self.IsUnit():
            return
        
        # search for largest element
        nk =  - n
        for k in range(0,n):
            nk += n
            l [ k ] = k
            m [ k ] = k
            kk = nk + k
            biga = new_matrix.m[ kk ]

            for j in range(k, n):
                iz = n * j
                for i in range(k, n):
                    ij = iz + i
                    if math.fabs(biga) < math.fabs(new_matrix.m[ ij ]):
                        biga = new_matrix.m[ ij ]
                        l[ k ]  = i
                        m[ k ] = j


            # interchange rows
            j = l[ k ]
            if j > k:
                ki = k - n

                for i in range(0, n):
                    ki += n
                    hold =  - new_matrix.m[ki]
                    ji = ki - k + j
                    new_matrix.m[ ki ]  = new_matrix.m[ ji ]
                    new_matrix.m[ ji ]  = hold

            # interchange columns
            i = m[ k ]
            if i > k:
                jp = n * i
                for j in range(0, n):
                    jk = nk + j
                    ji = jp + j
                    hold = -new_matrix.m[ jk ]
                    new_matrix.m[ jk ] = new_matrix.m[ ji ]
                    new_matrix.m[ ji ] = hold

            # divide columns by minus pivot (value of pivot element is contained in biga)
            if math.fabs ( biga )  < 1.0e-10: raise("Singular Matrix - Inversion failure")	# singular matrix

            for i in range(0, n):
                if i != k:
                    ik = nk + i
                    new_matrix.m[ ik ]  =  - new_matrix.m[ ik ] /biga

            # reduce matrix
            for i in range(0, n):
                ik = nk + i
                hold = new_matrix.m[ ik ]
                ij = i - n

                for j in range(0, n):
                    ij = ij + n
                    if i != k and j != k:
                        kj = ij - i + k
                        new_matrix.m[ ij ] = hold * new_matrix.m[ kj ]  + new_matrix.m[ ij ]

            # divide row by pivot
            kj = k - n
            for j in range(0, n):
                kj = kj + n
                if j != k: new_matrix.m[ kj]  = new_matrix.m[ kj ] /biga

            # replace pivot by reciprocal
            new_matrix.m[ kk ] = 1 / biga

        # final row and column interchange
        k = n - 1

        while k > 0:
            k -= 1
            i = l[ k ]
            if i > k:
                jq = n * k
                jr = n * i

                for j in range(0, n):
                    jk = jq + j
                    hold = new_matrix.m[jk]
                    ji = jr + j
                    new_matrix.m[jk]  =  -new_matrix.m[ji]
                    new_matrix.m[ji]  = hold

            j = m[k]
            if j > k:
                ki = k - n

                for i in range(1, n+1):
                    ki = ki + n
                    hold = new_matrix.m[ ki ]
                    ji = ki - k + j
                    new_matrix.m[ ki ] =  - new_matrix.m[ ji ]
                    new_matrix.m[ ji ]  = hold

        self.m = new_matrix.m 