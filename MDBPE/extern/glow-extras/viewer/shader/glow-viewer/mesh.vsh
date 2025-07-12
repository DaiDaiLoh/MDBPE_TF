uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

in vec3 aPosition;
in vec3 aNormal;
in vec3 aTangent;
in vec3 aColor;

out vec3 vPosition;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vColor;

void main()
{
    vPosition = vec3(uModel * vec4(aPosition, 1.0));
    vNormal = mat3(uModel) * aNormal;
    vTangent = mat3(uModel) * aTangent;
    vColor = aColor;

    gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
}
