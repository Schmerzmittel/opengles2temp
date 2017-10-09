
precision highp float;

uniform vec3 light_position;

varying vec4 fragment_color;
varying vec3 fragment_position;
varying vec3 fragment_normal;

void main ()
{
	vec3 light_vector = normalize (light_position - fragment_position);
	float distance = length (light_position - fragment_position);
	float diffuse = max (dot (fragment_normal, light_vector), 0.1) * (2.0 / (1.0 + (0.25 * distance * distance)));
	gl_FragColor = fragment_color * diffuse;
}
