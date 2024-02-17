/**
* Converting the silly CP1252 chars used in sid-files.
*/
#ifndef CP1252CONV_H
#define CP1252CONV_H

#include <string>

using namespace std;

string cp1252_to_utf(unsigned char *s);


#endif
