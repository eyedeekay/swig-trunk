import pybuf
import time
k=100000
n=10

t=time.time()
a = bytearray(b'a'*n)
for i in range(k):
  pybuf.upper1(a)
print("Time used by bytearray:",time.time()-t)

t=time.time()
b = 'b'*n
for i in range(k):
  pybuf.upper2(b)
print("Time used by string:",time.time()-t)
