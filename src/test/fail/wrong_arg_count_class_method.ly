###
SyntaxError: Function string::concat expects 2 args, but got 4.
Where: File "test/fail/wrong_arg_count_class_method.ly" at line 8
###

var s: string = ""

s.concat(1, 2, 3)
