
// This is an adaptation from the book Physically Based Rendering from Theory to implementation 
// to be on the safe side the copyright notice is included. However all code here is rewritten based 
// on the ideas presented in the book
// So the copyright notice probably doesn't need to be included... idk.
// either way here it is:
// Daniel 22 Feb 2018


/*
 * Copyright(c) 1998 - 2015, Matt Pharr, Greg Humphreys, and Wenzel Jakob.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met :
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// NOTE:
// 
// We have no acceleration structures in place
// if this turns out to be a hot spot 
// we should probably change how we voxelize the scene 
// and also keep track of which tris are in which voxels and utilize that in the ray tracing
// or even build an octtree directly where the voxels are the lowest level. Same idea.
//
// However until we confirmed that this is indeed the hotspot we'll hold off on that.
// it's quite a bit of work after all. 
// Daniel 22 Feb 2018


struct Ray {
	vec3 origin, dir;
};

typedef Eigen::Vector3d vec3d;

struct InternalRay {
	vec3d origin;
	// the direction is implicitly encoded in shear + permutation
	// after shear and permutation is applied the dir will be (0,0,1)
	// so the dir doesn't need to be accessed.
	// shear applied make ray.dir = (0,0,1)
	vec3d shear;
	// permuation so that ray.dir.z is larger than x,y 
	ivec3 permutation;

	float t_max;
};

int max_dimension(vec3 v) {
	if (v.x() > v.y()) {
		if (v.x() > v.z()) return 0;
		else return 2;
	} else {
		if (v.y() > v.z())return 1;
		else return 2;
	}
}

vec3d permute(vec3d v, ivec3 permutation) {
	return vec3d(v[permutation.x()], v[permutation.y()], v[permutation.z()]);
}


InternalRay make_internal_ray(Ray ray) {
	ivec3 permutation;
	// permute so that z is the largest dimension
	permutation.z() = max_dimension(ray.dir.cwiseAbs2());
	permutation.x() = (permutation.z() + 1)%3;
	permutation.y() = (permutation.x() + 1)%3;
	
	vec3d dir = permute(ray.dir.cast<double>(), permutation);
	
	// shear so that ray.dir -> (0,0,1)
	vec3d shear(
		-dir.x() / dir.z(),
		-dir.y() / dir.z(),
		1.0f / dir.z());
	
	return {ray.origin.cast<double>(), shear, permutation, FLT_MAX };
}
struct HitInfo {
	vec3 pos;
	float t;
	float b0;
	float b1;
	float b2;
};

bool intersect(InternalRay &ray, const Triangle &t, HitInfo *hit_info) {
	vec3d a = t.a.cast<double>() - ray.origin;
	vec3d b = t.b.cast<double>() - ray.origin;
	vec3d c = t.c.cast<double>() - ray.origin;

	//permute the triangle in accordence with the ray. 
	permute(a, ray.permutation);
	permute(b, ray.permutation);
	permute(c, ray.permutation);

	a.x() += ray.shear.x() * a.z();
	a.y() += ray.shear.y() * a.z();

	b.x() += ray.shear.x() * b.z();
	b.y() += ray.shear.y() * b.z();

	c.x() += ray.shear.x() * c.z();
	c.y() += ray.shear.y() * c.z();
	// hold off with shearing the z coord for as long as possible (early outs). 

	// now all we need to do is compute barycentric coordinates and see if the origin is within the triangle
	// the following does that but with a few early outs... Perf?
	double e0 = b.x() * c.y() - b.y() * c.x();
	double e1 = c.x() * a.y() - c.y() * a.x();
	double e2 = a.x() * b.y() - a.y() * b.x();

	if (e0 == 0 || e1 == 0 || e2 == 0) {
		printf("can't deside if inside of triangle, should recompute edgefunctions with doubles");
	}

	// if signs differ we're outside of the tri!
	if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0)) return false;
	
	// if we hit the triangle edge on we report no hit.
	double det = e0 + e1 + e2;
	if (det == 0) return false;

	// Now apply the shear to z coord
	a.z() *= ray.shear.z();
	b.z() *= ray.shear.z();
	c.z() *= ray.shear.z();

	double t_scaled = e0 * a.z() + e1 * b.z() + e2 * c.z();
	
	// check that the ray is inside of our range ie. that t>= 0 && t < t_max
	// done before div with det cause fp div is slow.
	if (det < 0 && (t_scaled >= 0 || t_scaled < ray.t_max*det)) return false;
	else if (det > 0 && (t_scaled <= 0 || t_scaled > ray.t_max*det)) return false;

	// finally compute barycentric coords
	double inv_det = 1.0f / det;
	float b0 = (float)(e0 * inv_det);
	float b1 = (float)(e1 * inv_det);
	float b2 = (float)(e2 * inv_det);

	// set info
	hit_info->t = t_scaled * inv_det;
	hit_info->pos = t.a*b0 + t.b*b1 + t.c*b2;
	hit_info->b0 = b0;
	hit_info->b1 = b1;
	hit_info->b2 = b2;
	return true;
}


/**
	Returns whether the ray hits anything, and if so sets uv to the lightmap uv coordinates of the hit point
*/
bool lightmap_uv_of_closest_intersection(Ray ray, Atlas_Output_Mesh *light_map_mesh, Mesh mesh, vec2 *hit_uv)
{
	int closest_tri = -1;
	HitInfo closest_hit_info;
	InternalRay &i_ray = make_internal_ray(ray);

	for (int face_idx = 0; face_idx < light_map_mesh->index_count / 3; face_idx++) {
		auto new_a_idx = light_map_mesh->index_array[face_idx * 3 + 0];
		auto new_b_idx = light_map_mesh->index_array[face_idx * 3 + 1];
		auto new_c_idx = light_map_mesh->index_array[face_idx * 3 + 2];

		auto a_idx = light_map_mesh->vertex_array[new_a_idx].xref;
		auto b_idx = light_map_mesh->vertex_array[new_b_idx].xref;
		auto c_idx = light_map_mesh->vertex_array[new_c_idx].xref;

		Triangle t =
		{
			mesh.verts[a_idx],
			mesh.verts[b_idx],
			mesh.verts[c_idx]
		};

		HitInfo hit_info;
		if (intersect(i_ray, t, &hit_info)) {
			i_ray.t_max = min(i_ray.t_max, hit_info.t);
			closest_tri = face_idx;
			closest_hit_info = hit_info;
		}
	}

	if (closest_tri != -1) {
		auto new_a_idx = light_map_mesh->index_array[closest_tri * 3 + 0];
		auto new_b_idx = light_map_mesh->index_array[closest_tri * 3 + 1];
		auto new_c_idx = light_map_mesh->index_array[closest_tri * 3 + 2];
		float u_a = light_map_mesh->vertex_array[new_a_idx].uv[0];
		float v_a = light_map_mesh->vertex_array[new_a_idx].uv[1];
		float u_b = light_map_mesh->vertex_array[new_b_idx].uv[0];
		float v_b = light_map_mesh->vertex_array[new_b_idx].uv[1];
		float u_c = light_map_mesh->vertex_array[new_c_idx].uv[0];
		float v_c = light_map_mesh->vertex_array[new_c_idx].uv[1];
		(*hit_uv)[0] = u_a * closest_hit_info.b0 + u_b * closest_hit_info.b1 + u_c * closest_hit_info.b2;
		(*hit_uv)[1] = v_a * closest_hit_info.b0 + v_b * closest_hit_info.b1 + v_c * closest_hit_info.b2;
		return true;
	} else {
		return false;
	}
}






