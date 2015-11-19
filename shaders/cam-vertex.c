in vec4 position;
out vec2 Texcoord;
uniform float angle;

void main() 
{
	const mat4 m_projection = mat4(
        vec4(3.0 / 4.0, 0.0, 0.0, 0.0),
        vec4(      0.0, 1.0, 0.0, 0.0),
        vec4(      0.0, 0.0, 0.5, 0.5),
        vec4(      0.0, 0.0, 0.0, 1.0)
    );

    mat4 m_rotation = mat4(
        vec4(1.0,                  0.0,                  0.0, 0.0),
        vec4(0.0,  cos(radians(angle)),  sin(radians(angle)), 0.0),
        vec4(0.0, -sin(radians(angle)),  cos(radians(angle)), 0.0),
        vec4(0.0,                  0.0,                  0.0, 1.0)
    );
    
    mat4 m_scale = mat4(
        vec4(4.0 / 3.0, 0.0, 0.0, 0.0),
        vec4(      0.0, 1.0, 0.0, 0.0),
        vec4(      0.0, 0.0, 1.0, 0.0),
        vec4(      0.0, 0.0, 0.0, 1.0)
    );

    gl_Position = m_projection * m_rotation * m_scale * position;
    Texcoord = position.xy * vec2(0.5) + vec2(0.5);
}
