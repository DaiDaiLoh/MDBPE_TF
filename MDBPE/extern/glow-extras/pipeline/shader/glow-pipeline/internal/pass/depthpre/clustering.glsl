#include <glow-pipeline/internal/common/globals.hh>
#include <glow-pipeline/internal/common/cszReconstruct.glsl>

uniform uvec3 uClusterDimensions;
uniform vec2 uClusteringScaleBias;

// Returns the cluster coordinate for the current fragment
uvec3 getCurrentCluster()
{
    uint zTile = uint(max(log2(reconstructCSZ(gl_FragCoord.z)) * uClusteringScaleBias.x + uClusteringScaleBias.y, 0.0));
    return uvec3(uvec2(gl_FragCoord.xy / GLOW_PIPELINE_CLUSTER_PIXEL_SIZE_X), zTile);
}

// Returns the 1D cluster index for a given cluster coordinate
uint getClusterIndex(in uvec3 cluster)
{
	return cluster.x + uClusterDimensions.x * cluster.y + (uClusterDimensions.x * uClusterDimensions.y) * cluster.z;
}

// Returns the 1D cluster index for the current fragment
uint getCurrentClusterIndex()
{
	return getClusterIndex(getCurrentCluster());
}
