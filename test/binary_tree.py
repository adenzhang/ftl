#!/usr/bin/python

class BinaryTree:

    def __init__(self, data=None):
        self.left = None
        self.right = None
        self.data = data

    def addLeft(self, left):
        self.left = left

    def addRight(self, right):
        self.right = right

    def preOrderTraverse(self, func, depth=0, siblingIndex=0):
        func(self, depth, siblingIndex)
        if self.left: self.left.preOrderTraverse(func, depth+1, 0)
        if self.right: self.right.preOrderTraverse(func, depth+1, 1)

    def inOrderTraverse(self, func, depth=0, siblingIndex=0):
        if self.left: self.left.inOrderTraverse(func, depth+1, 0)
        func(self, depth, siblingIndex)
        if self.right: self.right.inOrderTraverse(func, depth+1, 1)

    def postOrderTraverse(self, func, depth=0, siblingIndex=0):
        if self.left: self.left.postOrderTraverse(func, depth+1, 0)
        if self.right: self.right.postOrderTraverse(func, depth+1, 1)
        func(self, depth, siblingIndex)

    # return func(currentNode, leftResult rightResult)
    def postOrderJoinTraverse(self, func, depth=0, siblingIndex=0):
        leftResult = self.left.postOrderJoinTraverse(func, depth+1, 0) if self.left else None
        rightResult = self.right.postOrderJoinTraverse(func, depth+1, 1) if self.right else None
        return func(self, leftResult, rightResult, depth, siblingIndex)

    def toListPreOrder(self):
        if self.data == None: return []
        r = [self.data]
        if self.left: r += self.left.toListPreOrder()
        if self.right: r += self.right.toListPreOrder()
        return r

    def toListInOrder(self):
        if self.data == None: return []
        r = []
        if self.left: r += self.left.toListInOrder()
        r.append(self.data)
        if self.right: r += self.right.toListInOrder()
        return r

    def toListPostOrder(self):
        if self.data == None: return []
        r = []
        if self.left: r += self.left.toListPostOrder()
        if self.right: r += self.right.toListPostOrder()
        r.append(self.data)
        return r

    def prettyPrint(self):
        width = {} # {node:(leftWidth, rightWidth)}
        def visit(self, width, node):
            LL = width[node.left](0) if node.left else 0
            LR = width[node.left](1) if node.left else 0
            RL = width[node.right](0) if node.right else 0
            RR = width[node.right](1) if node.right else 0



    @staticmethod
    def fromLists(preOrd, inOrd):
        def toTree(preOrd, p0, p1, inOrd, i0, i1):
            if p0 > p1 or i0 > i1 : return None
            root = BinaryTree(preOrd[p0])
            try:
                iroot = inOrd.index(root.data)
            except ValueError as e :
                print "Failed of find ", root, "in", ord, "from", i0, "to", i1
                return None
            if iroot > i1:
                print "Failed of find ", root, "in", ord, "from", i0, "to", i1
                return None
            root.addLeft(toTree(preOrd, p0+1, p0+iroot-i0, inOrd, i0, iroot-1))
            root.addRight(toTree(preOrd, p0+iroot-i0+1, p1, inOrd, iroot+1, i1))
            return root
        return toTree(preOrd, 0, len(preOrd)-1, inOrd, 0, len(inOrd)-1)

if __name__ == '__main__':
    preOrd = [4,2,1,3,8,6,7,9,10]
    inOrd = [1,2,3,4,6,7,8,9,10]
    root = BinaryTree.fromLists(preOrd, inOrd)
    print "preOrdTree:", root.toListPreOrder()
    print "inOrdTree:", root.toListInOrder()
