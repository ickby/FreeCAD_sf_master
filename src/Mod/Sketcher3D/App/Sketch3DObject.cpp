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

Sketch3DObject::Sketch3DObject() : Part::Feature(), m_point_idx(0), m_curve_idx(0), m_surface_idx(0), m_cons_idx(0)  {

    ADD_PROPERTY_TYPE(m_solver, (0)  ,"Sketch3D",(App::PropertyType)(App::Prop_None),"three dimensional sketch solver");
};

Sketch3DObject::~Sketch3DObject() {};


SketchIdentifier Sketch3DObject::addGeometry(const Part::Geometry* geo) {

    m_solver.initChange();

    Part::Geometry* geoNew = geo->clone();

    if(geo->getTypeId() == Part::GeomPoint::getClassTypeId()) {
        Part::GeomPoint* p = dynamic_cast<Part::GeomPoint*>(geoNew);
        m_solver.createGeometry3D(p, std::make_pair(Point, ++m_point_idx));
        m_solver.finishChange();
        return std::make_pair(Point, m_point_idx);
    }
    else if(geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        Part::GeomLineSegment* lineSeg = dynamic_cast<Part::GeomLineSegment*>(geoNew);
        Sketcher3D::Shape3D_Ptr s = m_solver.createShape3D<dcm::segment3D>(lineSeg);

        //set the geometry index for all required stuff
        s->geometry(dcm::startpoint)->setIdentifier(std::make_pair(Point, ++m_point_idx));
        s->geometry(dcm::endpoint)->setIdentifier(std::make_pair(Point, ++m_point_idx));
        s->geometry(dcm::line)->setIdentifier(std::make_pair(Curve, ++m_curve_idx));

        m_solver.finishChange();
        return std::make_pair(Curve, m_curve_idx);
    };

    m_solver.finishChange();
    return std::make_pair(None, -1);
};

void Sketch3DObject::delGeometry(SketchIdentifier id) {

  Base::Console().Message("Type: %i, Id: %i", id.first, id.second);
    if(id.first == Sketcher3D::None)
        return;

    m_solver.initChange();

    try {
        if(m_solver.hasGeometry3D(id))
            m_solver.removeGeometry3D(id);
        else if(m_solver.hasShape3D(id))
            m_solver.removeShape3D(id);
        else
            Base::Console().Error("No type with this name available\n");
    }
    catch(...) {
        Base::Console().Error("Removing type failed\n");
    }

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


