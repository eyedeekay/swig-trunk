# file: runme.rb

require 'example'

# Call average with a Ruby array...

puts Example::average([1,2,3,4])

# ... or a wrapped std::vector<int>

v = Example::IntVector.new(4)
0.upto(v.length-1) { |i| v[i] = i+1 }
puts Example::average(v)


# half will return a Ruby array.
# Call it with a Ruby array...

w = Example::half([1, 1.5, 2, 2.5, 3])
0.upto(w.length-1) { |i| print w[i],"; " }
puts

# ... or a wrapped std::vector<double>

v = Example::DoubleVector.new(4)
0.upto(v.length-1) { |i| v[i] = i+1 }
w = Example::half(v)
0.upto(w.length-1) { |i| print w[i],"; " }
puts

# now halve a wrapped std::vector<double> in place

Example::halve_in_place(v)
0.upto(v.length-1) { |i| print v[i],"; " }
puts

