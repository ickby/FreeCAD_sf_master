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


#include "PreCompiled.h"

#ifndef _PreComp_

# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Section.hxx>
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <BRepOffsetAPI_DraftAngle.hxx>
# include <Precision.hxx>
# include <gp_Pln.hxx>
# include <Geom_Plane.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <TopoDS_Wire.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <ShapeAnalysis.hxx>
# include <ShapeAnalysis_Surface.hxx>
# include <IntTools_FClass2d.hxx>
# include <ShapeFix_Wire.hxx>
# include <ShapeFix_Shape.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <BRepPrimAPI_MakeRevol.hxx>
# include <BRepFeat_MakePrism.hxx>
#endif

#include <algorithm>

#include "Part2DObject.h"
#include "TopoShape.h"
#include "Geometry.h"
#include "modelRefine.h"
#include <Base/Exception.h>
#include <Base/Console.h>

using namespace Part;

#ifdef FC_DEBUG
#include <Base/Console.h>
#endif

// All these are defined in TopoShapeNaming.cpp
extern RefMap buildRefMapFromSketch(const TopoDS_Shape shape, const std::vector<Geometry*>& geometry);
extern RefMap buildRefMap(const TopoDS_Shape& newShape, const TopoDS_Shape& oldShape);
extern RefMap joinMap(const RefMap& oldMap, const RefMap& newMap);
extern RefMap buildRefMap(BRepPrimAPI_MakePrism &mkPrism, const TopoDS_Shape& oldShape);
extern RefMap buildRefMap(BRepPrimAPI_MakeRevol &mkRevol, const TopoDS_Shape& oldShape);
extern RefMap buildRefMap(BRepFeat_MakePrism &mkPrism, const TopoDS_Shape& oldShape);
extern RefMap buildRefMap(BRepBuilderAPI_MakeShape &mkShape, const TopoDS_Shape& oldShape);
extern TopoDS_Shape buildRefMap(BRepAlgoAPI_BooleanOperation &mkBool,
                                const std::vector<TopoDS_Shape> oldShape,
                                std::vector<RefMap>& result,
                                const std::vector<TopInfoMap> oldTopInfos,
                                TopInfoMap& newTopInfo);

// Check that the shape contains at least or exactly one shape of type "entity" and nothing else
void checkHasEntity(const TopoDS_Shape& shape, const TopAbs_ShapeEnum& entity, const bool exactlyOne) {
    TopExp_Explorer ex(shape, entity);
    if (!ex.More())
        throw Base::Exception("TopoShape: Shape is not of the required type");
    ex.Next();
    if (exactlyOne && ex.More())
        throw Base::Exception("TopoShape: Shape contains more than one entity of the required type");
    ex.Init(shape, TopAbs_SHAPE, entity);
    if (ex.More())
        throw Base::Exception("TopoShape: Shape contains additional entities of a different type");
}
/*
// (Re-)create the topological naming information which links Sketch objects with vertices and edges of the TopoDS_Wire
void TopoShape::renewFromSketch(const Part::Part2DObject* sketch, const std::vector<Geometry*>& geometry) {
    Base::Console().Error("=== MAKING SKETCH ===\n");

    // Check that the _Shape contains at least one wire and nothing else
    checkHasEntity(_Shape, TopAbs_WIRE, false);

    // Clear the old history (if any)
    History.clear();

    ShapeMap hist;
    hist.name = sketch->getNameInDocument();
    hist.Map = buildRefMapFromSketch(_Shape, geometry);

    History.push_back(hist);

#ifdef FC_DEBUG
    printHistory();
#endif
    // Note: If necessary, old topological information can be copied from sketch.Shape.getValue()
}*/

