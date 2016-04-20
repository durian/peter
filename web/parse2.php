<?php

$address = "localhost";
$service_port = 2048;
$socket = fsockopen($address, $service_port, $errno, $errstr, 30);

if ( $socket ) {

  fwrite( $socket, "pgui\n" );
  stream_set_timeout($socket, 10);

  $res = "";
  while ( ! feof($socket)) {
    $res .= fgets($socket, 128);
  }
  $lines = explode("\n", $res);
  echo "<div class=\"scheduler\">\n";
  foreach ($lines as $line) {
    //print $line;
    if ( $line != "" ) {
      $pieces = explode( " ", $line);
      $num = sizeof($pieces);
      //pieces[0] is name, rest is key:value
      echo "<div class=\"program\">\n";
      echo "<div class=\"N\">".$pieces[0]."</div>";
      for ( $i = 1; $i < $num; $i++ ) {
	$kv = explode(":", $pieces[$i]);
	echo "<div class=\"".$kv[0]."\">".$kv[1]."</div>";
      }
      echo "\n</div>\n";
    }
  }
  echo "</div>\n";

  //echo $res;
/*
  $info = stream_get_meta_data($socket);
  if ($info['timed_out']) {
    echo "{'status':'error'}";
  }
*/
  fclose( $socket );


 } else {
  echo "{'status':'error'}";
}
?>
