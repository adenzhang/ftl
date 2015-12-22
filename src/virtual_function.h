#pragma once

#include <functional>
#include <iostream>

namespace vf {

    struct Base {
        Base(int id=0)
            : id(id)
        {}

        virtual void printMe() {
            std::cout << "Base::printMe {id:" << id << "}" << std::endl;
        }

        void printId() {
            std::cout << "Base::printId {id:" << id << "}" << std::endl;
        }
        int id;
    };

    struct Derived: public Base {
        Derived(int id=0)
            : Base(id)
        {}
        virtual void printMe() {
            std::cout << "Derived::printMe {id:" << id << "}" << std::endl;
        }
        virtual void printId() {
            std::cout << "Derived::printId {id:" << id << "}" << std::endl;
        }
    };

    struct DDerived: public Derived {
        DDerived(int id=0)
            : Derived(id)
        {}
        virtual void printMe() {
            std::cout << "DDerived::printMe {id:" << id << "}" << std::endl;
        }
        virtual void printId() {
            std::cout << "DDerived::printId {id:" << id << "}" << std::endl;
        }
    };

    typedef std::function<void ()> VoidFunc;

    void call(const VoidFunc& f);
        
} // namespace vf
