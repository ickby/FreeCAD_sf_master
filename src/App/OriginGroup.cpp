/***************************************************************************
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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

#include "PreCompiled.h"
#include "OriginGroup.h"

#ifndef _PreComp_
#endif

#include <Base/Exception.h>

#include <App/Document.h>
#include "Origin.h"

#include "GeoFeature.h"

using namespace App;

PROPERTY_SOURCE(App::OriginGroupExtension, App::GeoFeatureGroupExtension);

OriginGroupExtension::OriginGroupExtension () {
    
    initExtension(OriginGroupExtension::getClassTypeId());
    
    ADD_PROPERTY_TYPE ( Origin, (0), 0, App::Prop_Hidden, "Origin linked to the group" );
}

OriginGroupExtension::~OriginGroupExtension ()
{ }

App::Origin *OriginGroupExtension::getOrigin () const {
    App::DocumentObject *originObj = Origin.getValue ();

    if ( !originObj ) {
        std::stringstream err;
        err << "Can't find Origin for \"" << getExtendedObject()->getNameInDocument () << "\"";
        throw Base::Exception ( err.str().c_str () );

    } else if (! originObj->isDerivedFrom ( App::Origin::getClassTypeId() ) ) {
        std::stringstream err;
        err << "Bad object \"" << originObj->getNameInDocument () << "\"(" << originObj->getTypeId().getName()
            << ") linked to the Origin of \"" << getExtendedObject()->getNameInDocument () << "\"";
        throw Base::Exception ( err.str().c_str () );
    } else {
            return static_cast<App::Origin *> ( originObj );
    }
}

App::DocumentObject *OriginGroupExtension::getGroupOfObject (const DocumentObject* obj, bool indirect) {
    const Document* doc = obj->getDocument();
    std::vector<DocumentObject*> grps = doc->getObjectsWithExtension ( OriginGroupExtension::getClassTypeId() );
    for (auto grpObj: grps) {
        OriginGroupExtension* grp = dynamic_cast <OriginGroupExtension* >(grpObj->getExtension(OriginGroupExtension::getClassTypeId()));
        if ( indirect ) {
            if ( grp->geoHasObject (obj) ) {
                return dynamic_cast<App::DocumentObject*>(grp);
            }
        } else {
            if ( grp->hasObject (obj) ) {
                return dynamic_cast<App::DocumentObject*>(grp);
            }
        }
    }

    return 0;
}

short OriginGroupExtension::extensionMustExecute() const {
    if (Origin.isTouched ()) {
        return 1;
    } else {
        return GeoFeatureGroupExtension::extensionMustExecute();
    }
}

App::DocumentObjectExecReturn *OriginGroupExtension::extensionExecute() {
    try { // try to find all base axis and planes in the origin
        getOrigin ();
    } catch (const Base::Exception &ex) {
        //getExtendedObject()->setError ();
        return new App::DocumentObjectExecReturn ( ex.what () );
    }

    return GeoFeatureGroupExtension::extensionExecute ();
}

void OriginGroupExtension::onExtendedSetupObject () {
    App::Document *doc = getExtendedObject()->getDocument ();

    std::string objName = std::string ( getExtendedObject()->getNameInDocument()).append ( "Origin" );

    App::DocumentObject *originObj = doc->addObject ( "App::Origin", objName.c_str () );

    assert ( originObj && originObj->isDerivedFrom ( App::Origin::getClassTypeId () ) );
    Origin.setValue (originObj);

    GeoFeatureGroupExtension::onExtendedSetupObject ();
}

void OriginGroupExtension::onExtendedUnsetupObject () {
    App::DocumentObject *origin = Origin.getValue ();
    if (origin && !origin->isDeleting ()) {
        origin->getDocument ()->remObject (origin->getNameInDocument());
    }

    GeoFeatureGroupExtension::onExtendedUnsetupObject ();
}