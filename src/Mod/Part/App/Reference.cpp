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
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>

//some helpers to easily create and stringify the needed enums inside the Reference class
#define TUPLES(r, data, elem) \
    (data::elem, std::string(BOOST_PP_STRINGIZE(elem)))
    
#define MAP(Name)\
    boost::bimaps::bimap< Reference::Name, std::string >
    
#define INIT_ENUM(Name, seq) \
    const MAP(Name) Reference::BOOST_PP_CAT(Name, map) = boost::assign::list_of<MAP(Name)::relation> BOOST_PP_SEQ_FOR_EACH(TUPLES, Name, seq );

namespace Part {

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
    if(isValid()) {
        stream << std::string(level*3, ' ') << asString(m_type) << " " 
            << asString(m_shape) << " from operation " << asString(m_operation)
            << "(" << m_operationUuid.getValue() << ") created as subtype " << asString(m_subtype) 
            <<" with count: "<< m_counter;
        
        if(!m_baseIDs.empty()) {
            stream << " based on\n";
            stream << std::string(level*3, ' ');
            for(auto id : m_baseIDs) {
                stream << id.hash();
                if(recursive) {
                    stream << "{ ";
                    id.asIndendetString(stream, level+1, recursive);
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

    return m_subtype == subtype;
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
}



Reference Reference::buildNew(Reference::Shape sh, Reference::Operation op, 
                                Reference::Name  sub)
{
    //new means no base identifiers
    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_subtype = sub;
    id.m_type = Type::New;
    
    return id;
}

Reference Reference::buildGenerated(Reference::Shape sh, Reference::Operation op, 
                                      const std::vector< Reference >& base, Reference::Name sub)
{
    Reference id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_subtype = sub;
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
    id.m_subtype = sub;
    id.m_type = Type::Generated;
    id.m_baseIDs.push_back(base);
    
    return id;
}

Reference Reference::buildComplex(BRepBuilderAPI_MakeShape* builder, TopoShape* base, 
                                    TopoShape* created, Operation op)
{

    //build all generated shapes 
    TopExp_Explorer Explorer(base->getShape(), TopAbs_VERTEX);
    while (Explorer.More()) {
        Reference baseId = base->subshapeReference(Explorer.Current());
        const TopTools_ListOfShape& list = builder->Generated(Explorer.Current());
        TopTools_ListIteratorOfListOfShape iterator(list);
        while(iterator.More())
            created->setSubshapeReference( iterator.Value(), Reference::buildGenerated(Reference::Shape::Vertex, op, baseId));
    }

    
    //build all modified shapes
}




} //Part
