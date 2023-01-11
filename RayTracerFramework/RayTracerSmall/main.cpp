// [header]
// A very basic raytracer example.
// [/header]
// [compile]
// c++ -o raytracer -O3 -Wall raytracer.cpp
// [/compile]
// [ignore]
// Copyright (C) 2012  www.scratchapixel.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// [/ignore]
#include <stdlib.h>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>
#include <iostream>
// Windows only
#include <algorithm>
#include <sstream>
#include <string.h>
// Local
#include "Sphere.h"
#include "Trackers.h"
#include "MemoryPool.h"
#include "ThreadManager.h"
#include "Json.h"

#define FILEIOENABLED true

//Globals
MemoryPool* defaultPool;
MemoryPool* spherePool;
bool defaultMemoryPoolEnabled;
bool sphereMemoryPoolEnabled;
bool renderJson;

ThreadManager* threadManager;
bool threadingEnabled;

Json* jsonRenderer;

std::mutex sphereCopyMutex;


#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
bool parallelProcessingEnabled;
int MAXPROCESSES = 12;
#include <sys/types.h>
#include <unistd.h>
#else
// Windows doesn't define these values by default, Linux does
#define M_PI 3.141592653589793
#define INFINITY 1e8
#endif

//[comment]
// This variable controls the maximum recursion depth
//[/comment]
#define MAX_RAY_DEPTH 5

float mix(const float &a, const float &b, const float &mix)
{
	return b * mix + a * (1 - mix);
}
//[comment]
//Same as other trace function just uses sphere tracker
//[comment]
Vec3f pooltrace(
	const Vec3f& rayorig,
	const Vec3f& raydir,
	std::vector<Sphere*> spheres,
	const int& depth)
{
	//if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
	float tnear = INFINITY;
	const Sphere* sphere = nullptr;
	// find intersection of this ray with the sphere in the scene
	for (unsigned i = 0; i < spheres.size(); ++i) {
		float t0 = INFINITY, t1 = INFINITY;
		if (spheres[i]->intersect(rayorig, raydir, t0, t1)) {
			if (t0 < 0) t0 = t1;
			if (t0 < tnear) {
				tnear = t0;
				sphere = spheres[i];
			}
		}
	}
	// if there's no intersection return black or background color
	if (!sphere) return Vec3f(2);
	Vec3f surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
	Vec3f phit = rayorig + raydir * tnear; // point of intersection
	Vec3f nhit = phit - sphere->center; // normal at the intersection point
	nhit.normalize(); // normalize normal direction
	// If the normal and the view direction are not opposite to each other
	// reverse the normal direction. That also means we are inside the sphere so set
	// the inside bool to true. Finally reverse the sign of IdotN which we want
	// positive.
	float bias = 1e-4; // add some bias to the point from which we will be tracing
	bool inside = false;
	if (raydir.dot(nhit) > 0) nhit = -nhit, inside = true;
	if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
		float facingratio = -raydir.dot(nhit);
		// change the mix value to tweak the effect
		float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);
		// compute reflection direction (not need to normalize because all vectors
		// are already normalized)
		Vec3f refldir = raydir - nhit * 2 * raydir.dot(nhit);
		refldir.normalize();
		Vec3f reflection = pooltrace(phit + nhit * bias, refldir, spheres, depth + 1);
		Vec3f refraction = 0;
		// if the sphere is also transparent compute refraction ray (transmission)
		if (sphere->transparency) {
			float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
			float cosi = -nhit.dot(raydir);
			float k = 1 - eta * eta * (1 - cosi * cosi);
			Vec3f refrdir = raydir * eta + nhit * (eta * cosi - sqrt(k));
			refrdir.normalize();
			refraction = pooltrace(phit - nhit * bias, refrdir, spheres, depth + 1);
		}
		// the result is a mix of reflection and refraction (if the sphere is transparent)
		surfaceColor = (
			reflection * fresneleffect +
			refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
	}
	else {
		// it's a diffuse object, no need to raytrace any further
		for (unsigned i = 0; i < spheres.size(); ++i) {
			//const Sphere& object = *spheres[i];
#if SIMDENABLED
			if (spheres[i]->emissionColor.vector(0) > 0) {
				// this is a light
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i]->center - phit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j]->intersect(phit + nhit * bias, lightDirection, t0, t1)) {
							transmission = 0;
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * transmission *
					std::max(float(0), nhit.dot(lightDirection)) * spheres[i]->emissionColor;
			}
#else
			if (spheres[i]->emissionColor.x > 0) {
				// this is a light
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i]->center - phit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j]->intersect(phit + nhit * bias, lightDirection, t0, t1)) {
							transmission = 0;
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * transmission *
					std::max(float(0), nhit.dot(lightDirection)) * spheres[i]->emissionColor;
			}
#endif
		}
	}
	return surfaceColor + sphere->emissionColor;
}

