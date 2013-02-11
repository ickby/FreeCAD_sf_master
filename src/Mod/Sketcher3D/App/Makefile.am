
lib_LTLIBRARIES=libSketcher3D.la Sketcher3D.la

libSketcher3D_la_SOURCES=\
		AppSketcher3DPy.cpp \
		PreCompiled.cpp \
		PreCompiled.h

includedir = @includedir@/Mod/Sketcher3D/App

# the library search path.
libSketcher3D_la_LDFLAGS = -L../../../Base -L../../../App $(all_libraries) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libSketcher3D_la_CPPFLAGS = -DSketcher3DAppExport=

libSketcher3D_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libSketcher3D

Sketcher3D_la_SOURCES=\
		AppSketcher3D.cpp

# the library search path.
Sketcher3D_la_LDFLAGS = $(libSketcher3D_la_LDFLAGS) -module -avoid-version
Sketcher3D_la_CPPFLAGS = $(libSketcher3D_la_CPPFLAGS)

Sketcher3D_la_LIBADD   = \
		$(libSketcher3D_la_LIBADD) \
		-lSketcher3D

Sketcher3D_la_DEPENDENCIES = libSketcher3D.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes)

libdir = $(prefix)/Mod/Sketcher3D

EXTRA_DIST = \
		CMakeLists.txt
