<?php 
/*
	converts all the 2SID files in the current folder to respective stereo versions
	
	usage: php convertStereo.php
	
			put the script into the root-folder containing the original 2SID files:	
	       (output will be generated into ../stereo/)
		   
		   processing will stop if an unsupported .sid file is encountered (see
		   messages on the console)
*/

function die2($msg) {
//	echo $msg;
	die($msg);
}
function convert2stereo($src, $dst) { 
	if (substr(strrchr($src,'.'),1) == 'sid') {

		$chan=0;
		
		$in = fopen($src, "r") or die2("Unable to open file! ".$src);
		$out = fopen($dst, "w") or die2("Unable to open file! ".$dst);

		$i = 0;
		while(!feof($in)) {
		  $c= ord(fread ($in, 1));
		  
		  if ($i==5) {
			if ($c == 0x03) {
				$c= 0x4e;	// replace type
			} else {
				die2("Unsupported input file type: ".$c." file: ".$src);
			}
		  }
		  
		  if ($i==7) {
			if ($c == 0x7c) {
				$c= 0x7e;	// "alloc" 2 extra header bytes
			} else {
				die2("Unsupported input file type: ".$c);
			}
		  }
		  
		  if ($i==0x77) {	// flags
			$chan= $c & 0x40;		// check channel used by original SID			
		  }
		  
		  if ($i==0x7b) {
			// configure stereo channel to use for 2nd SID
			if ($chan == 0) {
				fwrite($out, ''.chr(0x40));
			} else {
				fwrite($out, ''.chr(0x0));
			}
			// add 2 bytes "end" marker
			fwrite($out, ''.chr(0x0));
			fwrite($out, ''.chr(0x0));
		  } else {
			fwrite($out, ''.chr($c));	// just copy the char
		  }
		  
		  $i++;
		}
		fclose($in);
		fclose($out);
	}
}
  
function custom_copy($src, $dst) {  
  
    // open the source directory 
    $dir = opendir($src);  
  
    // Make the destination directory if not exist 
    @mkdir($dst);  
  
    // Loop through the files in source directory 
    while( $file = readdir($dir) ) {  
  
        if (( $file != '.' ) && ( $file != '..' )) {  
            if ( is_dir($src . '/' . $file) )  
            {  
  
                // Recursively calling custom copy function 
                // for sub directory  
                custom_copy($src . '/' . $file, $dst . '/' . $file);  
  
            }  
            else {  
                convert2stereo($src . '/' . $file, $dst . '/' . $file);  
            }  
        }  
    }  
  
    closedir($dir); 
}  
 

 
 
 
$src = "."; 
  
$dst = "../stereo"; 
  
custom_copy($src, $dst); 
  
?> 