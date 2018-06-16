#include <string>
#include <iostream>
#include <stack>

#include <stdio.h>

using namespace std;

class Solution {
public:
    int FindNonSpace(const string& s, int startpos=0) {
        for(int i=startpos; i<s.length(); ++i) {
            if( !isspace(s[i]) ) return i;
        }
        return -1;
    }
    int FindSpace(const string& s, int startpos=0) {
        for(int i=startpos; i<s.length(); ++i) {
            if( isspace(s[i]) ) return i;
        }
        return s.length();
    }

    void reverseWords(string &s) {
        stack<string> sstack;
        int p=0, q=0;
        do{
            p = FindNonSpace(s, q);
            if(p == -1)
                break;
            q = FindSpace(s, p);
            sstack.push(s.substr(p, q-p));
        }while( q<s.length() );

        s = "";
        int i=0;
        while(!sstack.empty()) {
            if(i>0)
                s += " ";
            s += sstack.top();
            sstack.pop();
            ++i;
        }
    }
};

int main_reverseWords()
{
    string s;
    Solution sln;
    sln.reverseWords(s="");
    printf("[%s]\n", s.c_str());

    sln.reverseWords(s=" ");
    printf("[%s]\n", s.c_str());

    sln.reverseWords(s="1");
    printf("[%s]\n", s.c_str());

    sln.reverseWords(s="  abc \n 123  ");
    printf("[%s]\n", s.c_str());


    sln.reverseWords(s="  abc 123  6 ");
    printf("[%s]\n", s.c_str());

    return 0;
}
