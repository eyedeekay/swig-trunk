import pybuf
import time
k=100000
n=1000

t=time.time()
a = bytearray(b'a'*n)
for i in range(k):
  pybuf.upper1(a)
print("Time used:",time.time()-t)

t=time.time()
b = 'b'*n
for i in range(k):
  pybuf.upper2(b)
print("Time used:",time.time()-t)
