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
#endif

#include <Mod/Part/App/GeometryPy.h>
#include "Mod/Sketcher3D/App/Sketch3DObject.h"

// inclusion of the generated files (generated out of SketchObjectSFPy.xml)
#include "Sketch3DObjectPy.h"
#include "Sketch3DObjectPy.cpp"
// other python types


using namespace Sketcher3D;

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
        return Py::new_reference_to(Py::Int(this->getSketch3DObjectPtr()->addGeometry(geo)));
    }
    Py_Return;
}

PyObject* Sketch3DObjectPy::delGeometry(PyObject* args) {
    int index;
    if(!PyArg_ParseTuple(args, "i", &index))
        return 0;

    this->getSketch3DObjectPtr()->delGeometry(index);

    Py_Return;
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

