project( heekscam )
cmake_minimum_required( VERSION 2.6 )

if( ${CMAKE_VERSION} STREQUAL "2.8.2" )
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif( ${CMAKE_VERSION} STREQUAL "2.8.2" )



add_definitions ( -Wall -DUSE_GENERIC_TREECTRL -DUNICODE -DTIXML_USE_STL )

# this figures out the Python include directories and adds them to the
# header file search path
execute_process(
    COMMAND python-config --includes
    COMMAND sed -r "s/-I//g; s/ +/;/g"
    COMMAND tr -d '\n'
    OUTPUT_VARIABLE Python_Includes
)
message(STATUS "Python include dir:" ${Python_Includes})

include_directories(${Python_Includes})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package( Boost COMPONENTS python REQUIRED)  # find BOOST and boost-python
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    MESSAGE(STATUS "found Boost: " ${Boost_LIB_VERSION})
    MESSAGE(STATUS "boost-incude dirs are: " ${Boost_INCLUDE_DIRS})
    MESSAGE(STATUS "boost-python lib is: " ${Boost_PYTHON_LIBRARY})
    MESSAGE(STATUS "boost_LIBRARY_DIRS is: " ${Boost_LIBRARY_DIRS})
    MESSAGE(STATUS "Boost_LIBRARIES is: " ${Boost_LIBRARIES})    
endif()

#set additionnal CMake module path
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

#find OCE or OpenCASCADE
set( CASCADE_LIBS TKernel )
if( UNIX )
  set( OCE_DIR "/usr/local/share/cmake/" )
else()
  set( OCE_DIR "c:/OCE-0.3.0/share/cmake" )
endif()
find_package ( OCE )
if( ${OCE_FOUND} )
  add_definitions ( -DHAVE_CONFIG_H )
  set( OpenCASCADE_LIBRARIES ${CASCADE_LIBS} )
  set( OpenCASCADE_INCLUDE_DIR ${OCE_INCLUDE_DIRS} )
else() #look for OpenCASCADE
  find_package ( OpenCASCADE REQUIRED COMPONENTS ${CASCADE_LIBS} )
endif()


find_package( OpenGL REQUIRED )
find_package( wxWidgets REQUIRED COMPONENTS base core gl aui )
find_package( PythonLibs REQUIRED )

include(${wxWidgets_USE_FILE})

include_directories ( SYSTEM
    ${OpenCASCADE_INCLUDE_DIR}
    )

