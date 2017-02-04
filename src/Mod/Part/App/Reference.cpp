/***************************************************************************
 *   Copyright (c) 2017 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "Reference.h"
#include "TopoShape.h"
#include <Base/Console.h>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Precision.hxx>
#include <gp_Vec.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS.hxx>

//some helpers to easily create and stringify the needed enums inside the Reference class
#define TUPLES(r, data, elem) \
    (data::elem, std::string(BOOST_PP_STRINGIZE(elem)))
    
#define MAP(Name)\
    boost::bimaps::bimap< Reference::Name, std::string >
    
#define INIT_ENUM(Name, seq) \
    const MAP(Name) Reference::BOOST_PP_CAT(Name, map) = boost::assign::list_of<MAP(Name)::relation> BOOST_PP_SEQ_FOR_EACH(TUPLES, Name, seq );

namespace Part {
    
//****************************************************//
//               Exception code                       //
//****************************************************//
    
ReferenceException::ReferenceException(const Reference& ref) : Exception(), m_ref(ref) {}

ReferenceException::ReferenceException(std::string sMessage) : Exception(sMessage) {}

ReferenceException::ReferenceException(std::string sMessage, const Reference& ref): Exception(sMessage), m_ref(ref) {}

ReferenceException::ReferenceException(): Exception() {}

const char* ReferenceException::what() const throw() {

    std::stringstream stream;
    stream << "Exception in Reference code: " << _sErrMsg;
    if(m_ref.isValid())
        stream << "due to reference " << m_ref.hashAsString() << ": " << m_ref.asString();
    
    return stream.str().c_str();
}

//****************************************************//
//               Helper functions                     //
//****************************************************//

Reference::Shape occToRefShape(TopAbs_ShapeEnum shape) {

    switch(shape) {
        case TopAbs_VERTEX:
            return Reference::Shape::Vertex;
        case TopAbs_EDGE:
            return Reference::Shape::Edge;
        case TopAbs_FACE:
            return Reference::Shape::Face;
    }
    return  Reference::Shape::None;
}

