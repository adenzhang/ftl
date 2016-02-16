"""
http://www.lintcode.com/en/problem/sliding-window-maximum/

Given an array of n integer with duplicate number, and a moving window(size k), move the window at each iteration from the start of the array, find the maximum number inside the window at each moving.

Example
For array [1, 2, 7, 7, 8], moving window size k = 3. return [7, 7, 8]
At first the window is at the start of the array like this
[|1, 2, 7| ,7, 8] , return the maximum 7;
then the window move one step forward.
[1, |2, 7 ,7|, 8], return the maximum 7;
then the window move one step forward again.
[1, 2, |7, 7, 8|], return the maximum 8;

Challenge
o(n) time and O(k) memory
"""
import operator

class PosValue:
    def __init__(self, pos, value):
        self.pos = pos
        self.value = value
    def __str__(self):
        return "(%d, %d)"%(self.pos, self.value)
        
class Solution:
    """
    @param nums: A list of integers.
    @return: The maximum number inside the window at each moving.
    
    naive implementation
    """
    def maxSlidingWindow(self, nums, k):
        # write your code here
        #- construct Q
        if len(nums) < k:
            return [max(*nums)]
            
        q = nums[:k]
        m = [max(*q)]
#        iq = sorted(range(len(q)), key=lambda i: q[i]) # sorted index
        for i in xrange(len(nums)-k):
            v = nums[i+k]
            #-- remove head
            del q[0]
            q.append(v)
            m.append(max(*q))
        return m
        
    def test(self):
        x = [1,2,7,7,8]
        y = self.maxSlidingWindow(x, 6)
        print y
        
    
    
if __name__ == "__main__":
    print "hello"
    sln = Solution()
    sln.test()