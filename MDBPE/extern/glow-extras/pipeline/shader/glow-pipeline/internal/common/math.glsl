#ifndef MATH_GLSL
#define MATH_GLSL

const float PI = 3.14159265359;

// Reference: http://stackoverflow.com/a/3380723/554283
float acosApproximation( float x ) 
{
   return (-0.69813170079773212 * x * x - 0.87266462599716477) * x + 1.5707963267948966;
}

#endif
