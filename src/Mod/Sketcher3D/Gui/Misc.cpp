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

#include "Misc.h"

std::string Sketcher3DGui::sketchIdAsString(const Sketcher3D::SketchIdentifier& id) {

    std::stringstream stream;
    switch(id.first) {

    case Sketcher3D::None:
        stream << "Nothing";
        return stream.str();
    case Sketcher3D::Point:
        stream << "Point";
        break;
    case Sketcher3D::Curve:
        stream << "Curve";
        break;
    case Sketcher3D::Surface:
        stream << "Surface";
        break;
    case Sketcher3D::Constraint:
        stream << "Constraint";
        break;
    default:
        stream << "None";
        return stream.str();
    };

    stream << id.second;
    return stream.str();
};
