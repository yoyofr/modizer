<?php
/*
 * Checks for duplicate song files.
 */
 
$PATH = "C:\\chiptune\\EUP-test\\";				// drive letter must be uppercase!

function endsWith($haystack, $needle) 
{
    $length = strlen($needle);
    if ($length == 0) {
        return true;
    }
    return (strtolower(substr($haystack, -$length)) === $needle);
}

if (!function_exists('str_contains')) 
{
    function str_contains($haystack, $needle) {
        return $needle !== '' && mb_strpos($haystack, $needle) !== false;
    }
}

function startsWith($haystack, $needle) 
{
     $length = strlen($needle);
     return (substr($haystack, 0, $length) === $needle);
}


function getPath($file) 
{ 
	$path_parts = pathinfo($file);
    return $path_parts['dirname']; 
} 



function fixFilenameCase($name) 
{
	// windows garbage is case insensitive..
	$rp= realpath($name);
	
	if (basename($rp) === basename($name)) {
		// OK
	} else {
//		echo 'rename: '.basename($rp).' > '.basename($name)."\r\n";
		rename($rp, $name);
	}
}


function scanFolders($dir) 
{
	global $map;
		
    $files = scandir($dir);

    foreach ($files as $key => $value) {
        $path = realpath($dir . DIRECTORY_SEPARATOR . $value);
        if (!is_dir($path)) {
			if (endsWith($path, 'fmb') || endsWith($path, 'pmb')) {
								
				// read lib file
				$filesize = filesize($path);
				$fp = fopen($path, 'rb');
				$binary = fread($fp, $filesize);
				fclose($fp);
										
				$md5 = md5($binary);
				$name = basename($path);
				
				if (!array_key_exists($name, $map)) {
					$map[$name] = array();
				}
				$lmap = &$map[$name];
				
				if (array_key_exists($md5, $lmap)) {
					// ignore known file
				} else {						
					$lmap[$md5] = $path;
				}
								
			}
        } else if ($value != "." && $value != "..") {
            scanFolders($path);
        }
    }
}

$map = array();
$pathInput = $PATH;
scanFolders($pathInput);


$croppedMap = array();
foreach ($map as $key => $value) {
	if (count($value) == 1) {
		// no variants.. ignore lib
	} else {
//		$croppedMap[$key] = $value;

		$libName = $key;
		$i = 0;
		foreach ($value as $md5 => $libPath) {
			
			// read lib file
			$filesize = filesize($libPath);
			$fp = fopen($libPath, 'rb');
			$binary = fread($fp, $filesize);
			fclose($fp);
			
			$fp = fopen($libPath, 'rb');
			
			$outName = $PATH . "duplibs" . DIRECTORY_SEPARATOR . $libName . "_" . $i;
			
			$fp = fopen($outName, 'wb');
			fwrite($fp, $binary);
			fclose($fp);
			
			$i++;
		}

	}
}






//echo json_encode($croppedMap, JSON_PRETTY_PRINT) . "\r\n";



?>