include_directories (
    ${CMAKE_SOURCE_DIR}/src
    ${PYTHON_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${wxWidgets_INCLUDE_DIRS}
    )

set ( EXECUTABLE_OUTPUT_PATH ../bin )

set( heekscad_HDRS
    AboutBox.h
    advprops.h
    Arc.h
    Area.h
    AreaOrderer.h
    AreaPocket.h
    AutoSave.h
    BezierCurve.h
    Box.h
    Circle.h
    clipper.h
    CNCPoint.h
    Cone.h
    ConversionTools.h
    CoordinateSystem.h
    CTool.h
    CToolDlg.h
    Cuboid.h
    CuboidDlg.h
    Curve.h
    Cylinder.h
    DepthOp.h
    DepthOpDlg.h
    DigitizedPoint.h
    DigitizeMode.h
    DimensionDrawing.h
    DoubleInput.h
    Drawing.h
    Drilling.h
    DrillingDlg.h
    DropCutter.h
    dxf.h
    Edge.h
    EndedObject.h
    ExtrudedObj.h
    Face.h
    FaceTools.h
    Geom.h
    geometry.h
    glfont2.h
    GLList.h
    GraphicsCanvas.h
    Grid.h
    Gripper.h
    GripperSelTransform.h
    Group.h
    GTri.h
    HAngularDimension.h
    HArc.h
    HArea.h
    HCircle.h
    HDialogs.h
    HDimension.h
    HDxf.h
    HeeksCAD.h
    HeeksColor.h
    HeeksConfig.h
    HeeksFrame.h
    HeeksObj.h
    HeeksObjDlg.h
    HeeksPrintout.h
    HeeksRibbon.h
    HEllipse.h
    HGear.h
    HILine.h
    HImage.h
    History.h
    HLine.h
    HPoint.h
    HSpline.h
    HText.h
    HXml.h
    IdNamedObj.h
    IdNamedObList.h
    Index.h
    InputMode.h
    InputModeCanvas.h
    Intersector.h
    LeftAndRight.h
    LineArcDrawing.h
    Loop.h
    MagDragWindow.h
    manager.h
    MarkedList.h
    MarkedObject.h
    Material.h
    NCCode.h
    NiceTextCtrl.h
    ObjList.h
    ObjPropsCanvas.h
    Observer.h
    odcombo.h
    Op.h
    OpDlg.h
    Operations.h
    OptionsCanvas.h
    OrientationModifier.h
    OutputCanvas.h
    Pattern.h
    PatternDlg.h
    Patterns.h
    Picking.h
    PictureFrame.h
    Plugins.h
    Pocket.h
    PocketDlg.h
    Point.h
    PointDrawing.h
    PointOrWindow.h
    Profile.h
    ProfileDlg.h
    Program.h
    ProgramCanvas.h
    ProgramDlg.h
    propdev.h
    PropertiesCanvas.h
    Property.h
    PropertyChange.h
    PropertyCheck.h
    PropertyChoice.h
    PropertyColor.h
    PropertyDouble.h
    PropertyFile.h
    PropertyInt.h
    PropertyLength.h
    PropertyList.h
    PropertyString.h
    PropertyTrsf.h
    PropertyVertex.h
    propgrid.h
    PythonInterface.h
    PythonString.h
    RegularShapesDrawing.h
    RemoveOrAddTool.h
    Reselect.h
    RuledSurface.h
    Ruler.h
    ScriptOp.h
    ScriptOpDlg.h
    Sectioning.h
    SelectMode.h
    Shape.h
    ShapeData.h
    ShapeTools.h
    Simulate.h
    Sketch.h
    SketchOp.h
    SketchOpDlg.h
    Solid.h
    SolidsDlg.h
    SolidTools.h
    SpeedOp.h
    SpeedOpDlg.h
    Sphere.h
    StlSolid.h
    Stock.h
    StockDlg.h
    Stocks.h
    strconv.h
    StretchTool.h
    Surface.h
    SurfaceDlg.h
    Surfaces.h
    svg.h
    Tag.h
    Tags.h
    tinystr.h
    tinyxml.h
    Tool.h
    ToolImage.h
    ToolList.h
    Tools.h
    TransformTool.h
    TransformTools.h
    TreeCanvas.h
    Vertex.h
    ViewPanning.h
    ViewPoint.h
    ViewRotating.h
    ViewZooming.h
    Window.h
    Wire.h
    wxImageLoader.h
    XYZBoxes.h
    )

set( heekscad_SRCS
    AboutBox.cpp
    advprops.cpp
    AutoSave.cpp
    Arc.cpp
    Area.cpp
    AreaClipper.cpp
    AreaOrderer.cpp
    AreaPocket.cpp
    BezierCurve.cpp
    Circle.cpp
    clipper.cpp
    CNCPoint.cpp
    Cone.cpp
    Construction.cpp
    ConversionTools.cpp
    CoordinateSystem.cpp
    CTool.cpp
    CToolDlg.cpp
    Cuboid.cpp
    CuboidDlg.cpp
    Curve.cpp
    Cylinder.cpp
    DepthOp.cpp
    DepthOpDlg.cpp
    DigitizedPoint.cpp
    DigitizeMode.cpp
    DimensionDrawing.cpp
    DoubleInput.cpp
    Drawing.cpp
    Drilling.cpp
    DrillingDlg.cpp
    DropCutter.cpp
    dxf.cpp
    Edge.cpp
    EndedObject.cpp
    ExitMainLoop.cpp
    ExtrudedObj.cpp
    Face.cpp
    FaceTools.cpp
    Finite.cpp
    Geom.cpp
    glfont2.cpp
    GLList.cpp
    GraphicsCanvas.cpp
    Grid.cpp
    Gripper.cpp
    GripperSelTransform.cpp
    Group.cpp
    HAngularDimension.cpp
    HArc.cpp
    HArea.cpp
    HCircle.cpp
    HDimension.cpp
    HDxf.cpp
    HeeksCAD.cpp
    HeeksCNC.cpp
    HeeksColor.cpp
    HeeksFrame.cpp
    HeeksObj.cpp
    HeeksObjDlg.cpp
    HeeksPrintout.cpp
    HeeksRibbon.cpp
    HEllipse.cpp
    HGear.cpp
    HILine.cpp
    HImage.cpp
    History.cpp
    HLine.cpp
    HPoint.cpp
    HSpline.cpp
    HText.cpp
    HXml.cpp
    IdNamedObj.cpp
    IdNamedObjList.cpp
    Input.cpp
    InputModeCanvas.cpp
    kurve.cpp
    LeftAndRight.cpp
    LineArcDrawing.cpp
    Loop.cpp
    MagDragWindow.cpp
    manager.cpp
    MarkedList.cpp
    MarkedObject.cpp
    Matrix.cpp
    NCCode.cpp
    NiceTextCtrl.cpp
    ObjList.cpp
    ObjPropsCanvas.cpp
    odcombo.cpp
    offset.cpp
    Op.cpp
    OpDlg.cpp
    Operations.cpp
    OptionsCanvas.cpp
    OrientationModifier.cpp
    OutputCanvas.cpp
    Pattern.cpp
    PatternDlg.cpp
    Patterns.cpp
    Picking.cpp
    PictureFrame.cpp
    Plugins.cpp
    Pocket.cpp
    PocketDlg.cpp
    Point.cpp
    PointDrawing.cpp
    PointOrWindow.cpp
    Profile.cpp
    ProfileDlg.cpp
    Program.cpp
    ProgramCanvas.cpp
    ProgramDlg.cpp
    PropertiesCanvas.cpp
    PropertyChange.cpp
    PropertyCheck.cpp
    PropertyChoice.cpp
    PropertyColor.cpp
    PropertyDouble.cpp
    PropertyFile.cpp
    PropertyInt.cpp
    PropertyLength.cpp
    PropertyList.cpp
    PropertyString.cpp
    PropertyTrsf.cpp
    PropertyVertex.cpp
    propgrid.cpp
    props.cpp
    PythonInterface.cpp
    PythonString.cpp
    RegularShapesDrawing.cpp
    RemoveOrAddTool.cpp
    Reselect.cpp
    RuledSurface.cpp
    Ruler.cpp
    ScriptOp.cpp
    ScriptOpDlg.cpp
    Sectioning.cpp
    SelectMode.cpp
    Shape.cpp
    ShapeData.cpp
    ShapeTools.cpp
    Simulate.cpp
    Sketch.cpp
    SketchOp.cpp
    SketchOpDlg.cpp
    Solid.cpp
    SolidsDlg.cpp
    SolidTools.cpp
    SpeedOp.cpp
    SpeedOpDlg.cpp
    Sphere.cpp
    StlSolid.cpp
    Stock.cpp
    StockDlg.cpp
    Stocks.cpp
    strconv.cpp
    StretchTool.cpp
    Surface.cpp
    SurfaceDlg.cpp
    Surfaces.cpp
    svg.cpp
    Tag.cpp
    Tags.cpp
    tinystr.cpp
    tinyxml.cpp
    tinyxmlerror.cpp
    tinyxmlparser.cpp
    ToolImage.cpp
    ToolList.cpp
    Tools.cpp
    TransformTool.cpp
    TransformTools.cpp
    TreeCanvas.cpp
    Vertex.cpp
    ViewPanning.cpp
    ViewPoint.cpp
    ViewRotating.cpp
    ViewZooming.cpp
    Window.cpp
    Wire.cpp
    wxImageLoader.cpp
    XYZBoxes.cpp
    )

# CMake summary
message (STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message (STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")

#-------------------- package generation -------------------------
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH i386)
else( CMAKE_SIZEOF_VOID_P EQUAL 4 )
  set(PKG_ARCH amd64)
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Dan Heeks <danheeks@gmail.com>")
set(CPACK_PACKAGE_NAME "heekscam")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_beta-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_${PKG_ARCH}")

#newlines in the summary MUST be followed by a space, or dpkg will refuse to install!
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HeeksCAM, CAD CAM based on Open CASCADE")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_VERSION "")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libwxbase2.8-0 (>= 2.8.5), libwxgtk2.8-0 (>= 2.8.5), libgtkglext1 (>= 1.2), python (>= 2.5), libboost-python1.49.0 | libboost-python1.48.0 | libboost-python1.46.1 | libboost-python1.42.0 | libboost-python1.40.0")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libopencascade-ocaf-6.3.0 | libopencascade-ocaf-6.5.0 | oce" )

include(CPack)

#---------- put subdirs down here so that the package version vars above are visible to them -----------
add_subdirectory( interface )
add_subdirectory( tinyxml )
add_subdirectory( translations )
add_subdirectory( src )  #needs libraries from other subdirs, so it goes last

