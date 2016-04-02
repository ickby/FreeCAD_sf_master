/***************************************************************************
 *   Copyright (c) Jan Rheinländer                                         *
 *                          (jrheinlaender@users.sourceforge.net) 2013     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


// NOTE: This file contains only functions used in TopoShapeGeometry.cpp
// The only reason to have them here is to cut down on the complexity of the TopoShapeGeometry.cpp file

#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRep_Tool.hxx>
# include <TopAbs_ShapeEnum.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <Precision.hxx>
# include <gp_Pnt.hxx>
# include <gp_Circ.hxx>
# include <gp_Pln.hxx>
# include <gp_Cylinder.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <NCollection_DataMap.hxx>
# include <ShapeAnalysis_Edge.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepFeat_MakePrism.hxx>
#endif

#include <strstream>
#include <algorithm>
#include "TopoShape.h"
#include "Geometry.h"
#include "modelRefine.h"
#include <Base/Exception.h>
#include <Base/Console.h>

using namespace Part;

#ifdef FC_DEBUG
#include <Base/Console.h>
#endif

// Utility definitions
// Only TopoDS_FACE, TopoDS_EDGE and TopoDS_VERTEX are stored in the history currently
// Note: Order is important. Handling of split edges relies on Vertex being first in the iteration
const ShapeRef::typeEnum shapeTypes[] = {ShapeRef::Vertex, ShapeRef::Edge, ShapeRef::Face};
std::pair<TopAbs_ShapeEnum, int> shapeTypeIndex_data[] = {
    std::make_pair(TopAbs_VERTEX, 0),
    std::make_pair(TopAbs_EDGE,   1),
    std::make_pair(TopAbs_FACE,   2)
};
std::map<TopAbs_ShapeEnum, int> shapeTypeIndex(shapeTypeIndex_data,
    shapeTypeIndex_data + sizeof shapeTypeIndex_data / sizeof shapeTypeIndex_data[0]);
const int numShapeTypes = 3;
const int idxVERTEX = shapeTypeIndex[TopAbs_VERTEX];
const int idxEDGE   = shapeTypeIndex[TopAbs_EDGE];
const int idxFACE   = shapeTypeIndex[TopAbs_FACE];

std::pair<ShapeRef::typeEnum, std::string> shapeTypeToString_data[] = {
    std::make_pair(ShapeRef::Solid,  "Solid"),
    std::make_pair(ShapeRef::Face,   "Face"),
    std::make_pair(ShapeRef::Edge,   "Edge"),
    std::make_pair(ShapeRef::Vertex, "Vertex"),
    std::make_pair(ShapeRef::New, "New"),
    std::make_pair(ShapeRef::GeometryVtx,  "GeometryVtx"),
    std::make_pair(ShapeRef::GeometryVertex,  "GeometryVertex"),
    std::make_pair(ShapeRef::GeometryEdge,  "GeometryEdge"),
    std::make_pair(ShapeRef::GeometryFace,  "GeometryFace"),
    std::make_pair(ShapeRef::NewVertex,  "NewVertex"),
    std::make_pair(ShapeRef::NewEdge,  "NewEdge"),
    std::make_pair(ShapeRef::NewFace,  "NewFace"),
    std::make_pair(ShapeRef::SweepVertex,   "SweepVertex"),
    std::make_pair(ShapeRef::SweepEdge,   "SweepEdge"),
    std::make_pair(ShapeRef::SweepFace,   "SweepFace"),
    std::make_pair(ShapeRef::Empty,  "Empty")
};

std::map<ShapeRef::typeEnum, std::string> shapeTypeToString(shapeTypeToString_data,
    shapeTypeToString_data + sizeof shapeTypeToString_data / sizeof shapeTypeToString_data[0]);

// ========== ShapeRef implementation ==============
ShapeRef::ShapeRef() : type(Empty), index(-1), modified(false)
{
}

ShapeRef::ShapeRef(const typeEnum& t, const Standard_Integer i)
    : type(t), index(i), modified(false)
{
}

ShapeRef::ShapeRef(const TopAbs_ShapeEnum& t, const Standard_Integer i)
    : type(typeEnum(t)), index(i), modified(false)
{
}

ShapeRef::ShapeRef(const int t, const Standard_Integer i)
    : type(typeEnum(t)), index(i), modified(false)
{
}

ShapeRef::ShapeRef(const std::string& subName)
{
    int indexpos = 0;

    for (std::map<ShapeRef::typeEnum, std::string>::const_iterator r = shapeTypeToString.begin();
         r != shapeTypeToString.end(); r++) {
        if (subName.find(r->second) != std::string::npos) {
            type = r->first;
            indexpos = r->second.length();
            break;
        }
    }

    if (indexpos == 0) {
        type = Empty;
        return;
    }

    index = atoi(subName.substr(indexpos).c_str()) - 1;
    modified = false;
}

const bool ShapeRef::isEmpty() const
{
    return (type == Empty);
}

const bool ShapeRef::isNew() const
{
    return (type > New);
}

const bool ShapeRef::operator==(const ShapeRef& other) const
{
    return ((type == other.type) && (index == other.index));
}

const bool ShapeRef::operator!=(const ShapeRef& other) const
{
    return ((type != other.type) || (index != other.index));
}

// Required for std::map keying
const bool ShapeRef::operator<(const ShapeRef& other) const
{
    if (type < other.type)
        return true;
    else {
        if (type == other.type) {
            if (index < other.index)
                return true;
        }
    }

    return false;
}

const std::string ShapeRef::toString(const bool _short) const
{
    if (isEmpty())
        return std::string("EmptyRef");

    std::stringstream strm;
    strm << shapeTypeToString[type] << index+1;
    if (!_short)
        strm << (modified ? "(m)" : "");
    return strm.str();
}

// ===================

/// Compare shapes the hard way by comparing their geometric properties (coordinates etc.)
// TODO: More surface types
const bool compareSurfaces(const BRepAdaptor_Surface& sf1, const BRepAdaptor_Surface& sf2)
{
    GeomAbs_SurfaceType t1 = sf1.GetType();
    GeomAbs_SurfaceType t2 = sf2.GetType();
    if (t1 != t2) {
        //Base::Console().Error("Surface types are different\n");
        return false;
    }

    bool result;
    switch (t1) {
    case GeomAbs_Plane:
        result = ((sf1.Plane().Axis().Direction().IsEqual(sf2.Plane().Axis().Direction(), Precision::Confusion())) &&
                  (sf1.Plane().Axis().Location().IsEqual(sf2.Plane().Axis().Location(), Precision::Confusion())));
        //Base::Console().Error("Planes are %s\n", result ? "identical" : "different");
        return result;
    case GeomAbs_Cylinder:
        result = ((sf1.Cylinder().Axis().Direction().IsEqual(sf2.Cylinder().Axis().Direction(), Precision::Confusion())) &&
                  (sf1.Cylinder().Axis().Location().IsEqual(sf2.Cylinder().Axis().Location(), Precision::Confusion())) &&
                  (sf1.Cylinder().Radius() == sf2.Cylinder().Radius()));
        //Base::Console().Error("Cylinders are %s\n", result ? "identical" : "different");
        return result;
    default:
        return false;
    }
}

const bool compareShapes(const TopoDS_Shape& s1, const TopoDS_Shape& s2, const TopAbs_ShapeEnum type) {
    // FIXME: Why doesn't IsPartner, IsEqual or IsSame work?
    if (s1.IsEqual(s2)) {
        Base::Console().Error("Compare:Found equal\n");
        return true;
    }
    if (s1.IsSame(s2)) {
        Base::Console().Error("Compare:Found same\n");
        return true;
    }
    if (s1.IsPartner(s2)) {
        Base::Console().Error("Compare:Found partner\n");
        return true;
    }

    // TODO: check ShapeType before cast?
    if (type == TopAbs_VERTEX) {
        TopoDS_Vertex v1 = TopoDS::Vertex(s1);
        TopoDS_Vertex v2 = TopoDS::Vertex(s2);
        gp_Pnt p1 = BRep_Tool::Pnt(v1);
        gp_Pnt p2 = BRep_Tool::Pnt(v2);      
        return p1.IsEqual(p2, Precision::Confusion());
    } else if (type == TopAbs_EDGE) {
        TopoDS_Edge e1 = TopoDS::Edge(s1);
        TopoDS_Edge e2 = TopoDS::Edge(s2);
        BRepAdaptor_Curve c1(e1);
        BRepAdaptor_Curve c2(e2);

        // First check that curve types are identical
        GeomAbs_CurveType t1 = c1.GetType();
        GeomAbs_CurveType t2 = c2.GetType();
        if (t1 != t2)
            return false;

        switch (t1) {
        case GeomAbs_Line:
        {
            // Check start and end points of the lines
            gp_Pnt pf1, pl1, pf2, pl2;
            c1.D0(c1.FirstParameter(), pf1);
            c1.D0(c1.LastParameter(), pl1);
            c2.D0(c2.FirstParameter(), pf2);
            c2.D0(c2.LastParameter(), pl2);
            //Base::Console().Error("Line1: %f, %f, %f - %f, %f, %f\n", pf1.X(), pf1.Y(), pf1.Z(), pl1.X(), pl1.Y(), pl1.Z());
            //Base::Console().Error("Line2: %f, %f, %f - %f, %f, %f\n", pf2.X(), pf2.Y(), pf2.Z(), pl2.X(), pl2.Y(), pl2.Z());

            if (!(pf1.IsEqual(pf2, Precision::Confusion()) && pl1.IsEqual(pl2, Precision::Confusion())))
                if (!(pf1.IsEqual(pl2, Precision::Confusion()) && pl1.IsEqual(pf2, Precision::Confusion())))
                    return false;
            return true; // Lines with same start and end points must be identical
        }
        case GeomAbs_Circle:
        {
            return ((std::fabs(c1.Circle().Radius() - c2.Circle().Radius()) <= Precision::Confusion()) &&
                    (c1.Circle().Axis().IsCoaxial(c2.Circle().Axis(), Precision::Confusion(), Precision::Confusion())));
        }
        default:
        {
            throw Base::Exception("Comparing edges other than linear or circular is not implemented yet");
        }
        }
    } else if (type == TopAbs_FACE) {
        TopoDS_Face f1 = TopoDS::Face(s1);
        TopoDS_Face f2 = TopoDS::Face(s2);
        BRepAdaptor_Surface sf1(f1);
        BRepAdaptor_Surface sf2(f2);

        // First check that surface types are identical
        GeomAbs_SurfaceType t1 = sf1.GetType();
        GeomAbs_SurfaceType t2 = sf2.GetType();
        if (t1 != t2)
            return false;

        switch (t1) {
        case GeomAbs_Plane:
        {
            // Compare all vertices of the planar face
            TopTools_IndexedMapOfShape firstM, secondM;
            TopExp::MapShapes(s1, TopAbs_VERTEX, firstM);
            TopExp::MapShapes(s2, TopAbs_VERTEX, secondM);
            if (firstM.Extent() != secondM.Extent())
                return false;

            // TODO: Is it safe to assume that the vertex ordering must be identical?
            for (int i=1; i<=firstM.Extent(); i++) {
                gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(firstM(i)));
                gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(secondM(i)));
                if (!p1.IsEqual(p2, Precision::Confusion()))
                    return false;
            }

            return true;
        }
        default:
        {
            throw Base::Exception("Comparing faces other than planes is not implemented yet");
        }
        }
    }

    return false;
}

std::vector<TopTools_IndexedMapOfShape*> extractSubShapes(const TopoDS_Shape& shape)
{
    // Extract all subshapes from the shape
    // Note: If we don't use pointers here there is some compiler error
    // about TopTools_IndexedMapOfShape::TopTools_IndexedMapOfShape() being private!
    std::vector<TopTools_IndexedMapOfShape*> result;
    TopTools_IndexedMapOfShape* M;
    for (int t = 0; t < numShapeTypes; t++) {
        M = new TopTools_IndexedMapOfShape();
        TopExp::MapShapes(shape, TopAbs_ShapeEnum(shapeTypes[t]), *M);
        result.push_back(M);
    }

    return result;
}

void clearSubShapes(std::vector<TopTools_IndexedMapOfShape*> map)
{
    for (int t = 0; t < numShapeTypes; t++)
        delete map[t];
    map.clear();
}

/// find the shape in the map
// Note: IsPartner is to general - e.g. it returns true for comparison of bottom and top face of a prism
//       IsEqual is too specific - it returns false comparing equivalent faces
// Note:  m->FindIndex(shape) will NOT work!
ShapeRef findRef(const std::vector<TopTools_IndexedMapOfShape*> M, const TopoDS_Shape& shape)
{
    if (shape.IsNull())
        return ShapeRef();

    TopTools_IndexedMapOfShape* m = M[shapeTypeIndex[shape.ShapeType()]];

    for (int i=1; i <= m->Extent(); i++) {
        if ((*m)(i).IsSame(shape)) {
            ShapeRef result(shape.ShapeType(), i-1);
            //Base::Console().Error("         Corresponds to mapped shape %s\n", result.toString().c_str());
            return result;
        }
    }

    return ShapeRef();
}

/// Return the shape referenced by the ShapeRef
TopoDS_Shape findShape(const std::vector<TopTools_IndexedMapOfShape*> M, const ShapeRef& ref) {
    return (*M[shapeTypeIndex[(TopAbs_ShapeEnum)ref.type]])(ref.index + 1);
}

/**
 * Ancestors are shapes that directly generated the given shape. E.g. a sketch vertex
 * generating an edge in a prism, or an edge in a shape generating a split segment in the result
 * shape of a boolean operation. Section edges have two, section vertices three ancestors.
 * Modifiers are any other shapes that modify the given shape, e.g. a face that splits the shape into segments
 */
