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

#ifndef SKETCHER3DGUI_VIEWPROVIDERSKETCH3D_H
#define SKETCHER3DGUI_VIEWPROVIDERSKETCH3D_H

#include "PreCompiled.h"
#include <Mod/Sketcher3D/App/Sketch3DObject.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderPythonFeature.h>
#include <Part/Gui/ViewProvider.h>
#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoIndexedMarkerSet.h>

#include <boost/variant.hpp>

namespace Sketcher3DGui {

struct EditData {
    EditData():
        EditRoot(0),
        PreselectPoint(-1),
        PreselectCurve(-1),
        PreselectCross(-1),
        PointsMaterials(0),
        CurvesMaterials(0),
        PointsCoordinate(0),
        CurvesCoordinate(0),
        CurveSet(0), EditCurveSet(0), RootCrossSet(0),
        PointSet(0)
    {}

    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;
    std::vector<int> CurvIdToGeoId; // conversion of SoLineSet index to GeoId

    int PreselectPoint;
    int PreselectCurve;
    int PreselectCross;
    
    // nodes for the visuals
    SoSeparator*   EditRoot;
    SoMaterial*    PointsMaterials;
    SoMaterial*    CurvesMaterials;
    SoMaterial*    RootCrossMaterials;
    SoMaterial*    EditCurvesMaterials;
    SoCoordinate3* PointsCoordinate;
    SoCoordinate3* CurvesCoordinate;
    SoCoordinate3* RootCrossCoordinate;
    SoCoordinate3* EditCurvesCoordinate;
    SoLineSet*     CurveSet;
    SoLineSet*     RootCrossSet;
    SoLineSet*     EditCurveSet;
    SoIndexedMarkerSet*   PointSet;

    SoText2*       textX;
    SoTranslation* textPos;

    SoGroup*       constrGroup;
};

class Sketcher3DGuiExport ViewProviderSketch3D : public PartGui::ViewProviderPart, public Gui::SelectionObserver {

    PROPERTY_HEADER(Sketcher3DGui::ViewProviderSketch3D);

public:
    ViewProviderSketch3D();
    virtual ~ViewProviderSketch3D();

    virtual void attach(App::DocumentObject*);
    virtual void updateData(const App::Property*);

    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);
    virtual bool onDelete(const std::vector<std::string> &);
    /// is called by the tree if the user double click on the object
    virtual bool doubleClicked(void);
    virtual bool mouseMove(const SbVec2s& pos, Gui::View3DInventorViewer* viewer);
    virtual bool keyPressed(bool pressed, int key);
    virtual bool mouseButtonPressed(int Button, bool pressed, const SbVec2s& pos,
                                    const Gui::View3DInventorViewer* viewer);

    /// Observer message from the Selection
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual void setEditViewer(Gui::View3DInventorViewer*, int ModNum);
    virtual void unsetEditViewer(Gui::View3DInventorViewer*);

private:
    EditData* edit;

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

    void draw();
    void createEditInventorNodes(void);
    void updateColor(void);
    
    Sketcher3D::Sketch3DObject* getSketch3DObject(void) const;
    
    // give projecting line of position
    void getProjectingLine(const SbVec2s&, const Gui::View3DInventorViewer *viewer, SbLine&) const;

    
    // helper to detect preselection
    bool detectPreselection(const SoPickedPoint *Point);

   
};

typedef Gui::ViewProviderPythonFeatureT<ViewProviderSketch3D> ViewProviderPython;

}


#endif // SKETCHER3DGUI_VIEWPROVIDERSKETCH3D_H
