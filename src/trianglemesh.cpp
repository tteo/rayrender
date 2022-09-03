#include "trianglemesh.h"

#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobj/tiny_obj_loader.h"
#endif


void LoadMtlMaterials(std::vector<std::shared_ptr<material> > &mtl_materials,
                      std::vector<tinyobj::material_t > &materials,
                      std::vector<unsigned char * > &obj_texture_data,
                      std::vector<unsigned char * > &bump_texture_data,
                      std::vector<std::shared_ptr<bump_texture> > &bump_textures,
                      std::vector<std::shared_ptr<alpha_texture> > &alpha_textures,
                      size_t &texture_size,
                      const std::string inputfile, const std::string basedir, bool has_sep,
                      std::shared_ptr<material> default_material, bool load_materials,
                      bool load_textures) {
  mtl_materials.reserve(materials.size()+1);
  obj_texture_data.reserve(materials.size()+1);
  bump_texture_data.reserve(materials.size()+1);
  bump_textures.reserve(materials.size()+1);
  alpha_textures.reserve(materials.size()+1);
  
  //For default texture
  alpha_textures.push_back(nullptr);
  bump_textures.push_back(nullptr);
  
  std::vector<vec3f > diffuse_materials(materials.size()+1);
  std::vector<vec3f > specular_materials(materials.size()+1);
  std::vector<Float > ior_materials(materials.size()+1);
  std::vector<bool > has_diffuse(materials.size()+1, false);
  std::vector<bool > has_transparency(materials.size()+1, false);
  std::vector<bool > has_single_diffuse(materials.size()+1, false);
  std::vector<bool > has_alpha(materials.size()+1, false);
  std::vector<bool > has_bump(materials.size()+1, false);
  std::vector<Float > bump_intensity(materials.size()+1);
  
  std::vector<int > nx_mat(materials.size()+1);
  std::vector<int > ny_mat(materials.size()+1);
  std::vector<int > nn_mat(materials.size()+1);
  
  std::vector<int > nx_mat_bump(materials.size()+1);
  std::vector<int > ny_mat_bump(materials.size()+1);
  std::vector<int > nn_mat_bump(materials.size()+1);
  int nx, ny, nn;
  
  if(load_materials) {
    for (size_t i = 0; i < materials.size(); i++) {
      nx = 0; ny = 0; nn = 0;
      if(strlen(materials[i].diffuse_texname.c_str()) > 0 && load_textures) {
        int ok;
        std::replace(materials[i].diffuse_texname.begin(), materials[i].diffuse_texname.end(), '\\', separator());
        if(has_sep) {
          ok = stbi_info((basedir + separator() + materials[i].diffuse_texname).c_str(), &nx, &ny, &nn);
          obj_texture_data.push_back(stbi_load((basedir + separator() + materials[i].diffuse_texname).c_str(), &nx, &ny, &nn, 0));
        } else {
          ok = stbi_info((materials[i].diffuse_texname).c_str(), &nx, &ny, &nn);
          obj_texture_data.push_back(stbi_load((materials[i].diffuse_texname).c_str(), &nx, &ny, &nn, 0));
        }

        if(!obj_texture_data[i] || !ok) {
          REprintf("Load failed: %s\n", stbi_failure_reason());
          if(has_sep) {
            throw std::runtime_error("Loading failed of: " + (basedir + separator() + materials[i].diffuse_texname) + 
                                     "-- nx/ny/channels :"  + std::to_string(nx)  +  "/"  +  std::to_string(ny)  +  "/"  +  std::to_string(nn));
          } else {
            throw std::runtime_error("Loading failed of: " + materials[i].diffuse_texname + 
                                     "-- nx/ny/channels :" + std::to_string(nx)  +  "/"  +  std::to_string(ny)  +  "/"  +  std::to_string(nn));
          }
        }
        if(nx == 0 || ny == 0 || nn == 0) {
          if(has_sep) {
            throw std::runtime_error("Could not find " + (basedir + separator() + materials[i].diffuse_texname));
          } else {
            throw std::runtime_error("Could not find " + materials[i].diffuse_texname);
          }
        }
        Rprintf("(%i/%i) Loading OBJ Material %s Dims: (%i/%i/%i) \n", i+1, materials.size(),materials[i].name.c_str(),nx,ny,nn);
        

        texture_size += sizeof(unsigned char) * nx * ny * nn;
        has_diffuse[i] = true;
        has_single_diffuse[i] = false;
        nx_mat[i] = nx;
        ny_mat[i] = ny;
        nn_mat[i] = nn;
        has_alpha[i] = false;
        if(nn == 4) {
          for(int j = 0; j < nx - 1; j++) {
            for(int k = 0; k < ny - 1; k++) {
              if(obj_texture_data[i][4*j + 4*nx*k + 3] != 255) {
                has_alpha[i] = true;
                break;
              }
            }
            if(has_alpha[i]) {
              break;
            }
          }
        } 
      } else if (sizeof(materials[i].diffuse) == 12 && materials[i].dissolve == 1) {
        obj_texture_data.push_back(nullptr);
        diffuse_materials[i] = vec3f(materials[i].diffuse[0],materials[i].diffuse[1],materials[i].diffuse[2]);
        has_diffuse[i] = true;
        has_alpha[i] = false;
        has_single_diffuse[i] = true;
      } else if(materials[i].dissolve < 1) {
        obj_texture_data.push_back(nullptr);
        specular_materials[i] = vec3f(materials[i].diffuse[0],materials[i].diffuse[1],materials[i].diffuse[2]);
        ior_materials[i] = materials[i].ior;
        has_alpha[i] = false;
        has_transparency[i] = true; 
      } else {
        obj_texture_data.push_back(nullptr);
        has_diffuse[i] = false;
        has_alpha[i] = false;
        has_single_diffuse[i] = false;
      }
      if(strlen(materials[i].bump_texname.c_str()) > 0 && load_textures) {
        std::replace(materials[i].bump_texname.begin(), materials[i].bump_texname.end(), '\\', separator());
        
        if(has_sep) {
          bump_texture_data[i] = stbi_load((basedir + separator() + materials[i].bump_texname).c_str(), &nx, &ny, &nn, 0);
        } else {
          bump_texture_data[i] = stbi_load(materials[i].bump_texname.c_str(), &nx, &ny, &nn, 0);
        }
        texture_size += sizeof(unsigned char) * nx * ny * nn;
        if(nx == 0 || ny == 0 || nn == 0) {
          if(has_sep) {
            throw std::runtime_error("Could not find " + basedir + separator() + materials[i].bump_texname);
          } else {
            throw std::runtime_error("Could not find " + materials[i].bump_texname);
          }
        }
        nx_mat_bump[i] = nx;
        ny_mat_bump[i] = ny;
        nn_mat_bump[i] = nn;
        bump_intensity[i] = materials[i].bump_texopt.bump_multiplier;
        has_bump[i] = true;
      } else {
        bump_texture_data.push_back(nullptr);
        bump_intensity[i] = 1.0f;
        has_bump[i] = false;
      }
      std::shared_ptr<alpha_texture> alpha = nullptr;
      std::shared_ptr<bump_texture> bump = nullptr;
      
      if(has_alpha[i]) {
        alpha = std::make_shared<alpha_texture>(obj_texture_data[i], 
                                                nx_mat[i], ny_mat[i], nn_mat[i]);
      } 
      if(has_bump[i]) {
        bump = std::make_shared<bump_texture>(bump_texture_data[i],
                                              nx_mat_bump[i], ny_mat_bump[i], nn_mat_bump[i],
                                              bump_intensity[i]);
      } 
      alpha_textures.push_back(alpha);
      bump_textures.push_back(bump);
    }
  }
  
  //First texture is default (when shapes[s].mesh.material_ids[f] == -1)
  mtl_materials.push_back(default_material);
  
  for(int material_num = 0; material_num < materials.size(); material_num++) {
    std::shared_ptr<material> tex = nullptr;
    
    if(has_transparency[material_num]) {
      tex = std::make_shared<dielectric>(specular_materials[material_num], 
                                         ior_materials[material_num], vec3f(0,0,0), 
                                         0);
    } else if(has_diffuse[material_num]) {
      if(has_single_diffuse[material_num]) {
        tex = std::make_shared<lambertian>(std::make_shared<constant_texture>(diffuse_materials[material_num]));
      } else {
        tex = std::make_shared<lambertian>(std::make_shared<image_texture_char>(obj_texture_data[material_num],
                                                                           nx_mat[material_num], 
                                                                           ny_mat[material_num],
                                                                           nn_mat[material_num]));
      }
    } else {
      tex = default_material;
    }
    mtl_materials.push_back(tex);
  }
}