void findInMap(const RefMap& m, const ShapeRef& ref, RefVec& ancestors, RefVec& modifiers)
{
    for (RefMap::const_iterator r = m.begin(); r != m.end(); r++) {
        RefVec::const_iterator it = std::find(r->second.begin(), r->second.end(), ref);
        if (it != r->second.end()) {
            if (it->modified)
                modifiers.push_back(r->first);
            else
                ancestors.push_back(r->first);
        }
    }
}

/// Find all shapes in the map that either generate or modify the given shape
RefVec findParentsInMap(const RefMap& m, const ShapeRef& ref)
{
    RefVec modifiers, result;
    findInMap(m, ref, result, modifiers);
    result.insert(result.end(), modifiers.begin(), modifiers.end());
    return result;
}

/// Return only the ancestors of the given shape
const RefVec findAncestorsInMap(const RefMap& m, const ShapeRef& ref)
{
    RefVec modifiers, result;
    findInMap(m, ref, result, modifiers);
    return result;
}

/// Return true if the given shape is mentioned as a child shape in the map
const bool isInMap(const RefMap& m, const ShapeRef& ref)
{
    for (RefMap::const_iterator r = m.begin(); r != m.end(); r++) {
        RefVec::const_iterator it = std::find(r->second.begin(), r->second.end(), ref);
        if (it != r->second.end())
            return true;
    }

    return false;
}

/**
  * If s is a closed edge, return the single vertex of this edge
  * If it is a closed face, return the seam edge
  */
TopoDS_Shape getSingularShape(const TopoDS_Shape& s)
{
    if ((s.ShapeType() == TopAbs_EDGE) && s.Closed()) {
        // Get the single vertex of the closed loop
        TopTools_IndexedMapOfShape V;
        TopExp::MapShapes(s, TopAbs_VERTEX, V);
        if (V.Extent() == 1)
            return V(1);
    } else if (s.ShapeType() == TopAbs_FACE) {
        // Does the face have a seam edge?
        TopTools_IndexedMapOfShape E;
        TopExp::MapShapes(s, TopAbs_EDGE, E);
        for (int e=1; e<=E.Extent(); e++)
            if (BRep_Tool::IsClosed(TopoDS::Edge(E(e)), TopoDS::Face(s)))
                return E(e);
    }

    // Other shapetypes are not supported
    return TopoDS_Shape();
}

/// Build map of relations between old and new shape just by comparing entities for geometric identity
RefMap buildRefMap(const TopoDS_Shape& newShape, const TopoDS_Shape& oldShape)
{
    RefMap result;

    // Extract all subshapes from old and new shape
    std::vector<TopTools_IndexedMapOfShape*> newM, oldM;
    newM = extractSubShapes(newShape);
    oldM = extractSubShapes(oldShape);
    // Create sets of indices for all new shapes to check that they have all been handled
    std::set<Standard_Integer> newIndices[numShapeTypes];

    // Look at all objects in the old shape and try to find the related object in the new shape
    for (int t = 0; t < numShapeTypes; t++) {
        // Fill set
        for (Standard_Integer i = 1; i <= newM[t]->Extent(); i++)
            newIndices[t].insert(i);

        Base::Console().Error("Investigating %s in old shape\n", shapeTypeToString[shapeTypes[t]].c_str());
        ShapeRef refOld(shapeTypes[t]);

        for (int i=1; i<=oldM[t]->Extent(); i++) {
            refOld.index = i-1; // adjust indices to start at zero
            Base::Console().Error("   Investigating subshape %s\n", refOld.toString().c_str());

            for (int j=1; j<=newM[t]->Extent(); j++) { // find object in newM corresponding to oldM
                ShapeRef refNew(shapeTypes[t], j-1);
                TopoDS_Shape oldShape = findShape(oldM, refOld);

                if (compareShapes(oldShape, findShape(newM, refNew), (TopAbs_ShapeEnum)refOld.type)) {
                    Base::Console().Error("      Corresponds to new %s\n", refNew.toString().c_str());
                    result[refOld].push_back(refNew);
                    newIndices[t].erase(j);
                    break;
                }
            }
        }

        // Handle shapes not found (e.g. the face generated from a closed loop of edges!)
        if (newIndices[t].size() > 1)
            Base::Console().Error("WARNING: More than one new shape! Could break naming if order changes!\n");
        int newIndex = 0; // Just to be consistent with buildRefMap(mkPrism, ...)
        for (std::set<Standard_Integer>::const_iterator i = newIndices[t].begin(); i != newIndices[t].end(); i++)
            result[ShapeRef(ShapeRef::NewVertex + t, newIndex++)].push_back(ShapeRef(shapeTypes[t], (*i)-1));
    }

    clearSubShapes(newM);
    clearSubShapes(oldM);
    return result;
}

/**
  * Build map using the information from the BRepBuilderAPI_MakeShape object
  * This is the basic method called by all specialized buildRefMap() methods
  * If splitShapes is not NULL, then return the ShapeRefs of all entities in oldM that
  * have been split into two or more parts by the mkShape operation
  */
RefMap buildGenericRefMap(BRepBuilderAPI_MakeShape& mkShape,
                          std::vector<TopTools_IndexedMapOfShape*> newM,
                          std::vector<TopTools_IndexedMapOfShape*> oldM,
                          RefVec* splitShapes = NULL)
{
    RefMap result;

    // Look at all objects in the old shape and try to find the related object in the new shape
    for (int t = 0; t < numShapeTypes; t++) {
        Base::Console().Error("Investigating %s in old shape\n", shapeTypeToString[shapeTypes[t]].c_str());
        ShapeRef refOld(shapeTypes[t]);

        for (int i=1; i<=oldM[t]->Extent(); i++) {
            Base::Console().Error("   Investigating subshape %u of %u\n", i, oldM[t]->Extent());
            refOld.index = i-1; // adjust indices to start at zero
            TopoDS_Shape oldShape = findShape(oldM, refOld);

            TopTools_ListIteratorOfListOfShape it;
            ShapeRef refNew;
            bool found = false;

            // Find all new objects that are a modification of the old object (e.g. a face was resized)
            // Note: If there is more than one object this is taken to mean that the old object has
            // been split into two or more parts. We save the origin of the split shapes so that
            // ambiguities can be resolved by the calling method
            const TopTools_ListOfShape& mod(mkShape.Modified(oldShape));
            if ((mod.Extent() > 1) && (splitShapes != NULL)) {
                Base::Console().Error("         Recording origin of split shapes %s\n", refOld.toString().c_str());
                splitShapes->push_back(refOld);
            }

            for (it.Initialize(mod); it.More(); it.Next()) {
                found = true;

                // find object in newM corresponding to Modified() object
                refNew = findRef(newM, it.Value());
                Base::Console().Error("      Found modified object: %s\n", refNew.toString().c_str());
                if (!refNew.isEmpty())
                    result[refOld].push_back(refNew);
            }

            // Find all new objects that were generated from an old object (e.g. a face generated from an edge)
            for (it.Initialize(mkShape.Generated(oldShape)); it.More(); it.Next()) {
                found = true;

                // The shape might not be found e.g. for the case of FACE -> SOLID
                // But the found = true remains valid anyway to avoid duplicate references!
                refNew = findRef(newM, it.Value());                
                if (!refNew.isEmpty()) {
                    Base::Console().Error("      Found generated object: %s\n", refNew.toString().c_str());
                    result[refOld].push_back(refNew);
                }
            }

            // Find all old objects that don't exist any more (e.g. a face was completely cut away)
            if (!found) {
                // Nothing was found yet
                // Note: BRepFilletAPI_MakeFillet.IsDeleted() claims far too many shapes as deleted, so we
                // can't use IsDeleted() here to avoid unnecessary findRef() calls
                // Note: BRepFeat_MakePrism throws an exception when IsDeleted is called with a vertex
                // of the face used to extrude the prism
//                if (mkShape.IsDeleted(oldShape)) {
//                    Base::Console().Error("      Found deleted object\n");
//                } else {
                    // This branch is actually the most likely one as all shapes that have not been
                    // touched by the mkShape operation fall into this category
                    // Search all shapes of the same type in the new shape
                    // We don't go here if something was already found as Generated() or Modified()
                    // to avoid duplicates occuring e.g. with boolean operations
                    refNew = findRef(newM, oldShape);
                    if (!refNew.isEmpty()) {
                        Base::Console().Error("      Found same object: %s\n", refNew.toString().c_str());
                        result[refOld].push_back(refNew);                                                
                    } else {
                        Base::Console().Error("      Found nothing for object, marked as deleted\n");
                    }
//                }
            }
        }
    }

    return result;
}

