#extension GL_EXT_gpu_shader4 : require

//////////////////////////////////////////////////////////////////////////////////////////////
// Tunable Parameters:

/** Increase to make depth edges crisper. Decrease to reduce flicker. */
#define EDGE_SHARPNESS     (1.0)

/** Step in 2-pixel intervals since we already blurred against neighbors in the
    first AO pass.  This constant can be increased while R decreases to improve
    performance at the expense of some dithering artifacts. 
    
    Morgan found that a scale of 3 left a 1-pixel checkerboard grid that was
    unobjectionable after shading was applied but eliminated most temporal incoherence
    from using small numbers of sample taps.
    */
#define SCALE               (2)

/** Filter radius in pixels. This will be multiplied by SCALE. */
#define R                   (4)


//////////////////////////////////////////////////////////////////////////////////////////////

/** Type of data to read from uInput.  This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_TYPE        float

/** Swizzle to use to extract the channels of uInput. This macro allows
    the same blur shader to be used on different kinds of input data. */
#define VALUE_COMPONENTS   r

#define VALUE_IS_KEY       0

/** Channel encoding the bilateral key value (which must not be the same as VALUE_COMPONENTS) */
#define KEY_COMPONENTS     gb


// Gaussian coefficients
const float gaussian[R + 1] = 
# if R == 3
    float[](0.153170, 0.144893, 0.122649, 0.092902);
# elif R == 4
    //float[](0.398943, 0.241971, 0.053991, 0.004432, 0.000134);  // stddev = 1.0
    float[](0.153170, 0.144893, 0.122649, 0.092902, 0.062970);  // stddev = 2.0
# elif R == 6
    float[](0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108); // stddev = 3.0
# endif

uniform sampler2D   uInput;

/** (1, 0) or (0, 1)*/
uniform ivec2       uAxis;

out vec3            fAOOut;

#define  result         fAOOut.VALUE_COMPONENTS
#define  keyPassThrough fAOOut.KEY_COMPONENTS

/** Returns a number on (0, 1) */
float unpackKey(vec2 p) {
    return p.x * (256.0 / 257.0) + p.y * (1.0 / 257.0);
}


void main() {
    ivec2 ssC = ivec2(gl_FragCoord.xy);

    vec4 temp = texelFetch(uInput, ssC, 0);
    
    keyPassThrough = temp.KEY_COMPONENTS;
    float key = unpackKey(keyPassThrough);

    VALUE_TYPE sum = temp.VALUE_COMPONENTS;

    if (key == 1.0) { 
        // Sky pixel (if you aren't using depth keying, disable this test)
        result = sum;
        return;
    }

    // Base weight for depth falloff.  Increase this for more blurriness,
    // decrease it for better edge discrimination
    float BASE = gaussian[0];
    float totalWeight = BASE;
    sum *= totalWeight;

    for (int r = -R; r <= R; ++r) {
        // We already handled the zero case above.  This loop should be unrolled and the static branch optimized out,
        // so the IF statement has no runtime cost
        if (r != 0) {
            temp = texelFetch(uInput, ssC + uAxis * (r * SCALE), 0);
            float      tapKey = unpackKey(temp.KEY_COMPONENTS);
            VALUE_TYPE value  = temp.VALUE_COMPONENTS;
            
            // spatial domain: offset gaussian tap
            float weight = 0.3 + gaussian[abs(r)];
            
            // range domain (the "bilateral" weight). As depth difference increases, decrease weight.
            weight *= max(0.0, 1.0
                - (EDGE_SHARPNESS * 2000.0) * abs(tapKey - key)
                );

            sum += value * weight;
            totalWeight += weight;
        }
    }
 
    const float epsilon = 0.0001;
    result = sum / (totalWeight + epsilon);	
}
