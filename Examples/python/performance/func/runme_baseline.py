#!/usr/bin/env python

import Simple_baseline
import time

t1 = time.clock()
x = Simple_baseline.MyClass()
for i in range(10000000) :
    x.func()
t2 = time.clock()
print "Simple_baseline took %f seconds" % (t2 - t1)
