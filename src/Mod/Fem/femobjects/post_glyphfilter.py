# ***************************************************************************
# *   Copyright (c) 2016 Bernd Hahnebach <bernd@bimstatik.org>              *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENCE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************

__title__ = "FreeCAD post glyph filter"
__author__ = "Stefan Tröger"
__url__ = "https://www.freecad.org"

## @package post_glyphfilter
#  \ingroup FEM
#  \brief Post processing filter creating glyphs for vector fields

from . import base_fempythonobject
_PropHelper = base_fempythonobject._PropHelper

class PostGlyphFilter(base_fempythonobject.BaseFemPythonObject):
    """
    A post processing filter adding glyphs
    """

    Type = "Fem::PostFilterPython"

    def __init__(self, obj):
        super().__init__(obj)

        for prop in self._get_properties():
            prop.add_to_object(obj)

        self._setupFilterPipeline(obj)

    def _get_properties(self):
        prop = []

        #prop.append(
        #    _PropHelper(
        #        type="App::PropertyLinkList",
        #        name="MeshBoundaryLayerList",
        #        group="Base",
        #        doc="Mesh boundaries need inflation layers",
        #        value=[],
        #    )
        #)
        return prop

    def _setupFilterPipeline(self, obj):

        print("setup pipeline")

        # create all vtkalgorithms and set them as filter pipeline
        from vtkmodules.vtkFiltersCore import vtkPassThrough

        self._pass = vtkPassThrough()
        obj.addFilterPipeline("default", self._pass, self._pass)
        obj.setActiveFilterPipeline("default")

    def onChanged(self, obj, prop):
        print("changed: ", prop)

    def onDocumentRestored(self, obj):

        print("document restored")
        # resetup the pipeline
        self._setupFilterPipeline(obj)

