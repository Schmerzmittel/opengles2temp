
precision highp float;

uniform mat4 view_perspective;
uniform mat4 model;

attribute vec3 position;
attribute vec3 normal;
attribute vec4 color;

varying vec4 fragment_color;
varying vec3 fragment_position;
varying vec3 fragment_normal;

void main ()
{
	vec4 final_position = model * vec4 (position, 1.0);
	fragment_color = color;
	fragment_position = final_position.xyz;
	fragment_normal = vec3 (model * vec4 (normal, 0.0));
	gl_Position = view_perspective * final_position;
}
