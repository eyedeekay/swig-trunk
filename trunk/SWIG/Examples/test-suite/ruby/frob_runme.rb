require 'frob'

foo = Frob::Bravo.new;
s = foo.abs_method;

raise RuntimeError if s != "Bravo::abs_method()"
