#pragma once
#include"nlohmann/json.hpp"
#include "Sphere.h"
#include "GlobalNewDelete.h"
#include <string>
#include <vector>
#include <mutex>
using json = nlohmann::json;

class Json
{
public:
	void ReadJson(std::vector<Sphere>& spheres, std::string& filePath);
	std::mutex jsonMutex;
};

