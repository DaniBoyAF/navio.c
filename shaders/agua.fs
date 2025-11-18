#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform float tempo;

void main()
{
    vec2 uv = fragTexCoord * 10.0;
    
    // Ondas animadas
    float onda1 = sin(uv.x * 2.0 + tempo * 2.0) * 0.5;
    float onda2 = cos(uv.y * 3.0 + tempo * 1.5) * 0.5;
    float altura = (onda1 + onda2) * 0.1;
    
    // Cor baseada na altura da onda
    vec3 corAgua = mix(
        vec3(0.0, 0.2, 0.5),  // azul escuro
        vec3(0.0, 0.5, 0.8),  // azul claro
        altura + 0.5
    );
    
    // Reflexos simples
    float reflexo = sin(uv.x * 5.0 + tempo * 3.0) * 0.1;
    corAgua += vec3(reflexo, reflexo, reflexo);
    
    finalColor = vec4(corAgua, 0.85);
}