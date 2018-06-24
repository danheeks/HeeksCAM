from OpenGL.GL import *
from OpenGL.GLU import *
import wx
import math
import geom

class Viewpoint:
    def __init__(self, viewport):
        self.viewport = viewport
        self.initial_point = wx.Point()
        self.initial_pixel_scale = 0.0
        self.perspective = 0.0
        self.section = False
        self.lens_point = geom.Point3D(0, 0, 200)
        self.target_point = geom.Point3D(0, 0, 0)
        self.vertical = geom.Point3D(0, 1, 0)
        self.pixel_scale = 10.0
        self.view_angle = 30.0
        self.projm = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        self.modelm = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]
        self.window_rect = [0, 0, 0, 0]
        self.matrix_valid = False
        self.near_plane = 0.0
        self.far_plane = 0.0
        self.extra_depth_box = geom.Box3D()
        self.extra_view_box = geom.Box3D()

    def rightwards_vector(self):
        return (self.target_point - self.lens_point) ^ self.vertical

    def forwards_vector(self):
        return self.target_point - self.lens_point

    def TurnAngles(self, ang_x, ang_y):
        f = self.target_point - self.lens_point
        fl = f.Length()
        uu = self.vertical.Normalized()
        r = (f ^ uu).Normalized()
        self.lens_point= self.lens_point - r * math.sin(ang_x)*fl
        self.lens_point = self.lens_point + f * (1 - math.cos(ang_x))
        f = f + r * math.sin(ang_x)* fl
        f = f - f * (1-math.cos(ang_x))
        r = (f^uu).Normalized()
        self.lens_point = self.lens_point + uu * math.sin(ang_y) * fl
        self.lens_point = self.lens_point + f * (1-math.cos(ang_y))
        self.vertical = self.vertical + f * math.sin(ang_y)/fl
        self.vertical = self.vertical - uu * (1-math.cos(ang_y))

    def Turn(self, point_diff):
        if point_diff.x > 100: point_diff.x = 100
        elif point_diff.x < -100: point_diff.x = -100
        if point_diff.y > 100: point_diff.y = 100
        elif point_diff.y < -100: point_diff.y = -100
        size = self.viewport.GetViewportSize()
        c = float(size.GetWidth() + size.GetHeight())/20
        self.TurnAngles(float(point_diff.x)/c, float(point_diff.y)/c)

    def TurnVertical(self, ang_x, ang_y):
        pass #to do

    def TurnVerticalPoint(self, point_diff):
        pass #to do

    def Shift(self, tv):
        r = self.rightwards_vector().Normalized()
        f = self.forwards_vector().Normalized()
        u = self.vertical.Normalized()
        new_vector = r * tv.x + f * tv.z + u * tv.y
        self.lens_point = self.lens_point + new_vector
        self.target_point = self.target_point + new_vector

    def ShiftPoint(self, point_diff, point):
        div_x = float(point_diff.x)/self.pixel_scale
        div_y = float(point_diff.y)/self.pixel_scale
        f = self.target_point - self.lens_point
        uu = self.vertical.Normalized()
        r = (f ^ uu).Normalized()
        self.target_point = self.target_point - r * div_x
        self.lens_point = self.lens_point - r * div_x
        self.target_point = self.target_point + uu * div_y
        self.lens_point = self.lens_point + uu * div_y

    def Scale(self, multiplier, use_initial_pixel_scale = False):
    	if use_initial_pixel_scale: self.pixel_scale = self.m_initial_pixel_scale
    	self.pixel_scale = self.pixel_scale * multiplier
    	if self.pixel_scale > 1000000: self.pixel_scale = 1000000
    	if self.pixel_scale < 0.000001: self.pixel_scale = 0.000001

    	# for perspective, move forward
    	if self.perspective:
            f = self.target_point - self.lens_point
            v= f * (multiplier - 1)
            self.lens_point = self.lens_point + v
            if self.lens_point.Distance(self.target_point) < 10:
    			self.target_point = self.lens_point + f.Normalized() * 10

    def ScalePoint(self, point, reversed = False):
        mouse_ydiff = point.y - self.initial_point.y
    	if reversed: mouse_ydiff = -mouse_ydiff
    	size = self.viewport.GetViewportSize()
    	fraction= float(mouse_ydiff)/size.GetHeight()
    	multiplier = fraction
    	increasing = (multiplier>0)
    	if increasing == False: multiplier = -multiplier
    	multiplier = 1 - multiplier
    	if multiplier<0.00001: multiplier=0.00001
    	if increasing: multiplier = 1/multiplier
    	if multiplier < 0.1: multiplier = 0.1
    	self.Scale(multiplier, True)

    def Twist(self, angle):
        pass #to do

    def Twist(self, start, point_diff):
        pass #to do

    def SetViewport(self):
    	size = self.viewport.GetViewportSize()
    	glViewport(0, 0, size.GetWidth(), size.GetHeight())

    def SetProjection2(self, use_depth_testing):
        rad = 0.0
        box = geom.Box3D()
        if use_depth_testing:
            self.viewport.cad.GetBox(box)
            box.InsertBox(self.extra_depth_box)
            box.InsertBox(self.extra_view_box)

        if use_depth_testing == False:
            self.near_plane = 0.0
            self.far_plane = 100000000.0
            rad = 2000000.0
        elif box.valid == False:
            self.near_plane = 0.2
            self.far_plane = 2000.0
            rad = 1000.0
        else:
            x, y, z = box.Centre()
            box_centre = geom.Point3D(x, y, z)
            rad = box.Radius()
            to_centre_of_box = geom.Point3D(self.lens_point, box_centre)
            f = self.forwards_vector()
            f.Normalize()
            distance = to_centre_of_box * f
            if self.section: rad = rad /100
            self.near_plane = distance - rad
            self.far_plane = distance + rad

        size = self.viewport.GetViewportSize()
        w = size.GetWidth()/self.pixel_scale
        h = size.GetHeight()/self.pixel_scale
        s = math.sqrt(w*w + h*h)
        self.near_plane = self.near_plane - s/2
        self.far_plane = self.far_plane + s/2

        hw = float(size.GetWidth())/2
        hh = float(size.GetHeight())/2
        if self.perspective:
            fovy = self.view_angle
            if h>w and w>0: fovy = 2 * 180/math.pi * math.atan( math.tan(self.view_angle/2 * math.pi/180) * h/w)
            if self.near_plane < self.far_plane / 200: self.near_plane= self.far_plane /200
            gluPerspective(fovy, w/h, self.near_plane, self.far_plane)
        else:
            glOrtho((-0.5 - hw)/self.pixel_scale, (hw+0.5)/self.pixel_scale, (-0.5-hh)/self.pixel_scale, (0.5+hh)/self.pixel_scale, self.near_plane, self.far_plane)


    def SetProjection(self, use_depth_testing):
    	glMatrixMode(GL_PROJECTION)
    	glLoadIdentity()
    	self.SetProjection2(use_depth_testing)
    	size = self.viewport.GetViewportSize()
    	self.window_rect[0] = 0
    	self.window_rect[1] = 0
    	self.window_rect[2]=size.GetWidth()
    	self.window_rect[3]=size.GetHeight()
    	self.projm = glGetDoublev (GL_PROJECTION_MATRIX)
    	self.matrix_valid = True

    def SetPickProjection(self, pick_box):
        pass #to do

    def SetModelView(self):
    	glMatrixMode(GL_MODELVIEW)
    	glLoadIdentity()
    	gluLookAt(self.lens_point.x, self.lens_point.y, self.lens_point.z, self.target_point.x, self.target_point.y, self.target_point.z, self.vertical.x, self.vertical.y, self.vertical.z)
    	self.modelm = glGetDoublev (GL_MODELVIEW_MATRIX)

    def SetView(self, unity, unitz):
    	self.target_point = geom.Point3D(0, 0, 0)
    	self.lens_point = self.target_point + unitz
    	self.vertical = unity
    	self.pixel_scale = 10
    	self.SetViewAroundAllObjects()

    def glUnproject(self, v):
    	if self.matrix_valid == False: return geom.Point3D(0, 0, 0)
    	x, y, z = gluUnProject(v.x, v.y, v.z, self.modelm, self.projm, self.window_rect)
    	return geom.Point3D(x, y, z)

    def glProject(self, v):
    	if self.matrix_valid == False: return geom.Point3D(0, 0, 0)
    	x, y, z = gluProject(v.x, v.y, v.z, self.modelm, self.projm, self.window_rect)
    	return geom.Point3D(x, y, z)

    def SetPolygonOffset(self):
        glPolygonOffset(1.0, 1.0)

    def WindowMag(self, window_box):
        pass #to do

    def SetViewAroundAllObjects(self):
        pass #to do

    def SetStartMousePoint(self, point):
        pass #to do

    def SightLine(self, point):
        pass #to do

    def ChooseBestPlane(self, plane):
        f = self.forwards_vector()
        orimat = self.viewport.cad.GetDrawMatrix(False)
        dp = [geom.Point3D(0, 0, 1).Transformed(orimat) * f, geom.Point3D(0, 1, 0).Transformed(orimat) * f, geom.Point3D(1, 0, 0).Transformed(orimat) * f]
        best_dp = 0.0
        best_mode = -1
        second_best_dp = 0.0
        second_best_mode = -1
        third_best_dp = 0.0
        third_best_mode = -1
        for i in range(0, 3):
            if best_mode == -1:
                best_mode = i
                best_dp = math.fabs(dp[i])
            else:
                if math.fabs(dp[i])>best_dp:
                    third_best_dp = second_best_dp
                    third_best_mode = second_best_mode
                    second_best_dp = best_dp
                    second_best_mode = best_mode
                    best_mode = i
                    best_dp = math.fabs(dp[i])
                else:
                    if second_best_mode == -1:
                        second_best_mode = i
                        second_best_dp = math.fabs(dp[i])
                    else:
                        if math.fabs(dp[i])>second_best_dp:
                            third_best_dp = second_best_dp
                            third_best_mode = second_best_mode
                            second_best_dp = math.fabs(dp[i])
                            second_best_mode = i
                        else:
                            third_best_dp = math.fabs(dp[i])
                            third_best_mode = i;
        if plane == 0:
            return best_mode
        elif plane == 1:
            return second_best_mode
        return third_best_mode

    def GetTwoAxes(self, flattened_onto_screen, plane):
        plane_mode = self.ChooseBestPlane(plane)
        orimat = self.viewport.cad.GetDrawMatrix(False)
        if plane_mode == 0:
            vx = geom.Point3D(1, 0, 0).Transformed(orimat)
            vy = geom.Point3D(0, 1, 0).Transformed(orimat)
        elif plane_mode == 1:
            vx = geom.Point3D(1, 0, 0).Transformed(orimat)
            vy = geom.Point3D(0, 0, 1).Transformed(orimat)
        elif plane_mode == 2:
            vx = geom.Point3D(0, 1, 0).Transformed(orimat)
            vy = geom.Point3D(0, 0, 1).Transformed(orimat)
        # find closest between vx and vy to screen y
        dpx = vx * self.vertical
        dpy = vy * self.vertical
        if math.fabs(dpx) > math.fabs(dpy):
            vtemp = geom.Point3D(vx.x, vx.y, vx.z)
            vx = vy
            vy = vtemp
        # make sure vz is towards us
        if (vx ^ vy ) * self.forwards_vector() > 0:
            vx = -vx
        if flattened_onto_screen:
            f = self.forwards_vector().Normalized()
            vx = geom.Point3D(vx - (f * (f * vx))).Normalized()
            vy = geom.Point3D(vy - (f * (f * vy))).Normalized()
            r = rightwards_vector()
            if math.fabs(vy * r) > math.fabs(vx * r):
                vtemp = geom.Point3D(vx.x, vx.y, vx.z)
                vx = vy
                vy = vtemp
        return vx, vy, plane_mode

    def Set90PlaneDrawMatrix(self, mat):
        pass #to do

    def SetPerspective(self, perspective):
        pass #to do

    def GetPerspective(self, perspective):
        pass #to do