//[comment]
//Same as other render function just uses sphere tracker
//[/comment]
void poolrender(const std::vector<Sphere*> spheres, int iteration)
{
	// quick and dirty
	unsigned width = 640, height = 480;
	// Recommended Testing Resolution
	//unsigned width = 640, height = 480;

	// Recommended Production Resolution
	//unsigned width = 1920, height = 1080;
	Vec3f* image = new Vec3f[width * height], * pixel = image;
	float invWidth = 1 / float(width), invHeight = 1 / float(height);
	float fov = 30, aspectratio = width / float(height);
	float angle = tan(M_PI * 0.5 * fov / 180.);
	// Trace rays
	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x, ++pixel) {
			float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
			float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
			Vec3f raydir(xx, yy, -1);
			raydir.normalize();
			*pixel = pooltrace(Vec3f(0), raydir, spheres, 0);
		}
	}
	std::cout << "Rendered and saved spheres" << iteration << ".ppm" << std::endl;

#if FILEIOENABLED
	unsigned char* imageBuffer = new unsigned char[width * height * 3];
	unsigned j = 0;
	for (unsigned i = 0; i < width * height; ++i)
	{
#if SIMDENABLED
		imageBuffer[j] = (unsigned char)(std::min(1.0f, image[i].vector(0)) * 255.0f);
		imageBuffer[j + 1] = (unsigned char)(std::min(1.0f, image[i].vector(1)) * 255.0f);
		imageBuffer[j + 2] = (unsigned char)(std::min(1.0f, image[i].vector(2)) * 255.0f);
#else
		imageBuffer[j] = (unsigned char)(std::min(1.0f, image[i].x) * 255.0f);
		imageBuffer[j + 1] = (unsigned char)(std::min(1.0f, image[i].y) * 255.0f);
		imageBuffer[j + 2] = (unsigned char)(std::min(1.0f, image[i].z) * 255.0f);
#endif
		j += 3;
	}

	char fileName[256];
	snprintf(fileName, sizeof(fileName), "./spheres%i.ppm", iteration);


	FILE* imageFile = fopen(fileName, "wb");
	if (!imageFile)
		return;

	fprintf(imageFile, "P6\n%d %d\n255\n", width, height);
	fwrite(imageBuffer, 1, width * height * 3, imageFile);

	delete[] image;
	delete[] imageBuffer;
	fclose(imageFile);
#else
	// Save result to a PPM image (keep these flags if you compile under Windows)
	std::stringstream ss;
	ss << "./spheres" << iteration << ".ppm";
	std::string tempString = ss.str();
	char* filename = (char*)tempString.c_str();

	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	ofs << "P6\n" << width << " " << height << "\n255\n";
#if SIMDENABLED
	for (unsigned i = 0; i < width * height; ++i) {
		ofs << (unsigned char)(std::min(float(1), image[i].vector(0)) * 255) <<
			(unsigned char)(std::min(float(1), image[i].vector(1)) * 255) <<
			(unsigned char)(std::min(float(1), image[i].vector(2)) * 255);
	}
#else
	for (unsigned i = 0; i < width * height; ++i) {
		ofs << (unsigned char)(std::min(float(1), image[i].x) * 255) <<
			(unsigned char)(std::min(float(1), image[i].y) * 255) <<
			(unsigned char)(std::min(float(1), image[i].z) * 255);
	}
#endif
	ofs.close();
	delete[] image;
#endif
}

