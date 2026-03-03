#pragma once
#include "hittable.h"
#include "math.h"
#include "vec3.h"
#include "material.h"

class sphere : public hittable {
public:
    sphere() : mat_ptr(nullptr) {}

    sphere(const point3& center, float r)
        : center_point(center), radius(r), mat_ptr(nullptr) {
    }

    sphere(const point3& center, float r, material* mat)
        : center_point(center), radius(r), mat_ptr(mat) {
    }

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const override;
    virtual bool hit_shadow(const ray& r, interval ray_t) const override;

    virtual ~sphere() {}

private:
    point3 center_point;
    float radius;
    material* mat_ptr;
};

bool sphere::hit(const ray& r, interval ray_t, hit_record& rec) const {
    vec3 oc = r.origin() - center_point;

    float a = r.direction().length_squared();
    float half_b = dot(oc, r.direction());
    float c = oc.length_squared() - radius * radius;

    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0) return false;

    float sqrt_disc = sqrt(discriminant);

    float root = (-half_b - sqrt_disc) / a;
    if (!ray_t.surrounds(root)) {
        root = (-half_b + sqrt_disc) / a;
        if (!ray_t.surrounds(root))
            return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    vec3 outward_normal = (rec.p - center_point) / radius;
    rec.set_face_normal(r, outward_normal);
    rec.m = mat_ptr;

    vec3 local_point = (rec.p - center_point) / radius;
    float theta = acos(-local_point.y());
    float phi = atan2(-local_point.z(), local_point.x()) + pi;
    rec.u = phi / (2 * pi);
    rec.v = theta / pi;

    return true;
}

bool sphere::hit_shadow(const ray& r, interval ray_t) const {
    vec3 oc = r.origin() - center_point;

    float a = r.direction().length_squared();
    float half_b = dot(oc, r.direction());
    float c = oc.length_squared() - radius * radius;

    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0) return false;

    float sqrt_disc = sqrt(discriminant);

    // Controlla entrambe le soluzioni
    float root = (-half_b - sqrt_disc) / a;
    if (ray_t.surrounds(root))
        return true;

    root = (-half_b + sqrt_disc) / a;
    if (ray_t.surrounds(root))
        return true;

    return false;
}