#version 460
#extension GL_EXT_ray_tracing : require

/*
layout(location = 0) rayPayloadEXT Payload {
	vec3 prevPos;
	vec3 prevNormal;
	vec3 directColor;
	float directShadow;
	vec3 indirectColor;
	float indirectShadow;
	bool directPass;
	bool indirectPass;
	bool terminateRay;
} payload;
*/
void main() {
	//if (payload.directPass) payload.directShadow += 1.0f;
	//payload.terminateRay = true;
}