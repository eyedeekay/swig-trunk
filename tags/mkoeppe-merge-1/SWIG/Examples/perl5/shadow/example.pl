# file: example.pl

# This file illustrates the low-level C++ interface
# created by SWIG.  In this case, all of our C++ classes
# get converted into function calls.

use example;

# ----- Object creation -----

print "Creating some objects:\n";
$c = new example::Circle(10);
print "    Created circle $c\n";
$s = new example::Square(10);
print "    Created square $s\n";

# ----- Access a static member -----

print "\nA total of $example::Shape_nshapes shapes were created\n";

# ----- Member data access -----

# Set the location of the object

$c->{'x'} = 20;
$c->{'y'} = 30;

# Now use the same functions in the base class
$s->{'x'} = -10;
$s->{'y'} = 5;

print "\nHere is their current position:\n";
print "    Circle = (",$c->{x},",", $c->{y},")\n";
print "    Square = (",$s->{x},",", $s->{y},")\n";

# ----- Call some methods -----

print "\nHere are some properties of the shapes:\n";
foreach $o ($c,$s) {
      print "    $o\n";
      print "        area      = ", $o->area(), "\n";
      print "        perimeter = ", $o->perimeter(), "\n";
  }
# Notice how the Shape_area() and Shape_perimeter() functions really
# invoke the appropriate virtual method on each object.

# ----- Delete everything -----

print "\nGuess I'll clean up now\n";

# Note: this invokes the virtual destructor
$c->DESTROY();
$s->DESTROY();

print $example::Shape_nshapes," shapes remain\n";
print "Goodbye\n";

