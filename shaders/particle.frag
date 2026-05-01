#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float dist = dot(uv, uv);
    if (dist > 1.0) {
        discard;
    }

    float glow = exp(-2.0 * dist);
    float alphaMask = smoothstep(1.0, 0.05, dist);
    float intensity = 0.35 + 0.9 * glow;
    FragColor = vec4(vColor.rgb * intensity, vColor.a * alphaMask);
}
