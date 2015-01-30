var ok = 1

# First a test of tuple literals.
var t: tuple[integer, string, list[integer]] = <[1, "1", [1]]>

# The last should default to any.
var t2: tuple[integer, integer, any] = <[11, 21, 31]>

# Make sure that t[1] yields the proper sig at emit time by forcing a call
# to string::concat after it.
var s: string = t[1].concat("1")

# Check tuple assignment too...
t2[0] = 12
if t2[0] != 12:
    ok = 0

if ok == 0:
    print("Failed.\n")