//[comment]
// This is the main trace function. It takes a ray as argument (defined by its origin
// and direction). We test if this ray intersects any of the geometry in the scene.
// If the ray intersects an object, we compute the intersection point, the normal
// at the intersection point, and shade this point using this information.
// Shading depends on the surface property (is it transparent, reflective, diffuse).
// The function returns a color for the ray. If the ray intersects an object that
// is the color of the object at the intersection point, otherwise it returns
// the background color.
//[/comment]
Vec3f trace(
	const Vec3f &rayorig,
	const Vec3f &raydir,
	const std::vector<Sphere> &spheres,
	const int &depth)
{
	//if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
	float tnear = INFINITY;
	const Sphere* sphere = NULL;
	// find intersection of this ray with the sphere in the scene
	for (unsigned i = 0; i < spheres.size(); ++i) {
		float t0 = INFINITY, t1 = INFINITY;
		if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
			if (t0 < 0) t0 = t1;
			if (t0 < tnear) {
				tnear = t0;
				sphere = &spheres[i];
			}
		}
	}
	// if there's no intersection return black or background color
	if (!sphere) return Vec3f(2);
	Vec3f surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray
	Vec3f phit = rayorig + raydir * tnear; // point of intersection
	Vec3f nhit = phit - sphere->center; // normal at the intersection point
	nhit.normalize(); // normalize normal direction
					  // If the normal and the view direction are not opposite to each other
					  // reverse the normal direction. That also means we are inside the sphere so set
					  // the inside bool to true. Finally reverse the sign of IdotN which we want
					  // positive.
	float bias = 1e-4; // add some bias to the point from which we will be tracing
	bool inside = false;
	if (raydir.dot(nhit) > 0) nhit = -nhit, inside = true;
	if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
		float facingratio = -raydir.dot(nhit);
		// change the mix value to tweak the effect
		float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);
		// compute reflection direction (not need to normalize because all vectors
		// are already normalized)
		Vec3f refldir = raydir - nhit * 2 * raydir.dot(nhit);
		refldir.normalize();
		Vec3f reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
		Vec3f refraction = 0;
		// if the sphere is also transparent compute refraction ray (transmission)
		if (sphere->transparency) {
			float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface?
			float cosi = -nhit.dot(raydir);
			float k = 1 - eta * eta * (1 - cosi * cosi);
			Vec3f refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k));
			refrdir.normalize();
			refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
		}
		// the result is a mix of reflection and refraction (if the sphere is transparent)
		surfaceColor = (
			reflection * fresneleffect +
			refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
	}
	else {
		// it's a diffuse object, no need to raytrace any further
		for (unsigned i = 0; i < spheres.size(); ++i) {
#if SIMDENABLED
			if (spheres[i].emissionColor.vector(0) > 0) {
				// this is a light
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i].center - phit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
							transmission = 0;
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * transmission *
					std::max(float(0), nhit.dot(lightDirection)) * spheres[i].emissionColor;
			}
#else
			if (spheres[i].emissionColor.x > 0) {
				// this is a light
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i].center - phit;
				lightDirection.normalize();
				for (unsigned j = 0; j < spheres.size(); ++j) {
					if (i != j) {
						float t0, t1;
						if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) {
							transmission = 0;
							break;
						}
					}
				}
				surfaceColor += sphere->surfaceColor * transmission *
					std::max(float(0), nhit.dot(lightDirection)) * spheres[i].emissionColor;
			}
#endif
		}
	}

	return surfaceColor + sphere->emissionColor;
}

//[comment]
// Main rendering function. We compute a camera ray for each pixel of the image
// trace it and return a color. If the ray hits a sphere, we return the color of the
// sphere at the intersection point, else we return the background color.
//[/comment]
void render(const std::vector<Sphere> &spheres, int iteration)
{
	// quick and dirty
	unsigned width = 640, height = 480;
	// Recommended Testing Resolution
	//unsigned width = 640, height = 480;

	// Recommended Production Resolution
	//unsigned width = 1920, height = 1080;
	Vec3f *image = new Vec3f[width * height], *pixel = image;
	float invWidth = 1 / float(width), invHeight = 1 / float(height);
	float fov = 30, aspectratio = width / float(height);
	float angle = tan(M_PI * 0.5 * fov / 180.);
	// Trace rays
	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x, ++pixel) {
			float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
			float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
			Vec3f raydir(xx, yy, -1);
			raydir.normalize();
			*pixel = trace(Vec3f(0), raydir, spheres, 0);
		}
	}

	sphereCopyMutex.lock();
	std::cout << "Rendered and saved spheres" << iteration << ".ppm" << std::endl;
	sphereCopyMutex.unlock();

#if FILEIOENABLED
	unsigned char* imageBuffer = new unsigned char[width * height * 3];
	unsigned j = 0;
	for (unsigned i = 0; i < width * height; ++i)
	{
#if SIMDENABLED
		imageBuffer[j] = (unsigned char)(std::min(1.0f, image[i].vector(0)) * 255.0f);
		imageBuffer[j + 1] = (unsigned char)(std::min(1.0f, image[i].vector(1)) * 255.0f);
		imageBuffer[j + 2] = (unsigned char)(std::min(1.0f, image[i].vector(2)) * 255.0f);
#else
		imageBuffer[j] = (unsigned char)(std::min(1.0f, image[i].x) * 255.0f);
		imageBuffer[j + 1] = (unsigned char)(std::min(1.0f, image[i].y) * 255.0f);
		imageBuffer[j + 2] = (unsigned char)(std::min(1.0f, image[i].z) * 255.0f);
#endif
		j += 3;
	}

	char fileName[256];
	snprintf(fileName, sizeof(fileName), "./spheres%i.ppm", iteration);


	FILE* imageFile = fopen(fileName, "wb");
	if (!imageFile)
		return;

	fprintf(imageFile, "P6\n%d %d\n255\n", width, height);
	fwrite(imageBuffer, 1, width * height * 3, imageFile);

	delete[] image;
	delete[] imageBuffer;
	fclose(imageFile);
#else
	// Save result to a PPM image (keep these flags if you compile under Windows)
	std::stringstream ss;
	ss << "./spheres" << iteration << ".ppm";
	std::string tempString = ss.str();
	char* filename = (char*)tempString.c_str();

	std::ofstream ofs(filename, std::ios::out | std::ios::binary);
	ofs << "P6\n" << width << " " << height << "\n255\n";
