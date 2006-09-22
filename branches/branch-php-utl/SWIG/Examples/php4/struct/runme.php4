<?

// Load module and PHP classes.
include("example.php");

$s = new AStruct();

$s->a = 15;
$v = get_Astruct_a($s);
print "value = $v ";

?>

