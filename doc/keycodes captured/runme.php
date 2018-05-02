<?php
# small script to actually work with the keycodes captured with saleae logic


$keycodes = [];


for ($i=1; $i < 7; $i++) { 
	$file = explode("\n", file_get_contents($i.".row.txt"));
	$row_raw = [];
	foreach ($file as $row) {
		array_push($row_raw, explode(",", $row));
	}
	# remove header and last empty row
	unset($row_raw[count($row_raw)-1]);
	unset($row_raw[0]);
	$previous = 0;
	$key = [];
	$keycodes[$i] = [];
	foreach ($row_raw as $event) {
		var_dump($event[1]);

		echo $event[1]."\n";
		# create up and down codes
		if ($previous === 0) {
			$key["down"] = $event[1];
			$previous++;
		} elseif ($previous === 1) {
			$key["up"] = $event[1];
			$previous++;
		} elseif ($previous === 2) {
			array_push($keycodes[$i], $key);
			$key = [];
			$previous = 0;
		}
	}
}

echo "Keycodes, down:\n";
foreach ($keycodes as $row) {
	foreach ($row as $key) {
		echo $key["down"]."\t";
	}
	echo "\n";
}

echo "keycode were captured by tapping each key once, starting from the top left going to the right, then doing the next row and so on. If a key sits in two rows, only the first row presses it.";

$offsets = [
	1 => [0, 1.5, 1,0,0,0, 0.5,0,0,0, 0.5,0,0,0, 0.5,0,0, 0.5,0,0,0 ],
	2 => [0,0, 0.5,0,0,0,0,0,0,0,0,0,0,0,0,0, 1.5,0,0, 0.5,0,0,0],
	3 => [0,0, 0.5,0.3,0,0,0,0,0,0,0,0,0,0,0,0, 1.2,0,0, 0.5,0,0,0],
	4 => [0,0, 0.5,0.7,0,0,0,0,0,0,0,0,0,0, 0,5.3,0,0 ],
	5 => [0,0, 0.5,0.3,0,0,0,0,0,0,0,0,0,0,0, 3.2,1.5,0, 0,0],
	6 => [0,0, 0.5,0.7,0,3,3,0.5,0, 1.3,0,0, 0.5,1],
];

$svg = '<?xml version="1.0"?><svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 300 100">';


foreach ($offsets as $rowNumber => $rowOffset) {
	$currentX = 1;
	foreach ($rowOffset as $keyNumber => $offset) {
		$currentX += $offset*10 + 10;
		// print_r($keycodes[$rowNumber]);
		// readline();
		$svg .= '<text x="'.$currentX.'" y="'.($rowNumber*10).'" font-family="Verdana" font-size="2" fill="black">'.$keycodes[$rowNumber][$keyNumber]["down"].'</text>';
	}
}
$svg .= "</svg>";
file_put_contents("keyboard.svg", $svg);


?>