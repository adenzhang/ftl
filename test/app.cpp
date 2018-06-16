#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <cmath>
#include <ctime>
#include <climits>
#include <deque>
#include <queue>
#include <stack>
#include <bitset>
#include <cstdio>
#include <limits>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ftl/container_serialization.h>

#define CATCH_CONFIG_MAIN
#include "ftl/catch.hpp"

using namespace std;
using namespace ftl::serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

struct A {
	virtual void f() {}
};
struct B: virtual A {
	virtual void f() {}
};
struct C: virtual A {
	virtual void f() {}
};
struct D: B, C  {
	virtual void f() {}
};

class String {
public:
	explicit String(char ){}
	String(const char *){}
private:
	void operator=(const char*){}
};
int main_app() {

	cout << "v:" << sqrt(5.9) << endl;
	cout << "m:" << sqrt(-4.5)<< endl;
}