// Validate a face and try to fix problems if there are any
TopoDS_Face validateFace(const TopoDS_Face& face) {
    BRepCheck_Analyzer aChecker(face);
    if (!aChecker.IsValid()) {
        TopoDS_Wire outerwire = ShapeAnalysis::OuterWire(face);
        TopTools_IndexedMapOfShape myMap;
        myMap.Add(outerwire);

        TopExp_Explorer xp(face,TopAbs_WIRE);
        ShapeFix_Wire fix;
        fix.SetFace(face);
        fix.Load(outerwire);
        fix.Perform();
        BRepBuilderAPI_MakeFace mkFace(fix.WireAPIMake());
        while (xp.More()) {
            if (!myMap.Contains(xp.Current())) {
                fix.Load(TopoDS::Wire(xp.Current()));
                fix.Perform();
                mkFace.Add(fix.WireAPIMake());
            }
            xp.Next();
        }

        aChecker.Init(mkFace.Face());
        if (!aChecker.IsValid()) {
            ShapeFix_Shape fix(mkFace.Face());
            fix.SetPrecision(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.SetMaxTolerance(Precision::Confusion());
            fix.Perform();
            fix.FixWireTool()->Perform();
            fix.FixFaceTool()->Perform();
            TopoDS_Face fixedFace = TopoDS::Face(fix.Shape());
            aChecker.Init(fixedFace);
            if (!aChecker.IsValid())
                Standard_Failure::Raise("Failed to validate broken face");
            return fixedFace;
        }
        return mkFace.Face();
    }

    return face;
}

// Check whether a wire is inside another wire
bool isInside(const TopoDS_Wire& wire1, const TopoDS_Wire& wire2) {
    Bnd_Box box1;
    BRepBndLib::Add(wire1, box1);
    box1.SetGap(0.0);

    Bnd_Box box2;
    BRepBndLib::Add(wire2, box2);
    box2.SetGap(0.0);

    if (box1.IsOut(box2))
        return false;

    double prec = Precision::Confusion();

    BRepBuilderAPI_MakeFace mkFace(wire1);
    if (!mkFace.IsDone())
        Standard_Failure::Raise("Failed to create a face from wire in sketch");
    TopoDS_Face face = validateFace(mkFace.Face());
    BRepAdaptor_Surface adapt(face);
    IntTools_FClass2d class2d(face, prec);
    Handle_Geom_Surface surf = new Geom_Plane(adapt.Plane());
    ShapeAnalysis_Surface as(surf);

    TopExp_Explorer xp(wire2,TopAbs_VERTEX);
    while (xp.More())  {
        TopoDS_Vertex v = TopoDS::Vertex(xp.Current());
        gp_Pnt p = BRep_Tool::Pnt(v);
        gp_Pnt2d uv = as.ValueOfUV(p, prec);
        if (class2d.Perform(uv) == TopAbs_IN)
            return true;
        // TODO: We can make a check to see if all points are inside or all outside
        // because otherwise we have some intersections which is not allowed
        else
            return false;
        xp.Next();
    }

    return false;
}
/*
// sort bounding boxes according to diagonal length
class TopoShape::Wire_Compare : public std::binary_function<const TopoDS_Wire&, const TopoDS_Wire&, bool> {
public:
    bool operator() (const TopoDS_Wire& w1, const TopoDS_Wire& w2)
    {
        Bnd_Box box1, box2;
        if (!w1.IsNull()) {
            BRepBndLib::Add(w1, box1);
            box1.SetGap(0.0);
        }

        if (!w2.IsNull()) {
            BRepBndLib::Add(w2, box2);
            box2.SetGap(0.0);
        }

        return box1.SquareExtent() < box2.SquareExtent();
    }
};
*/
// Make a face from a list of wires
TopoDS_Face createFace(std::list<TopoDS_Wire>& wires) {
    try {
        BRepBuilderAPI_MakeFace mkFace(wires.front());
        const TopoDS_Face& face = mkFace.Face();
        if (face.IsNull())
            throw Base::Exception("TopoShape: makeFace: Failed to create face from wire");
        gp_Dir axis(0,0,1);
        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() == GeomAbs_Plane)
            axis = adapt.Plane().Axis().Direction();

        wires.pop_front();
        for (std::list<TopoDS_Wire>::iterator it = wires.begin(); it != wires.end(); ++it) {
            BRepBuilderAPI_MakeFace mkInnerFace(*it);
            const TopoDS_Face& inner_face = mkInnerFace.Face();
            if (inner_face.IsNull())
                throw Base::Exception("TopoShape: makeFace: Failed to create inner face from wire");
            gp_Dir inner_axis(0,0,1);
            BRepAdaptor_Surface adapt(inner_face);
            if (adapt.GetType() == GeomAbs_Plane)
                inner_axis = adapt.Plane().Axis().Direction();
            // It seems that orientation is always 'Forward' and we only have to reverse
            // if the underlying plane have opposite normals.
            if (axis.Dot(inner_axis) < 0)
                it->Reverse();
            mkFace.Add(*it);
        }

        return validateFace(mkFace.Face());
    } catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        if (std::string(e->GetMessageString()) == "TopoDS::Face")
            throw Base::Exception("Could not create face from sketch.\n"
                "Intersecting sketch entities or multiple disjunct faces in a sketch are not allowed.");
        else
            throw Base::Exception(e->GetMessageString());
    }
}
/*
void TopoShape::makeFace() {
    Base::Console().Error("=== MAKEFACE ===\n");

    if (History.size() != 1)
        throw Base::Exception("TopoShape: makeFace: Only possible for a single history");

    // Get the wires
    std::vector<TopoDS_Wire> w;
    // this is a workaround for an obscure OCC bug which leads to empty tessellations
    // for some faces. Making an explicit copy of the linked shape seems to fix it.
    // The error almost happens when re-computing the shape but sometimes also for the
    // first time
    BRepBuilderAPI_Copy copy(_Shape);
    TopoDS_Shape newShape = copy.Shape();
    if (newShape.IsNull())
        throw Base::Exception("TopoShape: makeFace: Shape object is empty");

    TopExp_Explorer ex;
    for (ex.Init(newShape, TopAbs_WIRE); ex.More(); ex.Next())
        w.push_back(TopoDS::Wire(ex.Current()));

    if (w.empty()) // there can be several wires
        throw Base::Exception("TopoShape: makeFace: Shape object is not a wire");

    // Sort the wires so that disjunct wires are separated and outer and inner wires are ordered
    std::list< std::list<TopoDS_Wire> > sorted_wires;

    ///FIXME: Need a safe method to sort wire that the outermost one comes last
    // Currently it's done with the diagonal lengths of the bounding boxes
#if 1
    std::vector<TopoDS_Wire> wires = w;
    std::sort(wires.begin(), wires.end(), TopoShape::Wire_Compare());
    std::list<TopoDS_Wire> wire_list;
    wire_list.insert(wire_list.begin(), wires.rbegin(), wires.rend());
#else
    //bug #0001133: try alternative sort algorithm
    std::list<TopoDS_Wire> unsorted_wire_list;
    unsorted_wire_list.insert(unsorted_wire_list.begin(), w.begin(), w.end());
    std::list<TopoDS_Wire> wire_list;
    Wire_Compare wc;
    while (!unsorted_wire_list.empty()) {
        std::list<TopoDS_Wire>::iterator w_ref = unsorted_wire_list.begin();
        std::list<TopoDS_Wire>::iterator w_it = unsorted_wire_list.begin();
        for (++w_it; w_it != unsorted_wire_list.end(); ++w_it) {
            if (wc(*w_ref, *w_it))
                w_ref = w_it;
        }
        wire_list.push_back(*w_ref);
        unsorted_wire_list.erase(w_ref);
    }
#endif

    // separate the wires into several independent faces
        while (!wire_list.empty()) {
            std::list<TopoDS_Wire> sep_list;
            TopoDS_Wire wire = wire_list.front();
            wire_list.pop_front();
            sep_list.push_back(wire);

            std::list<TopoDS_Wire>::iterator it = wire_list.begin();
            while (it != wire_list.end()) {
                if (isInside(wire, *it)) {
                    sep_list.push_back(*it);
                    it = wire_list.erase(it);
                }
                else {
                    ++it;
                }
            }

            sorted_wires.push_back(sep_list);
        }

    // Make face(s) from the wires
    if (sorted_wires.size() == 1) {
        std::list<TopoDS_Wire>& wires = sorted_wires.front();
        newShape = createFace(wires);
    } else if (sorted_wires.size() > 1) {
        TopoDS_Compound comp;
        BRep_Builder builder;
        builder.MakeCompound(comp);
        for (std::list< std::list<TopoDS_Wire> >::iterator it = sorted_wires.begin(); it != sorted_wires.end(); ++it) {
            TopoDS_Shape aFace = createFace(*it);
            if (!aFace.IsNull())
                builder.Add(comp, aFace);
        }
        newShape = comp;
    } else {
        throw Base::Exception("TopoShape: makeFace: Could not build face from wires");
    }

    // Note: We assume that TopInfo is empty and stays empty
    RefMap newMap = buildRefMap(newShape, _Shape);
    History.front().Map = joinMap(History.front().Map, newMap);
    _Shape = newShape;

#ifdef FC_DEBUG
    printHistory();
#endif
}

void TopoShape::move(const TopLoc_Location& m)
{
    _Shape.Move(m);
}

void TopoShape::makePrism(const gp_Dir &dir, const Standard_Real L, const Standard_Real L2,
                          const bool midplane, const bool reversed)
{
    Base::Console().Error("=== MAKEPRISM (Length) ===\n");

    if (History.size() != 1)
        throw Base::Exception("TopoShape: makePrism: Only possible for a single history");

    // Check that _Shape is a single face
    checkHasEntity(_Shape, TopAbs_FACE, true);

    double Ltotal = L;
    double Loffset = 0.0;
    if (Precision::IsInfinite(L)) {
        // This is modelled as a very long, but finite prism to avoid problems with pockets
        // Note: 1E6 created problems once...
        Ltotal = 1E4;
    }

    if (L2 > Precision::Confusion()) {
        // midplane makes no sense here
        Loffset = -L2;
        Ltotal += L2;
    } else if (midplane)
        Loffset = -Ltotal/2;

    if ((L2 > Precision::Confusion()) || midplane) {
        gp_Trsf mov;
        mov.SetTranslation(Loffset * gp_Vec(dir));
        TopLoc_Location loc(mov);
        move(loc);
    } else if (reversed)
        Ltotal *= -1.0;

    // Its better not to use BRepFeat_MakePrism here even if we have a support because the
    // resulting shape creates problems with Pocket
    BRepPrimAPI_MakePrism PrismMaker(_Shape, Ltotal*gp_Vec(dir), 0,1); // finite prism
    if (!PrismMaker.IsDone())
        throw Base::Exception("TopoShape: MakePrism: Could not extrude the sketch!");   
    if (PrismMaker.Shape().IsNull())
        throw Base::Exception("TopoShape: MakePrism: Resulting shape is empty");

    // Note: We assume that TopInfo is empty and stays empty
    RefMap newMap = buildRefMap(PrismMaker, _Shape);
    History.front().Map = joinMap(History.front().Map, newMap);
    _Shape = PrismMaker.Shape();
#ifdef FC_DEBUG
    printHistory();
#endif
}

void TopoShape::makePrism(const TopoShape &base, const TopoDS_Face& supportface,
                          const gp_Dir& direction, const TopoDS_Shape& upToFace, const bool fuse)
{
    Base::Console().Error("=== MAKEPRISM (UpToFace) ===\n");

    if (History.size() != 1)
        throw Base::Exception("TopoShape: makePrism: Only possible for a single history");

    // Check that _Shape is a single face
    checkHasEntity(_Shape, TopAbs_FACE, true);

    // Check supportface for limits, otherwise Perform() throws an exception
    TopoDS_Face sf = supportface;
    TopExp_Explorer ex(supportface,TopAbs_WIRE);
    if (!ex.More())
        sf = TopoDS_Face();

    // This always requires a support object, and we need to use BRepFeat_MakePrism
    // Problem: For Pocket/UpToFirst (or an equivalent Pocket/UpToFace) the resulting shape is invalid
    // because the feature does not add any material. This only happens with the "2" option, though
    //
    // NOTE: It might be possible to pass a shell or a compound containing multiple faces
    // as the Until parameter of Perform()
    BRepFeat_MakePrism PrismMaker;
    PrismMaker.Init(base._Shape, _Shape, sf, direction, (fuse ? 0 : 2), 1);
    PrismMaker.Perform(upToFace);

    if (!PrismMaker.IsDone())
        throw Base::Exception("TopoShape: Up to face: Could not extrude the sketch!");
    if (PrismMaker.Shape().IsNull())
        throw Base::Exception("TopoShape: Up to face: Null shape when extruding the sketch!");
    ex.Init(PrismMaker.Shape(), TopAbs_SOLID);
    if (!ex.More())
        throw Base::Exception("TopoShape: Up to face: No solid was generated, check extrusion direction");

    // Note: The base TopoShape is not touched by the PrismMaker operation
    // Note: We assume that TopInfo is empty and stays empty
    // Map from sketch shape to feature
    RefMap sketchMap = buildRefMap(PrismMaker, _Shape);
    History.front().Map = joinMap(History.front().Map, sketchMap);
    RefMap baseMap = buildRefMap(PrismMaker, base._Shape);
    for (std::vector<ShapeMap>::const_iterator h = base.History.begin(); h != base.History.end(); h++) {
        ShapeMap resultHistory = *h;
        resultHistory.Map = joinMap(resultHistory.Map, baseMap);
        History.push_back(resultHistory);
    }

    // Note: The support TopoShape is not touched by the PrismMaker operation
    _Shape = PrismMaker.Shape();

#ifdef FC_DEBUG
    printHistory();
#endif
}

const bool checkLineCrossesFace(const gp_Lin &line, const TopoDS_Face &face)
{
#if 1
    BRepBuilderAPI_MakeEdge mkEdge(line);
    TopoDS_Wire wire = ShapeAnalysis::OuterWire(face);
    BRepExtrema_DistShapeShape distss(wire, mkEdge.Shape(), Precision::Confusion());
    if (distss.IsDone()) {
        if (distss.Value() > Precision::Confusion())
            return false;
        // build up map vertex->edge
        TopTools_IndexedDataMapOfShapeListOfShape vertex2Edge;
        TopExp::MapShapesAndAncestors(wire, TopAbs_VERTEX, TopAbs_EDGE, vertex2Edge);

        for (Standard_Integer i=1; i<= distss.NbSolution(); i++) {
            if (distss.PointOnShape1(i).Distance(distss.PointOnShape2(i)) > Precision::Confusion())
                continue;
            BRepExtrema_SupportType type = distss.SupportTypeShape1(i);
            if (type == BRepExtrema_IsOnEdge) {
                TopoDS_Edge edge = TopoDS::Edge(distss.SupportOnShape1(i));
                BRepAdaptor_Curve adapt(edge);
                // create a plane (pnt,dir) that goes through the intersection point and is built of
                // the vectors of the sketch normal and the rotation axis
                const gp_Dir& normal = BRepAdaptor_Surface(face).Plane().Axis().Direction();
                gp_Dir dir = line.Direction().Crossed(normal);
                gp_Pnt pnt = distss.PointOnShape1(i);

                Standard_Real t;
                distss.ParOnEdgeS1(i, t);
                gp_Pnt p_eps1 = adapt.Value(std::max<double>(adapt.FirstParameter(), t-10*Precision::Confusion()));
                gp_Pnt p_eps2 = adapt.Value(std::min<double>(adapt.LastParameter(), t+10*Precision::Confusion()));

                // now check if we get a change in the sign of the distances
                Standard_Real dist_p_eps1_pnt = gp_Vec(p_eps1, pnt).Dot(gp_Vec(dir));
                Standard_Real dist_p_eps2_pnt = gp_Vec(p_eps2, pnt).Dot(gp_Vec(dir));
                // distance to the plane must be noticable
                if (fabs(dist_p_eps1_pnt) > 5*Precision::Confusion() &&
                    fabs(dist_p_eps2_pnt) > 5*Precision::Confusion()) {
                    if (dist_p_eps1_pnt * dist_p_eps2_pnt < 0)
                        return true;
                }
            }
            else if (type == BRepExtrema_IsVertex) {
                // for a vertex check the two adjacent edges if there is a change of sign
                TopoDS_Vertex vertex = TopoDS::Vertex(distss.SupportOnShape1(i));
                const TopTools_ListOfShape& edges = vertex2Edge.FindFromKey(vertex);
                if (edges.Extent() == 2) {
                    // create a plane (pnt,dir) that goes through the intersection point and is built of
                    // the vectors of the sketch normal and the rotation axis
                    BRepAdaptor_Surface adapt(face);
                    const gp_Dir& normal = adapt.Plane().Axis().Direction();
                    gp_Dir dir = line.Direction().Crossed(normal);
                    gp_Pnt pnt = distss.PointOnShape1(i);

                    // from the first edge get a point next to the intersection point
                    const TopoDS_Edge& edge1 = TopoDS::Edge(edges.First());
                    BRepAdaptor_Curve adapt1(edge1);
                    Standard_Real dist1 = adapt1.Value(adapt1.FirstParameter()).SquareDistance(pnt);
                    Standard_Real dist2 = adapt1.Value(adapt1.LastParameter()).SquareDistance(pnt);
                    gp_Pnt p_eps1;
                    if (dist1 < dist2)
                        p_eps1 = adapt1.Value(adapt1.FirstParameter() + 2*Precision::Confusion());
                    else
                        p_eps1 = adapt1.Value(adapt1.LastParameter() - 2*Precision::Confusion());

                    // from the second edge get a point next to the intersection point
                    const TopoDS_Edge& edge2 = TopoDS::Edge(edges.Last());
                    BRepAdaptor_Curve adapt2(edge2);
                    Standard_Real dist3 = adapt2.Value(adapt2.FirstParameter()).SquareDistance(pnt);
                    Standard_Real dist4 = adapt2.Value(adapt2.LastParameter()).SquareDistance(pnt);
                    gp_Pnt p_eps2;
                    if (dist3 < dist4)
                        p_eps2 = adapt2.Value(adapt2.FirstParameter() + 2*Precision::Confusion());
                    else
                        p_eps2 = adapt2.Value(adapt2.LastParameter() - 2*Precision::Confusion());

                    // now check if we get a change in the sign of the distances
                    Standard_Real dist_p_eps1_pnt = gp_Vec(p_eps1, pnt).Dot(gp_Vec(dir));
                    Standard_Real dist_p_eps2_pnt = gp_Vec(p_eps2, pnt).Dot(gp_Vec(dir));
                    // distance to the plane must be noticable
                    if (fabs(dist_p_eps1_pnt) > Precision::Confusion() &&
                        fabs(dist_p_eps2_pnt) > Precision::Confusion()) {
                        if (dist_p_eps1_pnt * dist_p_eps2_pnt < 0)
                            return true;
                    }
                }
            }
        }
    }

    return false;
#else
    // This is not as easy as it looks, because a distance of zero might be OK if
    // the axis touches the sketchshape in in a linear edge or a vertex
    // Note: This algorithm does not catch cases where the sketchshape touches the
    // axis in two or more points
    // Note: And it only works on closed outer wires
    TopoDS_Wire outerWire = ShapeAnalysis::OuterWire(face);
    BRepBuilderAPI_MakeEdge mkEdge(line);
    if (!mkEdge.IsDone())
        throw Base::Exception("Revolve: Unexpected OCE failure");
    BRepAdaptor_Curve axis(TopoDS::Edge(mkEdge.Shape()));

    TopExp_Explorer ex;
    int intersections = 0;
    std::vector<gp_Pnt> intersectionpoints;

    // Note: We need to look at evey edge separately to catch coincident lines
    for (ex.Init(outerWire, TopAbs_EDGE); ex.More(); ex.Next()) {
        BRepAdaptor_Curve edge(TopoDS::Edge(ex.Current()));
        Extrema_ExtCC intersector(axis, edge);

        if (intersector.IsDone()) {
            for (int i = 1; i <= intersector.NbExt(); i++) {


#if OCC_VERSION_HEX >= 0x060500
                if (intersector.SquareDistance(i) < Precision::Confusion()) {
#else
                if (intersector.Value(i) < Precision::Confusion()) {
#endif
                    if (intersector.IsParallel()) {
                        // A line that is coincident with the axis produces three intersections
                        // 1 with the line itself and 2 with the adjacent edges
                        intersections -= 2;
                    } else {
                        Extrema_POnCurv p1, p2;
                        intersector.Points(i, p1, p2);
                        intersectionpoints.push_back(p1.Value());
                        intersections++;
                    }
                }
            }
        }
    }

    // Note: We might check this inside the loop but then we have to rely on TopExp_Explorer
    // returning the wire's edges in adjacent order (because of the coincident line checking)
    if (intersections > 1) {
        // Check that we don't touch the sketchface just in two identical vertices
        if ((intersectionpoints.size() == 2) &&
            (intersectionpoints[0].IsEqual(intersectionpoints[1], Precision::Confusion())))
            return false;
        else
            return true;
    }

    return false;
#endif
}
/*
void TopoShape::makeRevolution(const gp_Ax1& axis, const Standard_Real angle, const bool midplane, const bool reversed) {
    //Base::Console().Error("=== MAKEREVOLUTION (Length) ===\n");

    // Check that _Shape is a single face
    checkHasEntity(_Shape, TopAbs_FACE, true);

    // Check distance between face and axis - to avoid failures and crashes
    if (checkLineCrossesFace(gp_Lin(axis.Location(), axis.Direction()), TopoDS::Face(_Shape)))
        throw Base::Exception("TopoShape: makeRevolution: Axis intersects the face");

    // Rotate the face by half the angle to get Revolution symmetric to sketch plane
    if (midplane) {
        gp_Trsf mov;
        mov.SetRotation(axis, angle * (-1.0) / 2.0);
        TopLoc_Location loc(mov);
        move(loc);
    }

    // revolve the face to a solid
    BRepPrimAPI_MakeRevol RevolMaker(_Shape, axis, (reversed && !midplane) ? -1.0 * angle : angle);

    if (!RevolMaker.IsDone())
        throw Base::Exception("TopoShape: makeRevolution: Could not extrude the sketch!");
    if (RevolMaker.Shape().IsNull())
        throw Base::Exception("TopoShape: makeRevolution: Resulting shape is empty");

    // Note: We assume that TopInfo is empty and stays empty
    RefMap newMap = buildRefMap(RevolMaker, _Shape);
    History.front().Map = joinMap(History.front().Map, newMap);

    // Note: The support TopoShape is not touched by the PrismMaker operation
    _Shape = RevolMaker.Shape();

#ifdef FC_DEBUG
    printHistory();
#endif
}
*/
TopoDS_Shape getSolid(const TopoDS_Shape& shape)
{
    if (!shape.IsNull()) {
        TopExp_Explorer xp;
        xp.Init(shape,TopAbs_SOLID);
        if (!xp.More())
            return TopoDS_Shape();
        TopoDS_Shape result = xp.Current();
        xp.Next();
        if (xp.More())
            Base::Console().Warning("Resulting shape contains more than one solid. All solids except the first have been dropped");
        return result;
    }

    return TopoDS_Shape();
}

