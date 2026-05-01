#version 330 core
in vec4 vColor;
out vec4 FragColor;

void main() {
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float dist = dot(uv, uv);
    if (dist > 1.0) {
        discard;
    }

    float glow = exp(-3.0 * dist);
    FragColor = vec4(vColor.rgb * glow, vColor.a * glow);
}