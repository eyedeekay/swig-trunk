import return_const_value
import sys

p = return_const_value.FooPtr.getPtr()
if (p.getVal() != 17):
	print "Runtime test1 faild. p.getVal()=", p.getVal()
	sys.exit(1)

p = return_const_value.FooPtr.getConstPtr()
if (p.getVal() != 17):
	print "Runtime test2 faild. p.getVal()=", p.getVal()
	sys.exit(1)
