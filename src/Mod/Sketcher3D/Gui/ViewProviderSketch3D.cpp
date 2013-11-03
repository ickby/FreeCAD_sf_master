/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger        <stefantroeger@gmx.net>       *
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

#include "ViewProviderSketch3D.h"
#include "Misc.h"
#include <Base/Console.h>
#include <Mod/Part/App/Geometry.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/Control.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/View3DInventor.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/nodes/SoCamera.h>

using namespace Sketcher3DGui;

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)


ViewProviderSketch3D::ViewProviderSketch3D() {

    LineColor.setValue(1,1,1);
    PointColor.setValue(1,1,1);
    PointSize.setValue(4);
}

ViewProviderSketch3D::~ViewProviderSketch3D() {

}

void ViewProviderSketch3D::attach(App::DocumentObject* obj) {

    ViewProviderPart::attach(obj);
    Base::Console().Message("Attach\n");
}

void ViewProviderSketch3D::updateData(const App::Property* prop) {

    ViewProviderPart::updateData(prop);
    if(m_machine)
        draw();
    Base::Console().Message("UpdateData\n");
}

void ViewProviderSketch3D::setupContextMenu(QMenu* menu, QObject* receiver, const char* member) {

    Base::Console().Message("Setup Context Menu\n");
}

bool ViewProviderSketch3D::onDelete(const std::vector<std::string>&) {

    Base::Console().Message("OnDelete\n");
}

bool ViewProviderSketch3D::doubleClicked(void) {

    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& pos, Gui::View3DInventorViewer* viewer) {

    if(!m_machine)
        return false;

    // Calculate 3d point to the mouse position
    //SbLine line;
    //getProjectingLine(pos, viewer, line);

    //double x,y;
    //getCoordsOnSketchPlane(x,y,line.getPosition(),line.getDirection());

    SoPickedPoint* pp = this->getPointOnRay(pos, viewer);
    EvMouseMove ev;
    ev.pickedPoint = pp;
    ev.pos = pos;
    m_machine->process_event(ev);
    delete pp;

    return true;
}

bool ViewProviderSketch3D::keyPressed(bool pressed, int key) {

    if(!m_machine)
        return false;

    if(pressed) {
        EvKeyPressed ev;
        ev.key = key;
        m_machine->process_event(ev);
    }
    else {
        EvKeyReleased ev;
        ev.key = key;
        m_machine->process_event(ev);
    }
}

bool ViewProviderSketch3D::mouseButtonPressed(int Button, bool pressed, const SbVec2s& pos,
        const Gui::View3DInventorViewer* viewer) {

    if(!m_machine)
        return false;

    SoPickedPoint* pp = this->getPointOnRay(pos, viewer);
    if(pressed) {
	Base::Console().Message("Pressed = true\n");
        EvMouseButtonPressed ev;
        ev.pickedPoint = pp;
        ev.pos = pos;
        m_machine->process_event(ev);
    }
    else {
      Base::Console().Message("Pressed = false\n");
        EvMouseButtonReleased ev;
        ev.pickedPoint = pp;
        ev.pos = pos;
        m_machine->process_event(ev);
    }
    delete pp;
}

void ViewProviderSketch3D::onSelectionChanged(const Gui::SelectionChanges& msg) {

}

bool ViewProviderSketch3D::setEdit(int ModNum) {
    // When double-clicking on the item for this sketch the
    // object unsets and sets its m_machine mode without closing
    // the task panel
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if(dlg) {
        QMessageBox msgBox;
        msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if(ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // create the container for the additional m_machine data
    m_machine = boost::shared_ptr<SketchMachine>(new SketchMachine(pcRoot, getSketch3DObject()));
    m_machine->initiate();

    this->hide(); // avoid that the wires interfere with the m_machine lines

    draw();

    return true;
}


void ViewProviderSketch3D::unsetEdit(int ModNum) {

    m_machine = boost::shared_ptr<SketchMachine>();
}

void ViewProviderSketch3D::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum) {
    viewer->setEditing(true);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);
}

void ViewProviderSketch3D::unsetEditViewer(Gui::View3DInventorViewer* viewer) {
    viewer->setEditing(false);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);
}


void ViewProviderSketch3D::draw() {

    m_machine->process_event(EvRedraw());
}

void ViewProviderSketch3D::createEditInventorNodes(void) {

}

void ViewProviderSketch3D::updateColor(void) {
}

void ViewProviderSketch3D::getProjectingLine(const SbVec2s& pnt, const Gui::View3DInventorViewer* viewer, SbLine& line) const
{
    const SbViewportRegion& vp = viewer->getViewportRegion();

    short x,y;
    pnt.getValue(x,y);
    SbVec2f siz = vp.getViewportSize();
    float dX, dY;
    siz.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = (float)x / float(vp.getViewportSizePixels()[0]);
    float pY = (float)y / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    if(fRatio > 1.0f) {
        pX = (pX - 0.5f*dX) * fRatio + 0.5f*dX;
    }
    else if(fRatio < 1.0f) {
        pY = (pY - 0.5f*dY) / fRatio + 0.5f*dY;
    }

    SoCamera* pCam = viewer->getCamera();
    if(!pCam)
        return;
    SbViewVolume  vol = pCam->getViewVolume();

    float focalDist = pCam->focalDistance.getValue();

    vol.projectPointToLine(SbVec2f(pX,pY), line);
}


Sketcher3D::Sketch3DObject* ViewProviderSketch3D::getSketch3DObject(void) const {

    return dynamic_cast<Sketcher3D::Sketch3DObject*>(pcObject);
}


namespace Gui {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Sketcher3DGui::ViewProviderPython, Sketcher3DGui::ViewProviderSketch3D)
/// @endcond

// explicit template instantiation
template class Sketcher3DGuiExport ViewProviderPythonFeatureT<Sketcher3DGui::ViewProviderSketch3D>;
}
