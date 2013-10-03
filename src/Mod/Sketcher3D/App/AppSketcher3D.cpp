/***************************************************************************
 *   Copyright (c) Stefan Tröger        <stefantroeger@gmx.net>            *
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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>


/* registration table  */
extern struct PyMethodDef Sketcher3D_methods[];

PyDoc_STRVAR(module_Sketcher3D_doc,
"This module is the Sketcher3D module.");


/* Python entry */
extern "C" {
void Sketcher3DExport initSketcher3D() {

    // load dependent module
    try {
        Base::Interpreter().runString("import Part");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }
    
    (void) Py_InitModule3("Sketcher3D", Sketcher3D_methods, module_Sketcher3D_doc);   /* mod name, table ptr */
    
    // NOTE: To finish the initialization of our own type objects we must
    // call PyType_Ready, otherwise we run into a segmentation fault, later on.
    // This function is responsible for adding inherited slots from a type's base class.
 
    Sketcher3D::Sketch3DObject::init();
    Sketcher3D::Sketch3DObjectPython::init();
    
    Base::Console().Log("Loading Sketcher3D module... done\n");
}

} // extern "C"
