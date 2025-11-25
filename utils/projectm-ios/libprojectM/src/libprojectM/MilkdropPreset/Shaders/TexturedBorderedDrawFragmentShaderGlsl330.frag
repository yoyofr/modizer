#extension GL_EXT_shader_framebuffer_fetch : require

precision mediump float;

in vec4 fragment_color;
in vec2 fragment_texture;
in vec4 fragment_border;

uniform sampler2D texture_sampler;

layout(location = 0) inout vec4 color;

void main(){
    vec4 newColor;
    int frag_border_y_int=int(fragment_border.y);
    bool isAdditive=(frag_border_y_int&0x100)!=0;
    bool isTexture=(frag_border_y_int&0x200)!=0;
    if (fragment_border.z>=1.0) {
            newColor.r = float(((int(fragment_border.x))>>16)&0xFF)/255.0;
            newColor.g = float(((int(fragment_border.x))>>8)&0xFF)/255.0;
            newColor.b = float(((int(fragment_border.x))>>0)&0xFF)/255.0;
            newColor.a = float((frag_border_y_int>>0)&0xFF)/255.0;
    } else {
        if (isTexture) newColor = fragment_color * texture(texture_sampler, fragment_texture.st);
        else newColor=fragment_color;
    }
    if (isAdditive) color=newColor*newColor.a + color;
    else color=newColor*newColor.a + color*(1.0 - newColor.a);
}
