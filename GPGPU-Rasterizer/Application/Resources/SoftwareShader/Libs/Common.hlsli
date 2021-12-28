#ifndef DEF_COMMON_HLSLI
#define DEF_COMMON_HLSLI

inline float cross2d(float2 a, float2 b)
{
	return a.x * b.y - a.y * b.x;
}

inline uint FlattenedGlobalThreadId(uint groupIndex, uint3 groupId, uint3 groupDimensions, uint3 dispatchDimensions)
{
	return groupIndex + (groupId.z * dispatchDimensions.x * dispatchDimensions.y + groupId.y * dispatchDimensions.x + groupId.x) * groupDimensions.x * groupDimensions.y * groupDimensions.z;
}

#endif
