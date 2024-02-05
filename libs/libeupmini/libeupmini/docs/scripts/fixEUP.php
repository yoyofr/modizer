<?php
/*
 * Fixes all the Euphony files to use lower-case library names and all
 * lower-case file extensions.
 *
 * script also reports missing library files.
 */
 
$PATH = "C:\\chiptune\\EUP-test\\TOWNS EUP 2015";				// drive letter must be uppercase!

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


function patchToLower(&$binary, $offset) 
{
	$lib1= b'';
	for ($i = 0; $i <= 7; $i++) {
		$t = ord($binary[$offset + $i]);

		if ($t == 0) break;
		
		$c = strtolower(chr($t));
		$lib1 = $lib1.$c;
		
		$binary[$offset + $i] = $c;
	}
	return rtrim($lib1);
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

function renameLib($path, $lib, $ext) 
{
	if ($lib != '') {
		$libFile = getPath($path) . DIRECTORY_SEPARATOR . $lib . $ext;

		if (file_exists($libFile)) {
			fixFilenameCase($libFile);			
		} else {
			echo "error - lib not found: ".$libFile."\r\n";
		}
	}
}

function scanFolders($dir) 
{
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
								
				$fmb = patchToLower($binary, 0x6e2);
				$pmb = patchToLower($binary, 0x6e2 + 8);
												
				$newName= substr($path, 0, strlen($path)-3) . $ext;
								
				fixFilenameCase($newName);		// name sure windows doesn't keep the old name	
			
				$fp = fopen($newName, 'wb');
				fwrite($fp, $binary);
				fclose($fp);
								
				renameLib($path, $fmb, ".fmb");
				renameLib($path, $pmb, ".pmb");
			}
        } else if ($value != "." && $value != "..") {
            scanFolders($path);
        }
    }
}

$pathInput = $PATH;
scanFolders($pathInput);


?>