void TopoShape::tnFuse(const TopoShape& other, const bool thisIsBase ) {
    Base::Console().Error("=== MAKEFUSE ===\n");

    // Ignore request to fuse with an empty TopoShape
    if (other._Shape.IsNull())
        return;

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);
    // Check that other._Shape has solids only
    checkHasEntity(other._Shape, TopAbs_SOLID, false);

    BRepAlgoAPI_Fuse mkFuse((thisIsBase ? _Shape : other._Shape), (thisIsBase ? other._Shape : _Shape));
    if (!mkFuse.IsDone())
        throw Base::Exception("TopoShape: Fusion failed");    
    // Check if the result is a solid (fuse sometimes creates compounds)
    if (getSolid(mkFuse.Shape()).IsNull())
        throw Base::Exception("TopoShape: Fuse: Resulting shape is not a solid");
    // FIXME: Here we need to drop extra solids and account for it in the History!

    // This is important, otherwise we may get weird circular edge segments split in arbitrary places!
    mkFuse.RefineEdges();

    std::vector<RefMap> result(2);
    std::vector<TopoDS_Shape> oldShapes;
    oldShapes.push_back(_Shape);
    oldShapes.push_back(other._Shape);
    std::vector<TopInfoMap> TopInfos;
    TopInfos.push_back(TopInfo);
    TopInfos.push_back(other.TopInfo);
    TopoDS_Shape resultShape = buildRefMap(mkFuse, oldShapes, result, TopInfos, TopInfo);

    for (std::vector<ShapeMap>::iterator h = History.begin(); h != History.end(); h++)
        h->Map = joinMap(h->Map, result[0]);
    for (std::vector<ShapeMap>::const_iterator h = other.History.begin(); h != other.History.end(); h++) {
        ShapeMap resultHistory = *h;
        resultHistory.Map = joinMap(resultHistory.Map, result[1]);
        History.push_back(resultHistory);
    }

    _Shape = resultShape;
