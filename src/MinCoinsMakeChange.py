#!usr/bin/python

def min_coins_make_change(coins, A):
    """ given values of coins, return mininum number of coins required to
        make amount S.
        E.g. Amount: 5, coins = [1,2,3], result = 2 ( 3+2=5)
    """
    N = len(coins)
    if A <= 0 or N == 0: return 0;

    dp = [s/coins[0] if s%coins[0] == 0 else 0 for s in range(A+1)]
    print "initial dp:", dp
    for x in coins[1:]:
        tdp = [0]*(A+1)
        for j in reversed(range(A+1)):
            k = 0; m = j/x if j%x == 0 else 0
            while j-k*x > 0:
                if dp[j-k*x] > 0 and (m == 0 or m > dp[j-k*x]+k) : m = dp[j-k*x]+k
                k += 1
            tdp[j] = m
        dp = tdp
        print dp
    return dp[A]

def test_min_coins_make_change():
    c = [1,2,3]
    print "expected 2 :", min_coins_make_change(c, 5)

if __name__ == '__main__':
    test_min_coins_make_change()