/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2010     *
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
# include <sstream>
#include <qvarlengtharray.h>
#endif

#include <Mod/Part/App/GeometryPy.h>
#include "Mod/Sketcher3D/App/Sketch3DObject.h"
#include <3rdParty/salomesmesh/inc/Rn.h>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_rule.hpp>
#include <boost/spirit/include/qi_string.hpp>
#include <boost/spirit/include/phoenix.hpp>

// inclusion of the generated files (generated out of SketchObjectSFPy.xml)
#include "Sketch3DObjectPy.h"
#include "Sketch3DObjectPy.cpp"
// other python types


using namespace Sketcher3D;

std::string sketchstring(SketchIdentifier s) {
    std::stringstream stream;
    switch(s.first) {

    case None:
        stream << "Nothing";
        return stream.str();
    case Point:
        stream << "Point";
        break;
    case Curve:
        stream << "Curve";
        break;
    case Surface:
        stream << "Surface";
        break;
    default
            :
        stream << "None";
        return stream.str();
    };

    stream << s.second;
    return stream.str();
};

SketchIdentifier generateID(const std::string& str)
{
    namespace qi = boost::spirit::qi;
    namespace phx = boost::phoenix;
    std::string::const_iterator begin = str.begin();
    std::string::const_iterator end = str.end();

    Sketch3DGeomTypes type = None;
    int index = -1;

    qi::parse(begin, end,
              (qi::lit("Point")[phx::ref(type)=Point] | qi::lit("Curve")[phx::ref(type)=Curve] | qi::lit("Surface")[phx::ref(type)=Surface])
              >> qi::int_[phx::ref(index) = qi::_1]
             );

    return std::make_pair(type, index);
}

// returns a string which represents the object e.g. when printed in python
std::string Sketch3DObjectPy::representation(void) const {
    Base::Console().Message("sketch3dobjectpy representation\n");
    return "<Sketcher3D::Sketch3DObject>";
}


PyObject* Sketch3DObjectPy::addGeometry(PyObject* args) {
    PyObject* pcObj;
    if(!PyArg_ParseTuple(args, "O", &pcObj))
        return 0;

    if(PyObject_TypeCheck(pcObj, &(Part::GeometryPy::Type))) {
        Part::Geometry* geo = static_cast<Part::GeometryPy*>(pcObj)->getGeometryPtr();
        return Py::new_reference_to(Py::String(sketchstring(this->getSketch3DObjectPtr()->addGeometry(geo))));
    }
    Py_Return;
}

PyObject* Sketch3DObjectPy::delGeometry(PyObject* args) {

    char* name;
    if(!PyArg_ParseTuple(args, "s",&name))      // convert args: Python->C
        return NULL;                             // NULL triggers exception

    PY_TRY {
        std::pair<Sketch3DGeomTypes, int> p = generateID(std::string(name));
        this->getSketch3DObjectPtr()->delGeometry(p);

        Py_Return;
    } PY_CATCH;
}

Py::Int Sketch3DObjectPy::getGeometryCount(void) const {
    //return Py::Int(this->getSketch3DObjectPtr()->Geometry.getSize());
}

PyObject* Sketch3DObjectPy::getCustomAttributes(const char* /*attr*/) const {
    return 0;
}

int Sketch3DObjectPy::setCustomAttributes(const char* attr, PyObject* obj) {
    Base::Console().Message("set custom attribute\n");
    // search in PropertyList
    App::Property* prop = getSketch3DObjectPtr()->getPropertyByName(attr);
    if(prop) {
        // Read-only attributes must not be set over its Python interface
        short Type =  getSketch3DObjectPtr()->getPropertyType(prop);
        if(Type & App::Prop_ReadOnly) {
            std::stringstream s;
            s << "Object attribute '" << attr << "' is read-only";
            throw Py::AttributeError(s.str());
        }

        prop->setPyObject(obj);

        //if(strcmp(attr,"Geometry") == 0)
        //    getSketch3DObjectPtr()->rebuildVertexIndex();

        return 1;
    }

    return 0;
}

