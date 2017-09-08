/*
Implement regular expression matching with support for '.' and '*'.

'.' Matches any single character.
'*' Matches zero or more of the preceding element.

The matching should cover the entire input string (not partial).

The function prototype should be:
bool isMatch(const char *s, const char *p)

Some examples:
isMatch("aa","a") → false
isMatch("aa","aa") → true
isMatch("aaa","aa") → false
isMatch("aa", "a*") → true
isMatch("aa", ".*") → true
isMatch("ab", ".*") → true
isMatch("aab", "c*a*b") → true
*/
#include <ftl/container_serialization.h>
#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <stack>
#include <sstream>

using namespace std;

namespace RegExMatching {

class Solution {
public:
	inline bool isWild(const string& p, size_t p0) {
		return (p.length() > p0 + 1) && p[p0+1] == '*';// like ".*" or "a*"
	}
	bool nextWildMatch(const string& s, size_t& s0, const string& p, size_t p0) {
		// advance s0 to end of match
		if( s.length() <= s0 ) return false;
		if( p[p0] == '.' || s[s0] == p[p0] ) {
			s0 += 1;
			return true;
		}
		return false;
	}
	bool match(const string& s, size_t s0, const string& p, size_t p0) {
		if( s.length() <= s0 ) { // no more chars in s
			if( p.length() <= p0) {
				return true;
			}else{
				return isWild(p, p0) && match(s, s0, p, p0+2);
			}
		}
		if( p.length() <= p0 ) // no more chars in p
			return false;
		if( isWild(p, p0) ) {
			do{
				if( match(s, s0, p, p0+2) )
					return true;
			}while(nextWildMatch(s, s0, p, p0));
			return false;
		} else {
			return (p[p0] == '.' || s[s0] == p[p0]) ? match(s, s0+1, p, p0+1): false;
		}
	}
	bool isMatch(const string& s, const string& p) {
		return match(s, 0, p, 0);
	}
};

}

int main_RegExMatching()
{
	using namespace RegExMatching;
	Solution sln;
	cout << sln.isMatch("aa", "a") << endl; // false
	cout << sln.isMatch("aa", "aa") << endl; // true
	cout << sln.isMatch("aaa", "aa") << endl; // false
	cout << sln.isMatch("aa", "a*") << endl; // true
	cout << sln.isMatch("aa", ".*") << endl; // true
	cout << sln.isMatch("ab", ".*") << endl; // true
	cout << sln.isMatch("aab", "c*a*b") << endl; // true
	cout << sln.isMatch("a", "ab*a") << endl; // false
}
