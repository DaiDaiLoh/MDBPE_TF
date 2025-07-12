vec3 sharpen(sampler2DRect tex, vec2 fragCoord, float scaleOffset, float intensity) {
    vec3 color = texture(tex, fragCoord).rgb;
    vec3 colorUp = texture(tex, fragCoord + vec2(0, -scaleOffset)).rgb;
    vec3 colorRight = texture(tex, fragCoord + vec2(scaleOffset, 0)).rgb;
    vec3 colorLeft = texture(tex, fragCoord + vec2(-scaleOffset, 0)).rgb;
    vec3 colorDown = texture(tex, fragCoord + vec2(0, scaleOffset)).rgb;
    return clamp(color + (4 * color - colorUp - colorRight - colorLeft - colorDown) * intensity, 0, 1);
}
