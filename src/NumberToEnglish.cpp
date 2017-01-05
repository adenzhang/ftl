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
static const string ONES[] = {"Zero", "One", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten"
        , "Eleven", "Twelve", "Thirteen", "Fourteen", "Fifteen", "Sixteen", "Seventeen", "Eighteen", "Nineteen", "Twenty"};
static const string TENS[] = {"Zero", "Ten", "Twenty", "Thirty", "Forty", "Fifty", "Sixty", "Seventy", "Eighty", "Ninety", "Hundred"};

static const string MM[] = {"", "Thousand", "Million", "Billion"};

class Solution {
public:


    int inThousand(stringstream& ss, int n) {
        int H = n/100, T, O; // Hundreds, Tens, Ones
        int k = 0 ; // return digit count
        if( H ) {
            ss << ONES[H] << " " << TENS[10];
            ++k;
        }
        n %= 100;
        if( n <= 20 && n > 0) {
            if( k++ ) ss << " ";
            ss << ONES[n];
            return k;
        }
        T = n/10;
        O = n%10;
        if( T ) {
            if( k++ ) ss << " ";
            ss << TENS[T];
        }
        if( O ) {
            if( k++ ) ss << " ";
            ss << ONES[O];
        }
        return k;
    }
    string numberToWords(int num) {
        if( !num ) {
            return ONES[0];
        }

        stringstream ss;
        int m = 1000;
        for(int i=3, km = 1000000000; i>=0; --i, km/=m ) { // from Billions, to Ones
            int t = num/km;
            if( t ) {
                if( ss.tellp() ) ss << " ";
                inThousand(ss, t);
                if( i > 0) {
                    ss << " " << MM[i];
                }
            }
            num %= km;
        }
        return ss.str();
    }
};

int main()
{
    Solution sln;
//    cout << sln.numberToWords(0) << endl;
//    cout << sln.numberToWords(1) << endl;
//    cout << sln.numberToWords(10) << endl;
//    cout << sln.numberToWords(11) << endl;
//    cout << sln.numberToWords(20) << endl;
    cout << "[" << sln.numberToWords(12345) << "]"<< endl;
    cout << "[" << sln.numberToWords(200220113) << "]"<< endl;
}
