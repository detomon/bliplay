<?php

foreach (glob ('*.blip') as $file) {
	$content = file_get_contents ($file);

	$content = preg_replace ('/gs:/', 'stepticks:', $content);
	$content = preg_replace ('/gv:/', 'volume:', $content);
	$content = preg_replace ('/t:(begin|end)/', 'track:\\1', $content);
	$content = preg_replace ('/g:(begin|end)/', 'grp:\\1', $content);
	$content = preg_replace ('/i:(begin|end)/', 'instr:\\1', $content);
	$content = preg_replace ('/w:(begin|end)/', 'wave:\\1', $content);
	$content = preg_replace ('/s:(begin|end)/', 'samp:\\1', $content);

	file_put_contents ($file, $content);
}
