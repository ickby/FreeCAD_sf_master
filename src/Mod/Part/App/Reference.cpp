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
    try {
    for_each_subshape(subshape->getShape(), [&](const TopoDS_Shape& subsubshape, Reference::Shape) {
        if(base->hasSubshapeReference(subsubshape))
            subshape->setSubshapeReference(subsubshape, base->subshapeReference(subsubshape));
        else {
            //Even though subshape is extracted from base, sometimes not all geometric matching shapes 
            //are topological matching. Hence setting the subshape reference 
            //directly may fail even thought the shapes are actually the same. Occ, you know...
            TopExp_Explorer exp(base->getShape(), subsubshape.ShapeType());
            for(; exp.More(); exp.Next()) {
                if(compareShapes(subsubshape, exp.Current())) {
                    subshape->setSubshapeReference(subsubshape, base->subshapeReference(exp.Current()));
                    break;
                }
            }
        }
    });
    }
    catch(Base::Exception& e) {
        Base::Console().Error("%s\n",  e.what());
    }
}



} //Part
