BEGIN {
  readflag=0;
}
/^#/ {readflag=0;}
// {
  if (readflag==1) {
     if (length($0)<=1) {
        readflag=0;
        gsub(/"/,"'",info);
        print title "\t" info;
     } else {
        info= info substr($0,1,length($0)-1) "\\n";
     }
  }
}
/^\/MUSICIANS/ {
   title=substr($0,1,length($0)-1);
   readflag=1;
   info="";
}
/^\/GAMES/ {
   title=substr($0,1,length($0)-1);
   readflag=1;
   info="";
}
/^\/DEMOS/ {
   title=substr($0,1,length($0)-1);
   readflag=1;
   info="";
}
