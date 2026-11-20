#version 330 core
in float intensity;
in float aspect;
in vec2 streakDirection;
out vec4 FragColor;

void main() {
    vec2 point = gl_PointCoord * 2.0 - 1.0;
    vec2 perpendicular = vec2(-streakDirection.y, streakDirection.x);
    float along = dot(point, streakDirection);
    float across = dot(point, perpendicular);
    float radius = length(vec2(along, across * aspect));
    float angle = atan(across, along);
    float irregularity = sin(angle * 3.0 + intensity * 19.0) * 0.055 + sin(angle * 7.0 - intensity * 31.0) * 0.025;
    radius *= 1.0 + irregularity;
    if (radius > 1.0) discard;
    float edge = 1.0 - smoothstep(0.52, 1.0, radius);
    float core = exp(-radius * radius * 5.0);
    float structure = 0.9 + 0.1 * sin(angle * 9.0 + intensity * 43.0) * sin(radius * 17.0 + intensity * 21.0);
    vec3 orange = mix(vec3(0.58, 0.045, 0.003), vec3(1.0, 0.29, 0.025), intensity);
    vec3 color = orange * edge * (0.35 + core * 0.65) * structure * intensity * 0.72;
    FragColor = vec4(color, edge * 0.8);
}
