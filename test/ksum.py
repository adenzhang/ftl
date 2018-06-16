#!/usr/bin/python

from copy import deepcopy
def ksum(A, k, s):
    """ Given array of distinct integers, select k elements which sums s.
    return number of solutions."""

    def ksum_sub(A, p, k, s, curr, solutions):
        if s == 0 and k == 0:
            solutions.append(deepcopy(curr))
            return 1
        if len(A) == p or s == 0 or k == 0:
            return 0
        if A[p] <= s:
            curr.append( A[p] )
            r0 = ksum_sub(A, p+1, k-1, s-A[p], curr, solutions)
            curr.pop()
            r1 = ksum_sub(A, p+1, k, s, curr, solutions)
            return r1+r0
        return 0

    curr = []
    solutions = []
    print "Solutions:", ksum_sub(A, 0, k, s, curr, solutions)
    print solutions
    return len(solutions)

if __name__ == '__main__':
    A = [1,2,3,4,5,6,7,8,9]
    ksum(A, 3, 15)
