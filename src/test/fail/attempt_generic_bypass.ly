###
SyntaxError: Function f, argument #2 is invalid:
Expected Type: A
Received Type: integer
Where: File "test/fail/attempt_generic_bypass.ly" at line 17
###

function f[A](A v1, A v2) {  }

function g[A](A v1) {
	# It's tempting to use signature == signature comparison to dodge generic
	# matching. However, this demonstrates a case where that is a bad idea.
	# In this case, the first argument is A and A is wanted. However, because
	# the wanted value is a generic, f's A is now g's A.
	# If generic matching isn't done, then this will succeed because it will
	# think that 10 is the first thing to resolve f's A (and it isn't).
	f(v1, 10)
}