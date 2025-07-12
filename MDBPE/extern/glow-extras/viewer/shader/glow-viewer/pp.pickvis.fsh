uniform isampler2DRect uTexPick;
uniform int uFragID;
uniform int uRenderableID;
uniform int uNeighborhoodSize;
uniform vec3 uColor;
uniform vec3 uColorBorder;

out vec4 fColor;

vec2 o_1 = vec2(1,0);
vec2 o_2 = vec2(0,1);

 bool isOutline(int neighborhood_size, vec2 fragC, uint FragID)
 { 
    vec2 fragC_n = fragC - vec2(float(neighborhood_size), float(neighborhood_size));
    for(int u = 0; u <= neighborhood_size * 2; u++)
    {
        for(int v = 0; v <= neighborhood_size * 2; v++)
        {
            vec2 o = u * o_1 + v * o_2;
            uvec4 curr_id = uvec4(texture(uTexPick, fragC_n + o));
            if(FragID != uFragID && curr_id.y == uFragID && curr_id.x == uRenderableID)
            {
                return true;
            }
        }
    }
    return false;
 }

void main()
{
    vec2 fragCoord = gl_FragCoord.xy;
    uvec4 id = uvec4(texture(uTexPick, fragCoord));

    if (id.y == uFragID && id.x == uRenderableID)
    {
        fColor = vec4(uColor,1.0);
    }
    else if (isOutline(uNeighborhoodSize, fragCoord, id.y))
    {
        fColor = vec4(uColorBorder,1.0);
    }
    else 
    {
        discard;
    }
}
