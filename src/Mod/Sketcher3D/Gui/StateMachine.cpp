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

#include "StateMachine.h"
#include "Misc.h"

#include <Gui/Command.h>
#include <Base/Console.h>
#include <App/Application.h>

#include <Inventor/SoPath.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/details/SoLineDetail.h>

#include <boost/statechart/transition.hpp>

using namespace Sketcher3DGui;

SbColor SketchMachine::VertexColor(1.0f,0.149f,0.0f);              // #FF2600 -> (255, 38,  0)
SbColor SketchMachine::CurveColor(1.0f,1.0f,1.0f);                 // #FFFFFF -> (255,255,255)
SbColor SketchMachine::CurveDraftColor(0.0f,0.0f,0.86f);           // #0000DC -> (  0,  0,220)
SbColor SketchMachine::CurveExternalColor(0.8f,0.2f,0.6f);         // #CC3399 -> (204, 51,153)
SbColor SketchMachine::CrossColorH(0.8f,0.4f,0.4f);                // #CC6666 -> (204,102,102)
SbColor SketchMachine::CrossColorV(0.4f,0.8f,0.4f);                // #66CC66 -> (102,204,102)
SbColor SketchMachine::CrossColorZ(0.2f,0.2f,0.9f);                // #66CC66 -> (102,204,102)
SbColor SketchMachine::FullyConstrainedColor(0.0f,1.0f,0.0f);      // #00FF00 -> (  0,255,  0)
SbColor SketchMachine::ConstrDimColor(1.0f,0.149f,0.0f);           // #FF2600 -> (255, 38,  0)
SbColor SketchMachine::ConstrIcoColor(1.0f,0.149f,0.0f);           // #FF2600 -> (255, 38,  0)
SbColor SketchMachine::PreselectColor(0.88f,0.88f,0.0f);           // #E1E100 -> (225,225,  0)
SbColor SketchMachine::SelectColor(0.11f,0.68f,0.11f);             // #1CAD1C -> ( 28,173, 28)



