
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${SOQT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
)

set(Sketcher3DGui_LIBS
    Sketcher3D
    FreeCADGui
)

qt4_add_resources(Sketcher3D_QRC_SRCS Resources/Sketcher3D.qrc)

SET(Sketcher3DGui_SRCS
    ${Sketcher3D_QRC_SRCS}
    AppSketcher3DGui.cpp
    AppSketcher3DGuiPy.cpp
    Command.cpp
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)

add_library(Sketcher3DGui SHARED ${Sketcher3DGui_SRCS})
target_link_libraries(Sketcher3DGui ${Sketcher3DGui_LIBS})

fc_target_copy_resource(Sketcher3DGui 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sketcher3D
    ${CMAKE_BINARY_DIR}/Mod/Sketcher3D
    InitGui.py)

if(MSVC)
    set_target_properties(Sketcher3DGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sketcher3DGui PROPERTIES DEBUG_OUTPUT_NAME "Sketcher3DGui_d")
    set_target_properties(Sketcher3DGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3DGui PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Sketcher3DGui PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sketcher3DGui PROPERTIES DEBUG_OUTPUT_NAME "Sketcher3DGui_d")
    set_target_properties(Sketcher3DGui PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3DGui PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Sketcher3DGui PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3DGui PROPERTIES PREFIX "")
    set_target_properties(Fem PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

install(TARGETS Sketcher3DGui DESTINATION lib)
