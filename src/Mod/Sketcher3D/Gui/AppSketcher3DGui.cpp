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
#ifndef _PreComp_
# include <Python.h>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Gui/Application.h>

#include "Workbench.h"
#include "ViewProviderSketch3D.h"

// use a different name to CreateCommand()
void CreateSketcher3DCommands(void);


/* registration table  */
extern struct PyMethodDef Sketcher3DGui_methods[];

PyDoc_STRVAR(module_Sketcher3DGui_doc,
"This module is the Sketcher3DGui module.");


/* Python entry */
extern "C" {
void Sketcher3DGuiExport initSketcher3DGui()
{
    if (!Gui::Application::Instance) {
        PyErr_SetString(PyExc_ImportError, "Cannot load Gui module in console application.");
        return;
    }
    try {
        Base::Interpreter().runString("import PartGui");
        Base::Interpreter().runString("import Sketcher3D");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return;
    }

    // instanciating the commands
    CreateSketcher3DCommands();
    Sketcher3DGui::Workbench::init();

    Sketcher3DGui::ViewProviderSketch3D::init();
    //Sketcher3DGui::ViewProviderPython::init();

    (void) Py_InitModule3("Sketcher3DGui", Sketcher3DGui_methods, module_Sketcher3DGui_doc);   /* mod name, table ptr */
    Base::Console().Log("Loading GUI of Sketcher3D module... done\n");
}

} // extern "C"
