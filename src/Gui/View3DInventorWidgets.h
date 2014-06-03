/*
 * This file is part of the FreeCAD CAx development system.
 * Copyright (C) 2014  Stefan Tröger <stefantroeger@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef GUI_OVERLAYWIDGETMANAGER_H
#define GUI_OVERLAYWIDGETMANAGER_H

#include <QWidget>
#include <QHBoxLayout>
#include <QToolBar>
#include "Tree.h"
#include "View.h"
#include "Document.h"
#include "PropertyView.h"
#include "PythonConsole.h"

namespace Gui {

class View3DInventorViewer;

/**
 * @brief FreeCAD's tree widget adopted for 3D overlay purposes
 *
 * It derives from the standart TreeWidget and has therefore the exact same behaviour, it just
 * changes a view visual aspects (e.g. transperent background). Furthermore we need to implement
 * the BaseView methods to get attached to the document framework. 
 *
 * @author Stefan Tröger
 */
class GuiExport View3DInventorTreeWidget : public Gui::TreeWidget, public BaseView {

public:
    View3DInventorTreeWidget(Gui::Document* doc);
    
protected:
    virtual bool viewportEvent(QEvent* event);
    //virtual void drawRow(QPainter* painter, const QStyleOptionViewItem & option, const QModelIndex& index) const;
    
   /** @name methods to overrride from BaseView
   */
  //@{
  /// get called when the document is updated
  virtual void onUpdate(void){}
  /// returns the name of the view (important for messages)
  virtual const char *getName(void) const { return "DockWindow"; }
  /// Message handler
  virtual bool onMsg(const char* pMsg,const char** ppReturn){ return false; }
  /// Message handler test
  virtual bool onHasMsg(const char* pMsg) const { return false; }
  /// overwrite when checking on close state
  virtual bool canClose(void){return true;}
  //@}
};

class GuiExport View3DInventorCommandWidget : public QToolBar {

public:
    View3DInventorCommandWidget(QWidget* parent = 0);
};

class GuiExport View3DInventorPropertyWidget : public Gui::PropertyView {

public:
    View3DInventorPropertyWidget(QWidget* parent = 0);
};

class GuiExport View3dInventorPythonWidget : public Gui::PythonConsole {
public:
    View3dInventorPythonWidget(QWidget* parent = 0);
};

}
#endif // OVERLAYWIDGETMANAGER_H
