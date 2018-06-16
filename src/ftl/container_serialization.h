/*
 * ContainerSerialization.h
 *
 * Serialize stl containers in JSON format
 *  Created on: May 26, 2016
 *      Author: aden
 */

#ifndef CONTAINERSERIALIZATION_H_
#define CONTAINERSERIALIZATION_H_
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>
#include <deque>
#include <stack>
#include <list>
#include <set>

namespace ftl{

template <class TempIO, class IOType>
struct scoped_stream_redirect {
    std::streambuf* rdbuf;
    IOType& io;

    scoped_stream_redirect() = delete;
    scoped_stream_redirect(const scoped_stream_redirect&) = delete;
    scoped_stream_redirect(scoped_stream_redirect&& a)
        : rdbuf(a.rdbuf)
        , io(a.io)
    {
    }

    scoped_stream_redirect(TempIO& ss, IOType& io)
        : rdbuf(io.rdbuf())
        , io(io)
    {
        io.rdbuf(ss.rdbuf());
    }
    ~scoped_stream_redirect()
    {
        io.rdbuf(rdbuf);
    }
};

template <class TempIO, class IOType>
scoped_stream_redirect<TempIO, IOType> make_scoped_stream_redirect(TempIO& ss, IOType& io)
{
    return { ss, io };
};


namespace serialization {

inline std::ostream& operator<<(std::ostream& os, const std::string& s) {
	return os << '\"' << s << '\"';
}
template<typename List>
std::ostream& printList(std::ostream& os, const List& v) {
	os << "[";
	for(typename List::const_iterator it2=v.begin(); it2!=v.end(); ++it2) {
		if( it2 == v.begin() ) {
			os << *it2;
		}else{
			os << "," << *it2;
		}
	}
	os << "]";
	return os;
}
template<typename Map>
std::ostream& printMap(std::ostream& os, const Map& v) {
	os << "{";
	for(typename Map::const_iterator it2=v.begin(); it2!=v.end(); ++it2) {
		if( it2 == v.begin() ) {
			os << it2->first <<":" << it2->second;
		}else{
			os << "," << it2->first <<":" << it2->second;
		}
	}
	os << "}";
	return os;
}
template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::map<Key,Value>& v) {
	return printMap(os, v);
}
template<typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<Key, Value>& v) {
	return printMap(os, v);
}
template<typename Elm>
std::ostream& operator<<(std::ostream& os, const std::vector<Elm>& v) {
	return printList(os, v);
}
template<typename Elm>
std::ostream& operator<<(std::ostream& os, const std::list<Elm>& v) {
	return printList(os, v);
}
template<typename Elm>
std::ostream& operator<<(std::ostream& os, const std::set<Elm>& v) {
	return printList(os, v);
}

//=========== read ============================
// return number of chars skipped
inline int skipSpace(std::istream& is) {
	int count=0;
	for(; std::isspace(is.peek()); ++count, is.ignore() ) ;
	return count;
}
inline std::istream& operator>>(std::istream& is, std::string& s) {
	skipSpace(is);
	if( '\"' != is.peek() ) return is;
	is.ignore();
	int prev=0;
	while(is) {
		int c = is.get();
		if( '\"' == c && '\"' != prev) {
			return is;
		}else if( EOF == c){
			return is;
		}else{
			s += char(c);
			prev = c;
		}
	}
	return is;
}
template<typename List>
std::istream& readList(std::istream& is, List& alist) {
	skipSpace(is);
	if( '[' != is.peek() ) {
		std::cerr << "error reading list: expected starting '['" << std::endl;
		return is; //error
	}
	is.ignore();
	while(is) {
		skipSpace(is);
		int x = is.peek();
		if( ']' == x ) {
			is.ignore();
			return is;
		}else if( ',' == x ){
			is.ignore();
		}else if( EOF == x){
			std::cerr << "error reading list: expected ending ']'" << std::endl;
			return is; // error
		}else{
			typename List::value_type e;
			is >> e;
			alist.push_back(e);
		}
	}
	std::cerr << "error reading list: expected ending ']'" << std::endl;
	return is; //error
}
template<typename Map>
std::istream& readMap(std::istream& is, Map& amap) {
	skipSpace(is);
	if( '{' != is.peek() ) return is; // error
	is.ignore();
	while(is) {
		skipSpace(is);
		int x = is.peek();
		if( '}' == x ) {
			is.ignore();
			return is;
		}else if( ',' == x ){
			is.ignore();
		}else if( EOF == x ) {
			std::cerr << "error reading list: expected ending '}'" << std::endl;
			return is;
		}else{
			typename Map::key_type key;
			typename Map::mapped_type value;
			is >> key;
			skipSpace(is);
			if(is.get() != ':'){
				std::cerr << "error reading map: expected ':'" << std::endl;
				return is; // error
			}
			skipSpace(is);
			is >> value;
			amap.insert(std::make_pair(key,value));
		}
	}
	std::cerr << "error reading map: expected ending '}'" << std::endl;
	return is; // error
}
template<typename Key, typename Value>
std::istream& operator>>(std::istream& os, std::map<Key,Value>& v) {
	return readMap(os, v);
}
template<typename Key, typename Value>
std::istream& operator>>(std::istream& os, std::unordered_map<Key, Value>& v) {
	return readMap(os, v);
}
template<typename Elm>
std::istream& operator>>(std::istream& os, std::vector<Elm>& v) {
	return readList(os, v);
}
template<typename Elm>
std::istream& operator>>(std::istream& os, std::list<Elm>& v) {
	return readList(os, v);
}
template<typename Elm>
std::istream& operator>>(std::istream& os, std::set<Elm>& v) {
	return readList(os, v);
}

} }


#endif /* CONTAINERSERIALIZATION_H_ */
