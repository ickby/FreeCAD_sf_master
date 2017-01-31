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

#ifndef PART_IDENTIFIER_H
#define PART_IDENTIFIER_H

#include <Base/Uuid.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/preprocessor.hpp>

//some helpers to easily create and stringify the needed enums inside the Identifier class.
//don't forget to use INIT_ENUM in the cpp file!
#define ENUM(Name, seq) \
    public:\
        enum class Name { BOOST_PP_SEQ_ENUM( seq ) };\
    private:\
        typedef boost::bimaps::bimap< Name, std::string > BOOST_PP_CAT(Name, Map); \
        static const BOOST_PP_CAT(Name, Map) BOOST_PP_CAT(Name, map); \
        static std::string asString(const Name& name) { return BOOST_PP_CAT(Name, map).left.find(name)->second;}; \
        static Name BOOST_PP_CAT(stringAs, Name)(const std::string& string) {return BOOST_PP_CAT(Name, map).right.find(string)->second;}; \
   
        
namespace Part {

#define SHAPE_SEQ  (None)(Vertex)(Edge)(Face)
#define OP_SEQ  (None)(Topology)(Box)(Sphere)
#define CTYPE_SEQ  (None)(New)(Generated)(Modified)
#define CSUBTYPE_SEQ  (None)(Top)(Bottom)(Front)(Back)(Left)(Right)
    
class PartExport Identifier {

    ENUM(Shape, SHAPE_SEQ)
    ENUM(Operation, OP_SEQ)
    ENUM(CreationType, CTYPE_SEQ)
    ENUM(CreationSubType, CSUBTYPE_SEQ)
    
public:
    bool isGeneratedFrom(std::size_t hash);
    bool isModificationOf(std::size_t hash);
    bool isOperation(std::string op);
    
    bool operator==(CreationType type);
    bool operator!=(CreationType type) {return !operator==(type);};
    bool operator==(CreationSubType subtype);
    bool operator!=(CreationSubType subtype) {return !operator==(subtype);};
    bool operator==(Operation op);
    bool operator!=(Operation op) {return !operator==(op);};
    bool operator==(std::size_t hash);
    bool operator!=(std::size_t hash) {return !operator==(hash);};
    
    std::string asString();
    std::size_t hash();
    std::string hashAsString();
    
private:
    std::vector<Identifier> m_baseIDs;
    Shape                   m_shape;
    Operation               m_operation;
    Base::Uuid              m_operationUuid;
    CreationType            m_type;
    CreationSubType         m_subtype;
    unsigned int            m_counter = 1;
};

} //Part

#endif// PART_IDENTIFIER_H