/**
  * Join two maps.
  */
/*
  * If merge is true, then records in the oldMap that do not match any records
  * in the newMap get copied to the result. Otherwise, the old shape is marked
  * as deleted in the map
*/
// e.g. old map: VERTEX0 -> EDGE0, new map: EDGE0 -> FACE2, result map: VERTEX0 -> FACE2
RefMap joinMap(const RefMap& oldMap, const RefMap& newMap) // const bool merge = false
{
    if (oldMap.size() == 0)
        return newMap;

    RefMap result;

    // Iterate through all old entries and join the corresponding new entries to them
    for (RefMap::const_iterator o = oldMap.begin(); o != oldMap.end(); o++) {
        //if (merge)
        //    result[o->first] = o->second; // add new history on top of old history

        for (RefVec::const_iterator r = o->second.begin(); r != o->second.end(); r++) {
            for (RefMap::const_iterator n = newMap.begin(); n != newMap.end(); n++) {
                if (*r == n->first) {
                    for (RefVec::const_iterator r2 = n->second.begin(); r2 != n->second.end(); r2++) {
                        ShapeRef second = *r2;
                        second.modified = (r->modified || r2->modified);
                        result[o->first].push_back(second);
                    }
                }
            }
        }
    }

    // Copy over newly created entries
    for (RefMap::const_iterator n = newMap.begin(); n != newMap.end(); n++) {
        if (n->first.isNew()) {
            // TODO: Can there be a collision of new shapes?
            result[n->first] = n->second;
        }
    }

    return result;
}

// Build a RefMap from a sketch's geometry
RefMap buildRefMapFromSketch(const TopoDS_Shape shape, const std::vector<Geometry*>& geometry)
{
    RefMap map;
/*
    // Extract all subshapes from the shape (having one or more wires)
    std::vector<TopTools_IndexedMapOfShape*> M = extractSubShapes(shape);

    // Look at all entities in the geometry
    // Note: Currently sketches can only contain points, lines, arcs, circles and ellipses
    // TODO: Can we assume that the first entity allways gets mapped to the first edge etc. ? That would save a lot of time
    for (unsigned entity = 0; entity < geometry.size(); entity++) {
        // Get entity type
        TopAbs_ShapeEnum type;
        if (geometry[entity]->isDerivedFrom(Part::GeomPoint::getClassTypeId()))
            type = TopAbs_VERTEX;
        else if (geometry[entity]->isDerivedFrom(Part::GeomCurve::getClassTypeId()))
            type = TopAbs_EDGE;
        else if (geometry[entity]->isDerivedFrom(Part::GeomSurface::getClassTypeId()))
            type = TopAbs_FACE;

        Base::Console().Error("Looking at %s with uid %u\n", shapeTypeToString[ShapeRef::typeEnum(type)].c_str(), geometry[entity]->uid);

        // Find the corresponding object in the shape
        TopoDS_Shape geoShape = geometry[entity]->toShape();
        bool found = false;

        for (int i=1; i<=M[shapeTypeIndex[type]]->Extent(); i++) {
            // TODO: Extract information from geometry directly, without creating a shape first?
            // For example Part::GeomPoint::compare(TopoDS_Vertex)
            // Also its unnecessary to extract the curvetype and vertices of M[](i) again and again...
            TopoDS_Shape newShape = (*M[shapeTypeIndex[type]])(i);

            if (compareShapes(geoShape, newShape, type)) {
                Base::Console().Error("   Found corresponding object %u\n", i);
                int geotype = ShapeRef::GeometryVertex + shapeTypeIndex[type];
                ShapeRef geoRef(geotype, geometry[entity]->uid - 1); // The -1 only to be consistent when debug-printing...
                map[geoRef].push_back(ShapeRef(type, i-1));

                // Get the points of the edge because they are not contained in geometry
                TopTools_IndexedMapOfShape V;
                TopExp::MapShapes(newShape, TopAbs_VERTEX, V);
                geoRef.type = ShapeRef::GeometryVtx; // set unique type so the vertex doesn't overlap a GeometryVertex
                geoRef.index = geometry[entity]->uid - 1;
                for (int v=1; v<=V.Extent(); v++) {
                    if (V(v).Orientation() == TopAbs_FORWARD)
                        // One vertex per edge is enough, since every vertex belongs to two edges...
                        map[geoRef].push_back(findRef(M, V(v)));
                }
                found = true;
                break;
            }
        }

        if (!found)
            Base::Console().Error("   WARNING: Unhandled entity\n");
    }

    clearSubShapes(M);*/
    return map;
}

// Specializations of buildRefMap for different BRepBuilderAPI_MakeShape classes

/**
  * Build map using the information from the BRepBuilderAPI_MakeShape object
  * This is a fall-through method if none of the above specialized methods fit
  * or if no specialization is necessary as for BRepBuilderAPI_RefineModel
  */
RefMap buildRefMap(BRepBuilderAPI_MakeShape &mkShape, const TopoDS_Shape& oldShape)
{
    // Extract all subshapes from old and new shape
    std::vector<TopTools_IndexedMapOfShape*> newM, oldM;
    newM = extractSubShapes(mkShape.Shape());
    oldM = extractSubShapes(oldShape);

    // Build general history for all BRepBuilderAPI_MakeShape classes
    RefMap result = buildGenericRefMap(mkShape, newM, oldM);

    clearSubShapes(newM);
    clearSubShapes(oldM);

    return result;
}

/**
  * Build map using the information from the BRepPrimAPI_MakeSweep object
  * oldShapes contains the base shape of the sweep (i.e. the face created from a sketch). As a 
  * sweep can be created from multiple profiles many oldshapes are accepted.
  * The map from each oldShape to mkSweep.Shape() is returned in result
  */
std::vector<RefMap> buildRefMap(BRepPrimAPI_MakeSweep &mkSweep, const std::vector<TopoDS_Shape> oldShapes)
{
    // Extract all subshapes from old and new shape
    typedef std::vector<TopTools_IndexedMapOfShape*> ShapeMap;
    ShapeMap newM;
    newM = extractSubShapes(mkSweep.Shape());

    std::vector<ShapeMap> oldM(oldShapes.size());    
    for(unsigned int i=0; i<oldShapes.size(); ++i)
        oldM[i] = extractSubShapes(oldShapes[i]);

    // Build general history for all BRepBuilderAPI_MakeShape classes
    std::vector<RefMap> result(oldShapes.size());
    for(unsigned int i=0; i< oldM.size(); ++i)
        result[i] = buildGenericRefMap(mkSweep, newM, oldM[i]);
    
    return result;
}
    
/**
  * Build map using the information from the BRepPrimAPI_MakePrism object
  * oldShape contains the base shape of the prism (i.e. the face created from a sketch)
  * The map from oldShape to mkPrism.Shape() is returned in result
  */
RefMap buildRefMap(BRepPrimAPI_MakePrism &mkPrism, const TopoDS_Shape& oldShape)
{
    // Extract all subshapes from old and new shape
    std::vector<TopTools_IndexedMapOfShape*> newM, oldM;
    newM = extractSubShapes(mkPrism.Shape());
    oldM = extractSubShapes(oldShape);

    // Build general history for all BRepBuilderAPI_MakeShape classes
    RefMap result = buildGenericRefMap(mkPrism, newM, oldM);

    // Look at all objects in the base shape and try to find the related object at the
    // beginning and end of the prism
    for (int t = 0; t < numShapeTypes; t++) {
        Base::Console().Error("Investigating %s in base shape of prism\n", shapeTypeToString[shapeTypes[t]].c_str());
        ShapeRef refOld(shapeTypes[t]);

        for (int i=1; i<=oldM[t]->Extent(); i++) {
            Base::Console().Error("   Investigating subshape %u of %u\n", i, oldM[t]->Extent());
            refOld.index = i-1;
            TopoDS_Shape oldShape = findShape(oldM, refOld);

            ShapeRef refNew = findRef(newM, mkPrism.FirstShape(oldShape));
            if (!refNew.isEmpty())
                result[refOld].push_back(refNew);

            refNew = findRef(newM, mkPrism.LastShape(oldShape));
            if (!refNew.isEmpty()) {
                Base::Console().Error("      Found new object: %s\n", refNew.toString().c_str());
                // Make sure indices are unique. This only works because refOld.index corresponds
                // to the UID of the SketchGeometry
                int type = ShapeRef::SweepVertex + t; //shapeTypeIndex[TopAbs_ShapeEnum(refOld.type)];
                result[ShapeRef(type, refOld.index)].push_back(refNew);
            }
        }
    }

    clearSubShapes(newM);
    clearSubShapes(oldM);
    return result;
}

