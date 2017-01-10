#include <iostream>
#include "SharedObjectPool.h"

using namespace std;
namespace {
struct A {
    int id;
    A(int id=0):id(id) {
        std::cout << "A()" << id << endl;
    }
    A(const A& a): id(a.id) {
        std::cout << "A(A)" << id << endl;
    }
    ~A() {
        std::cout << "~A()" << id << endl;
    }
};
typedef std::shared_ptr<A> APtr;

}

int main_shared_object_pool_test()
{
    using namespace ftl;
    typedef shared_object_pool<A> SharedAPool;

    struct my    {
        static APtr getObject() {
            SharedAPool::Ptr pool = SharedAPool::create(); //std::make_shared<SharedAPool>();
            static int count = 0;
            return pool->getObject(++count);
        }
    };
    APtr a;
    {
        APtr b, c;
        a = my::getObject();
        cout << "Got A:" << a->id << endl;
        b = a;
        c = my::getObject();
        a = c;
    }
}
