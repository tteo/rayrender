#include "trimesh.h"

trimesh::trimesh(std::string inputfile, std::string basedir, Float scale, Float sigma,
                 std::shared_ptr<material> default_material,
        Float shutteropen, Float shutterclose, int bvh_type, random_gen rng,
        std::shared_ptr<Transform> ObjectToWorld, std::shared_ptr<Transform> WorldToObject, bool reverseOrientation) : 
  hitable(ObjectToWorld, WorldToObject, reverseOrientation) {
  mesh = std::unique_ptr<TriangleMesh>(new TriangleMesh(inputfile, basedir, default_material,
                                                        ObjectToWorld, WorldToObject, reverseOrientation));
  size_t n = mesh->nTriangles;
  for(size_t i = 0; i < n; i += 3) {
    triangles.add(std::make_shared<triangle>(mesh.get(), 
                                             &mesh->vertexIndices[i], 
                                             &mesh->normalIndices[i],
                                             &mesh->texIndices[i],
                                             ObjectToWorld, WorldToObject, reverseOrientation));
  }
  tri_mesh_bvh = std::make_shared<bvh_node>(triangles, shutteropen, shutterclose, bvh_type, rng);
}

bool trimesh::hit(const ray& r, Float t_min, Float t_max, hit_record& rec, random_gen& rng) {
  return(tri_mesh_bvh->hit(r, t_min, t_max, rec, rng));
}

bool trimesh::hit(const ray& r, Float t_min, Float t_max, hit_record& rec, Sampler* sampler) {
  return(tri_mesh_bvh->hit(r, t_min, t_max, rec, sampler));
}

bool trimesh::bounding_box(Float t0, Float t1, aabb& box) const {
  return(tri_mesh_bvh->bounding_box(t0,t1,box));
}


Float trimesh::pdf_value(const point3f& o, const vec3f& v, random_gen& rng, Float time) {
  return(triangles.pdf_value(o,v, rng, time));
}

Float trimesh::pdf_value(const point3f& o, const vec3f& v, Sampler* sampler, Float time) {
  return(triangles.pdf_value(o,v, sampler, time));
  
}

vec3f trimesh::random(const point3f& o, random_gen& rng, Float time) {
  return(triangles.random(o, rng, time));
}

vec3f trimesh::random(const point3f& o, Sampler* sampler, Float time) {
  return(triangles.random(o, sampler, time));
  
}

size_t trimesh::GetSize() {
  size_t total_size = tri_mesh_bvh->GetSize() + sizeof(*this) + mesh->texture_size;
  total_size += sizeof(Float*) * mesh->bump_texture_data.size();
  total_size += sizeof(Float*) * mesh->obj_texture_data.size();
  return(total_size);
}

std::pair<size_t,size_t> trimesh::CountNodeLeaf() {
  return(tri_mesh_bvh->CountNodeLeaf());
}
