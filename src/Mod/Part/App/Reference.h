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
#include <Base/Persistence.h>
#include <Base/Exception.h>
#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/preprocessor.hpp>
#include <BRepBuilderAPI_MakeShape.hxx>

//some helpers to easily create and stringify the needed enums inside the Reference class.
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
   
class TopoShape;

#define SHAPE_SEQ  (None)(Geometry)(Vertex)(Edge)(Face)
#define OP_SEQ  (None)(Topology)(Geometry)(Box)(Sphere)
#define TYPE_SEQ  (None)(New)(Generated)(Modified)(Constructed)
#define NAME_SEQ  (None)(Top)(Bottom)(Front)(Back)(Left)(Right)(Start)(End)
    
class PartExport Reference : public Base::Persistence {

    ENUM(Shape, SHAPE_SEQ)
    ENUM(Operation, OP_SEQ)
    ENUM(Type, TYPE_SEQ)
    ENUM(Name, NAME_SEQ)
    
public:
    bool isGeneratedFrom(std::size_t hash) const;
    bool isModificationOf(std::size_t hash) const;
    
    //compare subtypes
    bool operator==(Type type) const;
    bool operator!=(Type type) const {return !operator==(type);};
    bool operator==(Name subtype) const;
    bool operator!=(Name subtype) const {return !operator==(subtype);};
    bool operator==(Operation op) const;
    bool operator!=(Operation op) const {return !operator==(op);};
    bool operator==(const Base::Uuid& uid) const;
    bool operator!=(const Base::Uuid& uid) const {return !operator==(uid);};
    
    //compare the whole identifier
    bool operator==(std::size_t hash) const;
    bool operator!=(std::size_t hash) const {return !operator==(hash);};
    
    //access some important data
    std::string asString() const;
    std::size_t hash() const;
    std::string hashAsString() const;
    
    //creation methods
    static Reference buildNew(Shape sh, Operation op, Name = Name::None);
    static Reference buildGenerated(Shape sh, Operation op, const Reference& base, 
                                     Name = Name::None);
    static Reference buildGenerated(Shape sh, Operation op, const std::vector<Reference>& base, 
                                     Name = Name::None);
    static Reference buildModified(Shape sh, Operation op, const Reference& base, Name = Name::None);
    
    static void      populateSubshape(TopoShape* base, TopoShape* subshape);
    static void      populateOperation(BRepBuilderAPI_MakeShape* builder, TopoShape* base,
                                  TopoShape* created, Operation op, Base::Uuid opID = Base::Uuid());
    static void      populateOperation(BRepBuilderAPI_MakeShape* builder, std::vector<TopoShape*> bases,
                                  TopoShape* created, Operation op, Base::Uuid opID = Base::Uuid());
    
    //methods for setting important properties
    Reference& setOperationID(const Base::Uuid& id);
    const Base::Uuid& operationID();
    
    //check validity
    bool isValid() const;
    operator bool() const {return isValid();};
    
    //Persistence methods 
    virtual void Save(Base::Writer&) const {};
    virtual void Restore(Base::XMLReader&) {};
    virtual unsigned int getMemSize(void) const {};
    
protected:
    std::vector<Reference>  m_baseIDs;
    Shape                   m_shape = Shape::None;
    Operation               m_operation = Operation::None;
    Base::Uuid              m_operationUuid;
    Type                    m_type = Type::None;
    Name                    m_name;
    unsigned short int      m_counter = 1;
    
    void asIndendetString(std::stringstream& stream, int level, bool recursive) const;
};

class PartExport ReferenceException : public Base::Exception {
    
public:
    /// With massage and Reference
    ReferenceException(std::string sMessage, const Reference& ref);
    /// With massage and file name
    ReferenceException(std::string sMessage);
    /// Construction
    ReferenceException(const Reference& ref);
    /// With massage and file name
    ReferenceException();
    /// Destruction
    virtual ~ReferenceException() throw() {}
  
    /// Description of the exception
    virtual const char* what() const throw();
protected:
    Reference m_ref;
  
};

} //Part

#endif// PART_IDENTIFIER_H
