in vec2 aPosition;

out vec4 vHDCPosition;
out vec4 vPrevHDCPosition;

uniform mat4 uCleanVp;
uniform mat4 uPrevCleanVp;

void main() {
    vec4 outPosition = vec4(aPosition * 2 - 1, 0, 1);
    vHDCPosition = uCleanVp * outPosition;
    vPrevHDCPosition = uPrevCleanVp * outPosition;

    gl_Position = outPosition;
}
