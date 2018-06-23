import sys

sys.path.append('C:/Users/Dan Heeks/Downloads/wxPython-4.0.1-cp34-cp34m-win32')# path for wx

import wx
import sys

# This working example of the use of OpenGL in the wxPython context
# was assembled in August 2012 from the GLCanvas.py file found in
# the wxPython docs-demo package, plus components of that package's
# run-time environment.

# Note that dragging the mouse rotates the view of the 3D cube or cone.

try:
    from wx import glcanvas
    haveGLCanvas = True
except ImportError:
    haveGLCanvas = False

try:
    # The Python OpenGL package can be found at
    # http://PyOpenGL.sourceforge.net/
    from OpenGL.GL import *
    #from OpenGL.GLUT import *
    haveOpenGL = True
except ImportError:
    haveOpenGL = False

#----------------------------------------------------------------------

buttonDefs = {
    wx.NewId() : ('CubeCanvas',      'Cube'),
    wx.NewId() : ('ConeCanvas',      'Cone'),
    }

class ButtonPanel(wx.Panel):
    def __init__(self, parent):
        wx.Panel.__init__(self, parent, -1)

        box = wx.BoxSizer(wx.VERTICAL)
        box.Add((20, 30))
        keys = buttonDefs.keys()
        #keys.sort()
        for k in keys:
            text = buttonDefs[k][1]
            btn = wx.Button(self, k, text)
            box.Add(btn, 0, wx.ALIGN_CENTER|wx.ALL, 15)
            self.Bind(wx.EVT_BUTTON, self.OnButton, btn)

        # With this enabled, you see how you can put a GLCanvas on the wx.Panel
        if 1:
            c = CubeCanvas(self)
            c.SetMinSize((200, 200))
            box.Add(c, 0, wx.ALIGN_CENTER|wx.ALL, 15)

        self.SetAutoLayout(True)
        self.SetSizer(box)

    def OnButton(self, evt):
        if not haveGLCanvas:
            dlg = wx.MessageDialog(self,
                                   'The GLCanvas class has not been included with this build of wxPython!',
                                   'Sorry', wx.OK | wx.ICON_WARNING)
            dlg.ShowModal()
            dlg.Destroy()

        elif not haveOpenGL:
            dlg = wx.MessageDialog(self,
                                   'The OpenGL package was not found.  You can get it at\n'
                                   'http://PyOpenGL.sourceforge.net/',
                                   'Sorry', wx.OK | wx.ICON_WARNING)
            dlg.ShowModal()
            dlg.Destroy()

        else:
            canvasClassName = buttonDefs[evt.GetId()][0]
            canvasClass = eval(canvasClassName)
            cx = 0
            if canvasClassName == 'ConeCanvas': cx = 400
            frame = wx.Frame(None, -1, canvasClassName, size=(400,400), pos=(cx,400))
            canvasClass(frame) # CubeCanvas(frame) or ConeCanvas(frame); frame passed to MyCanvasBase
            frame.Show(True)

class MyCanvasBase(glcanvas.GLCanvas):
    def __init__(self, parent):
        glcanvas.GLCanvas.__init__(self, parent, -1, attribList=[glcanvas.WX_GL_RGBA, glcanvas.WX_GL_DOUBLEBUFFER, glcanvas.WX_GL_DEPTH_SIZE, 24])
        self.init = False
        self.context = glcanvas.GLContext(self)
        
        # initial mouse position
        self.lastx = self.x = 30
        self.lasty = self.y = 30
        self.size = None
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnMouseDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnMouseUp)
        self.Bind(wx.EVT_MOTION, self.OnMouseMotion)

    def OnEraseBackground(self, event):
        pass # Do nothing, to avoid flashing on MSW.

    def OnSize(self, event):
        wx.CallAfter(self.DoSetViewport)
        event.Skip()

    def DoSetViewport(self):
        size = self.size = self.GetClientSize()
        self.SetCurrent(self.context)
        #glViewport(0, 0, size.width, size.height)
        
    def OnPaint(self, event):
        dc = wx.PaintDC(self)
        self.SetCurrent(self.context)
        if not self.init:
            self.InitGL()
            self.init = True
        self.OnDraw()

    def OnMouseDown(self, evt):
        self.CaptureMouse()
        self.x, self.y = self.lastx, self.lasty = evt.GetPosition()

    def OnMouseUp(self, evt):
        self.ReleaseMouse()

    def OnMouseMotion(self, evt):
        if evt.Dragging() and evt.LeftIsDown():
            self.lastx, self.lasty = self.x, self.y
            self.x, self.y = evt.GetPosition()
            self.Refresh(False)

