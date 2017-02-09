#!/usr/bin/python

def jump_steps(N):
    """ jump up N steps. each jump up 1 or 2 steps. Count number of ways.
    """
    def jump(steps, N):
        """ retur number of sucess
        """
        if N == 0 :
            print steps
            return 1

        n2 = 0
        if N > 1 :
            steps.append(2)
            n2 = jump(steps, N-2)
            steps.pop()
        steps.append(1)
        n1 = jump(steps, N-1)
        steps.pop()
        return n1+n2
    steps = []
    nw = jump(steps, N)
    print "number of ways:", nw

if __name__ == '__main__':
    jump_steps(5)