#version 450
#extension GL_ARB_separate_shader_objects : enable

/*
layout(location = 0) in vec2 vert_pos;

//out (location = 0) vec2 frag_pos;

void main() {
   //tex_coord = vert_pos + vec2(.5, .5);
   //pos = vert_pos * 100.0;
   //frag_pos = vert_pos;
   gl_Position = vec4(vert_pos.x, vert_pos.y, 0, 0);
}*/

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 fragColor;

vec2 positions[4] = vec2[](
    vec2(-1, -1),
    vec2(1, -1),
    vec2(-1, 1),
    vec2(1, 1)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec3(positions[gl_VertexIndex], 1.0);
}
