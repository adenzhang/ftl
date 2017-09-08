#ifndef _SHAREDOBJECTPOOL_H_
#define _SHAREDOBJECTPOOL_H_
#include <memory>
#include <deque>

namespace ftl {

template<typename T>
class safe_enable_shared_from_this : public std::enable_shared_from_this<T>
{
    public:
    template<typename... _Args>
        static ::std::shared_ptr<T> create(_Args&&... p_args) {
            return ::std::allocate_shared<T>(Alloc(), std::forward<_Args>(p_args)...);
        }

    protected:
    struct Alloc : std::allocator<T>
    {
        template<typename _Up, typename... _Args>
        void construct(_Up* __p, _Args&&... __args)
        { ::new((void *)__p) _Up(std::forward<_Args>(__args)...); }
    };
    safe_enable_shared_from_this() = default;
    safe_enable_shared_from_this(const safe_enable_shared_from_this&) = delete;
    safe_enable_shared_from_this& operator=(const safe_enable_shared_from_this&) = delete;
};

template<typename Object>
class shared_object_pool: public safe_enable_shared_from_this<shared_object_pool<Object> >
{
public: // should be protected
    shared_object_pool(): allocatedCount(0){}
    friend struct safe_enable_shared_from_this<shared_object_pool<Object> >::Alloc;

protected:
    typedef std::deque<Object*> ObjectQ;
    ObjectQ freeQ;
    size_t allocatedCount;
public:
    typedef std::shared_ptr<Object> shared_object_ptr;
    typedef std::shared_ptr<shared_object_pool> Ptr;

    struct return_object{
        Ptr pPool;  // ensure pool existence
        return_object(const Ptr& p): pPool(p) {}
        return_object(const return_object& d): pPool(d.pPool) {
            std::cout << "return_object(return_object&)" << std::endl;
        }
        void operator()(Object *p) {
            // return it to freeQ
            { // todo lock
                pPool->freeQ.push_back(p);
            }
            std::cout << "delete Object: allocated:" << pPool->allocatedCount << ", free size:" << pPool->freeQ.size() << std::endl;
        }
    };

    ~shared_object_pool() {
        std::cout << "~shared_object_pool, allocated size:" << allocatedCount << " == freeQ size:" << freeQ.size() << std::endl;
        { // todo lock
            std::default_delete<Object> del;
            for(typename ObjectQ::iterator it=freeQ.begin(); it != freeQ.end(); ++it) {
                del(*it);
            }
        }
    }

//    template<typename Arg>
    template<typename... Args>
    shared_object_ptr getObject(Args&&... args) {
        // todo lock
        if( !freeQ.empty() ) {
            Object *p = freeQ.front();
            freeQ.pop_front();

            new (p) Object(std::forward<Args>(args)...);
            shared_object_ptr ptr(p, return_object(me()));
            return ptr;
        }else{
            // if freeQ empty
            ++allocatedCount;
            return shared_object_ptr(new Object(std::forward<Args>(args)...), return_object(me()));
        }
    }

    size_t purge() {
        std::default_delete<Object> del;
        { // todo lock
            size_t n = freeQ.size();
            while(!freeQ.empty()) {
                del(freeQ.front);
                freeQ.pop_front();
            }
            return n;
        }
    }

    Ptr me() {
        return this->shared_from_this();
    }

};

}
#endif // _SHAREDOBJECTPOOL_H_