TriangleMesh::TriangleMesh(std::string inputfile, std::string basedir,
                           std::shared_ptr<material> default_material, 
                           bool load_materials, bool load_textures,
                           std::shared_ptr<Transform> ObjectToWorld, 
                           std::shared_ptr<Transform> WorldToObject, 
                           bool reverseOrientation) : nTriangles(0) {
  std::string warn, err;
  texture_size = 0;
  vertexIndices.clear();
  normalIndices.clear();
  texIndices.clear();
  face_material_id.clear();
  
  bool has_sep = true;
  bool ret = true;
  
  tinyobj::ObjReaderConfig reader_config;
  reader_config.mtl_search_path = basedir.c_str(); // Path to material files
  
  tinyobj::ObjReader reader;
  
  if (!reader.ParseFromFile(inputfile, reader_config)) {
    ret = false;
  }
  
  if (!reader.Warning().empty()) {
    Rcpp::Rcout << "TinyObjReader Warning: " << reader.Warning();
  }
  
  auto& attrib = reader.GetAttrib();
  auto& shapes = reader.GetShapes();
  auto& materials = reader.GetMaterials();
  
  if(strlen(basedir.c_str()) == 0) {
    has_sep = false;
  }
  if(ret) {
    nVertices = attrib.vertices.size();
    for (size_t s = 0; s < shapes.size(); s++) {
      nTriangles += shapes[s].mesh.indices.size();
      for(size_t m = 0; m < shapes[s].mesh.indices.size(); m++) {
        vertexIndices.push_back(shapes[s].mesh.indices[m].vertex_index);
        normalIndices.push_back(shapes[s].mesh.indices[m].normal_index);
        texIndices.push_back(shapes[s].mesh.indices[m].texcoord_index);
      }
    }
    
    nNormals = attrib.normals.size();
    nTex = attrib.texcoords.size();
    p.reset(new point3f[nVertices / 3]);
    for (size_t i = 0; i < nVertices; i += 3) {
      p[i / 3] = (*ObjectToWorld)(point3f(attrib.vertices[i+0],
                                          attrib.vertices[i+1],
                                          attrib.vertices[i+2]));
    }
    
    
    if(nNormals > 0) {
      n.reset(new normal3f[nNormals / 3]);
      for (size_t i = 0; i < nNormals; i += 3) {
        n[i / 3] = (*ObjectToWorld)(normal3f(attrib.normals[i+0],
                                             attrib.normals[i+1],
                                             attrib.normals[i+2]));
      }
    } else {
      n = nullptr;
    }
    
    if(nTex > 0) {
      uv.reset(new point2f[nTex / 2]);
      for (size_t i = 0; i < nTex; i += 2) {
        uv[i / 2] = point2f(attrib.texcoords[i+0],
                            attrib.texcoords[i+1]);
      }
    } else {
      uv = nullptr;
    }
    
    LoadMtlMaterials(mtl_materials, materials, obj_texture_data,
                     bump_texture_data, bump_textures, alpha_textures,
                     texture_size, inputfile, basedir, has_sep, default_material,
                     load_materials, load_textures);
    for (size_t s = 0; s < shapes.size(); s++) {
      // Loop over faces(polygon)
      for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
        // per-face material (-1 is default material, which is indexed at 0, hence the +1)
        int material_num = shapes[s].mesh.material_ids[f] + 1;
        face_material_id.push_back(material_num);
      }
    }
  } else {
    std::string mes = "Error reading " + inputfile + ": ";
    throw std::runtime_error(mes + reader.Error());
  }
}

size_t TriangleMesh::GetSize() {
  size_t size = sizeof(*this);
  size += nTex / 2 * sizeof(point2f) + 
          nNormals / 3 * sizeof(normal3f) + 
          nVertices / 3 * sizeof(point3f);
  for(size_t i = 0; i < mtl_materials.size(); i++) {
    size += mtl_materials[i]->GetSize();
  }
  size += face_material_id.size()*sizeof(int);
  size += sizeof(unsigned char *) * bump_texture_data.size();
  size += sizeof(unsigned char *) * obj_texture_data.size();
  size += sizeof(std::shared_ptr<alpha_texture>) * alpha_textures.size();
  size += sizeof(std::shared_ptr<bump_texture>)  * bump_textures.size();
  size += sizeof(int) * vertexIndices.size();
  size += sizeof(int) * normalIndices.size();
  size += sizeof(int) * texIndices.size();
  Rcpp::Rcout << "Trimesh Storage Size: " << size << "\n";
  
  size += texture_size;
  return(size);
}
