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
#include "Identifier.h"

//some helpers to easily create and stringify the needed enums inside the Identifier class
#define TUPLES(r, data, elem) \
    (data::elem, std::string(BOOST_PP_STRINGIZE(elem)))
    
#define MAP(Name)\
    boost::bimaps::bimap< Identifier::Name, std::string >
    
#define INIT_ENUM(Name, seq) \
    const MAP(Name) Identifier::BOOST_PP_CAT(Name, map) = boost::assign::list_of<MAP(Name)::relation>BOOST_PP_SEQ_FOR_EACH(TUPLES, Name, seq );

namespace Part {

INIT_ENUM(Shape, SHAPE_SEQ)
INIT_ENUM(Operation, OP_SEQ)
INIT_ENUM(CreationType, CTYPE_SEQ)
INIT_ENUM(CreationSubType, CSUBTYPE_SEQ)

std::string Identifier::asString() {

    std::stringstream stream;
    stream << asString(m_shape) << " from operation " << asString(m_operation) << " created as "
           << asString(m_type) << " (" << asString(m_subtype) <<", "<< m_counter << ") " << " from ";
    for(auto id : m_baseIDs)
        stream << id.hashAsString() << " ";
    
    return stream.str();
}
 
std::size_t Identifier::hash() {

    std::hash<std::string> hash;
    return hash(asString());
}

std::string Identifier::hashAsString() {

    std::stringstream stream;
    stream << hash();
    return stream.str();
}

bool Identifier::operator==(std::size_t hash_) {

    return hash() == hash_;
}

bool Identifier::operator==(CreationType type) {
    
    return m_type == type;
}

bool Identifier::operator==(Operation op) {

    return m_operation == op;
}

bool Identifier::operator==(CreationSubType subtype) {

    return m_subtype == subtype;
}

bool Identifier::operator==(Base::Uuid uid)
{
    return m_operationUuid == uid;
}

bool Identifier::isGeneratedFrom(std::size_t hash)
{

}

bool Identifier::isModificationOf(std::size_t hash)
{

}

Identifier Identifier::buildNew(Identifier::Shape sh, Identifier::Operation op, 
                                Identifier::CreationSubType  sub)
{
    //new means no base identifiers
    Identifier id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_subtype = sub;
    id.m_type = CreationType::New;
    
    return id;
}

Identifier Identifier::buildGenerated(Identifier::Shape sh, Identifier::Operation op, 
                                      const std::vector< Identifier >& base, Identifier::CreationSubType sub)
{
    Identifier id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_subtype = sub;
    id.m_type = CreationType::Generated;
    id.m_baseIDs = base;
    
    return id;
}

Identifier Identifier::buildGenerated(Identifier::Shape sh, Identifier::Operation op, const Identifier& base, 
                                      Identifier::CreationSubType sub)
{

    Identifier id;
    id.m_shape = sh;
    id.m_operation = op;
    id.m_subtype = sub;
    id.m_type = CreationType::Generated;
    id.m_baseIDs.push_back(base);
    
    return id;
}



} //Part
