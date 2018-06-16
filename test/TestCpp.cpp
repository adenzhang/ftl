#include <list>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <stack>
#include <sstream>
#include <memory>

using namespace std;


int totalScore(std::vector<std::string> blocks, int n)
{
    typedef std::vector<std::string> SVec;
    typedef deque<int> IQue;
    int T = 0;
    IQue lastV;
    int t = 0;
    for(int i=0;i < n; ++i) {
        string& s = blocks[i];
        switch( s[0] ) {
        case 'X':
            lastV.push_back(lastV.back() *2);
            T += lastV.back();
            break;
        case '+':
            lastV.push_back( lastV.back() + lastV[lastV.size()-2]);
            T += lastV.back();
            break;
        case 'Z':
            T -= lastV.back();
            lastV.pop_back();
            break;
        default:
            lastV.push_back(atoi(s.c_str()));
            T += lastV.back();
        }
    }
    return T;
}
// FUNCTION SIGNATURE ENDS

using namespace std;
int main_testcpp(int argn, const char** argv)
{
    std::vector<std::string> blocks={"1", "2", "+", "Z"};//{"5", "-2", "4", "Z", "X", "9", "+", "+"};
    std::cout << "Begin TestCpp!" << totalScore(blocks, blocks.size())<< std::endl;
    return 0;
}

