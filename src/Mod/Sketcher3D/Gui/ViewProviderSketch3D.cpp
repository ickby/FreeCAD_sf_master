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

#include "ViewProviderSketch3D.h"
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
#include <Inventor/nodes/SoCamera.h>

using namespace Sketcher3DGui;

SbColor ViewProviderSketch3D::VertexColor(1.0f,0.149f,0.0f);              // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch3D::CurveColor(1.0f,1.0f,1.0f);                 // #FFFFFF -> (255,255,255)
SbColor ViewProviderSketch3D::CurveDraftColor(0.0f,0.0f,0.86f);           // #0000DC -> (  0,  0,220)
SbColor ViewProviderSketch3D::CurveExternalColor(0.8f,0.2f,0.6f);         // #CC3399 -> (204, 51,153)
SbColor ViewProviderSketch3D::CrossColorH(0.8f,0.4f,0.4f);                // #CC6666 -> (204,102,102)
SbColor ViewProviderSketch3D::CrossColorV(0.4f,0.8f,0.4f);                // #66CC66 -> (102,204,102)
SbColor ViewProviderSketch3D::CrossColorZ(0.2f,0.2f,0.9f);                // #66CC66 -> (102,204,102)
SbColor ViewProviderSketch3D::FullyConstrainedColor(0.0f,1.0f,0.0f);      // #00FF00 -> (  0,255,  0)
SbColor ViewProviderSketch3D::ConstrDimColor(1.0f,0.149f,0.0f);           // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch3D::ConstrIcoColor(1.0f,0.149f,0.0f);           // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch3D::PreselectColor(0.88f,0.88f,0.0f);           // #E1E100 -> (225,225,  0)
SbColor ViewProviderSketch3D::SelectColor(0.11f,0.68f,0.11f);             // #1CAD1C -> ( 28,173, 28)

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)


//draw functor
struct drawer : boost::static_visitor<void> {

    EditData* data;
    int index;
    drawer(EditData* d) : data(d) {};

    void setID(int i) {
        index = i;
    };

    void operator()(Part::GeomPoint* const& p) {

        const Base::Vector3d point = p->getPoint();
        data->PointsCoordinate->point.set1Value(index, point[0], point[1], point[2]);
        data->PointSet->coordIndex.set1Value(index, index);
    };
};

ViewProviderSketch3D::ViewProviderSketch3D() : edit(NULL) {

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
    if(edit) draw();
    Base::Console().Message("UpdateData\n");
}

void ViewProviderSketch3D::setupContextMenu(QMenu* menu, QObject* receiver, const char* member) {

    Base::Console().Message("Setup Context Menu\n");
}

bool ViewProviderSketch3D::onDelete(const std::vector<std::string> &) {

    Base::Console().Message("OnDelete\n");
}

bool ViewProviderSketch3D::doubleClicked(void) {

    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& pos, Gui::View3DInventorViewer* viewer) {

    if(!edit)
        return false;
    assert(edit);

    // Calculate 3d point to the mouse position
    //SbLine line;
    //getProjectingLine(pos, viewer, line);

    //double x,y;
    //getCoordsOnSketchPlane(x,y,line.getPosition(),line.getDirection());

    SoPickedPoint* pp = this->getPointOnRay(pos, viewer);
    int PtIndex,GeoIndex,ConstrIndex,CrossIndex;
    detectPreselection(pp);
    delete pp;

    return false;
}

bool ViewProviderSketch3D::keyPressed(bool pressed, int key) {

    Base::Console().Message("KeyPressed\n");
}

bool ViewProviderSketch3D::mouseButtonPressed(int Button, bool pressed, const SbVec2s& pos,
        const Gui::View3DInventorViewer* viewer) {

    Base::Console().Message("mousebuttonpressed\n");
}

void ViewProviderSketch3D::onSelectionChanged(const Gui::SelectionChanges& msg) {

    Base::Console().Message("onSelectionChanged\n");
}

