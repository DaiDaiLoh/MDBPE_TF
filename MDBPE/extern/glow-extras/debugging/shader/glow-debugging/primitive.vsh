in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uVP;

void main()
{
    gl_Position = uVP * uModel * vec4(aPosition, 1.0);
}
