/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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
#endif

#include "GUIfreeInventorView.h"
#include "SoFCSelectionAction.h"
#include "Document.h"
#include "GUIfreeInventorViewPy.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Inventor/fields/SoSFString.h>

#include <Inventor/nodes/SoSeparator.h>

using namespace Gui;

TYPESYSTEM_SOURCE_ABSTRACT(Gui::GUIfreeInventorView, Gui::InventorView);

GUIfreeInventorView::GUIfreeInventorView(Gui::Document* pcDocument) : InventorView(pcDocument), _viewerPy(0)
{
    _pcIsInventor = true;
 
    // attach parameter Observer
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);
    
    //create the viewer 
    _viewer = new GUIfreeInventorViewer();

    // apply the user settings
    OnChange(*hGrp,"EyeDistance");
    OnChange(*hGrp,"Gradient");
    OnChange(*hGrp,"BackgroundColor");
    OnChange(*hGrp,"BackgroundColor2");
    OnChange(*hGrp,"BackgroundColor3");
    OnChange(*hGrp,"BackgroundColor4");
    OnChange(*hGrp,"UseBackgroundColorMid");
    OnChange(*hGrp,"Orthographic");
    OnChange(*hGrp,"HeadlightColor");
    OnChange(*hGrp,"HeadlightDirection");
    OnChange(*hGrp,"HeadlightIntensity");
    OnChange(*hGrp,"EnableBacklight");
    OnChange(*hGrp,"BacklightColor");
    OnChange(*hGrp,"BacklightDirection");
    OnChange(*hGrp,"BacklightIntensity");
}

GUIfreeInventorView::~GUIfreeInventorView()
{
    hGrp->Detach(this);

    if (_viewerPy) {
        static_cast<GUIfreeInventorViewPy*>(_viewerPy)->_view = 0;
        Py_DECREF(_viewerPy);
    }

    // here is from time to time trouble!!!
    delete _viewer;
}

PyObject *GUIfreeInventorView::getPyObject(void)
{
    if (!_viewerPy)
        _viewerPy = new GUIfreeInventorViewPy(this);

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

void GUIfreeInventorView::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
}

void GUIfreeInventorView::onRename(Gui::Document *pDoc)
{
    SoSFString name;
    name.setValue(pDoc->getDocument()->getName());
    SoFCDocumentAction cAct(name);
    cAct.apply(_viewer->getSceneGraph());
}

const char *GUIfreeInventorView::getName(void) const
{
    return "GUIfreeInventor";
}

bool GUIfreeInventorView::onMsg(const char* pMsg, const char** ppReturn)
{
    return false;
}

bool GUIfreeInventorView::onHasMsg(const char* pMsg) const
{
    return false;
}

void GUIfreeInventorView::deleteSelf()
{
}

InventorViewer* GUIfreeInventorView::getInventorViewer(void) {
    return _viewer;
}
