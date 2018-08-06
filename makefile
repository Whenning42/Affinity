all:
	glslangValidator -V shaders/quad.vert -o shaders/quad.vert.spv
	glslangValidator -V shaders/quad.frag -o shaders/quad.frag.spv
	g++ --std=c++14 -g vulkan_bitmap.cpp -lSDL2 -lvulkan
