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
#include "Tree.h"
#include "View.h"
#include "Document.h"

namespace Gui {

class View3DInventorViewer;

/**
 * @brief Manages all widgets which shall be overlayed over the 3D scene graph.
 *
 * This class is responsible for the layout, resizing and correct event
 * handling regarding mouse events for all overlay widgets.
 *
 * @author Stefan Tröger
 */
class GuiExport View3DInventorWidgetManager :  public QWidget
{
    Q_OBJECT

public:

    enum Position { TopLeft, TopRight, BottomLeft, BottomRight };

    View3DInventorWidgetManager(QWidget* parent, View3DInventorViewer* viewer);
    ~View3DInventorWidgetManager();

    void addWidget(QWidget* w, Position p);
    void removeWidget(QWidget* w);
    bool processEvent(QEvent* event);

private:
    QHBoxLayout* m_topLeft;
    QHBoxLayout* m_topRight;
    QHBoxLayout* m_bottomLeft;
    QHBoxLayout* m_bottomRight;
    QWidget*     m_parent;
    View3DInventorViewer* m_viewer;
};


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
    
   /** @name methods to overrride 
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

}

#endif // OVERLAYWIDGETMANAGER_H
