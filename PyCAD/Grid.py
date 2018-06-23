import geom
from OpenGL.GL import *
import math
from Color import HeeksColor

def RenderGrid2(cad, view_point, max_number_across, in_between_spaces, miss_main_lines, bg, cc, brightness, plane_mode):
    zval = 0.5
    size = view_point.viewport.GetViewportSize()
    sp = []
    sp.append( geom.Point3D(0, 0, zval) )
    sp.append( geom.Point3D(size.GetWidth(), 0, zval) )
    sp.append( geom.Point3D(size.GetWidth(), size.GetHeight(), zval) )
    sp.append( geom.Point3D(0, size.GetHeight(), zval) )

    vx, vy, plane_mode2 = view_point.GetTwoAxes(False, plane_mode)
    datum = geom.Point3D(0, 0, 0)
    orimat = cad.GetDrawMatrix(False)
    datum.Transform(orimat)
    orimat = geom.Matrix(datum, vx, vy)
    unit_forward = view_point.forwards_vector().Normalized()
    plane_dp = math.fabs(geom.Point3D(0, 0, 1).Transformed(orimat) * unit_forward)
    if plane_dp < 0.3: return
    plane = OCC.gp.gp_Pln(geom.Point3D(0, 0, 0).Transformed(orimat), OCC.gp.gp_Dir(0, 0, 1).Transformed(orimat))

    for i in range(0, 4):
        p1 = view_point.glUnproject(sp[i])
        sp[i].SetZ(0)
        p2 = view_point.glUnproject(sp[i])
        if p1.Distance(p2) < 0.00000000001: return

        line = make_line_pp(p1, p2);

        pnt = intersect_line_plane(line, plane)
        if pnt != None:
            sp[i].SetX((geom.Point3D(pnt.XYZ()) * vx) - (geom.Point3D(datum.XYZ()) * vx))
            sp[i].SetY((geom.Point3D(pnt.XYZ()) * vy) - (geom.Point3D(datum.XYZ()) * vy))
            sp[i].SetZ(0)

    b = Box()
    for i in range(0, 4):
        b.InsertPoint(sp[i].X(), sp[i].Y(), sp[i].Z())

    width = b.Width()
    height = b.Height()
    biggest_dimension = None
    if height > width: biggest_dimension = height
    else: biggest_dimension = width
    widest_spacing = biggest_dimension/max_number_across
    dimmer = False
    dimness_ratio = 1.0
    spacing = None

    if cad.draw_to_grid:
        spacing = cad.digitizing_grid
        if miss_main_lines == False: spacing = spacing * 10
        if spacing<0.0000000001: return
        if biggest_dimension / spacing > max_number_across * 1.5: return
        if biggest_dimension / spacing > max_number_across:
            dimmer = True
            dimness_ratio = (max_number_across * 1.5 - biggest_dimension / spacing)/ (max_number_across * 0.5)
    else:
        l = math.log10(widest_spacing / cad.view_units)
        intl = int(l)
        if l>0: intl = intl + 1
        spacing = math.pow(10.0, intl) * cad.view_units

    if cad.grid_mode == 3:
        dimmer = True
        dimness_ratio = dimness_ratio * plane_dp
        dimness_ratio = dimness_ratio * plane_dp

    ext2d = [b.x[0], b.x[1], b.x[3], b.x[4]]

    for i in range(0, 4):
        intval = int(ext2d[i]/spacing)

        if i <2:
            if ext2d[i]<0: intval -= 1
            elif ext2d[i]>0: intval += 1
            ext2d[i] = intval * spacing
    if cc:
        col = HeeksColor(cc.red, cc.green, cc.blue)
        if cad.grid_mode == 3:
            if plane_mode2 == 0:
                col.green = int(0.6 * float(bg.green))
            elif plane_mode2 == 1:
                col.red = int(0.9 * float(bg.red))
            else:
                col.blue = bg.blue

        if dimmer:
            d_brightness = float(brightness) * dimness_ratio
            uc_brightness = int(d_brightness)
            glColor4ub(col.red, col.green, col.blue, uc_brightness)
        else:
            glColor4ub(col.red, col.green, col.blue, brightness)

    glBegin(GL_LINES)
    extra = 0.0
    if in_between_spaces: extra = spacing * 0.5

    x = ext2d[0] - extra
    while x<ext2d[2] + extra:
        if miss_main_lines:
            xr = x/spacing/5
            temp = xr
            if temp > 0: temp += 0.5
            else: temp -= 0.5
            temp = int(temp)
            temp = float(temp)
            if math.fabs(  xr - temp ) < 0.1:
                x += spacing
                continue
        temp = geom.Point3D(datum.XYZ() + (vx.XYZ() * x) + (vy.XYZ() * ext2d[1]))
        glVertex3d(temp.X(), temp.Y(), temp.Z())
        temp = geom.Point3D(datum.XYZ() + (vx.XYZ() * x) + (vy.XYZ() * ext2d[3]))
        glVertex3d(temp.X(), temp.Y(), temp.Z())
        x += spacing

    y = ext2d[1] - extra
    while y<ext2d[3] + extra:
        if miss_main_lines:
            yr = y/spacing/5
            temp = yr
            if temp > 0: temp += 0.5
            else: temp -= 0.5
            temp = int(temp)
            temp = float(temp)
            if math.fabs(  yr - temp ) < 0.1: y += spacing; continue
        temp = geom.Point3D(datum.XYZ() + (vx.XYZ() * ext2d[0]) + (vy.XYZ() * y))
        glVertex3d(temp.X(), temp.Y(), temp.Z())
        temp = geom.Point3D(datum.XYZ() + (vx.XYZ() * ext2d[2]) + (vy.XYZ() * y))
        glVertex3d(temp.X(), temp.Y(), temp.Z())
        y += spacing

    glEnd()

