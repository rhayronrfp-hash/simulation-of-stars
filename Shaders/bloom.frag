#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
uniform int mode;
uniform int horizontal;
uniform vec2 texelSize;
uniform float threshold;

void main() {
    if (mode == 0) {
        vec3 color = texture(image, TexCoords).rgb;
        float brightness = max(max(color.r, color.g), color.b);
        float contribution = max(brightness - threshold, 0.0) / max(brightness, 0.0001);
        FragColor = vec4(color * contribution, 1.0);
        return;
    }

    vec2 direction = horizontal == 1 ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);
    vec3 result = texture(image, TexCoords).rgb * 0.227027;
    result += texture(image, TexCoords + direction * 3.0).rgb * 0.1945946;
    result += texture(image, TexCoords - direction * 3.0).rgb * 0.1945946;
    result += texture(image, TexCoords + direction * 6.0).rgb * 0.1216216;
    result += texture(image, TexCoords - direction * 6.0).rgb * 0.1216216;
    result += texture(image, TexCoords + direction * 9.0).rgb * 0.054054;
    result += texture(image, TexCoords - direction * 9.0).rgb * 0.054054;
    result += texture(image, TexCoords + direction * 12.0).rgb * 0.016216;
    result += texture(image, TexCoords - direction * 12.0).rgb * 0.016216;
    FragColor = vec4(result, 1.0);
}
