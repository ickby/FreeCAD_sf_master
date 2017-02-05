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

#	def testBoxCase(self):
#		self.Box = App.ActiveDocument.addObject("Part::Box","Box")
#		self.Doc.recompute()
#		self.failUnless(len(self.Box.Shape.Faces)==6)
		
	def testBasicShapeReference(self):
            
            p = Part.Point(App.Vector(0,0,0))
            v1 = Part.Vertex(1,1,0)
            v2 = Part.Vertex(2,2,0)
            v3 = Part.Vertex(p)
            v4 = Part.Vertex(p)
            v5 = Part.Vertex(v4)
            
            self.failUnless(v1.getConstructedFrom(p)[0].Reference == v1.Reference)
            self.failUnless(v1.getConstructedFrom(p)[0].isGeometricalEqual(v1))
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
            self.failUnless(e4.subshape(v1.Reference).Reference == v1.Reference)
            self.failUnless(e3.subshape(e2.Vertexes[0].Reference).Reference == e2.Vertexes[0].Reference)
            self.failUnless(e4.getConstructedFrom(v1)[0].isSame(e4.getConstructedFrom(v2)[0]))
            self.failUnless(e4.getConstructedFrom(v1)[0].isGeometricalEqual(e4.getConstructedFrom(v2)[0]))
            self.failUnless(e4.getConstructedFrom(v1)[0].Reference == e4.Reference)
            
            l2 = Part.LineSegment(App.Vector(2,2,0), App.Vector(2,0,0))
            e5 = Part.Edge(l2)
            w1 = Part.Wire(e1)
            w2 = Part.Wire([e4,e5])
            
            self.failUnless(w1.Edges[0].Reference == e1.Reference)
            self.failUnless(w1.Vertexes[0].Reference == e1.Vertexes[0].Reference)
            self.failUnless(w1.Vertexes[1].Reference == e1.Vertexes[1].Reference)
            self.failUnless(w2.Edges[0].Reference == e4.Reference)
            self.failUnless(w2.Edges[1].Reference == e5.Reference)
            self.failUnless(w2.Edges[0].Vertexes[0].Reference == e4.Vertexes[0].Reference)
            self.failUnless(w2.Vertexes[0].Reference == v1.Reference)
            self.failUnless(w2.Vertexes[2].Reference == e5.Vertexes[1].Reference)
            self.failUnless(w2.subshape(v1.Reference).isGeometricalEqual(v1))
            self.failUnless(w2.getConstructedFrom(v1)[0].Reference == w2.Edges[0].Reference)
            self.failUnless(w2.findSubshapes(Base=v1, Type="Constructed", Strict=True)[0].Reference == w2.Edges[0].Reference)
            self.failUnless(w2.getMergedFrom(v2.Reference)[0].Reference == w2.Vertexes[1].Reference)
            self.failUnless(w2.getMergedFrom(v2)[0].Reference == w2.getMergedFrom(e5.Vertexes[0])[0].Reference)
            
            v6 = Part.Vertex(2,0,0)
            e6 = Part.Edge(v6, v1)
            w3 = Part.Wire([w2, e6])
            self.failUnless(w3.isClosed())
            self.failUnless(w3.Edges[0].Reference == w2.Edges[0].Reference)
            self.failUnless(w3.Edges[1].Reference == w2.Edges[1].Reference)
            self.failUnless(w3.Edges[2].Reference == e6.Reference)
            
            f1 = Part.Face(w3)
            self.failUnless(f1.Edges[0].Reference == w3.Edges[0].Reference)
            self.failUnless(f1.Edges[1].Reference == w3.Edges[1].Reference)
            self.failUnless(f1.Edges[2].Reference == w3.Edges[2].Reference)   
            
            f1v2 = f1.getMergedFrom(v2)[0]
            self.failUnless(f1v2.Reference == f1.findSubshapes(Base=v2, Shape="Vertex", Type="Merged", Strict=True)[0].Reference)
            
            plane = Part.Plane()
            f2 = Part.Face(plane, w3)
            self.failUnless(f2.Edges[0].Reference == w3.Edges[0].Reference)
            self.failUnless(f2.Edges[1].Reference == w3.Edges[1].Reference)
            self.failUnless(f2.Edges[2].Reference == w3.Edges[2].Reference)
            f2v2 = f2.getMergedFrom(v2)[0]
            self.failUnless(f2v2.Reference == f2.findSubshapes(Base=v2, Shape="Vertex", Type="Merged", Strict=True)[0].Reference)
            self.failUnless(f2v2.Reference == f1v2.Reference)
            
		
	def tearDown(self):
		#closing doc
		FreeCAD.closeDocument("PartTest")
		#print ("omit clos document for debuging")