def GetGridBox(cad, view_point, ext):
    zval = 0.5
    size = cad.current_viewport.GetViewportSize()
    sp = []
    sp.append( geom.Point3D(0, 0, zval) )
    sp.append( geom.Point3D(size.GetWidth(), 0, zval) )
    sp.append( geom.Point3D(size.GetWidth(), size.GetHeight(), zval) )
    sp.append( geom.Point3D(0, size.GetHeight(), zval) )
    vx, vy = view_point.GetTwoAxes(False, 0)
    datum = geom.Point3D(0, 0, 0)
    orimat = cad.GetDrawMatrix(False)
    datum.Transform(orimat)
    orimat = make_matrix(datum, vx, vy)
    plane = OCC.gp.gp_Pln(datum, geom.Point3D(0, 0, 1).Transformed(orimat))

    for i in range(0, 4):
        p1 = view_point.glUnproject(sp[i])
        sp[i].SetZ(0)
        p2 = view_point.glUnproject(sp[i])
        line = make_line_pp(p1, p2)
        pnt = intersect_line_plane(line, plane)
        if pnt != None:
            ext.InsertPoint(pnt.X(), pnt.Y(), pnt.Z())

def RenderGrid(cad, view_point, plane = 0):
    if cad.grid_mode == 1:
        cc = cad.background_color[0].best_black_or_white()
        v_bg = geom.Point3D(bg.red, bg.green, bg.blue)
        v_cc = geom.Point3D(cc.red, cc.green, cc.blue)
        v_contrast = v_cc - v_bg
        unit_contrast = v_contrast.Normalized()
        l1, l2, l3 = 100, 0, 10
        if v_cc * geom.Point3D(1,1,1)>0: l1, l2, l3 = 200, 130, 80

        if l1>v_contrast.Magnitude(): l1 = v_contrast.Magnitude()
        if l2>v_contrast.Magnitude(): l2 = v_contrast.Magnitude()
        if l3>v_contrast.Magnitude(): l3 = v_contrast.Magnitude()
        uf = (view_point.forwards_vector()).Normalized()
        vx, vy = view_point.GetTwoAxes(false, plane)
        datum = geom.Point3D(0, 0, 0)
        orimat = cad.GetDrawMatrix(False)
        datum = datum.Transformed(orimat)
        orimat = make_matrix(datum, vx, vy)
        v_up = geom.Point3D(0,0, 1).Transformed(orimat)
        fufz = math.fabs(uf * v_up)
        if fufz<0.7:
            there = (fufz - 0.3) / 0.4
            l1 *= there
            l2 *= there

        v_gc1 = v_bg + unit_contrast * l1
        v_gc2 = v_bg + unit_contrast * l2
        v_gc3 = v_bg + unit_contrast * l3

        glColor3ub(v_gc3.X(), v_gc3.Y(), v_gc3.Z())
        RenderGrid2(cad, view_point, 200, False, True, None, None, 0, plane)
        glColor3ub(v_gc2.X(), v_gc2.Y(), v_gc2.Z())
        RenderGrid2(cad, view_point, 20, True, False, None, None, 0, plane)
        glColor3ub(v_gc1.X(), v_gc1.Y(), v_gc1.Z())
        RenderGrid2(cad, view_point, 20, False, False, None, None, 0, plane)

    elif cad.grid_mode == 2 or cad.grid_mode == 3:
        bg = cad.background_color[0]
        cc = bg.best_black_or_white()
        light_color = ((cc.red + cc.green + cc.blue) > 384)
        cad.EnableBlend()
        RenderGrid2(cad, view_point, 200, False, True, bg, cc, 40 if light_color else 10, plane)
        RenderGrid2(cad, view_point, 20, True, False, bg, cc, 80 if light_color else 20, plane)
        RenderGrid2(cad, view_point, 20, False, False, bg, cc, 120 if light_color else 30, plane)
        cad.DisableBlend()
