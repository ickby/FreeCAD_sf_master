/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
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

#ifndef __InventorAll__
# include "InventorAll.h"
#endif

#include "GUIfreeInventorViewPy.h"
#include "GUIfreeInventorView.h"
#include <Base/Interpreter.h>

using namespace Gui;


void GUIfreeInventorViewPy::init_type()
{
    behaviors().name("GUIfreeInventorViewPy");
    behaviors().doc("Python binding class for the GUI free Inventor viewer class");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();

    add_varargs_method("getSceneGraph",&GUIfreeInventorViewPy::getSceneGraph,"getSceneGraph()");
}

GUIfreeInventorViewPy::GUIfreeInventorViewPy(GUIfreeInventorView *vi) : _view(vi)
{
}

GUIfreeInventorViewPy::~GUIfreeInventorViewPy()
{
    Base::PyGILStateLocker lock;
    for (std::list<PyObject*>::iterator it = callbacks.begin(); it != callbacks.end(); ++it)
        Py_DECREF(*it);
}

Py::Object GUIfreeInventorViewPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    if (!_view)
        throw Py::RuntimeError("Cannot print representation of deleted object");
    s_out << "GUIfreeInventorView";
    return Py::String(s_out.str());
}

GUIfreeInventorViewPy::method_varargs_handler GUIfreeInventorViewPy::pycxx_handler = 0;

PyObject *GUIfreeInventorViewPy::method_varargs_ext_handler(PyObject *_self_and_name_tuple, PyObject *_args)
{
    try {
        return pycxx_handler(_self_and_name_tuple, _args);
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(e.what());
    }
    catch (const std::exception& e) {
        throw Py::Exception(e.what());
    }
    catch(...) {
        throw Py::Exception("Unknown C++ exception");
    }
}

Py::Object GUIfreeInventorViewPy::getattr(const char * attr)
{
    if (!_view) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        Py::Object obj = Py::PythonExtension<GUIfreeInventorViewPy>::getattr(attr);
        if (PyCFunction_Check(obj.ptr())) {
            PyCFunctionObject* op = reinterpret_cast<PyCFunctionObject*>(obj.ptr());
            if (!pycxx_handler)
                pycxx_handler = op->m_ml->ml_meth;
            op->m_ml->ml_meth = method_varargs_ext_handler;
        }
        return obj;
    }
}

int GUIfreeInventorViewPy::setattr(const char * attr, const Py::Object & value)
{
    if (!_view) {
        std::string s;
        std::ostringstream s_out;
        s_out << "Cannot access attribute '" << attr << "' of deleted object";
        throw Py::RuntimeError(s_out.str());
    }
    else {
        return Py::PythonExtension<GUIfreeInventorViewPy>::setattr(attr, value);
    }
}

Py::Object GUIfreeInventorViewPy::getSceneGraph(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();

    try {
        SoNode* scene = static_cast<GUIfreeInventorViewer*>(_view->getInventorViewer())->getSceneGraph();
        PyObject* proxy = 0;
        proxy = Base::Interpreter().createSWIGPointerObj("pivy.coin", "SoSeparator *", (void*)scene, 1);
        scene->ref();
        return Py::Object(proxy, true);
    }
    catch (const Base::Exception& e) {
        throw Py::Exception(e.what());
    }
}