bool ViewProviderSketch3D::setEdit(int ModNum) {
    // When double-clicking on the item for this sketch the
    // object unsets and sets its edit mode without closing
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

    // create the container for the additional edit data
    assert(!edit);
    edit = new EditData();

    createEditInventorNodes();
    this->hide(); // avoid that the wires interfere with the edit lines


    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;

    // set the point color
    unsigned long color = (unsigned long)(VertexColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedVertexColor", color);
    VertexColor.setPackedValue((uint32_t)color, transparency);
    // set the curve color
    color = (unsigned long)(CurveColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedEdgeColor", color);
    CurveColor.setPackedValue((uint32_t)color, transparency);
    // set the construction curve color
    color = (unsigned long)(CurveDraftColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstructionColor", color);
    CurveDraftColor.setPackedValue((uint32_t)color, transparency);
    // set the fully constrained color
    color = (unsigned long)(FullyConstrainedColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstrainedColor", color);
    FullyConstrainedColor.setPackedValue((uint32_t)color, transparency);

    // set the highlight color
    unsigned long highlight = (unsigned long)(PreselectColor.getPackedValue());
    highlight = hGrp->GetUnsigned("HighlightColor", highlight);
    PreselectColor.setPackedValue((uint32_t)highlight, transparency);
    // set the selection color
    highlight = (unsigned long)(SelectColor.getPackedValue());
    highlight = hGrp->GetUnsigned("SelectionColor", highlight);
    SelectColor.setPackedValue((uint32_t)highlight, transparency);

    draw();

    return true;
}


void ViewProviderSketch3D::unsetEdit(int ModNum) {

    edit->EditRoot->removeAllChildren();
    pcRoot->removeChild(edit->EditRoot);
    delete edit;
    edit = NULL;
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

    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);
    edit->RootCrossSet->numVertices.set1Value(2,2);
    edit->RootCrossCoordinate->point.set1Value(0,SbVec3f(-10, 0.0f, 0.0f));
    edit->RootCrossCoordinate->point.set1Value(1,SbVec3f(10, 0.0f, 0.0f));
    edit->RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -10, 0.0f));
    edit->RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, 10, 0.0f));
    edit->RootCrossCoordinate->point.set1Value(4,SbVec3f(0.0f, 0.0f, -10));
    edit->RootCrossCoordinate->point.set1Value(5,SbVec3f(0.0f, 0.0f, 10));

    //draw geometry
    drawer draw(edit);
    Sketcher3D::Solver& solver = getSketch3DObject()->m_solver;
    typedef typename std::vector< Sketcher3D::Geom3D_Ptr >::iterator iter;
    for(iter it = solver.begin<Sketcher3D::Geometry3D>(); it != solver.end<Sketcher3D::Geometry3D>(); it++) {

        Sketcher3D::Geom3D_Ptr ptr = *it;
        draw.setID(ptr->getIdentifier());
        ptr->apply(draw);
    };

    updateColor();
    if(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        static_cast<Gui::View3DInventor*>(mdi)->getViewer()->render();
    }
}

void ViewProviderSketch3D::createEditInventorNodes(void) {
    assert(edit);

    edit->EditRoot = new SoSeparator;
    pcRoot->addChild(edit->EditRoot);
    edit->EditRoot->renderCaching = SoSeparator::OFF ;

    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    edit->PointsMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->PointsMaterials);

    SoMaterialBinding* MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    edit->EditRoot->addChild(MtlBind);

    edit->PointsCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->PointsCoordinate);

    SoDrawStyle* DrawStyle = new SoDrawStyle;
    DrawStyle->pointSize = 8;
    edit->EditRoot->addChild(DrawStyle);
    edit->PointSet = new SoIndexedMarkerSet;
    edit->PointSet->markerIndex = SoMarkerSet::CIRCLE_FILLED_7_7;
    edit->EditRoot->addChild(edit->PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    edit->CurvesMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    edit->EditRoot->addChild(MtlBind);

    edit->CurvesCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->CurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    edit->EditRoot->addChild(DrawStyle);

    edit->CurveSet = new SoLineSet;

    edit->EditRoot->addChild(edit->CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    edit->EditRoot->addChild(MtlBind);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 2;
    edit->EditRoot->addChild(DrawStyle);

    edit->RootCrossMaterials = new SoMaterial;
    edit->RootCrossMaterials->diffuseColor.set1Value(0,CrossColorH);
    edit->RootCrossMaterials->diffuseColor.set1Value(1,CrossColorV);
    edit->RootCrossMaterials->diffuseColor.set1Value(2,CrossColorZ);
    edit->EditRoot->addChild(edit->RootCrossMaterials);

    edit->RootCrossCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->RootCrossCoordinate);

    edit->RootCrossSet = new SoLineSet;
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);
    edit->RootCrossSet->numVertices.set1Value(2,2);
    edit->EditRoot->addChild(edit->RootCrossSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    edit->EditCurvesMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->EditCurvesMaterials);

    edit->EditCurvesCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->EditCurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    edit->EditRoot->addChild(DrawStyle);

    edit->EditCurveSet = new SoLineSet;
    edit->EditRoot->addChild(edit->EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoMaterial* CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->diffuseColor = cursorTextColor;
    edit->EditRoot->addChild(CoordTextMaterials);

    SoSeparator* Coordsep = new SoSeparator();
    // no caching for fluctuand data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    SoFont* font = new SoFont();
    font->size = 15.0;
    Coordsep->addChild(font);

    edit->textPos = new SoTranslation();
    Coordsep->addChild(edit->textPos);

    edit->textX = new SoText2();
    edit->textX->justification = SoText2::LEFT;
    edit->textX->string = "";
    Coordsep->addChild(edit->textX);

    edit->EditRoot->addChild(Coordsep);

    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 1;
    edit->EditRoot->addChild(DrawStyle);

    // add the group where all the constraints has its SoSeparator
    edit->constrGroup = new SoGroup();
    edit->EditRoot->addChild(edit->constrGroup);
}

