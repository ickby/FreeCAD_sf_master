/*   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2013    *
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



#ifndef SOLVER_H
#define SOLVER_H

#include <opendcm/core.hpp>
#include <opendcm/module3d.hpp>

#include <App/Property.h>
#include <Mod/Part/App/Geometry.h>

namespace dcm {

struct point_accessor {
    Base::Vector3d point;

    template<typename Scalar, int ID, typename T>
    Scalar get(T& t) {
        return t->getPoint()[ID];
    };
    template<typename Scalar, int ID, typename T>
    void set(Scalar value, T& t) {
        point[ID] = value;
    };
    template<typename T>
    void finalize(T& t) {
        t->setPoint(point);
    };
};

template<>
struct geometry_traits< Part::GeomPoint* > {
    typedef tag::point3D  tag;
    typedef modell::XYZ modell;
    typedef point_accessor accessor;
};

} //dcm

//custom module to allow the use as property
struct ModuleFreeCAD {

    template<typename Sys>
    struct type {

        struct inheriter : public App::Property {

            Base::Type getClassTypeId(void) { return classTypeId; }; 
	    Base::Type getTypeId(void) const { return classTypeId; }; 
	    Base::Type classTypeId;  
	    void * create(void){
	      return new Sys ();
	    }	    
	    void init(void){
	      initSubclass(classTypeId, "Solver", "App::Property", &(Sys::create) ); 
	    }
	    //just needs a set value method for the macros, but we actually wont set values as we use the 
	    //dcm interface
	    void setValue(int) {};
	    void initChange() { aboutToSetValue(); };
	    void finishChange() { hasSetValue(); };
	    
	    
	    inheriter() {
	      m_this = (Sys*)this;
	      classTypeId = Base::Type::badType();
	    };

            // from base class
            virtual PyObject* getPyObject(void) {};
            virtual void setPyObject(PyObject*) {};

            virtual void Save(Base::Writer& writer) const {};
            virtual void Restore(Base::XMLReader& reader) {};

            virtual Property* Copy(void) const {
		return m_this->clone();
	    };
            virtual void Paste(const App::Property& from) {
	      
		const Sys& s = dynamic_cast<const Sys&>(from);
		m_this->clear();
		s.copyInto(*m_this);
	    };

            virtual unsigned int getMemSize(void) const {return 1;};
	    
	protected:
	    Sys* m_this;
        };

        typedef mpl::vector0<> objects;
        typedef mpl::vector0<> properties;
	typedef mpl::vector0<> geometries;
        typedef dcm::Unspecified_Identifier Identifier;

        static void system_init(Sys& sys) {};
        static void system_copy(const Sys& from, Sys& into) {};
    };
};

namespace Sketcher3D {

typedef dcm::Kernel<double> Kernel;
typedef dcm::Module3D< mpl::vector<Part::GeomPoint*>, int > Module3D;
typedef dcm::System<Kernel, Module3D, ModuleFreeCAD> Solver;


typedef typename Module3D::type<Solver>::Geometry3D Geometry3D;
typedef boost::shared_ptr<Geometry3D> Geom3D_Ptr;
//typedef typename dcm::system_traits<Solver>::getModule<dcm::m3d>::type::Constraint3D Constraint3D;

} //sketcher

#endif //SOLVER_H
