#version 450
layout(location = 0) out vec2 uv;

const vec2 vertices[] = { vec2(-1,-1), vec2(3,-1), vec2(-1, 3) };

void main() {
	gl_Position = vec4(vertices[gl_VertexID], 0, 1);
	uv = 0.5 * gl_Position.xy + vec2(0.5);
}