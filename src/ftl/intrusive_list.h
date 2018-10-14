#pragma once
#include <cassert>
#include <cstddef>
#include <ftl/gen_itertor.h>

namespace ftl {

template <typename T>
struct ObjectAsListNode {
    template <typename NodeT>
    static constexpr std::enable_if_t<sizeof(T) >= sizeof(NodeT), NodeT&> get_node(T& obj)
    {
        return reinterpret_cast<NodeT&>(obj);
    }

    template <typename NodeT>
    static constexpr std::enable_if_t<sizeof(T) >= sizeof(NodeT), T&> get_obj(NodeT& n)
    {
        return reinterpret_cast<T&>(n);
    }
};

template <typename T, typename MemberType, MemberType T::*memberT>
struct MemberAsListNode {
    using obj_member_type = MemberType T::*;
    static const obj_member_type pMember = memberT;
    static const size_t member_offset = reinterpret_cast<size_t>(&reinterpret_cast<T*>(0).*memberT);

    template <typename NodeT>
    static constexpr std::enable_if_t<sizeof(T) >= sizeof(NodeT), NodeT&> get_node(T& obj)
    {
        return reinterpret_cast<NodeT>(obj.*pMember);
    }
    template <typename NodeT>
    static constexpr std::enable_if_t<sizeof(T) >= sizeof(NodeT), T&> get_obj(NodeT& n)
    {
        return *reinterpret_cast<T*>(reinterpret_cast<size_t>(&n) - member_offset);
    }
};

//! intrusive type unsafe simple_list
//!    any type can be pushed / popped.
template <typename T, class NodeGetterT = ObjectAsListNode<T>>
struct intrusive_singly_list {
    using this_type = intrusive_singly_list;
    struct data_type {
        data_type* next = nullptr;

        void reset()
        {
            next = nullptr;
        }
    };
    using value_type = data_type;
    using node_type = data_type*;

    node_type mHead = nullptr;

    //- stack interface

    this_type& push_front(T& n)
    {
        assert(end() == find(n));
        NodeGetterT::template get_node<value_type>(n).next = mHead;
        mHead = &reinterpret_cast<value_type&>(n);
        return *this;
    }

    T& front()
    {
        return NodeGetterT::get_obj(*mHead);
    }
    T& top()
    {
        return front();
    }

    bool empty() const
    {
        return !mHead;
    }

    size_t size() const
    {
        size_t n = 0;
        for (auto p = mHead; p; next(p), ++n)
            ;
        return n;
    }

    T& pop_front()
    {
        node_type r = mHead;
        if (r)
            mHead = r->next;
        return NodeGetterT::get_obj(*r);
    }

    this_type& push(T& n)
    {
        return push_front(n);
    }
    T& pop()
    {
        return pop_front();
    }

    void clear()
    {
        mHead = nullptr;
    }

    //- iterator support

    using iterator = ftl::gen_iterator<this_type, false>;
    using const_iterator = ftl::gen_iterator<this_type, true>;

    bool is_end(const node_type& n) const
    {
        return !n;
    }
    bool next(node_type& n) const
    {
        if (!n)
            return false;
        n = n->next;
        return true;
    }
    bool prev(node_type& node) const
    {
        if (!node)
            return false;
        if (mHead == node)
            return false;
        node_type prev = find_if([&](node_type& n) { return n->next == node; });
        if (!prev) {
            node = prev;
            return true;
        }
        return false;
    }
    value_type* get_value(const node_type& n) const
    {
        return const_cast<node_type&>(n);
    }
    iterator begin()
    {
        return { mHead, this };
    }
    iterator end()
    {
        return { node_type(), this };
    }
    const_iterator cbegin() const
    {
        return { mHead, this };
    }
    const_iterator cend() const
    {
        return { node_type(), this };
    }

    //- modifiable by iterator

    iterator find(const T& value)
    {
        return iterator{ find_if([&](node_type n) { return &NodeGetterT::template get_node<value_type>(const_cast<T&>(value)) == n; }), this };
    }

    void erase(iterator it)
    {
        remove(it.get_node());
    }
    iterator insert_after(iterator it, value_type& n)
    {
        return { insert_after(it.get_node(), n), this };
    }

    //- internal interface

    template <typename F>
    node_type find_if(F&& func) const
    {
        for (auto p = mHead; p; p = p->next)
            if (func(p))
                return p;
        return nullptr;
    }

    node_type insert_after(node_type pos, value_type& d)
    {
        d.next = pos->next;
        pos->next = &d;
        return &d;
    }

    void remove_after(node_type node)
    {
        if (node->next) {
            auto p = node->next;
            node->next = node->next->next;
            p->reset();
        }
    }

    void remove(node_type node)
    {
        if (!node)
            return;
        if (node == mHead) {
            mHead = node->next;
            node->reset();
            return;
        } else {
            node_type prev = find_if([&](node_type& n) { return n->next == node; });
            remove_after(prev);
        }
    }
};
}
