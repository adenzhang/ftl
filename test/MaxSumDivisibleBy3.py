#!usr/bin/python

def maxSumDivisibleBy3(A):
    """ a is a list of positive integers".
        return max sum of subsequence that is divisible by 3
    """
    m = [0, 0, 0] # max sum with leftovers 0,1,2 divided by 3
    for x in A:
        r = x%3
        if r == 0 :
            m[0] += x
            m[1] += x
            m[2] += x
        elif r == 1:
            m0, m1, m2 = m[0], m[1], m[2]
            m[0] = max(m0, m2+x)
            m[1] = max(m1, m0+x)
            m[2] = max(m2, m1+x)
        else:
            m0, m1, m2 = m[0], m[1], m[2]
            m[0] = max(m0, m1+x)
            m[1] = max(m1, m2+x)
            m[2] = max(m2, m0+x)
    return m[0]

if __name__ == '__main__':
    print maxSumDivisibleBy3([3,4,1,1]) #

