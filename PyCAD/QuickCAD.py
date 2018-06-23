# a class for all the global variables for the CAD system, to replace HeeksCAD

from SelectMode import SelectMode
from Color import HeeksColor
from Material import Material
import geom
from OpenGL.GL import *
from OpenGL.GLU import *
from Grid import RenderGrid
import sys
sys.path.append('C:/Users/Dan/heeks/heekscad')

BackgroundModeOneColor = 0
BackgroundModeTwoColors = 1
BackgroundModeTwoColorsLeftToRight = 2
BackgroundModeFourColors = 3

class QuickCad:
    def __init__(self):
        self.input_mode = SelectMode(self)
        self.current_viewport = None
        self.mouse_wheel_forward_away = False
        self.background_mode = BackgroundModeTwoColors
        self.background_color = [HeeksColor(255, 175, 96), HeeksColor(198, 217, 119), HeeksColor(247, 198, 243), HeeksColor(193, 235, 236)]
        self.objects = []
        self.light_push_matrix = True
        self.on_glCommands_list = []
        self.transform_gl_list = []
        self.grid_mode = 3
        self.current_coordinate_system = None
        self.draw_to_grid = True
        self.digitizing_grid = 1.0

    def Viewport(self):
        from Viewport import Viewport
        return Viewport(self)

    def OnMouseEvent(self, viewport, event):
        viewport.OnMouseEvent(event)

    def GetBox(self, box):
        # to do
        pass

    def GetPixelScale(self):
        return self.current_viewport.view_point.pixel_scale

    def EnableBlend(self):
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    def DisableBlend(self):
    	glDisable(GL_BLEND)

    def glCommands(self, viewport):
        glDrawBuffer(GL_BACK)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1)
        glDisable(GL_BLEND)
        glDisable(GL_LINE_SMOOTH)

        viewport.SetViewport()

        if self.background_mode == BackgroundModeTwoColors or self.background_mode == BackgroundModeTwoColorsLeftToRight or self.background_mode == BackgroundModeFourColors:
            # draw graduated background
            glClear(GL_DEPTH_BUFFER_BIT)
            glMatrixMode (GL_PROJECTION)
            glLoadIdentity()
            gluOrtho2D(0.0, 1.0, 0.0, 1.0)
            glMatrixMode(GL_MODELVIEW)
            glLoadIdentity()

            # set up which colors to use
            c = []
            for i in range(0, 4):
                c.append(self.background_color[i])

            if self.background_mode == BackgroundModeTwoColors:
                c[2] = c[0]
                c[3] = c[1]
            elif self.background_mode == BackgroundModeTwoColorsLeftToRight:
                c[1] = c[0]
                c[3] = c[2]

            glShadeModel(GL_SMOOTH)
            glBegin(GL_QUADS)
            c[0].glColor()
            glVertex2f (0.0, 1.0)
            c[1].glColor()
            glVertex2f (0.0, 0.0)
            c[3].glColor()
            glVertex2f (1.0, 0.0)
            c[2].glColor()
            glVertex2f (1.0, 1.0)
            glEnd()
            glShadeModel(GL_FLAT)

        viewport.view_point.SetProjection(True)
        viewport.view_point.SetModelView()

        if self.background_mode == BackgroundModeOneColor:
               # clear the back buffer
            self.background_color[0].glClearColor(1.0)
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        else:
            glClear(GL_DEPTH_BUFFER_BIT)

        # render everything
        self.CreateLights()
        glDisable(GL_LIGHTING)
        Material().glMaterial(1.0)
        glDepthFunc(GL_LEQUAL)
        glEnable(GL_DEPTH_TEST)
        glLineWidth(1)
        glDepthMask(1)
        glEnable(GL_POLYGON_OFFSET_FILL)
        glShadeModel(GL_FLAT)
        viewport.view_point.SetPolygonOffset()

        after_others_objects = []
        for obj in self.objects:
            if obj.OnVisibleLayer() and obj.visible:
                if obj.DrawAfterOthers():
                    after_other_object.append(obj)
                else:
                    obj.glCommands()

        glDisable(GL_POLYGON_OFFSET_FILL)

        for callback in self.on_glCommands_list:
            callbackfunc()
        glEnable(GL_POLYGON_OFFSET_FILL)

        self.input_mode.OnRender()
        if self.transform_gl_list:
            glPushMatrix()
            m = extract_transposed(self.drag_matrix);
            glMultMatrixd(m);
            glCallList(self.transform_gl_list)
            glPopMatrix()

        # draw any last_objects
        for obj in after_others_objects:
            obj.glCommands()

        # draw the ruler
        #if(self.show_ruler and self.ruler.m_visible):
        #    self.ruler.glCommands()

        # draw the grid
        glDepthFunc(GL_LESS)
        RenderGrid(self, viewport.view_point)
        glDepthFunc(GL_LEQUAL)

        # draw the datum
        #RenderDatumOrCurrentCoordSys();

        self.DestroyLights()
        glDisable(GL_DEPTH_TEST)
        glDisable(GL_POLYGON_OFFSET_FILL)
        glPolygonMode(GL_FRONT_AND_BACK ,GL_FILL )
        #if(m_hidden_for_drag.size() == 0 || !m_show_grippers_on_drag)m_marked_list->GrippersGLCommands(false, false);

        # draw the input mode text on the top
        #if(m_graphics_text_mode != GraphicsTextModeNone)
        #{
        #    wxString screen_text1, screen_text2;

        #    if(m_sketch_mode)
        #        screen_text1.Append(_T("Sketch Mode:\n"));

        #    if(input_mode_object && input_mode_object->GetTitle())
        #    {
        #        screen_text1.Append(input_mode_object->GetTitle());
        #        screen_text1.Append(_T("\n"));
        #    }
        #    if(m_graphics_text_mode == GraphicsTextModeWithHelp && input_mode_object)
        #    {
        #        const wxChar* help_str = input_mode_object->GetHelpText();
        #        if(help_str)
        #        {
        #            screen_text2.Append(help_str);
        #        }
        #    }
        #    render_screen_text(screen_text1, screen_text2);

        # mark various XOR drawn items as not drawn
        viewport.render_on_front_done = False

    def CreateLights(self):
        amb = [0.8, 0.8, 0.8, 1.0]
        dif = [0.8, 0.8, 0.8, 1.0]
        spec =[0.8, 0.8, 0.8, 1.0]
        pos = [0.5, 0.5, 0.5, 0.0]
        lmodel_amb = [0.2, 0.2, 0.2, 1.0]
        local_viewer = [ 0.0 ]

        if self.light_push_matrix:
    		glPushMatrix()
    		glLoadIdentity()

        glLightfv(GL_LIGHT0, GL_AMBIENT, amb)
        glLightfv(GL_LIGHT0, GL_DIFFUSE, dif)
        glLightfv(GL_LIGHT0, GL_POSITION, pos)
        glLightfv(GL_LIGHT0, GL_SPECULAR, spec)
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_amb)
        glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, local_viewer)
        glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE)
        glLightfv(GL_LIGHT0,GL_SPOT_DIRECTION,pos)
        if self.light_push_matrix:
            glPopMatrix()
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        glEnable(GL_AUTO_NORMAL)
        glEnable(GL_NORMALIZE)
        glDisable(GL_LIGHT1)
        glDisable(GL_LIGHT2)
        glDisable(GL_LIGHT3)
        glDisable(GL_LIGHT4)
        glDisable(GL_LIGHT5)
        glDisable(GL_LIGHT6)
        glDisable(GL_LIGHT7)

    def DestroyLights(self):
        glDisable(GL_LIGHTING)
        glDisable(GL_LIGHT0)
        glDisable(GL_AUTO_NORMAL)
        glDisable(GL_NORMALIZE)

    def GetDrawMatrix(self, get_the_appropriate_orthogonal):
        if get_the_appropriate_orthogonal:
            # choose from the three orthoganal possibilities, the one where it's z-axis closest to the camera direction
            vx, vy = self.current_viewport.view_point.GetTwoAxes(False, 0)
            o = OCC.gp.gp_Pnt(0, 0, 0)
            if self.current_coordinate_system: o.Transform(self.current_coordinate_system.GetMatrix())
            return make_matrix(o, vx, vy)
        mat = geom.Matrix()
        if self.current_coordinate_system: mat = self.current_coordinate_system.GetMatrix()
        return mat

