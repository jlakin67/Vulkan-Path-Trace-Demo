#include "Model.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <unordered_map>
#include <stdexcept>
#include <iostream>

void Model::loadModel(std::string path, tinyobj::ObjReaderConfig config) {
	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(path, config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader error: " << reader.Error();
		}
		throw std::runtime_error("Failed to load OBJ!");
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader warning: " << reader.Warning();
	}
	
	const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
	const tinyobj::attrib_t& attributes = reader.GetAttrib();
	const std::vector<tinyobj::material_t>& materials = reader.GetMaterials();

	std::unordered_map<glm::vec3, uint32_t> uniqueVertices;
	for (size_t i = 0; i < shapes.size(); i++) {
		tinyobj::mesh_t mesh = shapes[i].mesh;
		size_t indicesIndex = 0;
		for (int j = 0; j < mesh.num_face_vertices.size(); j++) {
			size_t faceSize = mesh.num_face_vertices.at(j);
			int material_id = mesh.material_ids.at(j);
			tinyobj::material_t material = materials.at(material_id);
			glm::vec3 surfaceColor(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
			glm::vec3 emissiveColor(material.emission[0], material.emission[1], material.emission[2]);
			colors.push_back(surfaceColor);
			emissions.push_back(emissiveColor);
			for (int k = 0; k < faceSize; k++) {
				uint32_t index = mesh.indices.at(indicesIndex).vertex_index;
				glm::vec3 position;
				position.x = attributes.vertices.at(3u*size_t(index));
				position.y = attributes.vertices.at(3u* size_t(index) +1);
				position.z = attributes.vertices.at(3u* size_t(index) + 2);

				if (uniqueVertices.count(position) == 0) {
					uniqueVertices[position] = static_cast<uint32_t>(positions.size());
					positions.push_back(position);
				}

				indices.push_back(uniqueVertices[position]);

				indicesIndex++;
			}
		}
		
	}
	
	/*
	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
				glm::vec3 vertex(vx, vy, vz);
				indices.push_back(positions.size());
				positions.push_back(vertex);
				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
				}

				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
			}
			index_offset += fv;

			// per-face material
			tinyobj::material_t material =  materials[shapes[s].mesh.material_ids[f]];
			glm::vec3 emission(material.emission[0], material.emission[1], material.emission[2]);
			glm::vec3 color(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
			colors.push_back(color);
		}
	}
	*/
}