#if SIMDENABLED
	for (unsigned i = 0; i < width * height; ++i) {
		ofs << (unsigned char)(std::min(float(1), image[i].vector(0)) * 255) <<
			(unsigned char)(std::min(float(1), image[i].vector(1)) * 255) <<
			(unsigned char)(std::min(float(1), image[i].vector(2)) * 255);
	}
#else
	for (unsigned i = 0; i < width * height; ++i) {
		ofs << (unsigned char)(std::min(float(1), image[i].x) * 255) <<
			(unsigned char)(std::min(float(1), image[i].y) * 255) <<
			(unsigned char)(std::min(float(1), image[i].z) * 255);
	}
#endif
	ofs.close();
	delete[] image;
#endif
}

#if defined __linux__ || defined __APPLE__
std::vector<pid_t> globalPids;
void MakePids()
{
	std::vector<pid_t> pids;
	for (int i = 0; i < MAXPROCESSES - 1; i++)
	{
		//printf("Pushing pid\n");
		pid_t pidToPush;
		if (pidToPush == 0)
		{
			//printf("pid = %d\n", getpid());
			exit(0);
		}
		else
		{
			//printf("pid = %d\n", getpid());
			pids.push_back(pidToPush);
		}
	}
	globalPids = pids;
}


void PidRender(int num)
{
	int iterationFrames;
	iterationFrames = 100 / MAXPROCESSES;
	globalPids[num] = fork();
	if (globalPids[num] == 0)
	{
		printf("child[%d] --> pid = %d and ppid = %d\n", num,
			getpid(), getppid());
		std::vector<Sphere> spheres;
		for (float r = num * iterationFrames; r < (num + 1) * iterationFrames; r++)
		{
			spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
			spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

			render(spheres, r);
			std::cout << "Rendered and saved spheres in child" << r << ".ppm" << std::endl;

			// Dont forget to clear the Vector holding the spheres.
			spheres.clear();
		}
		exit(0);
	}
	else
	{
		printf("Run Parent\n");
	}
}
#endif

void BasicRender()
{
	if (!threadingEnabled)
	{
		if (sphereMemoryPoolEnabled)
		{
			std::vector<Sphere*> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
			spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

			// This creates a file, titled 1.ppm in the current working directory
			poolrender(spheres, 1);
			for (auto s : spheres)
			{
				delete(s);
			}
			spheres.clear();
		}
		else
		{
			std::vector<Sphere> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
			spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

			// This creates a file, titled 1.ppm in the current working directory
			render(spheres, 1);
			spheres.clear();
		}
	}
	else
	{
		std::vector<Sphere> spheres;
		// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

		spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
		spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
		spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
		spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

		int r = 1;
		// This creates a file, titled 1.ppm in the current working directory
		threadManager->QueueThreadTask([spheres, r] {render(spheres, r); });
		spheres.clear();
		//threadManager->StopThreads();
	}
}

void SimpleShrinking()
{
	if (!threadingEnabled)
	{
		if (sphereMemoryPoolEnabled)
		{
			std::vector<Sphere*> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (int i = 0; i < 4; i++)
			{
				if (i == 0)
				{
					spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
					spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 1)
				{
					spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), 3, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 2)
				{
					spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), 2, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 3)
				{
					spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), 1, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				poolrender(spheres, i);
				// Dont forget to clear the Vector holding the spheres.
				for (auto s : spheres)
				{
					delete(s);
				}
				spheres.clear();
			}
		}
		else
		{
			std::vector<Sphere> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (int i = 0; i < 4; i++)
			{
				if (i == 0)
				{
					spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
					spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 1)
				{
					spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 3, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 2)
				{
					spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 2, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				else if (i == 3)
				{
					spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
					spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 1, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
					spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
					spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				}
				render(spheres, i);
				// Dont forget to clear the Vector holding the spheres.
				spheres.clear();
			}
		}
	}
	else
	{
		std::vector<Sphere> spheres;
		// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

		for (int i = 0; i < 4; i++)
		{
			if (i == 0)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 4, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // The radius paramter is the value we will change
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

			}
			else if (i == 1)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 3, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
			}
			else if (i == 2)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 2, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
			}
			else if (i == 3)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), 1, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius--
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
			}

			// This creates a file, titled 1.ppm in the current working directory
			threadManager->QueueThreadTask([spheres, i] {render(spheres, i); });
			// Dont forget to clear the Vector holding the spheres.
			spheres.clear();
		}
		//threadManager->StopThreads();
	}
}