const bool compareShapes(const TopoDS_Shape& s1, const TopoDS_Shape& s2) {

    auto shapeType = s1.ShapeType();
    
    //it could be that easy
    if(shapeType != s2.ShapeType())
        return false;
    
    //this may work, but if it returns false, we are not yet sure that the two shapes are not equal.
    if (s1.IsSame(s2))
        return true;

    if (shapeType == TopAbs_VERTEX) {
        //vertices are trival to compare, hence no optimisation needed
        gp_Pnt p1 = BRep_Tool::Pnt(TopoDS::Vertex(s1));
        gp_Pnt p2 = BRep_Tool::Pnt(TopoDS::Vertex(s2));      
        return p1.IsEqual(p2, Precision::Confusion());
    } 
    else if (shapeType == TopAbs_EDGE) {
        
        auto curve1 = BRepAdaptor_Curve(TopoDS::Edge(s1));
        auto curve2 = BRepAdaptor_Curve(TopoDS::Edge(s2));
        
        //trival checks first
        if(curve1.GetType() != curve2.GetType())
            return false;
        
        if(curve1.IsClosed() != curve2.IsClosed())
            return false;
        
        //if the types are bspline comparison gets hard. However, for this type occ 
        //luckily provides a comparison method. Unfortunately not for bezier curves...
        if(curve1.GetType() == GeomAbs_BSplineCurve) {
            auto bspline1 = Handle(Geom_BSplineCurve)::DownCast(curve1.Curve().Curve());
            return bspline1->IsEqual(Handle(Geom_BSplineCurve)::DownCast(curve2.Curve().Curve()), Precision::Confusion());
        }
                
        //In the parameter range of the two curves the points and derivatives need to match. This is a 
        //universal way of comparing geometries, however, not 100% fail save. Their is a small chance
        //that a high-poles beziers may match point and derivatives at the control uv parameter while 
        //being different to to curve everywhere else. Hence we enlarge the amount of control points 
        //for bezier curves         
        int controlPoints = (curve1.GetType() == GeomAbs_BezierCurve) ? 10 : 4;
        for(int i=0; i< (!curve1.IsClosed() ? controlPoints : (controlPoints-1)); ++i) {
            Standard_Real u1 = curve1.FirstParameter() + (curve1.LastParameter() - curve1.FirstParameter())/controlPoints * i;
            Standard_Real u2 = curve2.FirstParameter() + (curve2.LastParameter() - curve2.FirstParameter())/controlPoints * i;
            gp_Pnt p1, p2;
            gp_Vec dp1, dp2, ddp1, ddp2;
            curve1.D2(u1, p1, dp1, ddp1);
            curve2.D2(u2, p2, dp2, ddp2);
            
            if(!p1.IsEqual(p2, Precision::Confusion()) ||
               !dp1.IsEqual(dp2, Precision::Confusion(), Precision::Angular()) ||
               !ddp1.IsEqual(ddp2, Precision::Confusion(), Precision::Angular())) {
                
                return false;
            }
        }
        return true;
        
    } else if (shapeType == TopAbs_FACE) {
        auto surface1 = BRepAdaptor_Surface(TopoDS::Face(s1));
        auto surface2 = BRepAdaptor_Surface(TopoDS::Face(s2));
        
        //trival checks first
        if(surface1.GetType() != surface2.GetType())
            return false;
        
        if(surface1.IsUClosed() != surface2.IsUClosed())
            return false;
        
        if(surface1.IsVClosed() != surface2.IsVClosed())
            return false;
                      
        //In the parameter range of the two curves the points and derivatives need to match. This is a 
        //universal way of comparing geometries, however, not 100% fail save. Their is a small chance
        //that a high-poles beziers may match point and derivatives at the control uv parameter while 
        //being different to to curve everywhere else. Hence we enlarge the amount of control points 
        //for bezier curves         
        int controlPoints = (surface1.GetType() <= GeomAbs_Torus) ? 4 : 10;
        for(int i=0; i< (!surface1.IsUClosed() ? controlPoints : (controlPoints-1)); ++i) {
            Standard_Real u1 = surface1.FirstUParameter() + (surface1.LastUParameter() - surface1.FirstUParameter())/controlPoints * i;
            Standard_Real u2 = surface2.FirstUParameter() + (surface2.LastUParameter() - surface2.FirstUParameter())/controlPoints * i;
            
            for(int j=0; j< (!surface1.IsVClosed() ? controlPoints : (controlPoints-1)); ++i) {
            
                Standard_Real v1 = surface1.FirstVParameter() + (surface1.LastVParameter() - surface1.FirstVParameter())/controlPoints * i;
                Standard_Real v2 = surface2.FirstVParameter() + (surface2.LastVParameter() - surface2.FirstVParameter())/controlPoints * i;
                
                gp_Pnt p1, p2;
                gp_Vec dp1U, dp1V, dp2U, dp2V, ddp1U, ddp1V, ddp2U, ddp2V, ddp1UV, ddp2UV;
                surface1.D2(u1, v1, p1, dp1U, dp1V, ddp1U, ddp1V, ddp1UV);
                surface1.D2(u2, v2, p2, dp2U, dp2V, ddp2U, ddp2V, ddp2UV);
                
                if(!p1.IsEqual(p2, Precision::Confusion()) ||
                   !dp1U.IsEqual(dp2U, Precision::Confusion(), Precision::Angular()) ||
                   !dp1V.IsEqual(dp2V, Precision::Confusion(), Precision::Angular()) ||
                   !ddp1U.IsEqual(ddp2U, Precision::Confusion(), Precision::Angular()) ||
                   !ddp1V.IsEqual(ddp2V, Precision::Confusion(), Precision::Angular()) ||
                   !ddp1UV.IsEqual(ddp2UV, Precision::Confusion(), Precision::Angular())) {
                    
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}    
    
//****************************************************//
//               Reference code                       //
//****************************************************//

INIT_ENUM(Shape, SHAPE_SEQ)
INIT_ENUM(Operation, OP_SEQ)
INIT_ENUM(Type, TYPE_SEQ)
INIT_ENUM(Name, NAME_SEQ)

bool Reference::isValid() const {

    if( (m_operation == Operation::None) ||
        (m_type == Type::None) ||
        (m_shape == Shape::None) )
        return false;
    
    return true;
}

std::string Reference::asString() const {

    std::stringstream stream;
    asIndendetString(stream, 0, true);
    return stream.str();
}
 
void Reference::asIndendetString(std::stringstream& stream, int level, bool recursive) const
{
    ++level;
    if(isValid()) {
        stream << asString(m_type) << " " 
            << asString(m_shape) << " from operation " << asString(m_operation)
            << " (" << m_operationUuid.getValue() << ") created as subtype " << asString(m_name) 
            <<" with count: "<< m_counter;
        
        if(!m_baseIDs.empty()) {
            stream << " based on\n";
            for(auto id : m_baseIDs) {
                stream << std::string(level*3, ' ');
                stream << id.hash();
                if(recursive) {
                    stream << "{ ";
                    id.asIndendetString(stream, level, recursive);
                    stream << "}\n";
                }
            }
        }
    }
    else 
        stream << "None";
}

 
std::size_t Reference::hash() const {

    if(isValid()) {
        std::hash<std::string> hash;
        std::stringstream stream;
        asIndendetString(stream, 0, false);
        return hash(stream.str());
    }
    else return 0;
}

std::string Reference::hashAsString() const {

    std::stringstream stream;
    stream << hash();
    return stream.str();
}

bool Reference::operator==(std::size_t hash_) const {

    return hash() == hash_;
}

bool Reference::operator==(Type type) const {
    
    return m_type == type;
}

bool Reference::operator==(Operation op) const {

    return m_operation == op;
}

bool Reference::operator==(Name subtype) const {

    return m_name == subtype;
}

bool Reference::operator==(const Base::Uuid& uid) const
{
    return m_operationUuid == uid;
}

bool Reference::isGeneratedFrom(std::size_t hash) const
{

}

bool Reference::isModificationOf(std::size_t hash) const
{

}

const Base::Uuid& Reference::operationID()
{
    return m_operationUuid;
}

Reference& Reference::setOperationID(const Base::Uuid& id)
{
    m_operationUuid = id;
    return *this;
}



Reference Reference::buildNew(Reference::Shape sh, Reference::Operation op, 
                                Reference::Name  sub)
{
    //new means no base identifiers
    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_name = sub;
    id.m_type = Type::New;
    
    return id;
}

Reference Reference::buildGenerated(Reference::Shape sh, Reference::Operation op, 
                                      const std::vector< Reference >& base, Reference::Name sub)
{
    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_name = sub;
    id.m_type = Type::Generated;
    id.m_baseIDs = base;
    
    return id;
}

Reference Reference::buildGenerated(Reference::Shape sh, Reference::Operation op, const Reference& base, 
                                      Reference::Name sub)
{

    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_name = sub;
    id.m_type = Type::Generated;
    id.m_baseIDs.push_back(base);
    
    return id;
}

Reference Reference::buildModified(Reference::Shape sh, Reference::Operation op, const Reference& base, Name name)
{
    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_name = name;
    id.m_type = Type::Modified;
    id.m_baseIDs.push_back(base);
    
    return id;
    
}

template<typename Functor>
void for_each_subshape(const TopoDS_Shape& shape, const Functor& functor) {
    
    //TopExp_Explorer visits the topology top down. This means that vertices, edges and faces 
    //are accessed multiple time, for example if a vertex is used by two edges. We don't want that,
    //hence we use a map to prevent double entries
    TopExp_Explorer exp;
    TopTools_IndexedMapOfShape map;
    
    for(exp.Init(shape, TopAbs_VERTEX); exp.More(); exp.Next())
        map.Add(exp.Current());
        
    for (Standard_Integer k = 1; k <= map.Extent(); k++)
        functor(map(k), Reference::Shape::Vertex);  

    map.Clear();
    for(exp.Init(shape, TopAbs_EDGE); exp.More(); exp.Next())
        map.Add(exp.Current());
        
    for (Standard_Integer k = 1; k <= map.Extent(); k++)
        functor(map(k), Reference::Shape::Edge); 
    
    map.Clear();
    for(exp.Init(shape, TopAbs_FACE); exp.More(); exp.Next())
        map.Add(exp.Current());
        
    for (Standard_Integer k = 1; k <= map.Extent(); k++)
        functor(map(k), Reference::Shape::Face); 
};



void Reference::populateOperation(BRepBuilderAPI_MakeShape* builder, std::vector<TopoShape*> bases, 
                                    TopoShape* created, Operation op, Base::Uuid opID)
{
    
    if(!builder || bases.empty() || !created || created->isNull())
        throw ReferenceException("Unable to build references: faulty data");

    //to make sure that we handle absolutely all subshapes we need a collection first
    std::vector<TopoDS_Shape> subshapes;
    for_each_subshape(created->getShape(), [&](const TopoDS_Shape& subshape, Reference::Shape s) {
        
        Base::Console().Message("Add subshape type %s\n", asString(s).c_str());
        subshapes.push_back(subshape);
    });
    
    for( TopoShape* base : bases) {
        
        if(!base || base->isNull())
            throw ReferenceException("Unable to build references: faulty data");            
        
        //build all generated and modified shapes 
        for_each_subshape(base->getShape(), [&](const TopoDS_Shape& subshape, Reference::Shape shapetype) {
            
            Reference baseId = base->subshapeReference(subshape);
            const TopTools_ListOfShape& list = builder->Generated(subshape);
            TopTools_ListIteratorOfListOfShape iterator(list);
            while(iterator.More()) {
                auto ref = Reference::buildGenerated(shapetype, op, baseId);
                ref.setOperationID(opID);
                created->setSubshapeReference( iterator.Value(), ref);
                subshapes.erase(std::remove(subshapes.begin(), subshapes.end(), iterator.Value()), subshapes.end());
                iterator.Next();
                Base::Console().Message("Add generated id: %s\n", ref.asString().c_str());
            }
            
            iterator.Initialize(builder->Modified(subshape));
            while(iterator.More()) {
                auto ref = Reference::buildModified(shapetype, op, baseId);
                ref.setOperationID(opID);
                created->setSubshapeReference( iterator.Value(), ref);
                subshapes.erase(std::remove(subshapes.begin(), subshapes.end(), iterator.Value()), subshapes.end());
                iterator.Next();
                Base::Console().Message("Add modified id: %s\n", ref.asString().c_str());
            }
        });
    }
    
    //find all unchanged objects between base and created shape
    TopExp_Explorer exp;
    std::vector<TopoDS_Shape> testshapes = subshapes;
    subshapes.clear();
    
    Base::Console().Message("Try to find %i direct mapped shapes\n", testshapes.size());
    for(const TopoDS_Shape& shape : testshapes) {
        std::vector<Reference> refbase;
        for(auto base : bases) {
            for(exp.Init(base->getShape(), shape.ShapeType()); exp.More(); exp.Next()) {
                if(compareShapes(shape, exp.Current()))
                    refbase.push_back(base->subshapeReference(exp.Current()));
            }
        }
        if(!refbase.empty()) {
            if(refbase.size() == 1) {
                created->setSubshapeReference(shape, refbase.front());
                Base::Console().Message("Copy id over: %s\n", refbase.front().asString().c_str());
            }
            else { 
                created->setSubshapeReference(shape, Reference::buildGenerated(occToRefShape(shape.ShapeType()), op, refbase));
                Base::Console().Message("Build id from multiple bases: %s\n", created->subshapeReference(shape).asString().c_str());
            }
        }
        else
            subshapes.push_back(shape);
    }
    
    //all other shapes should be unchanged and the identifiers can simply be forwarded
    if(!subshapes.empty()) {
        Base::Console().Warning("%i subshapes have not been named in complex reference code, adding them as new", subshapes.size());
        for(const TopoDS_Shape& shape : testshapes) {
            created->setSubshapeReference(shape, Reference::buildNew(occToRefShape(shape.ShapeType()), op));
        }
    }
}

void Reference::populateOperation(BRepBuilderAPI_MakeShape* builder, TopoShape* base,
                                  TopoShape* created, Operation op, Base::Uuid opID) {
    
    std::vector<TopoShape*> bases = {base};
    populateOperation(builder, bases, created, op, opID);
}

void Reference::populateSubshape(TopoShape* base, TopoShape* subshape)
{
    for_each_subshape(subshape->getShape(), [&](const TopoDS_Shape& subsubshape, Reference::Shape) {
        subshape->setSubshapeReference(subsubshape, base->subshapeReference(subsubshape));
    });
}



} //Part
