#version 330 core
layout (location = 0) in float aRadius;
layout (location = 1) in float aPhi;
layout (location = 2) in float aAngularSpeed;
layout (location = 3) in float aHeight;
layout (location = 4) in float aSize;
layout (location = 5) in float aIntensity;
layout (location = 6) in float aAspect;
layout (location = 7) in vec3 aFixedPosition;
layout (location = 8) in float aIsFixed;
uniform mat4 view;
uniform mat4 projection;
uniform float simulationTime;
uniform vec3 diskCenter;
uniform vec3 diskRadial;
uniform vec3 diskTangent;
uniform vec3 diskNormal;
out float intensity;
out float aspect;
out vec2 streakDirection;

void main() {
    float angle = aPhi + simulationTime * aAngularSpeed * 100.0;
    vec3 position = aIsFixed > 0.5
        ? aFixedPosition
        : diskCenter + aRadius * (cos(angle) * diskRadial + sin(angle) * diskTangent) + aHeight * diskNormal;
    vec3 orbitalDirection = -sin(angle) * diskRadial + cos(angle) * diskTangent;
    vec4 viewPosition = view * vec4(position, 1.0);
    vec3 directionView = mat3(view) * orbitalDirection;
    vec4 projectedDirection = projection * vec4(directionView, 0.0);
    float directionLength = length(projectedDirection.xy);
    streakDirection = directionLength > 0.0001 ? projectedDirection.xy / directionLength : vec2(1.0, 0.0);
    gl_Position = projection * viewPosition;
    gl_PointSize = clamp(aSize * (30.0 / max(-viewPosition.z, 1.0)), 1.0, 5.0);
    intensity = aIntensity;
    aspect = aAspect;
}
