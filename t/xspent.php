<?php

list($usec, $sec) = explode(" ", microtime());
$spoint = sprintf("d%03d.%03d", $sec % 1000, intval($usec * 1000));

list($usec, $sec) = explode(" ", microtime());
$epoint = sprintf("d%03d.%03d", $sec % 1000, intval($usec * 1000));

$custom_header = sprintf("X-Spent: %s;%s", $spoint, $epoint);

header($custom_header);
echo "hi\n";

?>
