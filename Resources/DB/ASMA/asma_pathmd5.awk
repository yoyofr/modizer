// { filename=substr($1,7,length($1)-8);
	  md5 = substr($2,2,length($2)-1);
	  print md5 ";" filename;
}