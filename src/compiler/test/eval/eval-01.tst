/*
    Test eval()
 */

eval("new Date")

//  Eval evaluates scripts into this interpreter

public var x = 2
eval("x = 3")
assert(x == 3)

//  Test eval expression return

assert(eval("5 + 4") == 9)
assert(eval("x = 2") == 2)


//  Test internal vars
var y = 7
eval("var y = 8")
assert(y == 8)
