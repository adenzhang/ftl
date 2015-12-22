#include <virtual_function.h>

void test_virtual_function()
{
    vf::Base    b(1);
    vf::Derived d(2);
    vf::DDerived dd(3);

    vf::Base   *pb2 = &d, *pb3 = &dd;
    vf::Derived *pd2 = &d, *pd3 = &dd;

    vf::call( std::bind(&vf::Base::printMe, b) );
//    vf::call( std::bind(&vf::Derived::printMe, b) ); // compiling error
    vf::call( std::bind(&vf::Base::printMe, d) );
    vf::call( std::bind(&vf::Derived::printMe, d) );

    //- test virtual function overrides non-virtual function
    std::cout << "--- virtual function overrides non-virtual function: bind" << std::endl;
    vf::call( std::bind(&vf::Base::printId, b) );
    vf::call( std::bind(&vf::Base::printId, d) );
    vf::call( std::bind(&vf::Derived::printId, d) );
    vf::call( std::bind(&vf::Base::printId, dd) );
    vf::call( std::bind(&vf::Derived::printId, dd) );
    vf::call( std::bind(&vf::DDerived::printId, dd) );

    std::cout << "--- virtual function overrides non-virtual function: directly call" << std::endl;
    
    pb2->printId();
    pb3->printId();
    pd2->printId();
    pd3->printId();
}

int main(int argn, const char** argv)
{
    std::cout << "Begin TestCpp!" << std::endl;
    test_virtual_function();
    return 0;
}