/**
  * Build map using the information from the BRepPrimAPI_MakeRevol object
  * oldShape contains the base shape of the revolution (i.e. the face created from a sketch)
  * The map from oldShape to mkRevol.Shape() is returned in result
  */
RefMap buildRefMap(BRepPrimAPI_MakeRevol &mkRevol, const TopoDS_Shape& oldShape)
{
    // TODO: This code is virtually identical with the MakePrism code, but sadly the superclass
    // MakeSweep does not have the LastShape/FirstShape(TopoDS_Shape) method ...
    // Extract all subshapes from old and new shape
    std::vector<TopTools_IndexedMapOfShape*> newM, oldM;
    newM = extractSubShapes(mkRevol.Shape());
    oldM = extractSubShapes(oldShape);

    // Build general history for all BRepBuilderAPI_MakeShape classes
    RefMap result = buildGenericRefMap(mkRevol, newM, oldM);

    // Look at all objects in the base shape and try to find the related object at the end of the sweep
    for (int t = 0; t < numShapeTypes; t++) {
        Base::Console().Error("Investigating %s in base shape of revolution\n", shapeTypeToString[shapeTypes[t]].c_str());
        ShapeRef refOld(shapeTypes[t]);

        for (int i=1; i<=oldM[t]->Extent(); i++) {
            Base::Console().Error("   Investigating subshape %u of %u\n", i, oldM[t]->Extent());
            refOld.index = i-1;
            TopoDS_Shape oldShape = findShape(oldM, refOld);

            ShapeRef refNew = findRef(newM, mkRevol.FirstShape(oldShape));
            if (!refNew.isEmpty())
                result[refOld].push_back(refNew);

            refNew = findRef(newM, mkRevol.LastShape(oldShape));
            if (!refNew.isEmpty()) {
                //result[refOld].push_back(refNew);
                int type = ShapeRef::SweepVertex + t; //shapeTypeIndex[TopAbs_ShapeEnum(refOld.type)];
                result[ShapeRef(type, refOld.index)].push_back(refNew);
            }
        }
    }

    clearSubShapes(newM);
    clearSubShapes(oldM);

    return result;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO: what about HasDegeneratedEdges() ?
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

RefMap buildRefMap(BRepFeat_MakePrism &mkPrism, const TopoDS_Shape& oldShape)
{
    // Extract all subshapes from old and new shape
    std::vector<TopTools_IndexedMapOfShape*> newM, oldM;
    newM = extractSubShapes(mkPrism.Shape());
    oldM = extractSubShapes(oldShape);
    RefVec splitOrigins;

    // Build general history for all BRepBuilderAPI_MakeShape classes    
    RefMap result = buildGenericRefMap(mkPrism, newM, oldM, &splitOrigins);

    clearSubShapes(newM);
    clearSubShapes(oldM);

    return result;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO: are there NewEdges() and TgtEdges() ? What about Curves() ? FirstShape()? LastShape()? splitOrigins?
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

/// Find the shape in "shape" that matches "s". Required e.g. for extracting the orientation
const TopoDS_Shape findSameShape(const TopoDS_Shape& shape, const TopoDS_Shape& s) {
    TopTools_IndexedMapOfShape M;
    TopExp::MapShapes(shape, s.ShapeType(), M);
    for (int m=1; m<=M.Extent(); m++) {
        if (M(m).IsSame(s))
            return M(m);
    }
    // TODO: Would M.FindIndex(s) be faster? But this probably compares the Orientation, too, which
    // we don't want to do

    // Note: This is not an error condition. Sometimes the shape does not exist
    return TopoDS_Shape();
}

// Utility struct to collect topological data
struct shapeData {
    RefVec vertices; // vertices of the shape
    RefSet adjShapes; // adjacent edges and faces of the shape
    RefSet touchFaces; // faces touching the shape at a vertex (but not adjacent to the shape itself)
    bool splitSegment; // shape is a split segment of a larger shape in the original shape
    bool section; // shape is a section edge or a vertex of a section edge
    bool original; // shape IsSame() with a shape on one of the originals of the boolean operation
    std::vector<BRepAdaptor_Surface> surfaces; // the underlying surfaces of adjacent faces of an edge
    shapeData() : splitSegment(false), section(false), original(false) {}
};

/**
 * Create a map which lists all children of a given type for each entry in the map M
 * e.g. map all vertices of a face for all faces in M
 */
// Note: TopTools_IndexedDataMapOfShapeListOfShape has similar functionality but some defects
// 1. TopExp::MapShapesAndAncestors(mkRefine.Shape(), TopAbs_VERTEX, TopAbs_FACE, mapVertexFace)
//    gave 6 faces as ancestors of every vertex (in other words, lots of duplicates)
// 2. The FindFromKey() method returns a list of shapes, whereas we really want a list of indices
// So its better to do the job ourselves

// Required for the NCollection_DataMap<TopoDS_Shape, std::set<int> >()
Standard_Boolean IsEqual(const TopoDS_Shape& s1, const TopoDS_Shape& s2) {
    return s1.IsSame(s2);
}
typedef NCollection_DataMap<TopoDS_Shape, std::set<int> > childrenMap;
childrenMap buildChildrenMap(const TopTools_IndexedMapOfShape* M, const TopAbs_ShapeEnum& type) {
    childrenMap result;

    for (int s = 1; s <= M->Extent(); s++) {
        TopTools_IndexedMapOfShape C;
        TopExp::MapShapes((*M)(s), type, C);
        for (int c = 1; c <= C.Extent(); c++) {
            if (result.IsBound(C(c))) {
                // child already exists
                result(C(c)).insert(s-1);
            } else {
                // new child entry
                std::set<int> entry;
                entry.insert(s-1);
                result.Bind(C(c), entry);
            }
        }
    }

    return result;
}

// Auxiliary method to shorten the code
// Extract orientation to disambiguate twin shapes
const TopAbs_Orientation getTwinOrientation(const ShapeRef& ref1, const ShapeRef& ref2,
        const std::vector<TopTools_IndexedMapOfShape*>& refineM, const std::map<ShapeRef, shapeData>& Shapes)
{
    TopAbs_Orientation o1, o2;
    TopoDS_Shape shape1 = findShape(refineM, ref1);
    TopoDS_Shape shape2 = findShape(refineM, ref2);
    RefSet adjShapes1 = Shapes.at(ref1).adjShapes;
    RefSet adjShapes2 = Shapes.at(ref2).adjShapes;

    // For vertices, the adjShapes can be either two edges, or an edge and a face, or two edges and a face
    // So we need to make sure the right shape types are paired off against one another
    RefVec v_ref1;
    RefVec v_ref2;
    if (ref1.type == ShapeRef::Vertex) {
        bool facepair = false;
        for (RefSet::const_iterator adj1 = adjShapes1.begin(); adj1 != adjShapes1.end(); ++adj1) {
            if (adj1->type == ShapeRef::Edge) {
                v_ref1.push_back(*adj1);
            } else {
                v_ref1.insert(v_ref1.begin(), *adj1);
            }
        }
        for (RefSet::const_iterator adj2 = adjShapes2.begin(); adj2 != adjShapes2.end(); ++adj2) {
            if (adj2->type == ShapeRef::Edge) {
                v_ref2.push_back(*adj2);
            } else {
                if (v_ref1.front().type == ShapeRef::Face) {
                    v_ref2.insert(v_ref2.begin(), *adj2);
                    facepair = true;
                } // Otherwise we don't need this face since it has no match in adjShapes1
            }
        }
        if (!facepair && (v_ref1.front().type == ShapeRef::Face))
            v_ref1.erase(v_ref1.begin());
    } else {
        v_ref1.insert(v_ref1.begin(), adjShapes1.begin(), adjShapes1.end());
        v_ref2.insert(v_ref2.begin(), adjShapes2.begin(), adjShapes2.end());
    }

    RefVec::const_iterator vadj1 = v_ref1.begin();
    RefVec::const_iterator vadj2 = v_ref2.begin();
    while ((vadj1 != v_ref1.end())) {
        o1 = findSameShape(findShape(refineM, *vadj1), shape1).Orientation();
        o2 = findSameShape(findShape(refineM, *vadj2), shape2).Orientation();
        if (o1 != o2)
            break;
        Base::Console().Error("      Orientation is identical on %s and %s\n", vadj1->toString().c_str(), vadj2->toString().c_str());
        vadj1++;
        vadj2++;
    }

    if (o1 != o2) {
        // Success!
        return o1;
    } else if (ref1.type == ShapeRef::Edge) {
        // Check for the special case of two adjacent section edges, and use the orientation
        // of the common vertex to set the symmetry flag. This commonly occurs when a closed
        // edge loop is split. The common vertex was created by the seam edge of the ancestor shape
        RefVec vertices1 = Shapes.at(ref1).vertices;
        RefVec vertices2 = Shapes.at(ref2).vertices;
        Base::Console().Error("      Searching for common vertex\n");
        for (RefVec::const_iterator v1 = vertices1.begin(); v1 != vertices1.end(); v1++) {
            for (RefVec::const_iterator v2 = vertices2.begin(); v2 != vertices2.end(); v2++) {
                if (*v1 == *v2) {
                    TopoDS_Shape commonVertex = findShape(refineM, *v1);
                    Base::Console().Error("      Found common vertex %s\n", v1->toString().c_str());
                    o1 = findSameShape(shape1, commonVertex).Orientation();
                    o2 = findSameShape(shape2, commonVertex).Orientation();
                    if (o1 != o2)
                        return o1;
                }
            }
        }
    }

    Base::Console().Error("      Orientation is identical, disambiguation failed\n");
    return TopAbs_INTERNAL; // Misuse this to report failure
}

/**
  * Build map using the information from the BooleanOperation object
  * The map from oldShape[0] to mkBool.Shape() is returned in result[0], and from
  * oldShape[1] to mkBool.Shape() in result[1] and so on. Number of old shapes is not limited, but 
  * must be the same as result size. Information about split segments and symmetry is returned in 
  * TopInfo. Returns the new shape with refined (merged) coplanar faces
  */
TopoDS_Shape buildRefMap(BRepAlgoAPI_BooleanOperation &mkBool,
                         const std::vector<TopoDS_Shape> oldShape,
                         std::vector<RefMap>& result,
                         const std::vector<TopInfoMap> oldTopInfos,
                         TopInfoMap& newTopInfo)
{    
    if(oldShape.size() != result.size())
        throw Base::Exception("Wrong usage of buildRefMap: different number of old shapes and result maps");
        
    int shapeNum = oldShape.size();
    
    // Join coplanar faces - this saves us a lot of trouble with split shapes and section edges!
    BRepBuilderAPI_RefineModel mkRefine(mkBool.Shape());
    if (!mkRefine.IsDone())
        throw Base::Exception("TopoShape: mkBool: Refine faces failed");        
    
    // Extract all subshapes from old and new shapes, and build general history
    // for all BRepBuilderAPI_MakeShape classes
    std::vector<TopTools_IndexedMapOfShape*> newM = extractSubShapes(mkBool.Shape());
    std::vector<std::vector<TopTools_IndexedMapOfShape*> > oldM(shapeNum);
    std::vector<RefVec> splitOrigins(shapeNum);
    for (int i = 0; i < shapeNum; i++) {
        oldM[i] = extractSubShapes(oldShape[i]);
        result[i] = buildGenericRefMap(mkBool, newM, oldM[i], &splitOrigins[i]);
    }

    // Join the history for mkRefine to the general history
    std::vector<TopTools_IndexedMapOfShape*> refineM = extractSubShapes(mkRefine.Shape());
    // Note: Currently when RefineModel() joins two coplanar faces, one of the original faces is
    // registered as deleted, while the other is registered as modified. From the point of view
    // of shape history, it would be better that both are registered as modified. I opened a thread
    // on this subject.
    Base::Console().Error("Building map for refined faces\n");
    RefMap refineMap = buildGenericRefMap(mkRefine, refineM, newM);
    for (int i = 0; i < shapeNum; i++)
        result[i] = joinMap(result[i], refineMap);

    // Create a new, combined TopInfoMap so that we can build a splitPath later on
    for (int i = 0; i < shapeNum; i++) {
        for (TopInfoMap::const_iterator t = oldTopInfos[i].begin(); t != oldTopInfos[i].end(); t++) {
            RefMap::const_iterator newRefs = result[i].find(t->first);
            if (newRefs == result[i].end())
                // Shape has disappeared in the ModelRefine
                continue;
            for (RefVec::const_iterator r = newRefs->second.begin(); r != newRefs->second.end(); r++) {
                if (!t->second.splitPath.empty())
                    newTopInfo[*r].splitPath = t->second.splitPath;
                if (t->second.symmetryFlag)
                    newTopInfo[*r].symmetryFlag = t->second.symmetryFlag;
            }
        }
    }

    // Collect the topological information required for the algorithms below
    Base::Console().Error("Collecting topological data\n");
    std::map<ShapeRef, shapeData> Shapes;

    // Note: Theoretically we can construct mapVertexFace from the other two maps
    childrenMap mapEdgeFace   = buildChildrenMap(refineM[idxFACE], TopAbs_EDGE);
    childrenMap mapVertexEdge = buildChildrenMap(refineM[idxEDGE], TopAbs_VERTEX);
    childrenMap mapVertexFace = buildChildrenMap(refineM[idxFACE], TopAbs_VERTEX);

    for (int i = 0; i < shapeNum; i++) {
        // Find all split segments
        RefVec splitSegments;
        for (RefVec::const_iterator o = splitOrigins[i].begin(); o != splitOrigins[i].end(); o++) {
            const RefVec& segments = result[i][*o];
            if (segments.size() < 2) continue; // Can happen after RefineModel()

            splitSegments.insert(splitSegments.end(), segments.begin(), segments.end());
        }

        // We only collect the data relevant to the shape segments created by
        // the boolean operation
        for (RefVec::const_iterator r = splitSegments.begin(); r != splitSegments.end(); r++) {
            TopoDS_Shape rShape = findShape(refineM, *r);
            Shapes[*r].splitSegment = true;
            Base::Console().Error("   Investigating split %s\n", r->toString().c_str());

            if (r->type == ShapeRef::Face) {
                // for split segment faces, find all adjacent faces because they may have caused the split
                TopTools_IndexedMapOfShape E;
                TopExp::MapShapes(rShape, TopAbs_EDGE, E);

                for (int e = 1; e <= E.Extent(); e++) {
                    const std::set<int>& faces = mapEdgeFace.Find(E(e));
                    for (std::set<int>::const_iterator f = faces.begin(); f != faces.end(); f++) {
                        if (r->index != *f) {
                            ShapeRef faceRef(ShapeRef::Face, *f);
                            // Other split faces do not count as adjacent faces
                            if (std::find(splitSegments.begin(), splitSegments.end(), faceRef) != splitSegments.end())
                                continue;
                            Base::Console().Error("      %s is adjacent to split %s\n", faceRef.toString().c_str(), r->toString().c_str());
                            Shapes[*r].touchFaces.insert(faceRef);
                        }
                    }
                }
             } else {
                // for split segment edges, find all faces that touch one of its vertices
                // but are not adjacent to the edge itself
                // First find faces adjacent to the edge (should be exactly two normally...)
                const std::set<int>& edgeFaces = mapEdgeFace.Find(rShape);

                // Now find faces touching the vertices
                TopTools_IndexedMapOfShape V;
                TopExp::MapShapes(findShape(refineM, *r), TopAbs_VERTEX, V);
                for (int v = 1; v <= V.Extent(); v++) {
                    std::set<int> vertexFaces = mapVertexFace.Find(V(v));
                    for (std::set<int>::const_iterator f = edgeFaces.begin(); f != edgeFaces.end(); f++) {
                        vertexFaces.erase(*f);
                    }
                    for (std::set<int>::const_iterator f = vertexFaces.begin(); f != vertexFaces.end(); f++) {
                        ShapeRef faceRef(ShapeRef::Face, *f);
                        // Other split faces do not count as adjacent faces
                        if (std::find(splitSegments.begin(), splitSegments.end(), faceRef) != splitSegments.end())
                            continue;
                        Base::Console().Error("      %s touches split %s\n", faceRef.toString().c_str(), r->toString().c_str());
                        Shapes[*r].touchFaces.insert(faceRef);
                    }
                }
            }
        }
    }

    // Find all section edges
    Base::Console().Error("Collecting section edges\n");
    TopTools_ListIteratorOfListOfShape it;
    RefVec sectionEdges;
    for (it.Initialize(mkBool.SectionEdges()); it.More(); it.Next()) {
        ShapeRef edgeRef(findRef(refineM, it.Value()));
        if (edgeRef.isEmpty())
            // SectionEdges() also returns the parts of edges that have been cut away!!
            // And RefineModel can have removed some section edges already
            continue;

        // SectionEdges() also returns edges that have been handled in the general RefMap
        // (when the section edge coincides with an edge of the sketch on the sketchface)
        // We can't completely skip this edge, though, because of its vertices
        for (int i = 0; i < shapeNum; i++) {
            if (isInMap(result[i], edgeRef)) {
                Shapes[edgeRef].original = true;
                Base::Console().Error("   %s belongs to original %u\n", edgeRef.toString().c_str(), i+1);
                break;
            }
        }

        Shapes[edgeRef].section = true;
        sectionEdges.push_back(edgeRef);
    }

    // Collect topological data for the section edges
    for (RefVec::const_iterator r = sectionEdges.begin(); r != sectionEdges.end(); r++) {
        Base::Console().Error("   Investigating section %s\n", r->toString().c_str());

        // Find the adjacent edges of the section edge's vertices, excluding other section edges
        TopTools_IndexedMapOfShape V;
        TopExp::MapShapes(findShape(refineM, *r), TopAbs_VERTEX, V);

        if (Shapes[*r].original && (V.Extent() == 1)) {
            // We don't need to look at this closed edge nor at its vertex
            Shapes.erase(*r);
            continue;
        }

        // Get adjacent faces of the edge
        TopoDS_Shape rShape = findShape(refineM, *r);
        const std::set<int>& edgeAdjFaces = mapEdgeFace.Find(rShape);
        // // Get adjacent surface data for geometric comparison
        for (std::set<int>::const_iterator f = edgeAdjFaces.begin(); f != edgeAdjFaces.end(); f++) {
            ShapeRef faceRef(ShapeRef::Face, *f);
            BRepAdaptor_Surface sf(TopoDS::Face(findShape(refineM, faceRef)));
            Shapes[*r].surfaces.push_back(sf);
        }

        for (int v = 1; v <= V.Extent(); v++) {
            ShapeRef vertexRef = findRef(refineM, V(v));
            Shapes[*r].vertices.push_back(vertexRef);
            Base::Console().Error("      %s is a section vertex of %s\n", vertexRef.toString().c_str(), r->toString().c_str());

            // Find the face that touches the vertex but is not adjacent to the edge itself
            // Note: We cannot use the touchFace of an already processed vertex because it is different
            //       depending on which section edge the vertex belongs to!
            std::set<int> vtxAdjFaces = mapVertexFace.Find(V(v));
            for (std::set<int>::const_iterator f = edgeAdjFaces.begin(); f != edgeAdjFaces.end(); f++) {
                vtxAdjFaces.erase(*f);
            }
            if (vtxAdjFaces.size() == 1) {
                ShapeRef faceRef(ShapeRef::Face, *vtxAdjFaces.begin());
                Shapes[vertexRef].touchFaces.insert(faceRef);
                Shapes[*r].touchFaces.insert(faceRef);
                Base::Console().Error("         %s touches section %s\n", faceRef.toString().c_str(), r->toString().c_str());
            } else {
                // This happens if the vertex is a singular vertex touching a seam edge
                //Base::Console().Error("Warning: Found %u faces touching the vertex. Check TNaming code\n", vertexAdjFaces.size());
            }

            if (Shapes[vertexRef].section == true) {
                Base::Console().Error("         This vertex has already been processed\n");
                continue; // We already handled this vertex in an adjacent section edge
            }
            if (!Shapes[vertexRef].original)
                Shapes[vertexRef].section = true;

            Shapes[vertexRef].surfaces = Shapes[*r].surfaces;

            const std::set<int>& edges = mapVertexEdge.Find(V(v));
            ShapeRef adjSectionEdge;

            for (std::set<int>::const_iterator e = edges.begin(); e != edges.end(); e++) {
                if (r->index != *e) {
                    ShapeRef edgeRef(ShapeRef::Edge, *e);

                    if (Shapes[edgeRef].section) {
                        Base::Console().Error("         Section %s is adjacent to section %s\n", edgeRef.toString().c_str(), vertexRef.toString().c_str());
                        adjSectionEdge = edgeRef;
                    } else {
                        Base::Console().Error("         %s is adjacent to section %s\n", edgeRef.toString().c_str(), vertexRef.toString().c_str());
                        Shapes[vertexRef].adjShapes.insert(edgeRef);
                    }
                }
            }

            // Find the adjacent faces of the section edge's vertices. If there are already two
            // adjacent edges to the vertex, then both shapes in the boolean operation are
            // providing an adjacent edge. We don't need to consider adjacent faces in this case
            const std::set<int>& vertexAdjFaces = mapVertexFace.Find(V(v));

            if (Shapes[vertexRef].adjShapes.size() == 1) {
                // We can eliminate the faces that are adjacent to the vertex's adjacent edge,
                // because these MUST belong to the same shape of the boolean operation that is
                // providing the adjacent edge
                TopoDS_Shape adjEdge = findShape(refineM, *Shapes[vertexRef].adjShapes.begin());
                const std::set<int>& adjEdgeAdjFaces = mapEdgeFace.Find(adjEdge);                
                for (std::set<int>::const_iterator f = vertexAdjFaces.begin(); f != vertexAdjFaces.end(); f++) {
                    if (std::find(adjEdgeAdjFaces.begin(), adjEdgeAdjFaces.end(), *f) == adjEdgeAdjFaces.end()) {
                        ShapeRef faceRef(ShapeRef::Face, *f);
                        Base::Console().Error("         %s is adjacent to section %s\n", faceRef.toString().c_str(), vertexRef.toString().c_str());
                        Shapes[vertexRef].adjShapes.insert(faceRef);
                    }
                }
            } else if (Shapes[vertexRef].adjShapes.size() != 2) {
                Base::Console().Error("Warning: Found %u adjacent edges to section edge. Check TNaming code\n", Shapes[vertexRef].adjShapes.size());
            }            

            // The adjacent section edge is sometimes required by getTwinOrientation()
            if (!adjSectionEdge.isEmpty())
                Shapes[vertexRef].adjShapes.insert(adjSectionEdge);
        }

        // This edge has already been handled in the general buildRefMap()
        if (Shapes[*r].original) {
            Base::Console().Error("      %s has been handled in general buildRefMap()\n", r->toString().c_str());
            Shapes.erase(*r);
            continue;
        }

        // Find the adjacent faces of the section edge (should be exactly two normally...)               
        for (std::set<int>::const_iterator f = edgeAdjFaces.begin(); f != edgeAdjFaces.end(); f++) {
            ShapeRef faceRef(ShapeRef::Face, *f);
            Base::Console().Error("      %s is adjacent to section %s\n", faceRef.toString().c_str(), r->toString().c_str());
            Shapes[*r].adjShapes.insert(faceRef);
        }
    }

    // Handle shapes (edges or faces) that were split by the boolean operation
    // The following information is extracted for these shapes and stored in newRefs[]
    // Ancestor:  The shape in the root feature that generated the split shape. E.g. a vertex
    //            in a sketch that generated an edge that was then split in two parts.
    //            This information is already present in splitOrigins[] and the ancestor has already
    //            been registered in the result[] map
    // Modifiers: The faces that were responsible for splitting the shape. Logically if the ancestor is
    //            found in one of the TopoShapes participating in the boolean operation, the Modifiers must
    //            belong to the other TopoShape (which we might call the "split tool").
    // Extra information extracted (might be redundant or unstable) and stored in newTopInfo:
    // Numbering: The split shape receives a number e.g. #2 of 3 segments
    // Symmetry:  When looped shapes (e.g. circles/cylinders) are split, they may have a common
    //            vertex or seam edge. So technically they are not split at all, they are both adjacent to
    //            this vertex/edge. The orientation of the common vertex/edge can be used to distinguish
    //            the two adjacent segments.
    // Note that newRefs[] will only contain Modifiers
    // Note: If we were sure that segment sequence by OCC is consistent, we wouldn't need Modifiers at
    //       all. But we don't trust OCC that far...
    Base::Console().Error("Investigating split shapes\n");
    std::vector<RefMap> newRefs(shapeNum);

    for (int i = 0; i < shapeNum; i++) {
        int j = (i == 0 ? 1 : 0); // TODO: Will !i do the trick?

        for (RefVec::const_iterator o = splitOrigins[i].begin(); o != splitOrigins[i].end(); o++) {
            RefVec& segments = result[i][*o];
            if (segments.size() < 2)
                continue; // Can happen as a result of the ModelRefine()

            Base::Console().Error("Investigating %s in old shape\n", o->toString().c_str());

            // Check if a shape loop was split, then we can use the orientation of the singular vertex/edge
            // as additional topological information for the adjacent segments
            TopoDS_Shape singularShape = getSingularShape(findShape(oldM[i], *o));
            ShapeRef singularRef = findRef(oldM[i], singularShape);
            int splitIndex = 0;

            for (RefVec::iterator seg = segments.begin(); seg != segments.end(); seg++) {
                // Number the split segments. This is usually redundant information but on some
                // multiple splits it might be required.
                // NOTE: This depends on OCC being consistent with the ordering of the split segments...
                newTopInfo[*seg].splitPath.push_back(std::pair<int, int>(splitIndex++, segments.size()));
                Base::Console().Error("   Looking at segment %s (%u of %u)\n", seg->toString().c_str(), splitIndex, segments.size());

                if (!singularRef.isEmpty()) {
                    // Set the symmetry flag on this segment to distinguish it from the other
                    // segment that is adjacent to the singular shape
                    // NOTE: This assumes that OCC is consistent in orienting the edge vertices and seam edge
                    // Note: And on some topologies the singular shape has identical orientation on both
                    // split segments...
                    Base::Console().Error("      Found singular %s in old shape %u\n", singularRef.toString().c_str(), i);
                    TopoDS_Shape segmentShape = findShape(refineM, *seg);
                    TopoDS_Shape newSingularShape = findSameShape(segmentShape, singularShape);
                    if (!newSingularShape.IsNull()) {
                        // If the shape was split into several segments, only two can be adjacent to the
                        // singular shape!
                        newTopInfo[*seg].symmetryFlag = (newSingularShape.Orientation() == TopAbs_REVERSED);
                        Base::Console().Error("      Setting symmetry flag to %s\n", newTopInfo[*seg].symmetryFlag == TopAbs_REVERSED ? "REVERSED" : "FORWARD");
                    } else {
                        Base::Console().Error("      This segment is not adjacent to the singular shape\n");
                    }
                }

                // The adjacent faces that are in the result map of the OTHER shape (result[j], the "split
                // tool") must be those that caused the split
                // Note: And if the adjacent face is in the result map of BOTH shapes then it is a united
                // co-planar face and cannot have split the object at all (see note above on the ModelRefine()
                // algorithm)
                // TODO: Would it be sufficient to register only ancestors here, and not the modifiers, too?
                // Probably not, since e.g. the same ancestor face can split an edge in multiple places
                RefSet cutShapes = Shapes[*seg].touchFaces;

                for (RefSet::const_iterator c = cutShapes.begin(); c != cutShapes.end(); c++) {
                    RefVec parents = findParentsInMap(result[j], *c);
                    for (RefVec::const_iterator o = parents.begin(); o != parents.end(); o++) {
                        // This face is responsible for splitting the object
                        Base::Console().Error("      Parent %s has split the object\n", o->toString().c_str());
                        newRefs[j][*o].push_back(*seg);
                    }
                }
            }
        }
    }

    for (int i = 0; i < shapeNum; i++) {
        // Register in the result[] map all faces stored in newRef as Modifiers of the split shape
        // Note: If we had inserted the newRefs into result[] right away, then there would have beeen
        // tons of duplicate origins because some of the split shapes will appear as modifiers in the
        // other RefMap and be registered as also having split a shape
        // This problem would go away if we register only ancestors findAncestorsInMap() instead of
        // all parents findParentsInMap()
        for (RefMap::const_iterator r = newRefs[i].begin(); r != newRefs[i].end(); r++)
            for (RefVec::const_iterator newRef = r->second.begin(); newRef != r->second.end(); newRef++) {
                ShapeRef ref = *newRef;
                ref.modified = true;
                result[i][r->first].push_back(ref);
            }
    }

    // Handle the new edges (section edges) and new vertices (section vertices) created by the boolean operation
    // The following information is extracted for section edges:
    // Ancestors:  Ancestor shapes in first and second TopoShape of the boolean operation. These
    //             are always one face from each TopoShape (the two faces that are adjacent to the
    //             section edge)
    // Modifiers: The two faces that touch the section edge's vertices at either end, but are not
    //            adjacent to the section edge itself. For the special case of a pair of section edge
    //            belong to a loop that was split at a seam edge, there is only one modifier face.
    // Extra information extracted (might be redundant or unstable) and stored in newTopInfo:
    // Symmetry: See documentation for handling of twinShapes
    // Numbering: If disambiguation by symmetry fails, the pair of edges is numbered instead
    //
    // Handle the  created by the boolean operation
    // The following information is extracted for section vertices:
    // Ancestors:  Ancestor shapes in first and second TopoShape of the boolean operation. Every vertex
    //             has three adjacent edges. One adjacent edge is itself a section edge and therefore
    //             does not count as an ancestor. For the remaining two edges there are two possibilities:
    //             a) Both are normal edges, one from each TopoShape
    //             b) One is a section edge. In this case the ancestor face of this section edge in the
    //                corresponding TopoShape is used instead
    // Modifiers: None
    // Extra information extracted (might be redundant or unstable) and stored in newTopInfo:
    // Symmetry: See documentation for handling of twinShapes
    // Numbering: If disambiguation by symmetry fails, the pair of vertices is numbered instead
    //
    Base::Console().Error("Investigating section edges and vertices\n");
    // Map to store ancestors and modifiers of adjacent shapes to check for ambiguities. Especially booleans of
    // cylinders tend to produce pairs of section edges and vertices with identical adjacent shapes
    std::map<ShapeRef, RefSet> ancMap, modMap;

    for (std::map<ShapeRef, shapeData>::iterator s = Shapes.begin(); s != Shapes.end(); s++) {
        if (!(s->second.section &&
             ((s->first.type == ShapeRef::Edge) || (s->first.type == ShapeRef::Vertex))))
            continue;
        if (s->second.splitSegment) {
            // TODO: Will this ever be reached? Otherwise the splitSegment flag might be unnecessary
            Base::Console().Error("SPLITSEGMENT REACHED\n");
            continue;
        }

        ShapeRef sectRef = s->first;
        Base::Console().Error("   Section%s\n", sectRef.toString().c_str());

        // Edge: Register the adjacent shapes as ancestors (usually there should be exactly
        // two adjacent faces and two ancestors)
        // Vertex: Register all adjacent edges that are not section edges as ancestors. If there is only
        //         one such adjacent edge, register the adjacent face as ancestor as well
        RefSet ancestors, modifiers; // must be a set so that ordering does not matter when doing the comparison
        RefSet adjShapes = s->second.adjShapes;

        for (RefSet::iterator s = adjShapes.begin(); s != adjShapes.end(); s++) {
            if (Shapes[*s].section)
                continue;
            Base::Console().Error("      %s is adjacent to %s\n", s->toString().c_str(), sectRef.toString().c_str());

            // Register all ancestors of the adjacent shape as ancestors of the section edge/vertex
            for (int i = 0; i < shapeNum; i++) {
                RefVec ancVec = findAncestorsInMap(result[i], *s);
                if (ancVec.empty())
                    continue; // Wrong result[] map

                for (RefVec::const_iterator a = ancVec.begin(); a != ancVec.end(); a++) {
                    Base::Console().Error("      %s is ancestor of %s in old shape %u\n", a->toString().c_str(), s->toString().c_str(), i);
                    result[i][*a].push_back(sectRef);
                    ancestors.insert(*a);
                }
            }
        }

        // Edge: Register the two faces that touch the edge's vertices as modifiers
        if (sectRef.type == ShapeRef::Edge) {
            RefSet limitShapes = Shapes[sectRef].touchFaces;
            ShapeRef sectRefMod = sectRef;
            sectRefMod.modified = true;

            for (RefSet::iterator s = limitShapes.begin(); s != limitShapes.end(); s++) {
                Base::Console().Error("      %s touches %s\n", s->toString().c_str(), sectRef.toString().c_str());

                // Register all ancestors of the touching faces as modifiers of the section edge
                for (int i = 0; i < shapeNum; i++) {
                    RefVec modVec = findAncestorsInMap(result[i], *s);
                    if (modVec.empty())
                        continue; // Wrong result[] map

                    for (RefVec::const_iterator m = modVec.begin(); m != modVec.end(); m++) {
                        Base::Console().Error("      %s is modifier of %s in old shape %u\n", m->toString().c_str(), s->toString().c_str(), i);
                        result[i][*m].push_back(sectRefMod);
                        modifiers.insert(*m);
                    }
                }
            }
        }

        // Will be set to false if at least one adjacent shape of the edge is not planar
        bool adjPlanar = true;
        std::vector<BRepAdaptor_Surface> surfaces = Shapes[sectRef].surfaces;
        for (std::vector<BRepAdaptor_Surface>::const_iterator sf = surfaces.begin(); sf != surfaces.end(); sf++) {
            if (sf->GetType() != GeomAbs_Plane) {
                adjPlanar = false;
                break;
            }
        }
        if ((sectRef.type == ShapeRef::Edge) && adjPlanar)
            continue;

        // Check if there is already a section edge/vertex with the identical parents of the adjacent shapes
        // Note: The ancestor/modifier comparison fails if the adjacent faces of the twin section edge
        //       belong to a parent face that was split more than one step back. Therefore we need to
        //       supplement it with a geometric comparison
        // TODO: Would it make any difference in performance or stability if we do only the geometric test?
        ShapeRef twinRef;
        for (std::map<ShapeRef, RefSet>::const_iterator a = ancMap.begin(); a != ancMap.end(); a++) {
            if (a->first.type != sectRef.type)
                continue;

            if ((a->second == ancestors) && (modMap[a->first] == modifiers)) {
                Base::Console().Error("   ! %s is symmetric to %s\n", a->first.toString().c_str(), sectRef.toString().c_str());
                twinRef = a->first;
                // There shouldn't be more than one pair of symmetric shapes...
                ancMap.erase(a->first);
                modMap.erase(a->first);
                break;
            }

            std::vector<BRepAdaptor_Surface> twinSurfaces = Shapes[a->first].surfaces;
            int identCount = 0;
            // TODO: A more elegant way would be to have the surfaces in a set, define a custom comparator
            // functor, and then check for number of elements after inserting all 4 surfaces
            for (std::vector<BRepAdaptor_Surface>::const_iterator sf = surfaces.begin(); sf != surfaces.end(); sf++) {
                for (std::vector<BRepAdaptor_Surface>::const_iterator twinsf = twinSurfaces.begin(); twinsf != twinSurfaces.end(); twinsf++) {
                    if (compareSurfaces(*sf, *twinsf))
                        identCount++;
                    if (identCount == 2)
                        break;
                }
                if (identCount == 2)
                    break;
            }
            if (identCount == 2) {
                Base::Console().Error("   ! Adjacent faces of %s and %s are identical\n", a->first.toString().c_str(), sectRef.toString().c_str());
                twinRef = a->first;
                // There shouldn't be more than one pair of symmetric shapes...
                ancMap.erase(a->first);
                modMap.erase(a->first);
                break;
            }
        }

        if (!twinRef.isEmpty()) {
            // Get the orientation of the shape with respect to the adjacent shape
            // We try to find an adjacent shape on which the two symmetric shapes have different orientation
            // Note: This depends on OCC being consistent in orienting the edges on the faces/vertices on the edges
            TopAbs_Orientation o = getTwinOrientation(sectRef, twinRef, refineM, Shapes);

            if (o != TopAbs_INTERNAL) {
                newTopInfo[sectRef].symmetryFlag = (o == TopAbs_REVERSED);
                newTopInfo[twinRef].symmetryFlag = (o == TopAbs_FORWARD);
                Base::Console().Error("      SymmetryFlag of %s: %s\n", sectRef.toString().c_str(), o == TopAbs_REVERSED ? "REVERSED" : "FORWARD");
                Base::Console().Error("      SymmetryFlag of %s: %s\n", twinRef.toString().c_str(), o == TopAbs_FORWARD ? "REVERSED" : "FORWARD");
            } else {
                newTopInfo[sectRef].splitPath.push_back(std::pair<int, int>(0,2));
                newTopInfo[twinRef].splitPath.push_back(std::pair<int, int>(1,2));
                Base::Console().Error("      Segment number of %s: #1/2\n", sectRef.toString().c_str());
                Base::Console().Error("      Segment number of %s: #2/2\n", twinRef.toString().c_str());

            }
        } else {
            ancMap[sectRef] = ancestors;
            modMap[sectRef] = modifiers;
        }
    }

    clearSubShapes(newM);
    clearSubShapes(refineM);
    for (int i = 0; i < shapeNum; i++)
        clearSubShapes(oldM[i]);

    // Note: The difference between Modified() and Modified2() is that the first returns
    // myBuilder->Modified() while the second returns myHistory->Modified() which in
    // practice seems to mean that Modified2() only returns results for arguments that are
    // faces (and those results appear to be identical with Modified() for the same arguments)

    return mkRefine.Shape();
}

// Note: This would sometimes return an empty section on the SAME input!!!
//std::set_intersection(vec1.begin(), vec1.end(), vec2.begin(), vec2.end(),
//                      std::back_inserter(section));

const RefVec intersect(const RefVec& vec1, const RefSet& vec2)
{
    RefVec section;
    for (RefVec::const_iterator r1 = vec1.begin(); r1 != vec1.end(); r1++) {
        for (RefSet::const_iterator r2 = vec2.begin(); r2 != vec2.end(); r2++) {
            if (*r1 == *r2) {
                section.push_back(*r1);
                   break; // There can't be any duplicates, and in any case we don't want them
            }
        }
    }

    return section;
}

// These two functions are not included in TopoShape:: because then ShapeRef would have to be declared
// publicly
/**
  * Return the origin of a shape
  * This is a list of "anchors" in sketches or primitives in the form "Featurename:ShapetypeIndex".
  * There should be either one or two ancestors for the shape.
  * There can be any number of modifiers
  */
OriginRec findOriginInHistory(const std::vector<ShapeMap>& history, const ShapeRef& subRef)
{
    Base::Console().Error("   Find origin in history for %s\n", subRef.toString(false).c_str());

    OriginRec result;

    for (std::vector<ShapeMap>::const_iterator h = history.begin(); h != history.end(); h++) {
        RefVec ancestors, modifiers;
        findInMap(h->Map, subRef, ancestors, modifiers);

        for (RefVec::const_iterator a = ancestors.begin(); a != ancestors.end(); a++) {
            Base::Console().Error("      Ancestor: %s:%s\n", h->name.c_str(), a->toString().c_str());
            result.Ancestors.push_back(AncestorRef(h->name, *a));
        }

        for (RefVec::const_iterator m = modifiers.begin(); m != modifiers.end(); m++) {
            Base::Console().Error("      Modifier: %s:%s\n", h->name.c_str(), m->toString().c_str());
            result.Modifiers.push_back(AncestorRef(h->name, *m));
        }
    }

    return result;
}

/**
  * Find the shape(s) for an origin that have the given shape type
  */
RefVec findShapesInHistory(const std::vector<ShapeMap>& history, const OriginRec& origin,
                           const ShapeRef::typeEnum& type)
{
    Base::Console().Error("   Find shapes in history\n");
    RefVec result;

    // Find all shapes generated by the ancestor. If there is more than one ancestor, then
    // only shapes generated by all ancestors are valid
    for (AncestorVec::const_iterator a = origin.Ancestors.begin(); a != origin.Ancestors.end(); a++) {
        for (std::vector<ShapeMap>::const_iterator h = history.begin(); h != history.end(); h++) {
            if (h->name == a->name) {
                Base::Console().Error("      Ancestor: %s in %s\n", a->ref.toString().c_str(), a->name.c_str());
                RefMap::const_iterator m = h->Map.find(a->ref);
                if (m == h->Map.end())
                    // This can happen if the ancestor doesn't point to any geometry any more
                    continue;

                // Eliminate refs with wrong shape type and shapes that were only modified by the ancestor
                RefSet newResult;
                for (RefVec::const_iterator r = m->second.begin(); r != m->second.end(); r++) {
                    if (r->type == type)
                        if (!r->modified)
                            newResult.insert(*r);
                }

                // Eliminate shapes that are not generated by all ancestors
                if (!result.empty()) {
                    RefVec section = intersect(result, newResult);

                    // This is a dilemma: Should we claim that the shape has been deleted? Better
                    // to just ignore this ancestor
                    if (!section.empty())
                        result = section;
                } else {
                    result.insert(result.begin(), newResult.begin(), newResult.end());
                }

                break;
            }
        }
    }

    for (RefVec::const_iterator r = result.begin(); r != result.end(); r++)
        Base::Console().Error("         Found from ancestor(s): %s\n", r->toString().c_str());

    if (result.size() == 1)
        return result;

    // Eliminate shapes based on the modifiers
    for (AncestorVec::const_iterator m = origin.Modifiers.begin(); m != origin.Modifiers.end(); m++) {
        Base::Console().Error("      Modifier: %s in %s\n", m->ref.toString().c_str(), m->name.c_str());

        for (std::vector<ShapeMap>::const_iterator h = history.begin(); h != history.end(); h++) {
            if (h->name == m->name) {
                RefMap::const_iterator r_it = h->Map.find(m->ref);
                if (r_it == h->Map.end())
                    // This can happen if the origin doesn't point to any geometry any more
                    continue;

                RefSet refs;

                // Eliminate refs with wrong shape type, and duplicates (through inserting into a set)
                for (RefVec::const_iterator r = r_it->second.begin(); r != r_it->second.end(); r++) {
                    if (r->type == type)
                        refs.insert(*r);
                }

                if (result.empty() && (m == origin.Modifiers.begin())) {
                    // Can this be reached?
                    result.insert(result.begin(), refs.begin(), refs.end());
                } else {
                    // Keep only the shapes that are modified by all modifiers
                    RefVec section = intersect(result, refs);

                    // This is a dilemma: Should we claim that the shape has been deleted? Better
                    // to just ignore this modifier
                    if (!section.empty())
                        result = section;
                }
            }
        }
    }

    if (!origin.Modifiers.empty())
        for (RefVec::const_iterator r = result.begin(); r != result.end(); r++)
            Base::Console().Error("         Found from modifier(s): %s\n", r->toString(false).c_str());

    if (result.size() == 1)
        return result;

    // Eliminate results that have other origins not specified here
    RefVec result2;
    for (RefVec::iterator r = result.begin(); r != result.end(); r++) {
        if (origin == findOriginInHistory(history, *r)) {
            Base::Console().Error("      Found matching origin: %s\n", r->toString(false).c_str());
            result2.push_back(*r);
        } else {
            Base::Console().Error("      Eliminated %s because origin does not match\n", r->toString(false).c_str());
        }
    }

    return result2;
}

#ifdef FC_DEBUG
const std::string TopoShape::printRef(const ShapeRef& r, const bool _short) const {
    std::stringstream strm;
    strm << r.toString(_short);
    TopInfoMap tim(TopInfo);
    
    strm << (tim[r].symmetryFlag ? "R" : "");
    if (tim[r].splitPath.size() > 0) {
        for (std::vector<std::pair<int, int> >::const_iterator i = tim[r].splitPath.begin(); i != tim[r].splitPath.end(); i++)
            strm << "#" << (i->first + 1) << "/" << i->second;
    }
    return strm.str();
}

void TopoShape::printHistory() const
{
    std::map<ShapeRef, std::vector<std::string> > inverse;

    Base::Console().Error("====== SUMMARY ======\n");
    for (std::vector<ShapeMap>::const_iterator h = History.begin(); h != History.end(); h++) {
        Base::Console().Error("=== SHAPE HISTORY FROM '%s' TO TOPOSHAPE ===\n", h->name.c_str());
        Base::Console().Error("Read as: Ancestor shape generates shape\n");
        Base::Console().Error("R: Symmetry flag set\n");
        Base::Console().Error("#i/j: Ancestor shape was split into j segments and this is segment #i\n");
        Base::Console().Error("(m): Ancestor shape only modifies this shape indirectly (e.g. if the ancestor\n");
        Base::Console().Error("     shape is a face which splits the referenced shape into segments)\n");

        for (RefMap::const_iterator r = h->Map.begin(); r != h->Map.end(); r++) {
            Base::Console().Error("%s\t ==> ", printRef(r->first).c_str());
            for (RefVec::const_iterator s = r->second.begin(); s != r->second.end(); ) {
                Base::Console().Error("%s", printRef(*s, false).c_str());
                s++;
                if (s != r->second.end()) Base::Console().Error(", ");
            }
            Base::Console().Error("\n");
        }

        // Add entries to inverse map of result shape to origin shape
        for (RefMap::const_iterator r = h->Map.begin(); r != h->Map.end(); r++) {
            for (RefVec::const_iterator s = r->second.begin(); s != r->second.end(); s++) {
                std::string refStr = h->name + ":" + printRef(r->first);
                if (s->modified)
                    refStr += "(m)";
                inverse[*s].push_back(refStr);
            }
        }
    }

    Base::Console().Error("====== INVERSE SHAPE HISTORY FOR RESULT TOPOSHAPE ======\n");
    Base::Console().Error("Read as: New shape depends on ancestor shape(s)\n");
    for (std::map<ShapeRef, std::vector<std::string> >::const_iterator r = inverse.begin(); r != inverse.end(); r++) {
        Base::Console().Error("%s\t <== ", printRef(r->first).c_str());
        for (std::vector<std::string>::const_iterator s = r->second.begin(); s != r->second.end(); ) {
            Base::Console().Error("%s", s->c_str());
            s++;
            if (s != r->second.end()) Base::Console().Error(", ");
        }
        Base::Console().Error("\n");
    }

    // Check for ambiguous references (means there is a flaw in the naming algorithm!)
    std::map<std::string, std::vector<RefVec> > duplicates;
    for (std::map<ShapeRef, std::vector<std::string> >::const_iterator r = inverse.begin(); r != inverse.end(); r++) {
        std::string refStr;

        for (std::vector<std::string>::const_iterator s = r->second.begin(); s != r->second.end(); ) {
            refStr += *s;
            s++;
            if (s != r->second.end()) refStr += (", ");
        }

        TopAbs_ShapeEnum type = this->getSubShape(r->first.toString().c_str()).ShapeType();
        if (duplicates.find(refStr) != duplicates.end()) {
            duplicates[refStr][shapeTypeIndex[type]].push_back(r->first);
        } else {
            for (int i = 0; i < numShapeTypes; i++)
                duplicates[refStr].push_back(RefVec()); // initialize the vector
            TopAbs_ShapeEnum type = this->getSubShape(r->first.toString().c_str()).ShapeType();
            duplicates[refStr][shapeTypeIndex[type]].push_back(r->first);
        }
    }

    TopInfoMap tim(TopInfo);
    std::map<std::string, RefSet> dups, symmetries, segments;
    for (std::map<std::string, std::vector<RefVec> >::const_iterator d = duplicates.begin(); d != duplicates.end(); d++) {
        std::string refStr = d->first;

        for (int i = 0; i < numShapeTypes; i++) {
            for (RefVec::const_iterator r1 = d->second[i].begin(); r1 != d->second[i].end(); r1++) {
                for (RefVec::const_iterator r2 = r1 + 1; r2 != d->second[i].end(); r2++) {
                    if (tim[*r1].symmetryFlag == tim[*r2].symmetryFlag) {
                        if (tim[*r1].splitPath == tim[*r2].splitPath) {
                            dups[refStr].insert(*r1);
                            dups[refStr].insert(*r2);
                        } else {
                            segments[refStr].insert(*r1);
                            segments[refStr].insert(*r2);
                        }
                    } else {
                        // Symmetry is probably more stable than the splitPath
                        //if (d->splitPath == r->first.splitPath) {
                            symmetries[refStr].insert(*r1);
                            symmetries[refStr].insert(*r2);
                        /*} else {
                            segments[refStr].push_back(r->first);
                            segments[refStr].push_back(*d);
                        }*/
                    }
                }
            }
        }
    }

    if (!symmetries.empty())
        Base::Console().Error("====== SYMMETRIES (indistinguishable except for symmetry flag) ======\n");
    for (std::map<std::string, RefSet>::const_iterator i = symmetries.begin(); i != symmetries.end(); i++) {
        for (RefSet::const_iterator r = i->second.begin(); r != i->second.end(); r++)
            Base::Console().Error("%s, ", printRef(*r).c_str());
        Base::Console().Error("\n");
    }
    if (!segments.empty())
        Base::Console().Error("====== SPLIT SEGMENTS (indistinguishable except for split path) ======\n");
    for (std::map<std::string, RefSet>::const_iterator i = segments.begin(); i != segments.end(); i++) {
        for (RefSet::const_iterator r = i->second.begin(); r != i->second.end(); r++)
            Base::Console().Error("%s, ", printRef(*r).c_str());
        Base::Console().Error("\n");
    }
    if (!dups.empty())
        Base::Console().Error("====== AMBIGUITIES (CHECK NAMING ALGORITHM!) ======\n");
    for (std::map<std::string, RefSet>::const_iterator i = dups.begin(); i != dups.end(); i++) {
        for (RefSet::const_iterator r = i->second.begin(); r != i->second.end(); r++)
            Base::Console().Error("%s, ", printRef(*r).c_str());
        Base::Console().Error("\n");
    }

    // Check that all shapes are handled
    std::vector<TopTools_IndexedMapOfShape*> newM = extractSubShapes(_Shape);
    unsigned totalNewShapes = 0;
    for (int i = 0; i < numShapeTypes; i++)
        totalNewShapes += newM[i]->Extent();
    if (inverse.size() != totalNewShapes) {
        Base::Console().Error("=== ERROR: THERE ARE UNHANDLED SHAPES ===\n");
        Base::Console().Error("Total new shapes: %u, total ShapeRefs: %u\n", totalNewShapes, inverse.size());
    }

    Base::Console().Error("====== FINISH SUMMARY ======\n");
    clearSubShapes(newM);

}
#endif