class Viewport:
    def __init__(self, cad, w = None, h = None):
        self.cad = cad
        self.LButton = False
        self.CurrentPoint = wx.Point()
        self.view_points = []
        self.frozen = False
        self.refresh_wanted_on_thaw = False
        self.w = 0
        if w != None: self.w = w
        self.h = 0
        if h != None: self.h = h
        self.view_point = Viewpoint(self)
        self.orthogonal = True
        self.need_update = False
        self.need_refresh = False

    def SetViewport(self):
        self.view_point.SetViewport()

    def SetViewpoint(self):
        if self.orthogonal:
            vz = getClosestOrthogonal(-self.view_point.forwards_vector())
            vy = getClosestOrthogonal(self.view_point.vertical)
            self.view_point.SetView(vy, vz)
            self.StoreViewPoint()

    def InsertViewBox(self, box):
        self.view_point.extra_view_box.InsertBox(box)

    def StoreViewPoint(self):
        self.view_points.append(self.view_point)

    def RestorePreviousViewPoint(self):
        if len(self.view_points > 0):
            self.view_point = self.view_points[len(self.view_points)-1]
            self.view_points.pop()

    def FindMarkedObject(self, point, marked_object):
        pass

    def SetIdentityProjection(self):
        glViewport(0, 0, self.w, self.h)
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        glOrtho(-0.5, self.w - 0.5, -0.5, self.h - 0.5, 0, 10)
        glMatrixMode(GL_MODELVIEW)
        glLoadidentity()

    def WidthAndHeightChanged(self, w, h):
        self.w = w
        self.h = h

    def GetViewportSize(self):
        return wx.Size(self.w, self.h)

    def OnMagExtents(self, rotate):
        pass

    def OnMouseEvent(self, event):
        self.cad.current_viewport = self
        self.cad.input_mode.OnMouse(event)
