#include "Json.h"
#include <fstream>
#include <iomanip>

void Json::ReadJson(std::vector<Sphere>& spheres, std::string& filePath)
{
    jsonMutex.lock();
    std::ifstream jsonFile(filePath);
    json jSpheres = json::parse(jsonFile);
    int sphereNumber = jSpheres["sphereNumber"];
    for (int i = 0; i < sphereNumber; i++)
    {
        //Retrieve current json object
        json currentJsonSphere = jSpheres["spheres"].at(i);

#if SIMDENABLED
        //Set Variables
        Vec3f center;
        center.vector(0) = currentJsonSphere["center"]["x"];
        center.vector(1) = currentJsonSphere["center"]["y"];
        center.vector(2) = currentJsonSphere["center"]["z"];

        Vec3f centerEnd;
        centerEnd.vector(0) = currentJsonSphere["centerEnd"]["x"];
        centerEnd.vector(1) = currentJsonSphere["centerEnd"]["y"];
        centerEnd.vector(2) = currentJsonSphere["centerEnd"]["z"];

        float radius = currentJsonSphere["radius"];
        float radiusEnd = currentJsonSphere["radiusEnd"];

        Vec3f surfaceColour;
        surfaceColour.vector(0) = currentJsonSphere["surfaceColour"]["x"];
        surfaceColour.vector(1) = currentJsonSphere["surfaceColour"]["y"];
        surfaceColour.vector(2) = currentJsonSphere["surfaceColour"]["z"];

        Vec3f surfaceColourEnd;
        surfaceColourEnd.vector(0) = currentJsonSphere["surfaceColourEnd"]["x"];
        surfaceColourEnd.vector(1) = currentJsonSphere["surfaceColourEnd"]["y"];
        surfaceColourEnd.vector(2) = currentJsonSphere["surfaceColourEnd"]["z"];

        float reflection = currentJsonSphere["reflection"];
        float reflectionEnd = currentJsonSphere["reflectionEnd"];

        float transparency = currentJsonSphere["transparency"];
        float transparencyEnd = currentJsonSphere["transparencyEnd"];


        Vec3f emissionColour;
        emissionColour.vector(0) = currentJsonSphere["emissionColour"]["x"];
        emissionColour.vector(1) = currentJsonSphere["emissionColour"]["y"];
        emissionColour.vector(2) = currentJsonSphere["emissionColour"]["z"];

        Vec3f emissionColourEnd;
        emissionColourEnd.vector(0) = currentJsonSphere["emissionColourEnd"]["x"];
        emissionColourEnd.vector(1) = currentJsonSphere["emissionColourEnd"]["y"];
        emissionColourEnd.vector(2) = currentJsonSphere["emissionColourEnd"]["z"];
#else
        //Set Variables
        Vec3f center;
        center.x = currentJsonSphere["center"]["x"];
        center.y = currentJsonSphere["center"]["y"];
        center.z = currentJsonSphere["center"]["z"];

        Vec3f centerEnd;
        centerEnd.x = currentJsonSphere["centerEnd"]["x"];
        centerEnd.y = currentJsonSphere["centerEnd"]["y"];
        centerEnd.z = currentJsonSphere["centerEnd"]["z"];

        float radius = currentJsonSphere["radius"];
        float radiusEnd = currentJsonSphere["radiusEnd"];

        Vec3f surfaceColour;
        surfaceColour.x = currentJsonSphere["surfaceColour"]["x"];
        surfaceColour.y = currentJsonSphere["surfaceColour"]["y"];
        surfaceColour.z = currentJsonSphere["surfaceColour"]["z"];

        Vec3f surfaceColourEnd;
        surfaceColourEnd.x = currentJsonSphere["surfaceColourEnd"]["x"];
        surfaceColourEnd.y = currentJsonSphere["surfaceColourEnd"]["y"];
        surfaceColourEnd.z = currentJsonSphere["surfaceColourEnd"]["z"];

        float reflection = currentJsonSphere["reflection"];
        float reflectionEnd = currentJsonSphere["reflectionEnd"];

        float transparency = currentJsonSphere["transparency"];
        float transparencyEnd = currentJsonSphere["transparencyEnd"];


        Vec3f emissionColour;
        emissionColour.x = currentJsonSphere["emissionColour"]["x"];
        emissionColour.y = currentJsonSphere["emissionColour"]["y"];
        emissionColour.z = currentJsonSphere["emissionColour"]["z"];

        Vec3f emissionColourEnd;
        emissionColourEnd.x = currentJsonSphere["emissionColourEnd"]["x"];
        emissionColourEnd.y = currentJsonSphere["emissionColourEnd"]["y"];
        emissionColourEnd.z = currentJsonSphere["emissionColourEnd"]["z"];
#endif
        Sphere sphere = Sphere(center, radius, surfaceColour, reflection, transparency, emissionColour);

        //Set End Variables
        sphere.centerEnd = centerEnd;
        sphere.radiusEnd = radiusEnd;
        sphere.surfaceColourEnd = surfaceColourEnd;
        sphere.reflectionEnd = reflectionEnd;
        sphere.transparencyEnd = transparencyEnd;
        sphere.emissionColourEnd = emissionColourEnd;

        //Put Sphere Into Vector To Send Back
        spheres.push_back(sphere);
    }

    //Close File
    jsonFile.close();
    jsonMutex.unlock();
}
