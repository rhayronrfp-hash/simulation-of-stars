#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform vec3 cameraPosition;
uniform vec3 blackHolePosition;
uniform float blackHoleMass;
uniform float spinParameter;
uniform float iscoRadius;
uniform float diskOuterRadius;
uniform float diskMapExtent;
uniform float diskTemperatureScale;
uniform mat4 viewInverse;
uniform mat4 projInverse;
uniform vec2 screenSize;
uniform vec3 diskNormal;
uniform vec3 diskRadial;
uniform vec3 diskTangent;
uniform sampler2D hdrMap;
uniform sampler2D diskDustMap;
uniform sampler1D diskFluxMap;

struct KerrState {
    float r;
    float vr;
    float theta;
    float vtheta;
    float phi;
};

struct KerrDerivative {
    float dr;
    float dvr;
    float dtheta;
    float dvtheta;
    float dphi;
};

KerrDerivative kerrDerivative(KerrState s, float E, float L, float Q, float M, float a) {
    float r = max(s.r, 1.0001 * (M + sqrt(max(M * M - a * a, 0.0))));
    float sinTheta = max(abs(sin(s.theta)), 0.0001);
    float cosTheta = cos(s.theta);
    float delta = max(r * r - 2.0 * M * r + a * a, 0.000001);
    float P = E * (r * r + a * a) - a * L;
    float K = Q + (L - a * E) * (L - a * E);
    float radialDerivative = 4.0 * E * r * P - 2.0 * (r - M) * K;
    float polarDerivative = -2.0 * a * a * E * E * sinTheta * cosTheta + 2.0 * L * L * cosTheta / (sinTheta * sinTheta * sinTheta);
    float azimuthDerivative = a * P / delta + L / (sinTheta * sinTheta) - a * E;
    KerrDerivative result;
    result.dr = s.vr;
    result.dvr = 0.5 * radialDerivative;
    result.dtheta = s.vtheta;
    result.dvtheta = 0.5 * polarDerivative;
    result.dphi = azimuthDerivative;
    return result;
}

KerrState offsetKerrState(KerrState s, KerrDerivative d, float amount) {
    KerrState result;
    result.r = s.r + d.dr * amount;
    result.vr = s.vr + d.dvr * amount;
    result.theta = s.theta + d.dtheta * amount;
    result.vtheta = s.vtheta + d.dvtheta * amount;
    result.phi = s.phi + d.dphi * amount;
    return result;
}

float radialPotential(float r, float E, float L, float Q, float M, float a) {
    float delta = r * r - 2.0 * M * r + a * a;
    float P = E * (r * r + a * a) - a * L;
    return P * P - delta * (Q + (L - a * E) * (L - a * E));
}

vec2 dirToUV(vec3 direction) {
    const float PI = 3.14159265359;
    float u = 0.5 + atan(direction.z, direction.x) / (2.0 * PI);
    float v = 0.5 - asin(clamp(direction.y, -1.0, 1.0)) / PI;
    return vec2(u, v);
}

vec3 planckSpectrum(float temperature) {
    vec3 wavelengths = vec3(0.65, 0.55, 0.46);
    float referenceTemperature = 6500.0;
    vec3 x = vec3(14387.77) / (wavelengths * max(temperature, 100.0));
    vec3 xReference = vec3(14387.77) / (wavelengths * referenceTemperature);
    vec3 numerator = exp(min(xReference, vec3(80.0))) - vec3(1.0);
    vec3 denominator = max(exp(min(x, vec3(80.0))) - vec3(1.0), vec3(0.000001));
    return numerator / denominator;
}

