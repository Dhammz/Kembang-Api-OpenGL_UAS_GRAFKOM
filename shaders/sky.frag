#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform float uTime;
uniform vec2 uResolution;

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.23);
    return fract(p.x * p.y);
}

void main() {
    vec3 horizon = vec3(0.03, 0.08, 0.18);
    vec3 zenith = vec3(0.0, 0.01, 0.05);
    float h = clamp(vUV.y, 0.0, 1.0);
    vec3 color = mix(horizon, zenith, h);

    vec2 pixel = vUV * max(uResolution, vec2(1.0));
    vec2 grid = floor(pixel / 4.0);
    float starSeed = hash21(grid);
    float star = step(0.9965, starSeed);
    float twinkle = 0.65 + 0.35 * sin(uTime * 4.0 + starSeed * 100.0);
    color += vec3(0.85, 0.9, 1.0) * star * twinkle;

    FragColor = vec4(color, 1.0);
}