void ViewProviderSketch3D::updateColor(void) {
    assert(edit);
    //Base::Console().Log("Draw preseletion\n");

    int PtNum = edit->PointsMaterials->diffuseColor.getNum();
    SbColor* pcolor = edit->PointsMaterials->diffuseColor.startEditing();
    int CurvNum = edit->CurvesMaterials->diffuseColor.getNum();
    SbColor* color = edit->CurvesMaterials->diffuseColor.startEditing();
    SbColor* crosscolor = edit->RootCrossMaterials->diffuseColor.startEditing();

    // colors of the point set
    /*   if (edit->FullyConstrained)
           for (int  i=0; i < PtNum; i++)
               pcolor[i] = FullyConstrainedColor;
       else*/
    for(int  i=0; i < PtNum; i++)
        pcolor[i] = VertexColor;

    if(edit->PreselectCross == 0)
        pcolor[0] = PreselectColor;
    else if(edit->PreselectPoint != -1)
        pcolor[edit->PreselectPoint + 1] = PreselectColor;

    for(std::set<int>::iterator it=edit->SelPointSet.begin();
            it != edit->SelPointSet.end(); it++)
        pcolor[*it] = SelectColor;

    // colors of the curves
    for(int  i=0; i < CurvNum; i++) {
        //int GeoId = edit->CurvIdToGeoId[i];
        /*if (edit->SelCurvSet.find(GeoId) != edit->SelCurvSet.end())
            color[i] = SelectColor;
        else if (edit->PreselectCurve == GeoId)
            color[i] = PreselectColor;
        else if (GeoId < -2)  // external Geometry
            color[i] = CurveExternalColor;
        else if (getSketchObject()->getGeometry(GeoId)->Construction)
            color[i] = CurveDraftColor;
        else if (edit->FullyConstrained)
            color[i] = FullyConstrainedColor;
        else*/
        color[i] = CurveColor;
    }

    // colors of the cross
    if(edit->SelCurvSet.find(-1) != edit->SelCurvSet.end())
        crosscolor[0] = SelectColor;
    else if(edit->PreselectCross == 1)
        crosscolor[0] = PreselectColor;
    else
        crosscolor[0] = CrossColorH;

    if(edit->SelCurvSet.find(-2) != edit->SelCurvSet.end())
        crosscolor[1] = SelectColor;
    else if(edit->PreselectCross == 2)
        crosscolor[1] = PreselectColor;
    else
        crosscolor[1] = CrossColorV;

    if(edit->SelCurvSet.find(-2) != edit->SelCurvSet.end())
        crosscolor[2] = SelectColor;
    else if(edit->PreselectCross == 3)
        crosscolor[2] = PreselectColor;
    else
        crosscolor[2] = CrossColorZ;

    // end editing
    edit->CurvesMaterials->diffuseColor.finishEditing();
    edit->PointsMaterials->diffuseColor.finishEditing();
    edit->RootCrossMaterials->diffuseColor.finishEditing();
}



bool ViewProviderSketch3D::detectPreselection(const SoPickedPoint* Point) {
    assert(edit);


    if(Point) {
        //Base::Console().Log("Point pick\n");
        SoPath* path = Point->getPath();
        SoNode* tail = path->getTail();
        SoNode* tailFather = path->getNode(path->getLength()-2);
        SoNode* tailFather2 = path->getNode(path->getLength()-3);

        // checking for a hit in the points
        if(tail == edit->PointSet) {
            const SoDetail* point_detail = Point->getDetail(edit->PointSet);
            if(point_detail && point_detail->getTypeId() == SoPointDetail::getClassTypeId()) {
                // get the index
                int Index = static_cast<const SoPointDetail*>(point_detail)->getCoordinateIndex();
                std::stringstream ss;
                ss << "Vertex" << Index;
                bool accepted =
                    Gui::Selection().setPreselect(getSketch3DObject()->getDocument()->getName()
                                                  ,getSketch3DObject()->getNameInDocument()
                                                  ,ss.str().c_str()
                                                  ,Point->getPoint()[0]
                                                  ,Point->getPoint()[1]
                                                  ,Point->getPoint()[2]);
            }
        }
    }
    return true;
}

void ViewProviderSketch3D::getProjectingLine(const SbVec2s& pnt, const Gui::View3DInventorViewer *viewer, SbLine& line) const
{
    const SbViewportRegion& vp = viewer->getViewportRegion();

    short x,y; pnt.getValue(x,y);
    SbVec2f siz = vp.getViewportSize();
    float dX, dY; siz.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = (float)x / float(vp.getViewportSizePixels()[0]);
    float pY = (float)y / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    if (fRatio > 1.0f) {
        pX = (pX - 0.5f*dX) * fRatio + 0.5f*dX;
    }
    else if (fRatio < 1.0f) {
        pY = (pY - 0.5f*dY) / fRatio + 0.5f*dY;
    }

    SoCamera* pCam = viewer->getCamera();
    if (!pCam) return;
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
