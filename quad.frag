R"(#version 330 core

in vec2 frag_pos;

out vec4 frag_color;

//uniform sampler2D sampler;

void main() {
    //frag_color = texture(sampler, tex_coord);
    frag_color = vec4(frag_pos*1000000.0 + vec2(.5, .5), 0.0, 1.0);
    //frag_color = vec4(1.0);
//    frag_color = texture(sampler, vec2((gl_FragCoord / 500.0).x, .5));
//    frag_color = vec4(vec2((gl_FragCoord/500.0).x), 0, 0);

}
)"
