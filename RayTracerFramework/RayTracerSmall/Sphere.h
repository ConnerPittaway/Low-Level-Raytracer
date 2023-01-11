#pragma once
#include <iostream>
#include "Eigen/Dense"
#include <cmath>
#include "Trackers.h"
#include "GlobalNewDelete.h"
#define SIMDENABLED true
#if SIMDENABLED
template<typename T>
class Vec3
{
public:
	//T x, y, z;
	Eigen::Vector3f vector;
	Vec3() : vector(0,0,0) {}
	Vec3(T xx) : vector(xx, xx, xx) {}
	Vec3(T xx, T yy, T zz) : vector(xx, yy, zz) {}
	Vec3& normalize()
	{
		T nor2 = vector.norm();
		if (nor2 > 0) {
			vector.normalize();
		}
		return *this;
	}
	Vec3<T> operator * (const T& f) const { 
		Eigen::Vector3f result;
		result = vector * f;
		return Vec3<T>(result(0), result(1), result(2));
	}
	Vec3<T> operator * (const Vec3<T>& v) const { 
		Eigen::Vector3f result;
		result = vector.cwiseProduct(v.vector);
		return Vec3<T>(result(0), result(1), result(2));
	}
	T dot(const Vec3<T>& v) const { return vector.dot(v.vector); }
	Vec3<T> operator - (const Vec3<T>& v) const 
	{
		Eigen::Vector3f result;
		result = vector - v.vector;
		return Vec3<T>(result(0), result(1), result(2));
	}
	Vec3<T> operator + (const Vec3<T>& v) const {
		Eigen::Vector3f result;
		result = vector + v.vector;
		return Vec3<T>(result(0), result(1), result(2));
	}
	Vec3<T>& operator += (const Vec3<T>& v) { vector(0) += v.vector(0), vector(1) += v.vector(1), vector(2) += v.vector(2); return *this; }
	Vec3<T>& operator *= (const Vec3<T>& v) { vector(0) *= v.vector(0), vector(1) *= v.vector(1), vector(2) *= v.vector(2); return *this; }
	Vec3<T> operator - () const { return Vec3<T>(-vector(0), -vector(1), -vector(2)); }
	//T length2() const { return vector(0) * vector(0) + vector(1) * vector(1) + vector(2) * vector(2); }
	//T length() const { return sqrt(length2()); }
	friend std::ostream& operator << (std::ostream& os, const Vec3<T>& v)
	{
		os << "[" << v.vector(0) << " " << v.vector(1) << " " << v.vector(2) << "]";
		return os;
	}
};
typedef Vec3<float> Vec3f;
#else
template<typename T>
class Vec3
{
public:
	T x, y, z;
	Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
	Vec3(T xx) : x(xx), y(xx), z(xx) {}
	Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}
	Vec3& normalize()
	{
		T nor2 = length2();
		if (nor2 > 0) {
			T invNor = 1 / sqrt(nor2);
			x *= invNor, y *= invNor, z *= invNor;
		}
		return *this;
	}
	Vec3<T> operator * (const T& f) const { return Vec3<T>(x * f, y * f, z * f); }
	Vec3<T> operator * (const Vec3<T>& v) const { return Vec3<T>(x * v.x, y * v.y, z * v.z); }
	T dot(const Vec3<T>& v) const { return x * v.x + y * v.y + z * v.z; }
	Vec3<T> operator - (const Vec3<T>& v) const { return Vec3<T>(x - v.x, y - v.y, z - v.z); }
	Vec3<T> operator + (const Vec3<T>& v) const { return Vec3<T>(x + v.x, y + v.y, z + v.z); }
	Vec3<T>& operator += (const Vec3<T>& v) { x += v.x, y += v.y, z += v.z; return *this; }
	Vec3<T>& operator *= (const Vec3<T>& v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
	Vec3<T> operator - () const { return Vec3<T>(-x, -y, -z); }
	T length2() const { return x * x + y * y + z * z; }
	T length() const { return sqrt(length2()); }
	friend std::ostream& operator << (std::ostream& os, const Vec3<T>& v)
	{
		os << "[" << v.x << " " << v.y << " " << v.z << "]";
		return os;
	}
};

typedef Vec3<float> Vec3f;
#endif

class Sphere
{
public:
	Vec3f center, centerEnd;                           /// position of the sphere
	float radius, radius2, radiusEnd;                  /// sphere radius and radius^2
	Vec3f surfaceColor, surfaceColourEnd, emissionColor, emissionColourEnd;      /// surface color and emission (light)
	float transparency, reflection, transparencyEnd, reflectionEnd;         /// surface transparency and reflectivity
	Sphere(
		const Vec3f& c,
		const float& r,
		const Vec3f& sc,
		const float& refl = 0,
		const float& transp = 0,
		const Vec3f& ec = 0) :
		center(c), radius(r), radius2(r* r), surfaceColor(sc), emissionColor(ec),
		transparency(transp), reflection(refl)
	{ /* empty */
	}
	//[comment]
	// Compute a ray-sphere intersection using the geometric solution
	//[/comment]
	bool intersect(const Vec3f& rayorig, const Vec3f& raydir, float& t0, float& t1) const
	{
		Vec3f l = center - rayorig;
		float tca = l.dot(raydir);
		if (tca < 0) return false;
		float d2 = l.dot(l) - tca * tca;
		if (d2 > radius2) return false;
		float thc = sqrt(radius2 - d2);
		t0 = tca - thc;
		t1 = tca + thc;

		return true;
	}

	void* operator new(size_t size);
	void operator delete(void* pMemory);
};