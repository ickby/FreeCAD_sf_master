#   (c) Juergen Riegel (FreeCAD@juergen-riegel.net) 2011      LGPL        *
#                                                                         *
#   This file is part of the FreeCAD CAx development system.              *
#                                                                         *
#   This program is free software; you can redistribute it and/or modify  *
#   it under the terms of the GNU Lesser General Public License (LGPL)    *
#   as published by the Free Software Foundation; either version 2 of     *
#   the License, or (at your option) any later version.                   *
#   for detail see the LICENCE text file.                                 *
#                                                                         *
#   FreeCAD is distributed in the hope that it will be useful,            *
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#   GNU Library General Public License for more details.                  *
#                                                                         *
#   You should have received a copy of the GNU Library General Public     *
#   License along with FreeCAD; if not, write to the Free Software        *
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#   USA                                                                   *
#**************************************************************************

import FreeCAD, os, sys, unittest, Part
App = FreeCAD

#---------------------------------------------------------------------------
# define the test cases to test the FreeCAD Part module
#---------------------------------------------------------------------------


class PartTestCases(unittest.TestCase):
	def setUp(self):
		self.Doc = FreeCAD.newDocument("PartTest")

	def testBoxCase(self):
		self.Box = App.ActiveDocument.addObject("Part::Box","Box")
		self.Doc.recompute()
		self.failUnless(len(self.Box.Shape.Faces)==6)
		
	def testBasicShapeReference(self):
            
            p = Part.Point(App.Vector(0,0,0))
            v1 = Part.Vertex(1,1,1)
            v2 = Part.Vertex(2,2,2)
            v3 = Part.Vertex(p)
            v4 = Part.Vertex(p)
            v5 = Part.Vertex(v4)
            
            self.failUnless(v1.Reference != v2.Reference)
            self.failUnless(v3.Reference != v4.Reference)
            self.failUnless(v4.Reference == v5.Reference)
            
            l = Part.LineSegment(App.Vector(0,0,0), App.Vector(1,1,1))
            e1 = Part.Edge(l)
            e2 = Part.Edge(l)
            e3 = Part.Edge(e2)
            e4 = Part.Edge(v1, v2)
            
            self.failUnless(e1.Reference != e2.Reference)
            self.failUnless(e1.Vertexes[0].Reference != e2.Vertexes[0].Reference)
            self.failUnless(e1.Vertexes[0].Reference != e2.Vertexes[1].Reference)
            self.failUnless(e1.Vertexes[1].Reference != e2.Vertexes[1].Reference)
            self.failUnless(e3.Reference == e2.Reference)
            self.failUnless(e3.Vertexes[0].Reference == e2.Vertexes[0].Reference)
            self.failUnless(e3.Vertexes[0].Reference != e2.Vertexes[1].Reference)
            self.failUnless(e3.Vertexes[1].Reference == e2.Vertexes[1].Reference)
            self.failUnless(e4.Reference != v1.Reference)
            self.failUnless(e4.Reference != v2.Reference)
            self.failUnless(e4.Vertexes[0].Reference == v1.Reference)
            self.failUnless(e4.Vertexes[1].Reference == v2.Reference)
		
	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("PartTest")
		#print ("omit clos document for debuging")
