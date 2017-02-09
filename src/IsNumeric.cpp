#include <string.h>
#include <ctype.h>
#include <iostream>
using namespace std;

class Solution {
public:
    const char* FindNonSpace(const char* s, const char *end=NULL) {
        for(const char* p=s; *p != '\0' || (end!=NULL && s!=end); ++p) {
            if( !isspace(*p) ) return p;
        }
        return NULL;
    }
    const char* FindSpace(const char* s, const char *end=NULL) {
        const char* p = s;
        for(; *p != '\0' || (end!=NULL && s!=end); ++p) {
            if( isspace(*p) ) return p;
        }
        return p;
    }
    const char* FindCharICase(const char *s, char ch, const char *end=NULL) {
        ch = toupper(ch);
        for(const char* p=s; *p != '\0' || (end!=NULL && s!=end); ++p) {
            if( toupper(*p) == ch ) return p;
        }
        return NULL;
    }

    // trimmed string: 123, +23.7, -234.8342
    bool isFloat(const char *s, const char*end=NULL) {
        if( !s ) return false;
        if( *s == '-' || *s == '+') {   // skip sign
            s++;
        }
        if( !isdigit(*s) ) return false; // begin with digit
        int bDot = 0;
        for(const char* p=s; *p != '\0' || (end!=NULL && s!=end); ++p) {
            if( isdigit(*p) ) continue;
            else if( *p == '.' ) {
                if( ++bDot > 1 ) return false;
                if( !isdigit(p[1]) ) return false;
            }else{  // other char
                return false;
            }
        }
        return true;
    }

    bool isInt(const char *s, const char*end=NULL) {
        if( !s ) return false;
        if( *s == '-' || *s == '+') {   // skip sign
            s++;
        }
        if( !isdigit(*s) ) return false; // begin with digit
        for(const char* p=s; *p != '\0' || (end!=NULL && s!=end); ++p) {
            if( isdigit(*p) ) continue;
            else{  // other char
                return false;
            }
        }
        return true;
    }
    bool isNumber(const char *s) {
        const char *p = FindNonSpace(s);
        if( p==NULL ) return false;
        const char *q = FindSpace(p);
        if( FindNonSpace(q) ) return false;

        const char *e = FindCharICase(p, 'e');
        if( e ) {
            if( !isInt(e+1, q) ) return false;
            q = e;
        }
        if( !isFloat(p, q) ) return false;

        return true;
    }
};

int main() //_IsNumeric()
{
    Solution sln;
    cout << sln.isNumber(NULL) << endl;
    cout << sln.isNumber("") << endl;
    cout << sln.isNumber("  ") << endl;
    cout << sln.isNumber(" 1 ") << endl;
    cout << sln.isNumber(" -231 ") << endl;
    cout << sln.isNumber(" +22.2 ") << endl;
    cout << sln.isNumber(" +32.2e23 ") << endl;
    cout << sln.isNumber(" +32.2e-23 ") << endl;
    cout << "---" << endl;
    cout << sln.isNumber(" +32.2e ") << endl;
    cout << sln.isNumber(" +32.2e 3") << endl;
    cout << sln.isNumber(" - 3") << endl;
    cout << sln.isNumber(" 3 3") << endl;
    cout << sln.isNumber(" 3+3") << endl;
    cout << sln.isNumber(" 3e3.") << endl;
    cout << sln.isNumber(" .3 ") << endl;
    cout << sln.isNumber(" 3. ") << endl;
    return 0;
}
