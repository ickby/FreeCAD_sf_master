/***************************************************************************
 *   Copyright (c) 2014 StefanTroeger <stefantroeger@gmx.net>              *
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

import QtQuick 1.1
import FreeCADLib 1.0

InterfaceItem {
 
    id: item 
    
    property alias proxy: proxyitem.proxy    
    property alias contentWidth:  proxyitem.width 
    property alias contentHeight: proxyitem.height
    
    Proxy {
        id: proxyitem
        objectName: "proxy"
        anchors.fill: parent;
    }    
    
//     proxyitem.onProxyChanged: {
//         console.debug("proxy changed");
//         minWidth  = proxyitem.proxy.minimumWidth;
//         minHeight = proxyitem.proxy.minimumHeight;
//         //force a parent recompute
//         item.height = item.height;
//     }
}