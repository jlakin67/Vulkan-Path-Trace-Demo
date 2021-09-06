#version 460
#extension GL_EXT_ray_tracing : require


layout(location = 0) rayPayloadInEXT Payload {
	vec3 prevPos;
	vec3 prevNormal;
	vec3 prevColor;
	vec3 directColor;
	float directShadow;
	vec3 indirectColor;
	float indirectShadow;
	bool directPass;
	bool indirectPass;
	bool terminateRay;
	vec3 lastLightPos;
} payload;

void main() {
	//if (payload.directPass) payload.directShadow += 1.0f;
	//payload.directColor = normalize(gl_LaunchIDEXT.xyz);
}