var fs = require('fs');
var buffer = fs.readFileSync(process.argv[2]);
var view = new Uint8Array(buffer);

var buf = [];

function toHex(d) {
  return ("0"+(Number(d).toString(16))).slice(-2).toUpperCase()
}

console.log('/** ' + process.argv[2] + '*/');

for(var i=0; i<view.length; i++) {
  buf.push('0x' + toHex(view[i]));
  if(buf.length==16) {
    console.log(buf.join(',') + ',');
    buf = [];
  }
}

if(0<buf.length) {
    console.log(buf.join(','));
}
