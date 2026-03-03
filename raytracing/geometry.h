#pragma once
#include "object.h"
#include "math.h"
#include "vec3.h"
#include "material.h"
#include "hittable.h"

class plane : public object {
public:
	plane() {}

	plane(const point3 p, const vec3& n) {
		point = p;
		normal = n;
	}

	virtual bool hit(const ray& r, float tmin, float tmax, hit_record& rec) const;

private:
	point3 point;
	vec3 normal;
};

bool plane::hit(const ray& r, float t_min, float t_max, hit_record& rec) const {
	float denom = dot(unit_vector(r.d), normal);
	if (denom > 0) {
		vec3 v = unit_vector(point - r.o);
		float temp = dot(v, normal) / denom;
		if (temp < t_max && temp > t_min) {
			rec.t = temp;
			rec.p = r.at(rec.t);
			rec.normal = normal;
			return true;
		}
	}
	return false;
}
