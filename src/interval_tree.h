#ifndef INTERVAL_TREE_H
#define INTERVAL_TREE_H

#include <utility>
#include <binary_search_tree.h>

namespace ftl {

// {start: <end, max>}
template <typename ValueType, typename KeyCompare=std::less<ValueType> >
class interval_tree: public binary_search_tree< ValueType, std::pair<ValueType, ValueType> >
{
public:
	typedef binary_search_tree< ValueType, std::pair<ValueType, ValueType> > SuperType;
	typedef typename SuperType::Node Node;
	typedef typename SuperType::iterator iterator;
	typedef typename SuperType::value_type value_type;

	typedef ValueType KeyType;
	typedef std::pair<ValueType, ValueType> interval_type;

public:
//	std::pair<value_type, bool> insert(const interval_type& aInterval) {
//	}

};

}
#endif
