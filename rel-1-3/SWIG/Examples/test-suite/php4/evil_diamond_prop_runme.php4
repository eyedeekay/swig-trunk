<?php
// Sample test file

require "tests.php4";
require "evil_diamond_prop.php";

check::classes(array("foo","bar","baz","spam"));
check::functions("test");
check::is_a("bar","foo");
check::is_a("baz","foo");
check::is_a("spam","foo");
check::is_a("spam","bar");
//No multiple inheritance introspection yet
//check::is_a("spam","baz");

$foo=new foo();
check::is_a($foo,"foo");
check::equal(1,$foo->_foo,"1==foo->_foo");

$bar=new bar();
check::is_a($bar,"bar");
check::equal(1,$bar->_foo,"1==bar->_foo");
check::equal(2,$bar->_bar,"2==bar->_bar");

$baz=new baz();
check::is_a($baz,"baz");
check::equal(1,$baz->_foo,"1==baz->_foo");
check::equal(3,$baz->_baz,"3==baz->_baz");

$spam=new spam();
check::is_a($spam,"spam");
check::equal(1,$spam->_foo,"1==spam->_foo");
check::equal(2,$spam->_bar,"2==spam->_bar");
check::equal(3,$spam->_baz,"3==spam->_baz");
check::equal(4,$spam->_spam,"4==spam->_spam");

check::done();
?>
