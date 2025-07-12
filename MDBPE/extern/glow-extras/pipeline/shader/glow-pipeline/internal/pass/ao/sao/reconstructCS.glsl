/**  vec4(-2.0f / (width*P[0][0]), 
          -2.0f / (height*P[1][1]),
          ( 1.0f - P[0][2]) / P[0][0], 
          ( 1.0f + P[1][2]) / P[1][1])
    
        mCameraData.projInfo = glm::vec4(
            float(-2.f / (mCameraData.resolution.x * mCameraData.proj[0][0])), float(-2.f / (mCameraData.resolution.y * mCameraData.proj[1][1])),
            float((1.f - mCameraData.proj[0][2]) / mCameraData.proj[0][0]), float((1.f + mCameraData.proj[1][2]) / mCameraData.proj[1][1]));
            
    where P is the projection matrix that maps camera space points 
    to [-1, 1] x [-1, 1].  That is, GCamera::getProjectUnit(). */
uniform vec4 uProjInfo;

/** Reconstruct camera-space P.xyz from screen-space S = (x, y) in
    pixels and camera-space z < 0.  Assumes that the upper-left pixel center
    is at (0.5, 0.5) [but that need not be the location at which the sample tap 
    was placed!]

    Costs 3 MADD.  Error is on the order of 10^3 at the far plane, partly due to z precision.
  */
vec3 reconstructCSPosition(vec2 S, float z) {
    return vec3((S.xy * uProjInfo.xy + uProjInfo.zw) * z, z);
}

/** Reconstructs screen-space unit normal from screen-space position */
vec3 reconstructCSFaceNormal(vec3 C) {
    return normalize(cross(dFdy(C), dFdx(C)));
}

vec3 reconstructNonUnitCSFaceNormal(vec3 C) {
    return cross(dFdy(C), dFdx(C));
}
