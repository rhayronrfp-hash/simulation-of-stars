#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D scene;
uniform sampler2D bloom;
uniform float bloomStrength;

void main() {
    vec3 sceneColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloom, TexCoords).rgb;
    vec3 hdrColor = sceneColor + bloomColor * bloomStrength;
    vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
    mapped = pow(max(mapped, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(mapped, 1.0);
}
