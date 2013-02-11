
lib_LTLIBRARIES=libSketcher3DGui.la Sketcher3DGui.la

BUILT_SOURCES=

libSketcher3DGui_la_SOURCES=\
		AppSketcher3DGuiPy.cpp \
		Command.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		Workbench.cpp \
		Workbench.h

includedir = @includedir@/Mod/Sketcher3D/Gui

# the library search path.
libSketcher3DGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		$(QT_LIBS) $(all_libraries) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

libSketcher3DGui_la_CPPFLAGS = -DSketcher3DAppExport= -DSketcher3DGuiExport=

libSketcher3DGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lSketcher3D

#--------------------------------------------------------------------------------------
# Loader of libSketcher3DGui

Sketcher3DGui_la_SOURCES=\
		AppSketcher3DGui.cpp

# the library search path.
Sketcher3DGui_la_LDFLAGS = $(libSketcher3DGui_la_LDFLAGS) -module -avoid-version
Sketcher3DGui_la_CPPFLAGS = $(libSketcher3DGui_la_CPPFLAGS)

Sketcher3DGui_la_LIBADD   = \
		$(libSketcher3DGui_la_LIBADD) \
		-lSketcher3DGui

Sketcher3DGui_la_DEPENDENCIES = libSketcher3DGui.la

#--------------------------------------------------------------------------------------

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: Resources/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

libdir = $(prefix)/Mod/Sketcher3D

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt \
		Resources/Sketcher3D.qrc