class CubeCanvas(MyCanvasBase):
    def InitGL(self):
        print('haveOpenGL = ' + str(haveOpenGL))
        if haveOpenGL:
            # set viewing projection
            glMatrixMode(GL_PROJECTION)
            glFrustum(-0.5, 0.5, -0.5, 0.5, 1.0, 3.0)

            # position viewer
            glMatrixMode(GL_MODELVIEW)
            glTranslatef(0.0, 0.0, -2.0)

            # position object
            glRotatef(self.y, 1.0, 0.0, 0.0)
            glRotatef(self.x, 0.0, 1.0, 0.0)

            glEnable(GL_DEPTH_TEST)
            glEnable(GL_LIGHTING)
            glEnable(GL_LIGHT0)
        else:
            cad.DoSomeOpenGL1(self.y, self.x)

    def OnDraw(self):
        #cad.DoSomeOpenGL2()
        #clear color and depth buffers
        glClearColor(0.75, 0.75, 0.1, 255)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        # draw six faces of a cube
        glBegin(GL_QUADS)
        glNormal3f( 0.0, 0.0, 1.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f(-0.5, 0.5, 0.5)
        glVertex3f(-0.5,-0.5, 0.5)
        glVertex3f( 0.5,-0.5, 0.5)

        glNormal3f( 0.0, 0.0,-1.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glVertex3f( 0.5, 0.5,-0.5)
        glVertex3f( 0.5,-0.5,-0.5)

        glNormal3f( 0.0, 1.0, 0.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f( 0.5, 0.5,-0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glVertex3f(-0.5, 0.5, 0.5)

        glNormal3f( 0.0,-1.0, 0.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f( 0.5,-0.5,-0.5)
        glVertex3f( 0.5,-0.5, 0.5)
        glVertex3f(-0.5,-0.5, 0.5)

        glNormal3f( 1.0, 0.0, 0.0)
        glVertex3f( 0.5, 0.5, 0.5)
        glVertex3f( 0.5,-0.5, 0.5)
        glVertex3f( 0.5,-0.5,-0.5)
        glVertex3f( 0.5, 0.5,-0.5)

        glNormal3f(-1.0, 0.0, 0.0)
        glVertex3f(-0.5,-0.5,-0.5)
        glVertex3f(-0.5,-0.5, 0.5)
        glVertex3f(-0.5, 0.5, 0.5)
        glVertex3f(-0.5, 0.5,-0.5)
        glEnd()

        if self.size is None:
            self.size = self.GetClientSize()
        w, h = self.size
        w = max(w, 1.0)
        h = max(h, 1.0)
        xScale = 180.0 / w
        yScale = 180.0 / h
        
        glRotatef((self.y - self.lasty) * yScale, 1.0, 0.0, 0.0)
        glRotatef((self.x - self.lastx) * xScale, 0.0, 1.0, 0.0)

        self.SwapBuffers()

class ConeCanvas(MyCanvasBase):
    def InitGL( self ):
#        glMatrixMode(GL_PROJECTION)
#        # camera frustrum setup
#        glFrustum(-0.5, 0.5, -0.5, 0.5, 1.0, 3.0)
#        glMaterial(GL_FRONT, GL_AMBIENT, [0.2, 0.2, 0.2, 1.0])
#        glMaterial(GL_FRONT, GL_DIFFUSE, [0.8, 0.8, 0.8, 1.0])
#        glMaterial(GL_FRONT, GL_SPECULAR, [1.0, 0.0, 1.0, 1.0])
#        glMaterial(GL_FRONT, GL_SHININESS, 50.0)
#        glLight(GL_LIGHT0, GL_AMBIENT, [0.0, 1.0, 0.0, 1.0])
#        glLight(GL_LIGHT0, GL_DIFFUSE, [1.0, 1.0, 1.0, 1.0])
#        glLight(GL_LIGHT0, GL_SPECULAR, [1.0, 1.0, 1.0, 1.0])
#        glLight(GL_LIGHT0, GL_POSITION, [1.0, 1.0, 1.0, 0.0])
#        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, [0.2, 0.2, 0.2, 1.0])
#        glEnable(GL_LIGHTING)
#        glEnable(GL_LIGHT0)
#        glDepthFunc(GL_LESS)
#        glEnable(GL_DEPTH_TEST)
#        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
#        # position viewer
#        glMatrixMode(GL_MODELVIEW)
#        # position viewer
#        glTranslatef(0.0, 0.0, -2.0);
#        #
#        glutInit(sys.argv)
        pass


    def OnDraw(self):
        # clear color and depth buffers
#        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
#        # use a fresh transformation matrix
#        glPushMatrix()
#        # position object
#        #glTranslate(0.0, 0.0, -2.0)
#        glRotate(30.0, 1.0, 0.0, 0.0)
#        glRotate(30.0, 0.0, 1.0, 0.0)

#        glTranslate(0, -1, 0)
#        glRotate(250, 1, 0, 0)
#        glutSolidCone(0.5, 1, 30, 5)
#        glPopMatrix()
#        glRotatef((self.y - self.lasty), 0.0, 0.0, 1.0);
#        glRotatef((self.x - self.lastx), 1.0, 0.0, 0.0);
        # push into visible buffer
        self.SwapBuffers()


#----------------------------------------------------------------------
class RunDemoApp(wx.App):
    def __init__(self):
        wx.App.__init__(self, redirect=False)

    def OnInit(self):
        frame = wx.Frame(None, -1, "RunDemo: ", pos=(0,0),
                        style=wx.DEFAULT_FRAME_STYLE, name="run a sample")
        #frame.CreateStatusBar()

        menuBar = wx.MenuBar()
        menu = wx.Menu()
        item = menu.Append(wx.ID_EXIT, "E&xit\tCtrl-Q", "Exit demo")
        self.Bind(wx.EVT_MENU, self.OnExitApp, item)
        menuBar.Append(menu, "&File")
        
        frame.SetMenuBar(menuBar)
        frame.Show(True)
        frame.Bind(wx.EVT_CLOSE, self.OnCloseFrame)

        win = runTest(frame)

        # set the frame to a good size for showing the two buttons
        frame.SetSize((200,400))
        win.SetFocus()
        self.window = win
        frect = frame.GetRect()

        self.SetTopWindow(frame)
        self.frame = frame
        return True
        
    def OnExitApp(self, evt):
        self.frame.Close(True)

    def OnCloseFrame(self, evt):
        if hasattr(self, "window") and hasattr(self.window, "ShutdownDemo"):
            self.window.ShutdownDemo()
        evt.Skip()

def runTest(frame):
    win = ButtonPanel(frame)
    return win

app = RunDemoApp()
app.MainLoop()