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


#ifndef SKETCHER3DGUI_STATEMACHINE_H
#define SKETCHER3DGUI_STATEMACHINE_H

#include "PreCompiled.h"

#include <boost/statechart/event.hpp>
#include <boost/statechart/state_machine.hpp>
#include <boost/statechart/simple_state.hpp>
#include <boost/statechart/state.hpp>
#include <boost/statechart/custom_reaction.hpp>

#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedMarkerSet.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Sketcher3D/App/Solver.hpp>
#include <Sketcher3D/App/Sketch3DObject.h>

namespace sc = boost::statechart;

namespace Sketcher3DGui {

//define all possible 3d events
struct EvKeyPressed : sc::event< EvKeyPressed > {
    int key;
};
struct EvKeyReleased : sc::event< EvKeyReleased > {
    int key;
};
struct EvMouseButtonPressed : sc::event< EvMouseButtonPressed > {
    SbVec2s pos;
    SoPickedPoint* pickedPoint;
};
struct EvMouseButtonReleased : sc::event< EvMouseButtonReleased > {
    SbVec2s pos;
    SoPickedPoint* pickedPoint;
};
struct EvMouseButtonDoubleClick : sc::event< EvMouseButtonDoubleClick > {
    SbVec2s pos;
    SoPickedPoint* pickedPoint;
};
struct EvMouseMove : sc::event< EvMouseMove > {
    SbVec2s pos;
    SoPickedPoint* pickedPoint;
};

//and some tool events
struct EvPointTool : sc::event< EvPointTool > {};
struct EvLineTool : sc::event< EvLineTool > {};

//the state machine needs to be defined first, together with it's initial state
struct EditMode;
struct SketchMachine : sc::state_machine< SketchMachine, EditMode > {

    // container to track our own selected parts
    std::set<Sketcher3D::SketchIdentifier> SelPointSet;
    std::set<Sketcher3D::SketchIdentifier> SelCurvSet;
    std::set<Sketcher3D::SketchIdentifier> SelConstraintSet;
    std::vector<Sketcher3D::SketchIdentifier> CurveIdMap;

    Sketcher3D::SketchIdentifier Preselect;
    std::vector<Sketcher3D::SketchIdentifier> Selection;

    // nodes for the visuals
    SoSeparator*   Root;
    SoSeparator*   EditRoot;
    SoMaterial*    PointsMaterials;
    SoMaterial*    CurvesMaterials;
    SoMaterial*    RootCrossMaterials;
    SoMaterial*    EditCurvesMaterials;
    SoCoordinate3* PointsCoordinate;
    SoCoordinate3* CurvesCoordinate;
    SoCoordinate3* RootCrossCoordinate;
    SoCoordinate3* EditCurvesCoordinate;
    SoIndexedLineSet*     CurveSet;
    SoLineSet*     	  RootCrossSet;
    SoIndexedLineSet*     EditCurveSet;
    SoIndexedMarkerSet*   PointSet;
    SoGroup*       constrGroup;
    
    // colors
    static SbColor VertexColor;
    static SbColor CurveColor;
    static SbColor CurveDraftColor;
    static SbColor CurveExternalColor;
    static SbColor CrossColorV;
    static SbColor CrossColorH;
    static SbColor CrossColorZ;
    static SbColor FullyConstrainedColor;
    static SbColor ConstrDimColor;
    static SbColor ConstrIcoColor;
    static SbColor PreselectColor;
    static SbColor SelectColor;
    
    Sketcher3D::Sketch3DObject* Object;


    SketchMachine(SoSeparator* root, Sketcher3D::Sketch3DObject* object);
    ~SketchMachine();

    Sketcher3D::SketchIdentifier hit(const SoPickedPoint* Point);
};

//the basic edit mode state in which we are in when no tool is used. The initial inner state is
//Unselected and needs to be declared first
struct Unselected;
struct EditMode : sc::simple_state<EditMode, SketchMachine, Unselected > {};

//now all other inner states for EditMode can be defined
struct Unselected : sc::state<Unselected, EditMode> {
    typedef sc::custom_reaction< EvMouseButtonPressed > reactions;
    
    Unselected(my_context ctx);
    sc::result react(const EvMouseButtonPressed&);
};
struct Preselect : sc::simple_state< Preselect, EditMode > {
    typedef mpl::list<
    sc::custom_reaction< EvMouseMove >,
       sc::custom_reaction< EvMouseButtonReleased > > reactions;

    Preselect();
    sc::result react(const EvMouseMove&);
    sc::result react(const EvMouseButtonReleased&);
    
    bool initalized;
    SbVec2s startPoint;
};
struct Select : sc::simple_state< Select, EditMode> {
    typedef mpl::list<
    sc::custom_reaction< EvKeyReleased >,
       sc::custom_reaction< EvMouseButtonPressed > > reactions;

    sc::result react(const EvKeyReleased&);
    sc::result react(const EvMouseButtonPressed&);
};
struct Drag : sc::simple_state< Drag, EditMode> {
    typedef mpl::list<
    sc::custom_reaction< EvMouseButtonPressed >,
       sc::custom_reaction< EvKeyReleased > > reactions;

    sc::result react(const EvKeyReleased&);
    sc::result react(const EvMouseButtonPressed&);
};
}; //Sketcher3dGui

#endif //SKETCHER3DGUI_STATEMACHINE_H


