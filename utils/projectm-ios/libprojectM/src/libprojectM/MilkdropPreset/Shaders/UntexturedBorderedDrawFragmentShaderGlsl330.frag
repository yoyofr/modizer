precision mediump float;

in vec4 fragment_color;
in vec4 fragment_border;

out vec4 color;

void main(){
    if (fragment_border.z>=1.0) {
            color.r = float(((int(fragment_border.x))>>8)&0xFF)/255.0;
            color.g = float(((int(fragment_border.x))>>0)&0xFF)/255.0;
            color.b = float(((int(fragment_border.y))>>8)&0xFF)/255.0;
            color.a = float(((int(fragment_border.y))>>0)&0xFF)/255.0;
    } else color = fragment_color;
}
