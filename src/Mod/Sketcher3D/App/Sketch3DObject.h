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

#ifndef SKETCHER3D_SKETCH3DOBJECT_H
#define SKETCHER3D_SKETCH3DOBJECT_H

#include <Part/App/PartFeature.h>
#include <Part/App/PropertyGeometryList.h>

#include "Solver.hpp"


namespace Sketcher3D {

class Sketcher3DExport Sketch3DObject : public Part::Feature {

    PROPERTY_HEADER(Sketcher3D::Sketch3DObject);
    
    Solver System;

public:

    Sketch3DObject();
    virtual ~Sketch3DObject();

    //Property
    Solver   m_solver;
    
    //geometry and constraint numbering counters
    int m_point_idx, m_curve_idx, m_surface_idx, m_cons_idx;
    

    SketchIdentifier addGeometry(const Part::Geometry* geo);
    void delGeometry(SketchIdentifier id);
    
    // returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "Sketcher3DGui::ViewProviderSketch3D";
    }
    
    // from base class
    virtual PyObject *getPyObject(void);
};

typedef App::FeaturePythonT<Sketch3DObject> Sketch3DObjectPython;

}

#endif // SKETCHER3D_SKETCH3DOBJECT_H
