define unary_helper( => integer)
{
    return 10
}

define unary_call_checker(a: integer => integer)
{
    return 10
}

var a = 0, b = 0, c = 0

a = unary_call_checker(!!0 + !!0)
b = -10 + --10 + -a + --a + unary_helper() + -unary_helper()
b = b + !!0

var lsi = [10]
c = !lsi[0]

if (b == 0 && c == 0) == 0:
    print("Failed.\n")