void SmoothScaling()
{
#if defined __linux__ || defined __APPLE__
	if (parallelProcessingEnabled)
	{
		//Run Children
		std::vector<Sphere> spheres;
		for (int i = 0; i < MAXPROCESSES - 1; i++)
		{
			PidRender(i);
		}

		//Parent
		int iF;
		iF = 100 / MAXPROCESSES;
		//Main Parent Thread
		for (float r = iF * (MAXPROCESSES - 1); r <= 100; r++)
		{
			spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
			spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));

			render(spheres, r);
			std::cout << "Rendered and saved spheres in parent" << r << ".ppm" << std::endl;

			// Dont forget to clear the Vector holding the spheres.
			spheres.clear();
		}
	}
	else if(threadingEnabled)
	{
		std::vector<Sphere> spheres; //Placeholder
		// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)
		for (float r = 0; r <= 100; r++)
		{
			spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
			spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
			threadManager->QueueThreadTask([spheres, r] {render(spheres, r); });
			//std::cout << "Rendered and saved spheres" << r << ".ppm" << std::endl;
			// Dont forget to clear the Vector holding the spheres.
			spheres.clear();
		}
	}
	else
	{
		if (sphereMemoryPoolEnabled)
		{
			std::vector<Sphere*> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (float r = 0; r <= 100; r++)
			{
				spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
				spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				poolrender(spheres, r);
				// Dont forget to clear the Vector holding the spheres.
				for (auto s : spheres)
				{
					delete(s);
				}
				// Dont forget to clear the Vector holding the spheres.
				spheres.clear();
			}
		}
		else
		{
			std::vector<Sphere> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (float r = 0; r <= 100; r++)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				render(spheres, r);
				// Dont forget to clear the Vector holding the spheres.
				spheres.clear();
			}
		}
	}
