in vec4 vHDCPosition;
in vec4 vPrevHDCPosition;

out vec2 fVelocity;

void main()
{
    vec2 a = (vHDCPosition.xy / vHDCPosition.w) * 0.5 + 0.5;
	vec2 b = (vPrevHDCPosition.xy / vPrevHDCPosition.w) * 0.5 + 0.5;
	fVelocity = a - b;
}
