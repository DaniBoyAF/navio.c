#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform float tempo;

void main()
{
    vec2 uv = fragTexCoord;
    float onda = sin(uv.x * 10.0 + tempo) * 0.05 + cos(uv.y * 8.0 + tempo * 0.7) * 0.03;
    
    vec3 corAgua = vec3(0.0, 0.3 + onda, 0.6 + onda * 0.5);
    finalColor = vec4(corAgua, 0.8);
}