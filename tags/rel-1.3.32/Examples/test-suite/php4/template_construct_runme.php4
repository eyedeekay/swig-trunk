<?php

require "tests.php4";
require "template_construct.php";

check::classes(array(foo_int));
$foo_int=new foo_int(3);
check::is_a($foo_int,"foo_int","Made a foo_int");

check::done();
?>
