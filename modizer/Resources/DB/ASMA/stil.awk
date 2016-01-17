BEGIN {
  readflag=0;
}
/^#/ {readflag=0;}
// {
  if (readflag==1) {
     if (length($0)<=1) {
        readflag=0;
        print title "\t" info;
     } else {
        info= info substr($0,1,length($0)-1) "\\n";
     }
  }
}
/^\/Composers/ {
   title=substr($0,1,length($0));
   readflag=1;
   info="";
}
/^\/Games/ {
   title=substr($0,1,length($0));
   readflag=1;
   info="";
}
/^\/Misc/ {
   title=substr($0,1,length($0));
   readflag=1;
   info="";
}
/^\/Unknown/ {
   title=substr($0,1,length($0));
   readflag=1;
   info="";
}
