#version 330

in vec2 UV;
in vec2 Position;

out vec2 f_UV;

void main()
{
    gl_Position = vec4(Position, 0.0f, 1.0f);
    f_UV = UV;
}