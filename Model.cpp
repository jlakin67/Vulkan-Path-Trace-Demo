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
				position.x = attributes.vertices.at(index);
				position.y = attributes.vertices.at(index+1);
				position.z = attributes.vertices.at(index + 2);

				if (uniqueVertices.count(position) == 0) {
					uniqueVertices[position] = static_cast<uint32_t>(positions.size());
					positions.push_back(position);
				}

				indices.push_back(uniqueVertices[position]);

				indicesIndex++;
			}
		}
		
	}
}