#else
	if (threadingEnabled)
	{
		std::vector<Sphere> spheres; //Placeholder
		// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)
		for (float r = 0; r <= 100; r++)
		{
			spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
			spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
			spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
			spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
			threadManager->QueueThreadTask([spheres, r] {render(spheres, r); });
			//std::cout << "Rendered and saved spheres" << r << ".ppm" << std::endl;
			// Dont forget to clear the Vector holding the spheres.
			spheres.clear();
		}
		//threadManager->StopThreads();
	}
	else
	{
		if (sphereMemoryPoolEnabled)
		{
			std::vector<Sphere*> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (float r = 0; r <= 100; r++)
			{
				spheres.push_back(new Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(new Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
				spheres.push_back(new Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(new Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				poolrender(spheres, r);
				// Dont forget to clear the Vector holding the spheres.
				for (auto s : spheres)
				{
					delete(s);
				}
				// Dont forget to clear the Vector holding the spheres.
				spheres.clear();
			}
		}
		else
		{
			std::vector<Sphere> spheres;
			// Vector structure for Sphere (position, radius, surface color, reflectivity, transparency, emission color)

			for (float r = 0; r <= 100; r++)
			{
				spheres.push_back(Sphere(Vec3f(0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
				spheres.push_back(Sphere(Vec3f(0.0, 0, -20), r / 100, Vec3f(1.00, 0.32, 0.36), 1, 0.5)); // Radius++ change here
				spheres.push_back(Sphere(Vec3f(5.0, -1, -15), 2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
				spheres.push_back(Sphere(Vec3f(5.0, 0, -25), 3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
				render(spheres, r);
				// Dont forget to clear the Vector holding the spheres.
				spheres.clear();
			}
		}
	}
#endif
}

void RenderJson(std::string filePath)
{
	std::vector<Sphere> spheres;
	jsonRenderer = new Json();
	//std::string fileName = "json";
	jsonRenderer->ReadJson(spheres, filePath);


	std::vector<float> radiusChange;
	std::vector<float> reflectionChange;
	std::vector<float> transparencyChange;

	std::vector<Vec3f> centerChange;
	std::vector<Vec3f> emissionColourChange;
	std::vector<Vec3f> surfaceColourChange;
	//Calculate Differences Per Frame
	for (int i = 0; i < spheres.size(); i++)
	{
		radiusChange.push_back((spheres.at(i).radiusEnd - spheres.at(i).radius) / 100);
		reflectionChange.push_back((spheres.at(i).reflectionEnd - spheres.at(i).reflection) / 100);
		transparencyChange.push_back((spheres.at(i).transparencyEnd - spheres.at(i).transparency) / 100);

#if SIMDENABLED
		centerChange.push_back(spheres.at(i).centerEnd - spheres.at(i).center);
		centerChange.at(i).vector(0) /= 100;
		centerChange.at(i).vector(1) /= 100;
		centerChange.at(i).vector(2) /= 100;

		surfaceColourChange.push_back(spheres.at(i).surfaceColourEnd - spheres.at(i).surfaceColor);
		surfaceColourChange.at(i).vector(0) /= 100;
		surfaceColourChange.at(i).vector(1) /= 100;
		surfaceColourChange.at(i).vector(2) /= 100;

		emissionColourChange.push_back(spheres.at(i).emissionColourEnd - spheres.at(i).emissionColor);
		emissionColourChange.at(i).vector(0) /= 100;
		emissionColourChange.at(i).vector(1) /= 100;
		emissionColourChange.at(i).vector(2) /= 100;
#else
		centerChange.push_back(spheres.at(i).centerEnd - spheres.at(i).center);
		centerChange.at(i).x /= 100;
		centerChange.at(i).y /= 100;
		centerChange.at(i).z /= 100;

		surfaceColourChange.push_back(spheres.at(i).surfaceColourEnd - spheres.at(i).surfaceColor);
		surfaceColourChange.at(i).x /= 100;
		surfaceColourChange.at(i).y /= 100;
		surfaceColourChange.at(i).z /= 100;

		emissionColourChange.push_back(spheres.at(i).emissionColourEnd - spheres.at(i).emissionColor);
		emissionColourChange.at(i).x /= 100;
		emissionColourChange.at(i).y /= 100;
		emissionColourChange.at(i).z /= 100;
#endif
	}

	int size = spheres.size();
	if (threadingEnabled)
	{
		for (int r = 0; r <= 100; r++)
		{

			for (int i = 0; i < spheres.size(); i++)
			{
				spheres.at(i).radius += radiusChange.at(i);
				float radius = spheres.at(i).radius;
				spheres.at(i).radius2 = radius * radius;
				spheres.at(i).reflection += reflectionChange.at(i);
				spheres.at(i).transparency += transparencyChange.at(i);

				spheres.at(i).center += centerChange.at(i);
				spheres.at(i).surfaceColor += surfaceColourChange.at(i);
				spheres.at(i).emissionColor += emissionColourChange.at(i);
			}
			threadManager->QueueThreadTask([spheres, r] {render(spheres, r); });
		}
		//threadManager->StopThreads();
	}
	else
	{
		for (float r = 0; r <= 100; r++)
		{
			for (int i = 0; i < spheres.size(); i++)
			{
				spheres.at(i).radius += radiusChange.at(i);
				float radius = spheres.at(i).radius;
				spheres.at(i).radius2 = radius * radius;
				spheres.at(i).reflection += reflectionChange.at(i);
				spheres.at(i).transparency += transparencyChange.at(i);

				spheres.at(i).center += centerChange.at(i);
				spheres.at(i).surfaceColor += surfaceColourChange.at(i);
				spheres.at(i).emissionColor += emissionColourChange.at(i);
			}
			render(spheres, r);
		}
	}
	spheres.clear();
	radiusChange.clear();
	reflectionChange.clear();
	transparencyChange.clear();
	centerChange.clear();
	emissionColourChange.clear();
	surfaceColourChange.clear();

}

//[comment]
// In the main function, we will create the scene which is composed of 5 spheres
// and 1 light (which is also a sphere). Then, once the scene description is complete
// we render that scene, by calling the render() function.
//[/comment]
int main(int argc, char **argv)
{
	// This sample only allows one choice per program execution. Feel free to improve upon this
	srand(13);
	//BasicRender();
	//SimpleShrinking();

#if defined __linux__ || defined __APPLE__
	int choice;
	while (choice != 1 && choice != 2 && choice != 3 && choice != 4)
	{
		std::cout << "Welcome to the Linux Raytracer..." << std::endl << "Please choose an option to continue: " << std::endl
			<< "(1) Basic Render, (2) Simple Shrinking, (3) Smooth Scaling, (4) JSON Render " << std::endl;
		std::cin >> choice;
		if (std::cin.fail())
		{
			std::cout << "Enter a number" << std::endl;
			std::cin.clear();
			std::cin.ignore(256, '\n');
		}
	}

	int parallelChoice = 0;
	if (choice == 3)
	{
		while (parallelChoice != 1 && parallelChoice != 2)
		{
			std::cout << "Enable Forking? (1) Yes, (2) No..." << std::endl;
			std::cin >> parallelChoice;
			if (std::cin.fail())
			{
				std::cout << "Enter a number" << std::endl;
				std::cin.clear();
				std::cin.ignore(256, '\n');
			}
		}
	}
	if (parallelChoice == 1)
	{
		parallelProcessingEnabled = true;
	}
	else
	{
		parallelProcessingEnabled = false;
	}

	//Threading Enabled Disabled
	int threadChoice = 0;
	if (!parallelProcessingEnabled)
	{
		while (threadChoice != 1 && threadChoice != 2)
		{
			std::cout << "Enable Threading? (1) Yes, (2) No..." << std::endl;
			std::cin >> threadChoice;
			if (std::cin.fail())
			{
				std::cout << "Enter a number" << std::endl;
				std::cin.clear();
				std::cin.ignore(256, '\n');
			}
		}
	}
	if (threadChoice == 1)
	{
		threadingEnabled = true;
	}
	else
	{
		threadingEnabled = false;
	}

	if (choice == 4)
	{
		renderJson = true;
	}

	int defaultPoolChoice = 0;
	int spherePoolChoice = 0;
	if (!renderJson)
	{
		//Pools Enabled
		if (!threadingEnabled)
		{
			while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
			{
				std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
				std::cin >> defaultPoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
			while (spherePoolChoice != 1 && spherePoolChoice != 2)
			{
				std::cout << "Enable Sphere Pools? (This will change the render functions to use pointers) (1) Yes, (2) No..." << std::endl;
				std::cin >> spherePoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
		}
		else
		{
			while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
			{
				std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
				std::cin >> defaultPoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
		}
	}
	else if (renderJson && !threadingEnabled)
	{
		while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
		{
			std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
			std::cin >> defaultPoolChoice;
			if (std::cin.fail())
			{
				std::cout << "Enter a number" << std::endl;
				std::cin.clear();
				std::cin.ignore(256, '\n');
			}
		}
	}

	if (defaultPoolChoice == 1)
	{
		defaultMemoryPoolEnabled = true;
	}
	else
	{
		defaultMemoryPoolEnabled = false;
	}

	if (spherePoolChoice == 1)
	{
		sphereMemoryPoolEnabled = true;
	}
	else
	{
		sphereMemoryPoolEnabled = false;
	}

	if (defaultMemoryPoolEnabled)
	{
		defaultPool = new MemoryPool(100, 1048576); //Allocate 100MB of space
		Trackers::GetDefaultTracker()->SetUsingMemoryPool(true);
		Trackers::GetDefaultTracker()->SetMemoryPool(defaultPool);
	}

	if (sphereMemoryPoolEnabled)
	{
		//Ensure thread safety with multiple pools
		threadingEnabled = false;
		spherePool = new MemoryPool(100, sizeof(Sphere));
		Trackers::GetSphereTracker()->SetUsingMemoryPool(true);
		Trackers::GetSphereTracker()->SetMemoryPool(spherePool);
	}

	if (threadingEnabled)
	{
		threadManager = new ThreadManager(); //Create thread manager
	}

	if (parallelChoice)
	{
		MakePids();
	}
	auto start = std::chrono::steady_clock::now();
	if (!renderJson)
	{
		if (choice == 1)
		{
			BasicRender();
		}
		else if (choice == 2)
		{
			SimpleShrinking();
		}
		else
		{
			SmoothScaling();
		}
	}
	else
	{
		std::string fileName;
		std::ifstream jsonFile;
		std::cout << "Input Json File Name: ";
		std::cin >> fileName;
		std::string filePath = "JsonFiles/" + fileName + ".json";
		jsonFile.open(filePath);
		while (!jsonFile.is_open())
		{
			std::cout << "File Not Found Input Json File Name: ";
			std::cin >> fileName;
			filePath = "JsonFiles/" + fileName + ".json";
			jsonFile.open(filePath);
		}
		jsonFile.close();
		start = std::chrono::steady_clock::now();
		RenderJson(filePath);
	}
	if (threadingEnabled)
	{
		threadManager->StopThreads();
	}
	auto end = std::chrono::steady_clock::now();

	if (threadingEnabled)
	{
		std::cout << "Deleting thread manager " << std::endl;
		delete threadManager;
	}

	if (renderJson)
	{
		delete jsonRenderer;
	}

	if (sphereMemoryPoolEnabled)
	{
		//std::cout << "Deleting sphere manager " << std::endl;
		delete spherePool;
	}

	/*if (defaultMemoryPoolEnabled)
	{
		delete defaultPool;
	}*/

	std::chrono::duration<float> time_passed = end - start;
	std::cout << "Time Taken = " << time_passed.count() << std::endl;
	std::cout << std::endl << "Default Tracker: " << std::endl;
	Trackers::GetDefaultTracker()->Walk();
	std::cout << std::endl << "Sphere Tracker: " << std::endl;
	Trackers::GetSphereTracker()->Walk();
	std::cout << std::endl << "Memory Pool Tracker: " << std::endl;
	Trackers::GetMemoryPoolTracker()->Walk();
	std::cout << std::endl << "Threading Tracker: " << std::endl;
	Trackers::GetThreadingTracker()->Walk();
	return 0;
#else
	int choice = 0;
	while (choice != 1 && choice != 2 && choice != 3 && choice != 4)
	{
		std::cout << "Welcome to the Windows Raytracer..." << std::endl << "Please choose an option to continue: " << std::endl
			<< "(1) Basic Render, (2) Simple Shrinking, (3) Smooth Scaling, (4) JSON Render " << std::endl;
		std::cin >> choice;
		if (std::cin.fail())
		{
			std::cout << "Enter a number" << std::endl;
			std::cin.clear();
			std::cin.ignore(256, '\n');
		}
	}

	//Threading Enabled Disabled
	int threadChoice = 0;
	while (threadChoice != 1 && threadChoice != 2)
	{
		std::cout << "Enable Threading? (1) Yes, (2) No..." << std::endl;
		std::cin >> threadChoice;
		if (std::cin.fail())
		{
			std::cout << "Enter a number" << std::endl;
			std::cin.clear();
			std::cin.ignore(256, '\n');
		}
	}
	if (threadChoice == 1)
	{
		threadingEnabled = true;
	}
	else
	{
		threadingEnabled = false;
	}

	if (choice == 4)
	{
		renderJson = true;
	}

	int defaultPoolChoice = 0;
	int spherePoolChoice = 0;
	if (!renderJson)
	{
		//Pools Enabled
		if (!threadingEnabled)
		{
			while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
			{
				std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
				std::cin >> defaultPoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
			while (spherePoolChoice != 1 && spherePoolChoice != 2)
			{
				std::cout << "Enable Sphere Pools? (This will change the render functions to use pointers) (1) Yes, (2) No..." << std::endl;
				std::cin >> spherePoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
		}
		else
		{
			while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
			{
				std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
				std::cin >> defaultPoolChoice;
				if (std::cin.fail())
				{
					std::cout << "Enter a number" << std::endl;
					std::cin.clear();
					std::cin.ignore(256, '\n');
				}
			}
		}
	}
	else if(renderJson && !threadingEnabled)
	{
		while (defaultPoolChoice != 1 && defaultPoolChoice != 2)
		{
			std::cout << "Enable Default Pools? (1) Yes, (2) No..." << std::endl;
			std::cin >> defaultPoolChoice;
			if (std::cin.fail())
			{
				std::cout << "Enter a number" << std::endl;
				std::cin.clear();
				std::cin.ignore(256, '\n');
			}
		}
	}

	if (defaultPoolChoice == 1)
	{
		defaultMemoryPoolEnabled = true;
	}
	else
	{
		defaultMemoryPoolEnabled = false;
	}

	if (spherePoolChoice == 1)
	{
		sphereMemoryPoolEnabled = true;
	}
	else
	{
		sphereMemoryPoolEnabled = false;
	}

	if (defaultMemoryPoolEnabled)
	{
		defaultPool = new MemoryPool(100, 1048576); //Allocate 100MB of space
		Trackers::GetDefaultTracker()->SetUsingMemoryPool(true);
		Trackers::GetDefaultTracker()->SetMemoryPool(defaultPool);
	}

	if (sphereMemoryPoolEnabled)
	{
		//Ensure thread safety with multiple pools
		threadingEnabled = false;
		spherePool = new MemoryPool(100, sizeof(Sphere));
		Trackers::GetSphereTracker()->SetUsingMemoryPool(true);
		Trackers::GetSphereTracker()->SetMemoryPool(spherePool);
	}

	if (threadingEnabled)
	{
		threadManager = new ThreadManager(); //Create thread manager
	}

	auto start = std::chrono::steady_clock::now();
	if (!renderJson)
	{
		if(choice == 1)
		{ 
			BasicRender();
		}
		else if (choice == 2)
		{
			SimpleShrinking();
		}
		else
		{
			SmoothScaling();
		}
	}
	else
	{
		std::string fileName;
		std::ifstream jsonFile;
		std::cout << "Input Json File Name: ";
		std::cin >> fileName;
		std::string filePath = "JsonFiles/" + fileName + ".json";
		jsonFile.open(filePath);
		while(!jsonFile.is_open())
		{
			std::cout << "File Not Found Input Json File Name: ";
			std::cin >> fileName;
			filePath = "JsonFiles/" + fileName + ".json";
			jsonFile.open(filePath);
		}
		jsonFile.close();
		start = std::chrono::steady_clock::now();
		RenderJson(filePath);
	}
	if (threadingEnabled)
	{
		threadManager->StopThreads();
	}
	auto end = std::chrono::steady_clock::now();

	if (threadingEnabled)
	{
#ifdef _DEBUG
		std::cout << "Deleting thread manager " << std::endl;
#endif
		delete threadManager;
	}

	if (renderJson)
	{
		delete jsonRenderer;
	}

	if (sphereMemoryPoolEnabled)
	{
		//std::cout << "Deleting sphere manager " << std::endl;
		delete spherePool;
	}

	/*if (defaultMemoryPoolEnabled)
	{
		//delete defaultPool;
	}*/

	std::chrono::duration<float> time_passed = end - start;
	std::cout << "Time Taken = " << time_passed.count() << std::endl;
	std::cout << std::endl <<"Default Tracker: " << std::endl;
	Trackers::GetDefaultTracker()->Walk();
	std::cout << std::endl << "Sphere Tracker: " << std::endl;
	Trackers::GetSphereTracker()->Walk();
	std::cout << std::endl << "Memory Pool Tracker: " << std::endl;
	Trackers::GetMemoryPoolTracker()->Walk();
	std::cout << std::endl << "Threading Tracker: " << std::endl;
	Trackers::GetThreadingTracker()->Walk();
	return 0;
#endif
}