void main() {
    vec2 ndc = (gl_FragCoord.xy / screenSize) * 2.0 - 1.0;
    vec4 rayClip = vec4(ndc, -1.0, 1.0);
    vec4 rayView = projInverse * rayClip;
    rayView = vec4(rayView.xy, -1.0, 0.0);
    vec3 rayWorld = normalize((viewInverse * rayView).xyz);

    float M = blackHoleMass;
    float a = spinParameter;
    float rPlus = M + sqrt(max(M * M - a * a, 0.0));
    vec3 cameraLocal = vec3(
        dot(cameraPosition - blackHolePosition, diskRadial),
        dot(cameraPosition - blackHolePosition, diskTangent),
        dot(cameraPosition - blackHolePosition, diskNormal)
    );
    vec3 rayLocal = normalize(vec3(
        dot(rayWorld, diskRadial),
        dot(rayWorld, diskTangent),
        dot(rayWorld, diskNormal)
    ));

    float cartesianRadius2 = dot(cameraLocal, cameraLocal);
    float radialTerm = cartesianRadius2 - a * a;
    float r0Squared = 0.5 * (radialTerm + sqrt(radialTerm * radialTerm + 4.0 * a * a * cameraLocal.z * cameraLocal.z));
    float r0 = sqrt(max(r0Squared, rPlus * rPlus * 1.01));
    float theta0 = acos(clamp(cameraLocal.z / r0, -1.0, 1.0));
    float phi0 = atan(cameraLocal.y, cameraLocal.x);
    float sinTheta0 = max(abs(sin(theta0)), 0.0001);
    float cosTheta0 = cos(theta0);
    vec3 eRadial0 = vec3(sinTheta0 * cos(phi0), sinTheta0 * sin(phi0), cosTheta0);
    vec3 eTheta0 = vec3(cosTheta0 * cos(phi0), cosTheta0 * sin(phi0), -sinTheta0);
    vec3 ePhi0 = vec3(-sin(phi0), cos(phi0), 0.0);
    float nRadial = dot(rayLocal, eRadial0);
    float nTheta = dot(rayLocal, eTheta0);
    float nPhi = dot(rayLocal, ePhi0);

    float sigma0 = r0 * r0 + a * a * cosTheta0 * cosTheta0;
    float delta0 = max(r0 * r0 - 2.0 * M * r0 + a * a, 0.000001);
    float sin2 = sinTheta0 * sinTheta0;
    float A0 = (r0 * r0 + a * a) * (r0 * r0 + a * a) - a * a * delta0 * sin2;
    float lapse = sqrt(max(sigma0 * delta0 / A0, 0.000001));
    float frameDragging = 2.0 * M * a * r0 / A0;
    float sqrtGPhiPhi = sqrt(max(A0 * sin2 / sigma0, 0.000001));
    nRadial *= sqrt(sigma0 / delta0);
    nTheta *= sqrt(sigma0) / r0;
    nPhi *= sqrt(A0 / sigma0) / r0;
    float directionNorm = max(length(vec3(nRadial, nTheta, nPhi)), 0.000001);
    nRadial /= directionNorm;
    nTheta /= directionNorm;
    nPhi /= directionNorm;
    float L = sqrtGPhiPhi * nPhi;
    float pt = -lapse - frameDragging * L;
    float E = -pt;
    float pTheta = sqrt(sigma0) * nTheta;
    float Q = pTheta * pTheta + cosTheta0 * cosTheta0 * (L * L / sin2 - a * a * E * E);

    KerrState state;
    state.r = r0;
    state.vr = sqrt(sigma0 * delta0) * nRadial;
    state.theta = theta0;
    state.vtheta = sqrt(sigma0) * nTheta;
    state.phi = phi0;

    vec3 diskRadiance = vec3(0.0);
    vec3 transmittance = vec3(1.0);
    bool captured = false;
    bool escaped = false;
    const float PI = 3.14159265359;

    for (int i = 0; i < 520; ++i) {
        if (state.r <= rPlus * 1.0015) {
            captured = true;
            break;
        }
        if (state.r >= 60.0 * M && state.vr > 0.0) {
            escaped = true;
            break;
        }

        KerrDerivative k1 = kerrDerivative(state, E, L, Q, M, a);
        float radialRate = max(abs(k1.dr), 1.0);
        float angularRate = max(max(abs(k1.dtheta), abs(k1.dphi)), 0.1);
        float radialStep = 0.25 * M / radialRate;
        float angularStep = 0.045 / angularRate;
        float stepSize = clamp(min(radialStep, angularStep), 0.000001, 0.03 * M);
        if (state.vr < 0.0) {
            stepSize = min(stepSize, 0.15 * (state.r - rPlus) / max(-state.vr, 0.001));
        }

        KerrDerivative k2 = kerrDerivative(offsetKerrState(state, k1, 0.5 * stepSize), E, L, Q, M, a);
        KerrDerivative k3 = kerrDerivative(offsetKerrState(state, k2, 0.5 * stepSize), E, L, Q, M, a);
        KerrDerivative k4 = kerrDerivative(offsetKerrState(state, k3, stepSize), E, L, Q, M, a);
        KerrState middle = offsetKerrState(state, k1, 0.5 * stepSize);

        float r = middle.r;
        float theta = clamp(middle.theta, 0.0001, PI - 0.0001);
        float sinTheta = max(abs(sin(theta)), 0.0001);
        float cosTheta = cos(theta);
        float sigma = r * r + a * a * cosTheta * cosTheta;
        float diskRadius = r;
        float diskX = sqrt(r * r + a * a) * sinTheta * cos(middle.phi);
        float diskY = sqrt(r * r + a * a) * sinTheta * sin(middle.phi);
        float diskHeight = r * cosTheta;

        if (diskRadius > iscoRadius && diskRadius < diskOuterRadius) {
            float halfThickness = max(0.045 * r, 0.025 * M);
            float verticalProfile = exp(-0.5 * pow(diskHeight / halfThickness, 2.0));
            float radialMask = smoothstep(iscoRadius, iscoRadius + 0.12 * M, diskRadius) * (1.0 - smoothstep(diskOuterRadius - 1.0 * M, diskOuterRadius, diskRadius));
            vec2 diskUV = vec2(diskX, diskY) / (2.0 * diskMapExtent) + 0.5;
            vec3 dust = texture(diskDustMap, diskUV).rgb;
            float dustDensity = clamp(max(max(dust.r, dust.g), dust.b) * 1.8, 0.0, 1.5);
            float clumpFactor = 0.68 + 0.52 * dustDensity;
            float radialDensity = pow(max(diskRadius / iscoRadius, 1.0), -0.75);
            float density = 4.8 * radialDensity * verticalProfile * clumpFactor;

            float flux = max(texture(diskFluxMap, clamp((diskRadius - iscoRadius) / max(diskOuterRadius - iscoRadius, 0.001), 0.0, 1.0)).r, 0.0);
            float effectiveTemperature = diskTemperatureScale * pow(flux, 0.25);
            float colorTemperature = 1.18 * effectiveTemperature;

            float x = diskRadius / M;
            float aStar = a / M;
            float sqrtX = sqrt(max(x, 0.001));
            float x32 = x * sqrtX;
            float circularDenominator = x32 - 3.0 * sqrtX + 2.0 * aStar;
            if (circularDenominator > 0.0001 && colorTemperature > 100.0) {
                float uTime = (x32 + aStar) / (pow(x, 0.75) * sqrt(circularDenominator));
                float omega = sqrt(M) / (pow(diskRadius, 1.5) + a * sqrt(M));
                float emitterEnergy = max(-uTime * (pt + omega * L), 0.0001);
                float redshift = clamp(1.0 / emitterEnergy, 0.03, 8.0);
                float observedTemperature = colorTemperature * redshift;

                vec3 wavelengths = vec3(0.65, 0.55, 0.46);
                vec3 emittedWavelengths = wavelengths * redshift;
                vec3 xFrequency = vec3(14387.77) / max(emittedWavelengths * colorTemperature, vec3(0.001));
                vec3 stimulatedEmission = vec3(1.0) - exp(-min(xFrequency, vec3(80.0)));
                float referenceStimulated = 1.0 - exp(-14387.77 / (0.55 * 6500.0));
                vec3 alphaFreeFree = 0.38 * density * density * pow(max(colorTemperature / 6500.0, 0.05), -0.5)
                    * pow(emittedWavelengths / 0.55, vec3(3.0)) * stimulatedEmission / referenceStimulated;
                float pathLength = emitterEnergy * sigma * stepSize;
                vec3 opticalDepth = alphaFreeFree * pathLength * radialMask;
                vec3 attenuation = exp(-min(opticalDepth, vec3(50.0)));
                vec3 segmentAlpha = vec3(1.0) - attenuation;
                vec3 sourceFunction = planckSpectrum(observedTemperature);
                diskRadiance += transmittance * sourceFunction * segmentAlpha;
                transmittance *= attenuation;
            }
        }

        state.r += (stepSize / 6.0) * (k1.dr + 2.0 * k2.dr + 2.0 * k3.dr + k4.dr);
        state.vr += (stepSize / 6.0) * (k1.dvr + 2.0 * k2.dvr + 2.0 * k3.dvr + k4.dvr);
        state.theta += (stepSize / 6.0) * (k1.dtheta + 2.0 * k2.dtheta + 2.0 * k3.dtheta + k4.dtheta);
        state.vtheta += (stepSize / 6.0) * (k1.dvtheta + 2.0 * k2.dvtheta + 2.0 * k3.dvtheta + k4.dvtheta);
        state.phi += (stepSize / 6.0) * (k1.dphi + 2.0 * k2.dphi + 2.0 * k3.dphi + k4.dphi);

        if (state.theta < 0.0) {
            state.theta = -state.theta;
            state.vtheta = -state.vtheta;
        }
        if (state.theta > PI) {
            state.theta = 2.0 * PI - state.theta;
            state.vtheta = -state.vtheta;
        }
        state.theta = clamp(state.theta, 0.0001, PI - 0.0001);

        if (max(max(transmittance.r, transmittance.g), transmittance.b) < 0.008) {
            break;
        }
    }

    if (!captured && !escaped && state.vr > 0.0) {
        escaped = true;
    }

    vec3 background = vec3(0.0);
    if (escaped && !captured) {
        float sinTheta = sin(state.theta);
        vec3 asymptoticKerrDirection = normalize(vec3(
            sinTheta * cos(state.phi),
            sinTheta * sin(state.phi),
            cos(state.theta)
        ));
        vec3 asymptoticWorldDirection = normalize(
            asymptoticKerrDirection.x * diskRadial +
            asymptoticKerrDirection.y * diskTangent +
            asymptoticKerrDirection.z * diskNormal
        );
        background = texture(hdrMap, dirToUV(asymptoticWorldDirection)).rgb;
    }

    FragColor = vec4(diskRadiance + transmittance * background, 1.0);
}
