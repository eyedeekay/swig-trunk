<?php

require "tests.php4";
require "abstract_inherit_ok.php";

check::classes(array(Foo,Spam));
$spam=new Spam();

check::equal(0,$spam->blah(),"spam object method");

check::done();
?>
