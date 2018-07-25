#include <optix_world.h>
using namespace optix;

// This is to be plugged into an RTgeometry object to represent
// a triangle mesh with a vertex buffer of triangle soup (triangle list)
// with an interleaved position, normal, texturecoordinate layout.

struct Vertex{
	float3 position;
	float3 normal;
};

rtBuffer<Vertex> vertex_buffer;
rtBuffer<int3>   index_buffer;

rtDeclareVariable(float3, geometric_normal, attribute geometric_normal, );
rtDeclareVariable(float3, shading_normal, attribute shading_normal, );
rtDeclareVariable(optix::Ray, ray, rtCurrentRay, );

RT_PROGRAM void mesh_intersect(int primIdx){
	int3 v_idx = index_buffer[primIdx];

	float3 p0 = vertex_buffer[v_idx.x].position;
	float3 p1 = vertex_buffer[v_idx.y].position;
	float3 p2 = vertex_buffer[v_idx.z].position;

	// Intersect ray with triangle
	float3 n;
	float  t, beta, gamma;
	if (intersect_triangle(ray, p0, p1, p2, n, t, beta, gamma)) {

		if (rtPotentialIntersection(t)) {

			float3 n0 = vertex_buffer[v_idx.x].normal;
			float3 n1 = vertex_buffer[v_idx.y].normal;
			float3 n2 = vertex_buffer[v_idx.z].normal;
			shading_normal = normalize(n0*(1.0f - beta - gamma) + n1*beta + n2*gamma);
			geometric_normal = normalize(n);

			rtReportIntersection(0);
		}
	}
}

RT_PROGRAM void mesh_bounds(int primIdx, optix::Aabb* aabb){
	const int3 v_idx = index_buffer[primIdx];

	const float3 v0 = vertex_buffer[v_idx.x].position;
	const float3 v1 = vertex_buffer[v_idx.y].position;
	const float3 v2 = vertex_buffer[v_idx.z].position;
	const float  area = length(cross(v1 - v0, v2 - v0));

	if (area > 0.0f && !isinf(area)) {
		aabb->m_min = fminf(fminf(v0, v1), v2);
		aabb->m_max = fmaxf(fmaxf(v0, v1), v2);
	}
	else {
		aabb->invalidate();
	}
}