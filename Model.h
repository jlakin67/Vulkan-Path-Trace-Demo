#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <tiny_obj_loader.h>

class Model {
public:
	void loadModel(std::string path, tinyobj::ObjReaderConfig config);
	std::vector<glm::vec3> positions;
	std::vector<uint32_t> indices;
	std::vector<glm::vec3> emissions;
	std::vector<glm::vec3> colors;
private:

};
