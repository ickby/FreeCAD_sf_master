/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#ifndef _PreComp_
#   include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Property.h"
#include "PropertyContainer.h"
#include "DocumentObject.h"
#include "Document.h"
#include <Base/Console.h>
#include <Base/Reader.h>
#include "DocumentObjectPy.h"

using namespace App;


//**************************************************************************
//**************************************************************************
// Property
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::Property , Base::Persistence);

//**************************************************************************
// Construction/Destruction

// here the implemataion! description should take place in the header file!
Property::Property()
    :father(0)
{

}

Property::~Property()
{

}

const char* Property::getName(void) const
{
    return father->getPropertyName(this);
}

short Property::getType(void) const
{
    return father->getPropertyType(this);
}

const char* Property::getGroup(void) const
{
    return father->getPropertyGroup(this);
}

const char* Property::getDocumentation(void) const
{
    return father->getPropertyDocumentation(this);
}

void Property::setContainer(PropertyContainer* Father)
{
    father = Father;
}

void Property::touch()
{
    if(father)
        father->onChanged(this);

    StatusBits.set(0);
}

void Property::hasSetValue(void)
{
    if(father)
        father->onChanged(this);

    StatusBits.set(0);
}

void Property::aboutToSetValue(void)
{
    if(father)
        father->onBeforeChange(this);
}

Property* Property::Copy(void) const
{
    // have to be reimplemented by a subclass!
    assert(0);
    return 0;
}

void Property::Paste(const Property& /*from*/)
{
    // have to be reimplemented by a subclass!
    assert(0);
}

std::string Property::encodeAttribute(const std::string& str) const
{
    std::string tmp;

    for(std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if(*it == '<')
            tmp += "&lt;";
        else if(*it == '"')
            tmp += "&quot;";
        else if(*it == '&')
            tmp += "&amp;";
        else if(*it == '>')
            tmp += "&gt;";
        else if(*it == '\r')
            tmp += "&#xD;";
        else if(*it == '\n')
            tmp += "&#xA;";
        else
            tmp += *it;
    }

    return tmp;
}

//**************************************************************************
//**************************************************************************
// PropertyLists
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLists , App::Property);

//**************************************************************************
//**************************************************************************
// Linkable Properties
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::LinkableProperty , App::Property);

LinkableProperty::LinkableProperty() : _pcLink(0), _pcLinkProperty(0)
{

}

LinkableProperty::~LinkableProperty()
{

}

void LinkableProperty::setLink(App::DocumentObject* o , Property* p)
{
    aboutToSetValue();
    _pcLink=o;
    _pcLinkProperty=p;
    hasSetValue();
}

App::DocumentObject* LinkableProperty::getLink(void) const
{
    return _pcLink;
}

App::DocumentObject* LinkableProperty::getLink(Base::Type t) const
{
    return (_pcLink && _pcLink->getTypeId().isDerivedFrom(t)) ? _pcLink : 0;
}

Property* LinkableProperty::getLinkedProperty(void) const
{
    return _pcLinkProperty;
}

Property* LinkableProperty::getLinkedProperty(Base::Type t) const
{
    return (_pcLinkProperty && _pcLinkProperty->getTypeId().isDerivedFrom(t)) ? _pcLinkProperty : 0;
}


PyObject* LinkableProperty::getPyObject(void)
{
    Py::Tuple tup(2);

    if(_pcLink && _pcLinkProperty) {
        tup[0] = Py::Object(_pcLink->getPyObject());
        tup[1] = Py::Object(Py::String(_pcLinkProperty->getName()));
        return Py::new_reference_to(tup);
    }
    else {
        return Py::new_reference_to(Py::None());
    }
}

bool LinkableProperty::setPythonObject(PyObject* value)
{
    if(PyTuple_Check(value) || PyList_Check(value)) {
        Py::Sequence seq(value);

        if(PyObject_TypeCheck(seq[0].ptr(), &(DocumentObjectPy::Type))) {
            DocumentObjectPy*  pcObj = (DocumentObjectPy*)seq[0].ptr();

            if(seq[1].isString()) {
                DocumentObject* doc = pcObj->getDocumentObjectPtr();
                setLink(doc, doc->getPropertyByName(((std::string)Py::String(seq[1])).c_str()));
                return true;
            }
            else {
                std::string error = std::string("type of second element in tuple must be a string, not ");
                error += seq[0].ptr()->ob_type->tp_name;
                throw Base::TypeError(error);
            }
        }
        else {
            std::string error = std::string("type of first element in tuple must be 'DocumentObject', not ");
            error += seq[0].ptr()->ob_type->tp_name;
            throw Base::TypeError(error);
        }
    }
    else if(Py_None == value) {
        setLink(0, 0);
        return true;
    }
    else 
        return false;
}

void LinkableProperty::CopyLinkInto(LinkableProperty* p) const
{
    p->_pcLink = _pcLink;
    p->_pcLinkProperty = _pcLinkProperty;
}


void LinkableProperty::Restore(Base::XMLReader& reader)
{
    // read link element if existing
    reader.readElement();
    if(strcmp(reader.localName(), "LinkProperty")==0) {

        // get the values of the document name (if there is one)
        if(reader.hasAttribute("value")) {
            std::string doc = reader.getAttribute("value");

            reader.readElement("Property");
            std::string prop = reader.getAttribute("value");

            DocumentObject* pcObject;
            App::Document* document = static_cast<DocumentObject*>(getContainer())->getDocument();
            pcObject = document ? document->getObject(doc.c_str()) : 0;
            Property* property = pcObject->getPropertyByName(prop.c_str());

            if(!pcObject || !property)
                Base::Console().Warning("Lost link to '%s' while loading, maybe "
                                        "an object was not loaded correctly\n", doc.c_str());

            setLink(pcObject, property);
        }
        else {
            setLink(0,0);
        }
        reader.readEndElement("LinkProperty");
    }
    else {
        setLink(0,0);
    };
}

void LinkableProperty::Save(Base::Writer& writer) const
{
    // it can happen that the object is still alive but is not part of the document anymore and thus
    // returns 0
    writer.Stream() << writer.ind() << "<LinkProperty";

    if(_pcLink && _pcLinkProperty) {
        writer.Stream() << " value=\"" <<  _pcLink->getNameInDocument() <<"\">" << std::endl;
        writer.incInd();
        writer.Stream() << writer.ind() << "<Property value=\"" << _pcLinkProperty->getName()<<"\"/>" << std::endl;
        writer.decInd();
        writer.Stream() << writer.ind() << "</LinkProperty>" << std::endl ;
    }
    else {
        writer.Stream() << "></LinkProperty>" << std::endl;
    }
}
