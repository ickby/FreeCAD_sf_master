/***************************************************************************
 *   Copyright (c) Stefan Tröger         (stefantroeger@gmx.net) 2013      *
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
#include "Sketch3DObject.h"
#include <Base/Console.h>
#include "Sketch3DObjectPy.h"

using namespace Sketcher3D;


PROPERTY_SOURCE(Sketcher3D::Sketch3DObject, Part::Feature)

Sketch3DObject::Sketch3DObject() : Part::Feature(), m_geom_idx(0), m_cons_idx(0)  {
  
      ADD_PROPERTY_TYPE(m_solver, (0)  ,"Sketch3D",(App::PropertyType)(App::Prop_None),"three dimensional sketch solver");
};

Sketch3DObject::~Sketch3DObject() {};


int Sketch3DObject::addGeometry(const Part::Geometry* geo) {

    m_solver.initChange();
    
    Part::Geometry* geoNew = geo->clone();
    m_geom_idx++;
    
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
      Part::GeomPoint* p = dynamic_cast<Part::GeomPoint*>(geoNew);
      m_solver.createGeometry3D(p, m_geom_idx);
    }
    
    m_solver.finishChange();
    
    return m_geom_idx;
};

void Sketch3DObject::delGeometry(int id) {
  
    m_solver.initChange();
    m_solver.removeGeometry3D(id);
    m_solver.finishChange();
}


PyObject* Sketch3DObject::getPyObject(void) {
    if(PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new Sketch3DObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Sketcher feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher3D::Sketch3DObjectPython, Sketcher3D::Sketch3DObject)
template<> const char* Sketcher3D::Sketch3DObjectPython::getViewProviderName(void) const {
    return "Sketcher3D::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class Sketcher3DExport FeaturePythonT<Sketcher3D::Sketch3DObject>;
}

