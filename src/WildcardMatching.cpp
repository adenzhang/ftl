/*
'?' Matches any single character.
'*' Matches any sequence of characters (including the empty sequence).

The matching should cover the entire input string (not partial).
The matching is greedy!
*/

#include <iostream>
#include <vector>
#include <initializer_list>
#include <unordered_map>
#include <assert.h>
#include <string.h>

using namespace std;

class Solution {
public:
    bool isMatch(const char *str, const char *pat) {
        int i, star;

     new_segment:

        star = 0;
        if (*pat == '*') {
           star = 1;
           do { pat++; } while (*pat == '*'); /* enddo */
        } /* endif */

     test_match:

        for (i = 0; pat[i] && (pat[i] != '*'); i++) {
           if (str[i] != pat[i]) { // if (mapCaseTable[str[i]] != mapCaseTable[pat[i]]) {
              if (!str[i]) return 0;
              if ((pat[i] == '?') && (str[i] != '.')) continue;
              if (!star) return 0;
              str++;
              goto test_match;
           }
        }
        if (pat[i] == '*') {
           str += i;
           pat += i;
           goto new_segment;
        }
        if (!str[i]) return 1;
        if (i && pat[i - 1] == '*') return 1;
        if (!star) return 0;
        str++;
        goto test_match;
    }
};


#define RUNTEST sln.isMatch
static void test() {
    Solution sln;
    assert( RUNTEST("as df", "*") );
    assert( RUNTEST("asdf", "?*") );
    assert( RUNTEST("a", "?*") );
    assert( RUNTEST("a", "*a") );
    assert( RUNTEST( 	"ho", "ho**") );

    assert( ! RUNTEST("aa", "a") );
    assert( ! RUNTEST("aa", "a**b") );

    assert( RUNTEST("abefcdgiescdfimde", "ab*cd?i**de") );
    assert( ! RUNTEST(
    "babbabbabaaaaabaabaaaaabbabaabbbaaaabbaabbbbbaabbabaabbbbaabbbabaabbaaabbbbabbbabbababaababbaaaaaabaabababbbaaabbaaaaaabbbabbbbabaabaaaabbabbaabaababbaaaababaaaaababbbaabaababbbaaabaababbabbabaaabbbbaa"
                , "*a*ab*b*ab*a*bb**bb**a**abb*bb*ab*a*bbbb***ba***aa**ba*b*ab*ba***aaabbbaa*ba*ba*b****baabb**b*aa*aaab*a"
        ) );
}

int main() {
    test();
    cout<< "The end." << endl;
    return 0;
}
