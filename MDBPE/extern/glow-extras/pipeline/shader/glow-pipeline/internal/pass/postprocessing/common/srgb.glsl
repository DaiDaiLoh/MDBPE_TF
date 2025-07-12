
vec3 sRGBtoLinear(vec3 color, float gamma)
{
    return pow(color, vec3(gamma));
}
vec3 linearToSRGB(vec3 color, float gamma)
{
    return pow(color, vec3(1 / gamma));
}
