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
#include "ContainerSerialization.h"
using namespace std;
using namespace serialization;

typedef vector<int> IntVector;
typedef unordered_map<int, int> IntIntHashMap;
typedef map<int, int> IntIntTreeMap;


int main() {
	IntVector v{1, 2, 3};

	IntIntHashMap m{{10, 1}, {20, 2}};

	cout << "v:" << sqrt(5, 9) << endl;
	cout << "m:" << sqrt(-4.5)<< endl;
}
