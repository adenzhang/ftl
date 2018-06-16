/*
here are N children standing in a line. Each child is assigned a rating value.

You are giving candies to these children subjected to the following requirements:

    Each child must have at least one candy.
    Children with a higher rating get more candies than their neighbors.

What is the minimum candies you must give?
*/

#include <iostream>
#include <vector>
#include <memory>

using namespace std;

struct Comparator {
    int asc;
    explicit Comparator( int b, int a) :asc( b-a) {
        //~ int d = second-first;
        //~ asc = d>0? 1: (d==0?0:(-1));
    }
    Comparator(): asc(1){}
    int operator()(int b, int a) {
        int d= b-a;
        //~ d = d>0? 1: (d==0?0:(-1));
        if( asc < 0 ) d = -d;
        return d;
    }
};

class Solution {
    
public:
    double findMedianSortedArrays(int A[], int m, int B[], int n) {
        Comparator compA(A[m-1], A[0]);
        Comparator compB(B[n-1], B[0]);
        int ascB = compA.asc * compB.asc < 0? (-1):1;
        int iM = (m+n)/2+1; // count
        int iA=0, iB=ascB==1? 0:(n-1);
        int k=0, preV=0, lastV=0;
        do{
            while(k < iM && iA < m && (iB <0 || iB >=n || compA(A[iA], B[iB]) <=0) ) {
				preV = lastV;
				lastV = A[iA];
				++iA;
				++k;
			}
            while(k < iM && iB < n && iB>=0 && (iA>=m || compA(B[iB], A[iA]) <=0) ) {
				preV = lastV;
				lastV = B[iB];
				iB += ascB;
				++k;
			}
		}while(k<iM);
		if( (m+n)%2 == 0 )
			return (preV+lastV)/2.0;
		else
			return lastV;
    }
};

static void  test() {
	Solution sln;
//    int A[] = {1,2,3};
//    int B[] = {1,2};

        int A[] = {2,3,5,6,7};
        int B[] = {1,4};

    cout << sln.findMedianSortedArrays(A, sizeof(A)/sizeof(int), B, sizeof(B)/sizeof(int)) << endl;
}
//=========== test cpp 
template <typename Ptr, typename T, typename... Args>
Ptr newObject(Args... args) {
	return Ptr(new T(args...));
};
template <typename Ptr, typename T>
Ptr newObjects(size_t n) {
	return Ptr(new T[n]);
};
struct Obj {
	int id;
	Obj() {
		printf("default constuctor\n");
	}
	Obj(int x, int b) {
		printf("param constuctor\n");
	}
	Obj(const Obj& a) {
		printf("copy constructor\n");
	}
	Obj& operator=(const Obj& a) {
		printf("assignment\n");
		return *this;
	}
};
//~ typedef shared_ptr<Obj> ObjPtr;
typedef Obj* ObjPtr;
void testcpp() {
	ObjPtr a(new Obj(1,2));
	ObjPtr b(new Obj());
	
	a->id = 1;
	b->id = 2;
}
int main_median() {
	testcpp();
    //~ test();

    cout<< "The end." << endl;
    return 0;
}
