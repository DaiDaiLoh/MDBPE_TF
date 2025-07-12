in vec2 aPosition;

out vec2 vPosition;
out vec4 vScreenPosition;

void main() {
    vPosition = aPosition;
    vScreenPosition = vec4(aPosition * 2 - 1, 0, 1);
    gl_Position = vScreenPosition;
}
