from InputMode import InputMode
import wx
import geom

class ClickPoint:
    def __init__(self, point, depth):
        self.pos = None
        self.point = point
        self.depth = depth

    def GetPos(self, viewport):
        if(self.pos == None):
            viewport.SetViewport()
            viewport.view_point.SetProjection(True)
            viewport.view_point.SetModelView()
            screen_pos = geom.Point3D(self.point.x, viewport.GetViewportSize().GetHeight() - self.point.y, self.depth/4294967295.0)
            world_pos = viewport.view_point.glUnproject(screen_pos)
            self.pos= world_pos

        return self.pos

class SelectMode(InputMode):
    def __init__(self, cad):
        InputMode.__init__(self)
        self.CurrentPoint = wx.Point()
        self.button_down_point= wx.Point()
        self.control_key_initially_pressed = False
        self.window_box = None
        self.doing_a_main_loop = False
        self.just_one = False
        self.prompt_when_doing_a_main_loop = ""
        self.last_click_point = None
        self.cad = cad

    def GetLastClickPosition(self):
        if last_click_point == None:
            return None

    def GetTitle(self):
        if self.doing_a_main_loop:
            return self.prompt_when_doing_a_main_loop
        return "Select Mode"

    def GetHelpText(self):
        str = "Left button for selecting objects\n( with Ctrl key for extra object)\n( with Shift key for similar objects)\nDrag with left button to window select"

        if self.doing_a_main_loop:
            str.append("\nPress Esc key to cancel")

        return str

    def OnMouse(self, event):
        if event.MiddleDown():
            self.button_down_point = wx.Point(event.GetX(), event.GetY())
            self.CurrentPoint = self.button_down_point
            self.cad.current_viewport.StoreViewPoint()
            self.cad.current_viewport.view_point.SetStartMousePoint(self.button_down_point)

        if event.Dragging():
            if event.MiddleIsDown():
                dm = wx.Point()
                dm.x= event.GetX() - self.CurrentPoint.x
                dm.y = event.GetY() - self.CurrentPoint.y
                self.cad.current_viewport.view_point.Turn(dm)
                self.cad.current_viewport.need_update = True
                self.cad.current_viewport.need_refresh = True

        if event.GetWheelRotation() != 0:
            wheel_value = float(event.GetWheelRotation())
            multiplier = wheel_value / 1000.0
            if self.cad.mouse_wheel_forward_away: multipler = -multiplier

            multiplier2 = 0
            if multiplier > 0:
                multiplier2 = 1 + multiplier
            else:
                multiplier2 = 1/(1-multiplier)

            client_size = self.cad.current_viewport.GetViewportSize()

            pixelscale_before = self.cad.GetPixelScale()
            self.cad.current_viewport.view_point.Scale(multiplier2)
            pixelscale_after = self.cad.GetPixelScale()

            event_x = float(event.GetX())
            event_y = float(event.GetY())
            center_x = float(client_size.GetWidth() / 2)
            center_y = float(client_size.GetHeight()/ 2)

            px = event_x - center_x
            py = event_y - center_y
            xbefore = px/pixelscale_before
            ybefore = py/pixelscale_before
            xafter = px/pixelscale_after
            yafter = py/pixelscale_after
            xchange = xafter -xbefore
            ychange = yafter -ybefore
            x_moved_by = xchange * pixelscale_after
            y_moved_by = ychange * pixelscale_after

            self.cad.current_viewport.view_point.ShiftPoint(wx.Point(int(x_moved_by), int(y_moved_by)), wx.Point(0, 0))
            self.cad.current_viewport.need_refresh = True

    def OnKeyDown(self, event):
        pass

    def OnKeyUp(self, event):
        pass

    def OnModeChange(self):
        return True

    def GetTools(self, t_list, p):
        pass

    def OnRender(self):
        pass

    def GetProperties(self, p_list):
        pass
