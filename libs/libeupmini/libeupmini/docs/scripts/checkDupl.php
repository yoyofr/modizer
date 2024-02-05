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
	
	$ext= 'eup';
	
    $files = scandir($dir);

    foreach ($files as $key => $value) {
        $path = realpath($dir . DIRECTORY_SEPARATOR . $value);
        if (!is_dir($path)) {
			if (endsWith($path, $ext)) {
				
				// read EUP file
				$filesize = filesize($path);
				$fp = fopen($path, 'rb');
				$binary = fread($fp, $filesize);
				fclose($fp);
						
				$binary = substr($binary, 0x40); // ignore the title meta info
				
				$md5 = md5($binary);
				
				if (array_key_exists($md5, $map)) {
					echo "duplicate: " . $path . " existing: " . $map[$md5] . "\r\n";					
				} else {
					$map[$md5] = $path;
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


?>