SketchMachine::SketchMachine(SoSeparator* root, Sketcher3D::Sketch3DObject* object) : Root(root), Object(object) {

  
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency = 0.;
    
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
    
    
    EditRoot = new SoSeparator;
    root->addChild(EditRoot);
    EditRoot->renderCaching = SoSeparator::OFF ;

    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    PointsMaterials = new SoMaterial;  
    EditRoot->addChild(PointsMaterials);

    SoMaterialBinding* MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    EditRoot->addChild(MtlBind);

    PointsCoordinate = new SoCoordinate3;
    EditRoot->addChild(PointsCoordinate);

    SoDrawStyle* DrawStyle = new SoDrawStyle;
    DrawStyle->pointSize = 8;
    EditRoot->addChild(DrawStyle);
    PointSet = new SoMarkerSet;
    PointSet->markerIndex = SoMarkerSet::CIRCLE_FILLED_7_7;
    EditRoot->addChild(PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    CurvesMaterials = new SoMaterial;
    EditRoot->addChild(CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    EditRoot->addChild(MtlBind);

    CurvesCoordinate = new SoCoordinate3;
    EditRoot->addChild(CurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    EditRoot->addChild(DrawStyle);

    CurveSet = new SoLineSet;

    EditRoot->addChild(CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    EditRoot->addChild(MtlBind);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 2;
    EditRoot->addChild(DrawStyle);

    RootCrossMaterials = new SoMaterial;
    RootCrossMaterials->diffuseColor.set1Value(0,CrossColorH);
    RootCrossMaterials->diffuseColor.set1Value(1,CrossColorV);
    RootCrossMaterials->diffuseColor.set1Value(2,CrossColorZ);
    EditRoot->addChild(RootCrossMaterials);

    RootCrossCoordinate = new SoCoordinate3;
    EditRoot->addChild(RootCrossCoordinate);

    RootCrossSet = new SoLineSet;
    RootCrossSet->numVertices.set1Value(0,2);
    RootCrossSet->numVertices.set1Value(1,2);
    RootCrossSet->numVertices.set1Value(2,2);
    EditRoot->addChild(RootCrossSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    EditCurvesMaterials = new SoMaterial;
    EditRoot->addChild(EditCurvesMaterials);

    EditCurvesCoordinate = new SoCoordinate3;
    EditRoot->addChild(EditCurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    EditRoot->addChild(DrawStyle);

    EditCurveSet = new SoLineSet;
    EditRoot->addChild(EditCurveSet);

    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoMaterial* CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->diffuseColor = cursorTextColor;
    EditRoot->addChild(CoordTextMaterials);

    SoSeparator* Coordsep = new SoSeparator();
    // no caching for fluctuand data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::OVERALL ;
    EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 1;
    EditRoot->addChild(DrawStyle);

    // add the group where all the constraints has its SoSeparator
    constrGroup = new SoGroup();
    EditRoot->addChild(constrGroup);
};

SketchMachine::~SketchMachine() {
    EditRoot->removeAllChildren();
    Root->removeChild(EditRoot);
};

Sketcher3D::SketchIdentifier SketchMachine::hit(const SoPickedPoint* Point) {

    if(Point) {
        SoPath* path = Point->getPath();
        SoNode* tail = path->getTail();
        SoNode* tailFather = path->getNode(path->getLength()-2);
        SoNode* tailFather2 = path->getNode(path->getLength()-3);

        // checking for a hit in the points
        if(tail == PointSet) {
            const SoDetail* point_detail = Point->getDetail(PointSet);
            if(point_detail && point_detail->getTypeId() == SoPointDetail::getClassTypeId()) {
                // get the index
                int Index = static_cast<const SoPointDetail*>(point_detail)->getCoordinateIndex();
                return PointIdMap[Index];
            }
        }
        else if(tail == CurveSet) {
            const SoDetail* line_detail = Point->getDetail(CurveSet);
            if(line_detail && line_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                // get the index
                int index = static_cast<const SoLineDetail*>(line_detail)->getLineIndex();
                return CurveIdMap[index];
            }
        }
    }
    return std::make_pair(Sketcher3D::None, 0);
};

Unselected::Unselected(my_context ctx) : my_base( ctx ) {
    outermost_context().Selection.clear();
    outermost_context().Preselect = std::make_pair(Sketcher3D::None, 0);
    post_event(EvRedraw());
};


sc::result Unselected::react(const EvMouseButtonPressed& event) {

    Sketcher3D::SketchIdentifier id = outermost_context().hit(event.pickedPoint);
    if(id.first != Sketcher3D::None) {
        outermost_context().Preselect = id;
        return transit<Preselect>();
    };
    return discard_event();
};

Preselect::Preselect() : initalized(false) {};

sc::result Preselect::react(const EvMouseMove& event) {

    if(!initalized) {
        startPoint = event.pos;
        initalized = true;
    }
    else {
        SbVec2s dist = (startPoint - event.pos);
        if(dist[0]*dist[0] + dist[1]*dist[1] > 9.)
            return transit<Drag>();
    };
    return discard_event();
};

sc::result Preselect::react(const EvMouseButtonReleased& event) {

    Sketcher3D::SketchIdentifier id = outermost_context().hit(event.pickedPoint);

    if(id.first == Sketcher3D::None) {
        return transit<Unselected>();
    }
    else {
        outermost_context().Selection.push_back(id);
	post_event(EvRedraw());
        return transit<Select>();
    }
};

sc::result Select::react(const EvKeyReleased& event) {

    if(event.key == SoKeyboardEvent::DELETE) {

        Sketcher3D::Sketch3DObject* object = outermost_context().Object;
        std::vector<Sketcher3D::SketchIdentifier>& vec = outermost_context().Selection;
        std::vector<Sketcher3D::SketchIdentifier>::iterator it;
        for(it = vec.begin(); it != vec.end(); it++) {
            if(it->first == Sketcher3D::Constraint)
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.delConstraint(\"%s\")",
                                        object->getNameInDocument(),
                                        Sketcher3DGui::sketchIdAsString(*it).c_str());
            else
                Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.delGeometry(\"%s\")",
                                        object->getNameInDocument(),
                                        Sketcher3DGui::sketchIdAsString(*it).c_str());

        };
        return transit<Unselected>();
    }
    else if(event.key == SoKeyboardEvent::ESCAPE)
        return transit<Unselected>();
    else
        discard_event();
}

sc::result Select::react(const EvMouseButtonPressed& event) {

    Sketcher3D::SketchIdentifier id = outermost_context().hit(event.pickedPoint);
    
    if(id.first == Sketcher3D::None)
        return transit<Unselected>();
    else {
        outermost_context().Preselect = id;
        return transit<Preselect>();
    }
}

sc::result Drag::react(const EvKeyReleased&) {

}

sc::result Drag::react(const EvMouseButtonPressed&) {

}
