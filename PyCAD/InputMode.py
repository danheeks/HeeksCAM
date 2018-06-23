class InputMode:
    def __init__(self):
        pass

    def GetTitle(self):
        return ""

    def TitleHighlighted(self):
        return True

    def GetHelpText(self):
        return None

    def OnMouse(self, event):
        pass

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