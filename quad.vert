R"(#version 330 core

layout (location = 0) in vec2 vert_pos;

out vec2 frag_pos;

void main() {
   //tex_coord = vert_pos + vec2(.5, .5);
//   pos = vert_pos * 100.0;
   gl_Position = vec4(vert_pos.x, vert_pos.y, 0, 0);
   frag_pos = vert_pos;
}
)"
