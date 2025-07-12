uniform sampler2DRect uInput;

uniform float uDistance;

out vec3 fColor;

void main() 
{
    vec3 color = vec3(0);
    color += texture(uInput, gl_FragCoord.xy + (uDistance + 0.5) * vec2(+1, +1)).rgb;
    color += texture(uInput, gl_FragCoord.xy + (uDistance + 0.5) * vec2(+1, -1)).rgb;
    color += texture(uInput, gl_FragCoord.xy + (uDistance + 0.5) * vec2(-1, +1)).rgb;
    color += texture(uInput, gl_FragCoord.xy + (uDistance + 0.5) * vec2(-1, -1)).rgb;
    fColor = color / 4.0;
}
