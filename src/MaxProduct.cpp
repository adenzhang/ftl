#include <algorithm>
#include <numeric>
#include <iostream>

class Solution0 {
public:

    int maxProduct(int A[], int n) {
		if( n<=0 ) return 0;
		
		//-- search for 0's
		for(int i=0; i<n; ++i) {
			if( A[i] == 0 )
				return std::max(0, std::max(maxProduct(A, i), maxProduct(A+i+1, n-i-1)));
		}
		
		int preS = 1, postS = 1, mS = 1;
		int iFirstNeg=-1, iLastNeg=-1;
		//-- look for first negative
        for(int i=0; i<n; ++i) {
			int x = A[i];
			if( x > 0 ) {
				// not found first negative
				preS *= x;
			}else if( x < 0 ) {
				iFirstNeg = i;
				break;
			}
		}
		//-- look for last negative
		for(int i=n-1; i>=0; --i) {
			int x = A[i];
			if( x > 0 ) {
				// not found last negative
				postS *= x;
			}else if( x < 0 ) {
				iLastNeg = i;
				break;
			}
		}
		
		//-- calculate inner mS
		
		if( iFirstNeg <0 )  // all are positives
			return std::accumulate(A, A+n, 1, std::multiplies<int>());
		if( iFirstNeg == iLastNeg)  { // only one negative
			return n > 1 ? std::max( preS, postS) : A[iFirstNeg];
		}
			
			
		mS = std::accumulate(A+iFirstNeg, A+iLastNeg+1, 1, std::multiplies<int>());
		if( mS > 0 )
			return mS * preS * postS; 
		int mS1 = mS/A[iLastNeg] * preS;
		int mS2 = mS/A[iFirstNeg] * postS;
		
		return std::max(mS1, mS2);
    }
};
class Solution {
public:

    int maxProduct(int A[], int n) {
		if (n == 0) {
			return 0;
		}

		int maxherepre = A[0];
		int minherepre = A[0];
		int maxsofar = A[0];
		int maxhere, minhere;

		for (int i = 1; i < n; i++) {
			maxhere = std::max(std::max(maxherepre * A[i], minherepre * A[i]), A[i]);
			minhere = std::min(std::min(maxherepre * A[i], minherepre * A[i]), A[i]);
			maxsofar = std::max(maxhere, maxsofar);
			maxherepre = maxhere;
			minherepre = minhere;
		}
		return maxsofar;
   	}
};
int main_MaxProduct()
{
	int A[] = {-1,0,2,1};
	Solution sln;
	std::cout << sln.maxProduct(A, sizeof(A)/sizeof(int)) << std::endl;
	return 0;
}
