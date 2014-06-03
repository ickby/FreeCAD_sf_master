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

#include "PreCompiled.h"
#include "View3DInventorWidgets.h"
#include "Application.h"
#include "View3DInventorViewer.h"
#include "Command.h"
#include <Base/Console.h>
#include <QEvent>
#include <QResizeEvent>
#include <qapplication.h>
#include <QScrollArea>
#include <QPainter>

Gui::View3DInventorTreeWidget::View3DInventorTreeWidget(Gui::Document* doc) : TreeWidget(NULL, true), Gui::BaseView(doc)
{
    //Visual aspects needed for 3D view overlay
    setAttribute(Qt::WA_TranslucentBackground, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Active, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Inactive, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255,255,255,0));
    setPalette(palette);

    //tweak the tree a bit
    this->setHeaderHidden(true);
    this->setFrameShape(QFrame::NoFrame);
    this->setMinimumWidth(300);

    //ensure we get maximal possible vertical space
    QSizePolicy p(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    p.setVerticalStretch(10);
    this->setSizePolicy(p);

    //ensure the active document is shown
    slotNewDocument(*doc);
}

bool Gui::View3DInventorTreeWidget::viewportEvent(QEvent* event)
{
    //check if we hit the background and therefore don't want the event
    if(event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::QEvent::MouseButtonDblClick ||
            event->type() == QEvent::Wheel ||
            event->type() == QEvent::MouseMove ||
	    event->type() == QEvent::ContextMenu) {

        QModelIndex index = indexAt(static_cast<QMouseEvent*>(event)->pos());

        if(!index.isValid()) {
            event->ignore();
            return false;
        }
    };

    return Gui::TreeWidget::viewportEvent(event);
}


Gui::View3DInventorCommandWidget::View3DInventorCommandWidget(QWidget* parent): QToolBar(parent)
{
    CommandManager& mgr = Application::Instance->commandManager();
    mgr.addTo("Std_ViewFitAll", this);
    mgr.addTo("Std_DrawStyle", this);
    this->addSeparator();
    mgr.addTo("Std_ViewAxo", this);
    this->addSeparator();
    mgr.addTo("Std_ViewFront", this);
    mgr.addTo("Std_ViewTop", this);
    mgr.addTo("Std_ViewRight", this);
    this->addSeparator();
    mgr.addTo("Std_ViewRear", this);
    mgr.addTo("Std_ViewBottom", this);
    mgr.addTo("Std_ViewLeft", this);

};

Gui::View3DInventorPropertyWidget::View3DInventorPropertyWidget(QWidget* parent): PropertyView(parent)
{
    setMinimumWidth(200);
    //ensure we get maximal possible horizontal space
    QSizePolicy p(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    this->setSizePolicy(p);

    //draw nice and transperant
    setAttribute(Qt::WA_TranslucentBackground, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Active, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Inactive, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255,255,255,0));
    setPalette(palette);
}

Gui::View3dInventorPythonWidget::View3dInventorPythonWidget(QWidget* parent)
{
    setMinimumHeight(300);
    setMinimumWidth(500);
    //ensure we get maximal possible horizontal space
    QSizePolicy p(QSizePolicy::Minimum, QSizePolicy::Minimum);
    p.setHorizontalStretch(10);
    setSizePolicy(p);

    //draw nice and transperant
    setAttribute(Qt::WA_TranslucentBackground, true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Active, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Inactive, QPalette::Base, QColor(255,255,255,0));
    palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255,255,255,0));
    setPalette(palette);
}



#include "moc_View3DInventorWidgets.cpp"

