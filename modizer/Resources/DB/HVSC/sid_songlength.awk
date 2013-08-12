function str_second(str) {
	split(str, a ,":");
    return (a[1]+0)*60+(a[2]+0);
}

BEGIN {
  line_type=0;
}
// { if (line_type==0) {
	   line_type=1;
	   filename=substr($2,1,length($2)-1);
	 } else {
	 line_type=0;
	   md5=$1;
	   tracklengths="";
	   for (i=2;i<=NF;i++)  {
	   	print md5 ";" i-1 ";" str_second($i);
	   }
	 }
}