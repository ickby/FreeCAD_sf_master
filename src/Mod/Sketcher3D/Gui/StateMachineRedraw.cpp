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

#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Base/Console.h>
#include <Inventor/nodes/SoMarkerSet.h>
#include "Misc.h"

using namespace Sketcher3DGui;

//draw functor
struct drawer : boost::static_visitor<void> {

    SketchMachine& data;
    Sketcher3D::Geom3D_Ptr  geometry;
    Sketcher3D::Shape3D_Ptr shape;
    drawer(SketchMachine& d) : data(d) {
        data.CurveIdMap.clear();
        data.PointIdMap.clear();
        data.CurvesCoordinate->point.setValue(0);
        data.PointsCoordinate->point.setValue(0);
        data.CurveSet->numVertices.setValue(0);
    };

    void drawPoint(const Base::Vector3d& point, Sketcher3D::SketchIdentifier id, bool Construction) {

        int pidx = data.PointsCoordinate->point.getNum();
        data.PointsCoordinate->point.set1Value(pidx, point[0], point[1], point[2]);

        //let's see how we want to color it
        if(std::find(data.Selection.begin(), data.Selection.end(), id) != data.Selection.end())
            data.PointsMaterials->diffuseColor.set1Value(pidx, data.SelectColor); //normal color
        else if(data.Preselect == id)
            data.PointsMaterials->diffuseColor.set1Value(pidx, data.PreselectColor); //normal color
        else if(Construction)
            data.PointsMaterials->diffuseColor.set1Value(pidx, data.CurveDraftColor); //normal color
        else
            data.PointsMaterials->diffuseColor.set1Value(pidx, data.VertexColor); //normal color

        //store the id for this point
        data.PointIdMap.push_back(id);
    };

    void operator()(Part::GeomPoint* const& p) {

        const Base::Vector3d point = p->getPoint();
        drawPoint(point, geometry->getIdentifier(), false);
    };

    void operator()(Part::GeomLineSegment* const& p) {

        const Base::Vector3d point1 = p->getStartPoint();
        const Base::Vector3d point2 = p->getEndPoint();

        drawPoint(point1, shape->geometry(dcm::startpoint)->getIdentifier(), false);
        drawPoint(point2, shape->geometry(dcm::endpoint)->getIdentifier(), false);

        int cidx = data.CurvesCoordinate->point.getNum();
        data.CurvesCoordinate->point.set1Value(cidx, point1[0], point1[1], point1[2]);
        data.CurvesCoordinate->point.set1Value(cidx+1, point2[0], point2[1], point2[2]);

        //store the real id for the incremental indexed lines
        Sketcher3D::SketchIdentifier line = shape->geometry(dcm::line)->getIdentifier();
        data.CurveIdMap.push_back(line);

        int lidx = data.CurveSet->numVertices.getNum();
        data.CurveSet->numVertices.set1Value(lidx, 2);

        //let's see how we want to color it
        if(std::find(data.Selection.begin(), data.Selection.end(), line) != data.Selection.end())
            data.CurvesMaterials->diffuseColor.set1Value(lidx-1, data.SelectColor);
        else if(data.Preselect == line)
            data.CurvesMaterials->diffuseColor.set1Value(lidx-1, data.PreselectColor);
        //else if(Construction)
        //    data.PointsMaterials->diffuseColor.set1Value(lidx-1, data.CurveDraftColor);
        else
            data.CurvesMaterials->diffuseColor.set1Value(lidx-1, data.CurveColor);
    };

    //default implementation
    template<typename T>
    void operator()(const T& p) {};
};

sc::result EditMode::react(const EvRedraw& event) {

    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();

    SketchMachine& machine = outermost_context();

    // set cross coordinates
    machine.RootCrossSet->numVertices.set1Value(0,2);
    machine.RootCrossSet->numVertices.set1Value(1,2);
    machine.RootCrossSet->numVertices.set1Value(2,2);
    machine.RootCrossCoordinate->point.set1Value(0,SbVec3f(-10, 0.0f, 0.0f));
    machine.RootCrossCoordinate->point.set1Value(1,SbVec3f(10, 0.0f, 0.0f));
    machine.RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -10, 0.0f));
    machine.RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, 10, 0.0f));
    machine.RootCrossCoordinate->point.set1Value(4,SbVec3f(0.0f, 0.0f, -10));
    machine.RootCrossCoordinate->point.set1Value(5,SbVec3f(0.0f, 0.0f, 10));

    //draw geometry
    machine.CurveIdMap.clear();
    drawer draw(machine);
    Sketcher3D::Solver& solver = machine.Object->m_solver;
    typedef typename std::vector< Sketcher3D::Geom3D_Ptr >::iterator iter;
    for(iter it = solver.begin<Sketcher3D::Geometry3D>(); it != solver.end<Sketcher3D::Geometry3D>(); it++) {

        Sketcher3D::Geom3D_Ptr ptr = *it;
        if(ptr->holdsType()) {
            draw.geometry = ptr;
            ptr->apply(draw);
        };
    };
    //draw shapes
    typedef typename std::vector< Sketcher3D::Shape3D_Ptr >::iterator siter;
    for(siter it = solver.begin<Sketcher3D::Shape3D>(); it != solver.end<Sketcher3D::Shape3D>(); it++) {

        Sketcher3D::Shape3D_Ptr ptr = *it;
        if(ptr->holdsType()) {
            draw.shape = *it;
            ptr->apply(draw);
        };
    };

    if(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        static_cast<Gui::View3DInventor*>(mdi)->getViewer()->render();
    }

    return discard_event();
};

