in vec3 aPosition;

// -- instancing data --
in vec4 aModelC0;
in vec4 aModelC1;
in vec4 aModelC2;
in vec4 aModelC3;

void main()
{
    mat4 model = mat4(aModelC0, aModelC1, aModelC2, aModelC3);
	gl_Position = model * vec4(aPosition, 1.0);
}
