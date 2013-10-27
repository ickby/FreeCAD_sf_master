
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_SOURCE_DIR}/src/3rdParty/openDCM
    ${CMAKE_SOURCE_DIR}/src/Mod
    ${Boost_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${PYTHON_INCLUDE_PATH}
    ${XERCESC_INCLUDE_DIR}
    ${EIGEN3_INCLUDE_DIR}
)

generate_from_xml(Sketch3DObjectPy)

set(Sketcher3D_LIBS
    Part
    FreeCADApp
)

SET(Sketcher3D_SRCS
    AppSketcher3D.cpp
    AppSketcher3DPy.cpp
    PreCompiled.cpp
    PreCompiled.h
    Sketch3DObject.h
    Sketch3DObject.cpp
)

SET(Python_SRCS
    Sketch3DObjectPy.xml
    Sketch3DObjectPyImp.cpp
)

SET(Sketcher3D_Module_SRCS
    ${Sketcher3D_SRCS}
    ${Python_SRCS}
)

add_library(Sketcher3D SHARED ${Sketcher3D_Module_SRCS})
target_link_libraries(Sketcher3D ${Sketcher3D_LIBS})


fc_target_copy_resource(Sketcher3D 
    ${CMAKE_SOURCE_DIR}/src/Mod/Sketcher3D
    ${CMAKE_BINARY_DIR}/Mod/Sketcher3D
    Init.py)

if(MSVC)
    set_target_properties(Sketcher3D PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sketcher3D PROPERTIES DEBUG_OUTPUT_NAME "Sketcher3D_d")
    set_target_properties(Sketcher3D PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3D PROPERTIES PREFIX "../")
elseif(MINGW)
    set_target_properties(Sketcher3D PROPERTIES SUFFIX ".pyd")
    set_target_properties(Sketcher3D PROPERTIES DEBUG_OUTPUT_NAME "Sketcher3D_d")
    set_target_properties(Sketcher3D PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3D PROPERTIES PREFIX "")
else(MSVC)
    set_target_properties(Sketcher3D PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Mod/Sketcher3D)
    set_target_properties(Sketcher3D PROPERTIES PREFIX "")
    set_target_properties(Sketcher3D PROPERTIES INSTALL_RPATH ${INSTALL_RPATH})
endif(MSVC)

install(TARGETS Sketcher3D DESTINATION lib)
