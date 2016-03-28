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


#ifndef GUI_GUIFREEINVENTOR_H
#define GUI_GUIFREEINVENTOR_H

#include "View.h"
#include "View3DInventorViewer.h"
#include <Base/Parameter.h>


namespace Gui {

class GUIfreeInventorViewer;
class GUIfreeInventorViewPy;

/**
* @brief An Inventor view that works without QWidgets
* This view provides full access to the scene graph without depending on any qt widgets. Hence 
* ths viewer can be used in the GUIless mode to build up the document scene graph and access it 
* from python. 
* @author Stefan Tröger
*/
class GuiExport GUIfreeInventorView : public InventorView, public ParameterGrp::ObserverType
{
    TYPESYSTEM_HEADER();

public:
    GUIfreeInventorView(Gui::Document* pcDocument);
    virtual ~GUIfreeInventorView();

    /// Message handler
    virtual bool onMsg(const char* pMsg, const char** ppReturn);
    virtual bool onHasMsg(const char* pMsg) const;
    /// Observer message from the ParameterGrp
    virtual void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason);
    /// get called when the document is updated
    virtual void onRename(Gui::Document *pDoc);
    virtual const char *getName(void) const;
    /// delete itself
    virtual void deleteSelf();

    virtual PyObject* getPyObject(void);
    
    InventorViewer* getInventorViewer(void);

protected:
    /// handle to the viewer parameter group
    ParameterGrp::handle hGrp;

private:
    GUIfreeInventorViewer * _viewer;
    PyObject *_viewerPy;

    // friends
    //friend class GuifreeInventorViewPy;
};

class GuiExport GUIfreeInventorViewer : public InventorViewer, public Gui::SelectionSingleton::ObserverType {
    
public:
    SoSeparator* getSceneGraph() {return pcViewProviderRoot;}
    /// Observer message from the Selection
    void OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                          Gui::SelectionSingleton::MessageType Reason) {};
};

} // namespace Gui

#endif  // GUI_GUIFREEINVENTOR_H

