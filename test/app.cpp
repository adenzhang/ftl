
#define CATCH_CONFIG_MAIN
#include <ftl/catch_or_ignore.h>

#include <algorithm>
#include <bitset>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <fstream>
#include <ftl/container_serialization.h>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace ftl::serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;

struct A {
    virtual void f() {}
};
struct B : virtual A {
    virtual void f() {}
};
struct C : virtual A {
    virtual void f() {}
};
struct D : B, C {
    virtual void f() {}
};

class String {
public:
    explicit String(char) {}
    String(const char*) {}

private:
    void operator=(const char*) {}
};
int main_app()
{

    cout << "v:" << sqrt(5.9) << endl;
    cout << "m:" << sqrt(-4.5) << endl;
    return 0;
}
