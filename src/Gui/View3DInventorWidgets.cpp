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

Gui::View3DInventorWidgetManager::View3DInventorWidgetManager(QWidget* parent, View3DInventorViewer* viewer)
    : QWidget(), m_parent(parent), m_viewer(viewer), m_child(NULL)
{
    //setup the base layout
    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    QVBoxLayout* leftHelperLayout = new QVBoxLayout();
    m_topLeft = new QHBoxLayout();
    QSpacerItem* tl_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_topLeft->addItem(tl_spacer);
    leftHelperLayout->addLayout(m_topLeft);
    m_bottomLeft = new QHBoxLayout();
    QSpacerItem* bl_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_bottomLeft->addItem(bl_spacer);
    leftHelperLayout->addLayout(m_bottomLeft);
    mainLayout->addLayout(leftHelperLayout);

    QVBoxLayout* rightHelperLayout = new QVBoxLayout();
    m_topRight = new QHBoxLayout();
    QSpacerItem* tr_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_topRight->addItem(tr_spacer);
    rightHelperLayout->addLayout(m_topRight);
    m_bottomRight = new QHBoxLayout();
    QSpacerItem* br_spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_bottomRight->addItem(br_spacer);
    rightHelperLayout->addLayout(m_bottomRight);
    mainLayout->addLayout(rightHelperLayout);

    //we need to be fully transparent, nobody wants to see the helper widget
    setAttribute(Qt::WA_TranslucentBackground, true);

    //set the correct position to ease the mouse position handling
    move(parent->mapToGlobal(QPoint(0,0)));
}

Gui::View3DInventorWidgetManager::~View3DInventorWidgetManager()
{

}

void Gui::View3DInventorWidgetManager::addWidget(QWidget* w, Gui::View3DInventorWidgetManager::Position p)
{
    //reparent the object to this widget to prevent any showing up of it in normal qt rendering
    w->setParent(this);

    //create sublayout and add spacer and widget
    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(w);

    if(p == TopLeft || p == TopRight)
        layout->insertStretch(-1);
    else
        layout->insertStretch(0);

    //add the sublayout at the correct position
    switch(p) {
    case TopLeft:
        m_topLeft->insertLayout(m_topLeft->count()-1,layout);
        break;

    case TopRight:
        m_topRight->insertLayout(1,layout);
        break;

    case BottomLeft:
        m_bottomLeft->insertLayout(m_bottomLeft->count()-1,layout);
        break;

    case BottomRight:
        m_bottomRight->insertLayout(1,layout);
        break;

    default:
        break;
    };
}

void Gui::View3DInventorWidgetManager::removeWidget(QWidget* w)
{

}

bool Gui::View3DInventorWidgetManager::processEvent(QEvent* event)
{
    if(event->type() == QEvent::Resize)  {
        resize(static_cast<QResizeEvent*>(event)->size());
        return false;
    }

    QMouseEvent* ev = static_cast<QMouseEvent*>(event);
    QWidget* child = this->childAt(ev->pos());

    if(event->type() == QEvent::Move)  {
        Base::Console().Message("Move\n");
        move(m_parent->mapToGlobal(static_cast<QMoveEvent*>(event)->pos()));
        return false;
    }
    else if(event->type() == QEvent::MouseButtonPress ||
            event->type() == QEvent::MouseButtonRelease ||
            event->type() == QEvent::MouseButtonDblClick ||
            event->type() == QEvent::MouseMove) {

        if(child) {
            //emulate the correct enter/leave events
            if(child != m_child) {
                if(m_child) {
                    QEvent e(QEvent::Leave);
                    qApp->sendEvent(m_child, &e);
                }

                QEvent e(QEvent::Enter);
                qApp->sendEvent(child, &e);
                m_child = child;
                m_viewer->scheduleRedraw();
            };

            //get the widget position
            QPoint cp = child->mapFrom(this, static_cast<QMouseEvent*>(event)->pos());

            QMouseEvent me(ev->type(), cp, cp+pos(), ev->button(), ev->buttons(), ev->modifiers());

            if(qApp->sendEvent(child, &me) && me.isAccepted()) {
                //we need to rerender the scene as the widget may have changed but it will only be
                //redrawn on full rerender
                m_viewer->scheduleRedraw();
                return true;
            };
        }
        else {
            //did we leave a child with the mouse?
            if(m_child) {
                QEvent e(QEvent::Leave);
                qApp->sendEvent(m_child, &e);
                m_child = NULL;
                m_viewer->scheduleRedraw();
                return true;
            }
        }
    };

    return false;
};

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
            event->type() == QEvent::MouseMove) {

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

