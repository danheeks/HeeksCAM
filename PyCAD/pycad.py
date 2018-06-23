# written for Python27 and using wxPython this is planned to be a pure Python rewrite of HeeksCAD
# this will be a proof of concept to see if I can store all the objects in python data structures
# I plan to also rewrite my geometry library "geom" in Python ( based on libarea ), using pyclipper

#!/usr/bin/env python
import wx
import geom
import copy

import pyclipper

app = wx.App(False)  # Create a new app, don't redirect stdout/stderr to a window.
frame = wx.Frame(None, wx.ID_ANY, "Hello World") # A Frame is a top-level window.
frame.Show(True)     # Show the frame.
app.MainLoop()
