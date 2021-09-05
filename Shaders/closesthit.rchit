#version 460
#extension GL_EXT_ray_tracing : require

//maxRecursion = 3

#define PI 3.14159265359
#define ONE_OVER_PI 0.3183099f
#define NUM_SAMPLES 16

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(binding = 2, set = 0) uniform SceneProperties 
{
	mat4 viewInverse;
	mat4 projInverse;
} scene;
layout(binding = 3, set = 0) buffer IndexBuffer { uint data[]; } indexBuffer;
layout(binding = 4, set = 0) buffer VertexBuffer { float data[]; } vertexBuffer;
layout(binding = 5, set = 0) buffer EmissionBuffer { float data[]; } emissionBuffer;
layout(binding = 6, set = 0) buffer ColorBuffer { float data[]; } colorBuffer;

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

//https://www.shadertoy.com/view/Xt23Ry
float rand(float co) { return fract(sin(co*(91.3458)) * 47453.5453); }
float rand(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453); }
float rand(vec3 co){ return rand(co.xy+rand(co.z)); }

//https://github.com/LWJGL/lwjgl3-demos/blob/main/res/org/lwjgl/demo/opengl/raytracing/randomCommon.glsl
vec3 randomSpherePoint(vec3 rand) {
  float ang1 = (rand.x + 1.0) * PI; // [-1..1) -> [0..2*PI)
  float u = rand.y; // [-1..1), cos and acos(2v-1) cancel each other out, so we arrive at [-1..1)
  float u2 = u * u;
  float sqrt1MinusU2 = sqrt(1.0 - u2);
  float x = sqrt1MinusU2 * cos(ang1);
  float y = sqrt1MinusU2 * sin(ang1);
  float z = u;
  return vec3(x, y, z);
}

vec3 randomHemispherePoint(vec3 rand, vec3 n) {
  /**
   * Generate random sphere point and swap vector along the normal, if it
   * points to the wrong of the two hemispheres.
   * This method provides a uniform distribution over the hemisphere, 
   * provided that the sphere distribution is also uniform.
   */
  vec3 v = randomSpherePoint(rand);
  return v * sign(dot(v, n));
}

hitAttributeEXT vec3 attribs;

void main() {
	//compute direct lighting and shadow
	vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
	ivec3 indices = ivec3(indexBuffer.data[3 * gl_PrimitiveID + 0], indexBuffer.data[3 * gl_PrimitiveID + 1], indexBuffer.data[3 * gl_PrimitiveID + 2]);
	vec3 vertexA = vec3(vertexBuffer.data[3 * indices.x + 0], vertexBuffer.data[3 * indices.x + 1], vertexBuffer.data[3 * indices.x + 2]);
	vec3 vertexB = vec3(vertexBuffer.data[3 * indices.y + 0], vertexBuffer.data[3 * indices.y + 1], vertexBuffer.data[3 * indices.y + 2]);
	vec3 vertexC = vec3(vertexBuffer.data[3 * indices.z + 0], vertexBuffer.data[3 * indices.z + 1], vertexBuffer.data[3 * indices.z + 2]);
	vec3 position = vertexA * barycentricCoords.x + vertexB * barycentricCoords.y + vertexC * barycentricCoords.z;
	vec3 normal = normalize(cross(vertexB - vertexA, vertexC - vertexA));
	vec3 lightColor = vec3(emissionBuffer.data[3 * gl_PrimitiveID + 0], emissionBuffer.data[3 * gl_PrimitiveID + 1], emissionBuffer.data[3 * gl_PrimitiveID + 2]);
	vec3 surfaceColor = vec3(colorBuffer.data[3 * gl_PrimitiveID + 0], colorBuffer.data[3 * gl_PrimitiveID + 1], colorBuffer.data[3 * gl_PrimitiveID + 2]);
	vec3 dir = position - payload.prevPos;
	float dist = min(1.0f / length(dir), 2.f);
	float diff = dist*dist*max(dot(normalize(dir), payload.prevNormal), 0.0f);
	if (payload.directPass) payload.directColor += (1.0f/NUM_SAMPLES)*diff*lightColor*surfaceColor;
	if (payload.indirectPass) {
		payload.indirectColor = (1.0f/NUM_SAMPLES)*diff*lightColor*surfaceColor;
		return;
	}

	float tmin = 0.001;
	float tmax = 10000.0;
	
	if (!payload.directPass) payload.directPass = true;
	else {
		payload.indirectPass = true;
		payload.directPass = false;	
	}
	for (int i = 0; i < NUM_SAMPLES; i++) {
		payload.indirectColor = vec3(0.0f);
		vec3 noise = vec3(rand(vec2(i,position.x)), rand(vec2(i,position.y)), rand(vec2(i,position.z)));
		vec3 randomDir = randomHemispherePoint(noise, normal);
		payload.prevPos = position;
		payload.prevNormal = normal;
		traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, position, tmin, randomDir, tmax, 0);
		dir = payload.prevPos - position;
		float dist = min(1.0f / length(dir), 2.f);
		diff = dist*dist*max(dot(normalize(dir), normal), 0.0f);
		if (payload.indirectPass) payload.directColor += (1.0f/NUM_SAMPLES)*diff*payload.indirectColor*surfaceColor;
	}
}