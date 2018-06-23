from OpenGL.GL import *

class Material:
    def __init__(self, col = None):
        self.matf = [0.3, 0.3, 0.3, 0.8, 0.8, 0.8, 0.3, 0.3, 0.3, 15.0]
        if col:
            self.matf[0] = 0.1 + 0.2666666667 * float(col.red)/255
            self.matf[1] = 0.1 + 0.2666666667 * float(col.green)/255
            self.matf[2] = 0.1 + 0.2666666667 * float(col.blue)/255
            self.matf[3] = 0.2 + 0.8 * float(col.red)/255
            self.matf[4] = 0.2 + 0.8 * float(col.green)/255
            self.matf[5] = 0.2 + 0.8 * float(col.blue)/255

    def glMaterial(self, opacity):
        blend_enabled = False
        if opacity<1:
            glEnable(GL_BLEND)
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA )
            glDepthMask(0)
            blend_enabled = True

        f = [0.0, 0.0, 0.0, opacity]
        for i in range(0, 3): f[i] = self.matf[i]
        glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, f )
        for i in range(0, 3): f[i] = self.matf[i+3]
        glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, f )
        for i in range(0, 3): f[i] = self.matf[i+6]
        glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, f )
        for i in range(0, 3): f[i] = 0.0
        glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, f );
        glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, self.matf[9] );
        return blend_enabled