#ifdef FC_DEBUG
    printHistory();
#endif
}
/*
void TopoShape::makeCut(const TopoShape& other, const bool thisIsBase) {
    Base::Console().Error("=== MAKECUT ===\n");

    // Ignore request to cut with an empty TopoShape
    if (other._Shape.IsNull())
        return;

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);
    // Check that other._Shape has solids only
    checkHasEntity(other._Shape, TopAbs_SOLID, false);

    const TopoShape* base = thisIsBase ? this : &other;
    const TopoShape* tool = thisIsBase ? &other : this;

    BRepAlgoAPI_Cut mkCut(base->_Shape, tool->_Shape);
    if (!mkCut.IsDone())
        throw Base::Exception("TopoShape: Cut failed");
    // Check if the result is a solid (cut might create a compound if a solid is split into several parts!)
    if (getSolid(mkCut.Shape()).IsNull())
        throw Base::Exception("TopoShape: Cut: Resulting shape is not a solid");
    // FIXME: Here we need to drop extra solids and account for it in the History!

    // This is important for topological naming, otherwise we get weird circular edge segments split in arbitrary places!
    mkCut.RefineEdges();

    std::vector<RefMap> result(2);
    std::vector<TopoDS_Shape> oldShapes;
    oldShapes.push_back(base->_Shape);
    oldShapes.push_back(tool->_Shape);
    std::vector<TopInfoMap> TopInfos;
    TopInfos.push_back(base->TopInfo);
    TopInfos.push_back(tool->TopInfo);
    TopoDS_Shape resultShape = buildRefMap(mkCut, oldShapes, result, TopInfos, TopInfo);

    for (std::vector<ShapeMap>::iterator h = History.begin(); h != History.end(); h++)
        h->Map = joinMap(h->Map, thisIsBase ? result[0] : result[1]);
    for (std::vector<ShapeMap>::const_iterator h = other.History.begin(); h != other.History.end(); h++) {
        ShapeMap resultHistory = *h;
        resultHistory.Map = joinMap(resultHistory.Map, thisIsBase ? result[1] : result[0]);
        History.push_back(resultHistory);
    }

    _Shape = resultShape;
#ifdef FC_DEBUG
    printHistory();
#endif
}

void TopoShape::makeCommon(const TopoShape& other) {
    Base::Console().Error("=== MAKECOMMON ===\n");

    // Ignore request to common with an empty TopoShape
    if (other._Shape.IsNull())
        return;

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);
    // Check that other._Shape has solids only
    checkHasEntity(other._Shape, TopAbs_SOLID, false);

    BRepAlgoAPI_Common mkCommon(_Shape, other._Shape);
    if (!mkCommon.IsDone())
        throw Base::Exception("TopoShape: Common operation failed");
    if (getSolid(mkCommon.Shape()).IsNull())
        throw Base::Exception("TopoShape: Common: Resulting shape is not a solid");
    // FIXME: Here we need to drop extra solids and account for it in the History!

    // This is important, otherwise we may get weird circular edge segments split in arbitrary places!
    mkCommon.RefineEdges();

    std::vector<RefMap> result(2);
    std::vector<TopoDS_Shape> oldShapes;
    oldShapes.push_back(_Shape);
    oldShapes.push_back(other._Shape);
    std::vector<TopInfoMap> TopInfos;
    TopInfos.push_back(TopInfo);
    TopInfos.push_back(other.TopInfo);
    TopoDS_Shape resultShape = buildRefMap(mkCommon, oldShapes, result, TopInfos, TopInfo);

    for (std::vector<ShapeMap>::iterator h = History.begin(); h != History.end(); h++)
        h->Map = joinMap(h->Map, result[0]);
    for (std::vector<ShapeMap>::const_iterator h = other.History.begin(); h != other.History.end(); h++) {
        ShapeMap resultHistory = *h;
        resultHistory.Map = joinMap(resultHistory.Map, result[1]);
        History.push_back(resultHistory);
    }

    _Shape = resultShape;
#ifdef FC_DEBUG
    printHistory();
#endif
}

void TopoShape::makeSection(const TopoShape& other) {
    Base::Console().Error("=== MAKESECTION ===\n");

    // Ignore request to section with an empty TopoShape
    if (other._Shape.IsNull())
        return;

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);
    // Check that other._Shape has solids only
    checkHasEntity(other._Shape, TopAbs_SOLID, false);

    BRepAlgoAPI_Section mkSection(_Shape, other._Shape);
    if (!mkSection.IsDone())
        throw Base::Exception("TopoShape: Section operation failed");
    if (getSolid(mkSection.Shape()).IsNull())
        throw Base::Exception("TopoShape: Section: Resulting shape is not a solid");
    // FIXME: Here we need to drop extra solids and account for it in the History!

    // This is important, otherwise we may get weird circular edge segments split in arbitrary places!
    mkSection.RefineEdges();

    std::vector<RefMap> result(2);
    std::vector<TopoDS_Shape> oldShapes;
    oldShapes.push_back(_Shape);
    oldShapes.push_back(other._Shape);
    std::vector<TopInfoMap> TopInfos;
    TopInfos.push_back(TopInfo);
    TopInfos.push_back(other.TopInfo);
    TopoDS_Shape resultShape = buildRefMap(mkSection, oldShapes, result, TopInfos, TopInfo);

    for (std::vector<ShapeMap>::iterator h = History.begin(); h != History.end(); h++)
        h->Map = joinMap(h->Map, result[0]);
    for (std::vector<ShapeMap>::const_iterator h = other.History.begin(); h != other.History.end(); h++) {
        ShapeMap resultHistory = *h;
        resultHistory.Map = joinMap(resultHistory.Map, result[1]);
        History.push_back(resultHistory);
    }

    _Shape = resultShape;
#ifdef FC_DEBUG
    printHistory();
#endif
}

void TopoShape::makeCompound(const TopoShape &other) {
    //Base::Console().Error("=== MAKECOMPOUND ===\n");

    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);
    if (!_Shape.IsNull())
        builder.Add(comp, _Shape);
    builder.Add(comp, other._Shape);

    _Shape = comp;

    // FIXME: History creation
}

void TopoShape::makeCompound(const std::vector<TopoShape> &others) {
    //Base::Console().Error("=== MAKECOMPOUND (Vector) ===\n");

    TopoDS_Compound comp;
    BRep_Builder builder;
    builder.MakeCompound(comp);
    if (!_Shape.IsNull())
        builder.Add(comp, _Shape);
    for (std::vector<TopoShape>::const_iterator o = others.begin(); o != others.end(); o++)
        builder.Add(comp, o->_Shape);

    _Shape = comp;

    // FIXME: History creation
}

void TopoShape::makeChamfer(const std::vector<std::string>& edges, const Standard_Real size) {
    //Base::Console().Error("=== MAKECHAMFER ===\n");

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);

    BRepFilletAPI_MakeChamfer mkChamfer(_Shape);

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(_Shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(_Shape, TopAbs_EDGE, mapOfEdges);

    for (std::vector<std::string>::const_iterator it=edges.begin(); it != edges.end(); ++it) {
        TopoDS_Edge edge = TopoDS::Edge(getSubShape(it->c_str()));
        const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
        mkChamfer.Add(size, edge, face);
    }

    mkChamfer.Build();
    if (!mkChamfer.IsDone())
        throw Base::Exception("TopoShape: makeChamfer: Failed to create chamfer");

    if (mkChamfer.Shape().IsNull())
        throw Base::Exception("TopoShape: makeChamfer: Resulting shape is null");

    _Shape = mkChamfer.Shape();

    // FIXME: History creation
}

void TopoShape::makeFillet(const std::vector<std::string>& edges, const Standard_Real radius) {
    //Base::Console().Error("=== MAKEFILLET===\n");

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);

    BRepFilletAPI_MakeFillet mkFillet(_Shape);

    for (std::vector<std::string>::const_iterator it=edges.begin(); it != edges.end(); ++it) {
        TopoDS_Edge edge = TopoDS::Edge(getSubShape(it->c_str()));
        mkFillet.Add(radius, edge);
    }

    mkFillet.Build();
    if (!mkFillet.IsDone())
        throw Base::Exception("TopoShape: makeFillet: Failed to create chamfer");

    if (mkFillet.Shape().IsNull())
        throw Base::Exception("TopoShape: makeFillet: Resulting shape is null");

    _Shape = mkFillet.Shape();

    // FIXME: History creation
}

void TopoShape::makeDraft(const std::vector<std::string> &faces, const gp_Dir &pullDirection,
                          const Standard_Real angle, const gp_Pln &neutralPlane) {
    //Base::Console().Error("=== MAKEDRAFT ===\n");

    // Check that _Shape has solids only
    checkHasEntity(_Shape, TopAbs_SOLID, false);

    BRepOffsetAPI_DraftAngle mkDraft;
    // Note:
    // LocOpe_SplitDrafts can split a face with a wire and apply draft to both parts
    //       Not clear though whether the face must have free boundaries
    // LocOpe_DPrism can create a stand-alone draft prism. The sketch can only have a single
    //       wire, though.
    // BRepFeat_MakeDPrism requires a support for the operation but will probably support multiple
    //       wires in the sketch

    bool success;
    std::vector<std::string> theFaces = faces;

    do {
        success = true;
        mkDraft.Init(_Shape);

        for (std::vector<std::string>::iterator it=theFaces.begin(); it != theFaces.end(); ++it) {
            TopoDS_Face face = TopoDS::Face(getSubShape(it->c_str()));
            // TODO: What is the flag for?
            mkDraft.Add(face, pullDirection, angle, neutralPlane);
            if (!mkDraft.AddDone()) {
                // Note: the function ProblematicShape returns the face on which the error occurred
                // Note: mkDraft.Remove() stumbles on a bug in Draft_Modification::Remove() and is
                //       therefore unusable. See https://sourceforge.net/apps/phpbb/free-cad/viewtopic.php?f=10&t=3209&start=10#p25341
                //       The only solution is to discard mkDraft and start over without the current face
                // mkDraft.Remove(face);
                Base::Console().Error("Adding face failed on %s. Omitted\n", it->c_str());
                success = false;
                theFaces.erase(it);
                break;
            }
        }
    }
    while (!success);

    mkDraft.Build();
    if (!mkDraft.IsDone())
        throw Base::Exception("TopoShape: makeDraft: Failed to create draft");

    if (mkDraft.Shape().IsNull())
        throw Base::Exception("TopoShape: makeDraft: Resulting shape is null");

    _Shape =  mkDraft.Shape();

    // FIXME: History creation
}

void TopoShape::makeTransform(const gp_Trsf &tr) {
    // Base::Console().Error("=== MAKETRANSFORM ===\n");

    // Make an explicit copy of the shape because the "true" parameter to BRepBuilderAPI_Transform
    // seems to be pretty broken
    BRepBuilderAPI_Copy copy(_Shape);
    _Shape = copy.Shape();
    if (_Shape.IsNull())
        throw Base::Exception("TopoShape: makeTransform: Linked shape object is empty");

    BRepBuilderAPI_Transform mkTrf(_Shape, tr, false); // No need to copy, now
    if (!mkTrf.IsDone())
        throw Base::Exception("TopoShape: makeTransform: Transformation failed");

    if (mkTrf.Shape().IsNull())
        throw Base::Exception("TopoShape: makeTransform: Resulting shape is null");

    _Shape =  mkTrf.Shape();

    // FIXME: History creation
}

void TopoShape::refine() {
    Base::Console().Error("=== REFINE ===\n");
    BRepBuilderAPI_RefineModel mkRefine(_Shape);

    if (!mkRefine.IsDone())
        throw Base::Exception("TopoShape: Refine: Failed");

    RefMap newMap = buildRefMap(mkRefine, _Shape);
    for (std::vector<ShapeMap>::iterator h = History.begin(); h != History.end(); h++)
        h->Map = joinMap(h->Map, newMap);
    // FIXME: Handle TopInfo!

    _Shape = mkRefine.Shape();

#ifdef FC_DEBUG
    printHistory();
#endif
}
*/