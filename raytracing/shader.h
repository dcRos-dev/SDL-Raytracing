#pragma once
#include <iostream>
#include <time.h>
#include <algorithm>
#include "camera.h"
#include "light.h"
#include "material.h"
#include "hittable.h"
#include "color.h"
#include "vec3.h"
#include "math.h"

/**
 * Reflects vector 'i' about normal 'n'.
 * Note: This version assumes 'i' is the vector pointing AWAY from the surface.
 */
inline vec3 reflect(const vec3& i, const vec3& n)
{
    // Standard formula if i points TOWARD the surface: i - 2.0f * dot(i, n) * n;
    return 2.0f * dot(i, n) * n - i;
}

inline bool refract(const vec3& v, const vec3& n, float ni_over_nt, vec3& refracted) {
    vec3 d = unit_vector(v);
    float dn = dot(d, n);
    float discriminant = 1.0f - ni_over_nt * ni_over_nt * (1.0f - dn * dn);

    if (discriminant > 0.0f) {
        refracted = ni_over_nt * (d - n * dn) - n * sqrt(discriminant);
        return true;
    }
    else {
        return false;
    }
}

color phong_shading(point_light& light, hit_record& hr, point3& camera_pos) {
    color ambient(0.0f, 0.0f, 0.0f);
    color diffuse(0.0f, 0.0f, 0.0f);
    color specular(0.0f, 0.0f, 0.0f);

    // ====== TEXTURE SAMPLING ======
    color base_color = hr.m->kd;

    if (hr.m->texture != nullptr) {
        // Sample the texture at UV coordinates
        color texture_color = hr.m->texture->value(hr.u, hr.v, hr.p);
        // Multiply texture by kd (kd acts as a tint/modulation)
        base_color = texture_color * hr.m->kd;
    }
    // ==============================

    ambient = hr.m->ka * light.ambient;

    vec3 L = unit_vector(light.position - hr.p);
    float LDotN = max(dot(L, hr.normal), 0.0f);

    if (LDotN > 0.0f) {
        diffuse = base_color * light.diffuse * LDotN; // Uses base_color (textured or solid)

        vec3 R = reflect(L, hr.normal);
        vec3 V = unit_vector(camera_pos - hr.p);
        float VDotR = std::pow(max(dot(V, R), 0.0f), hr.m->alpha);
        specular = hr.m->ks * light.specular * VDotR;

        return ambient + diffuse + specular;
    }
    else {
        return ambient;
    }
}

color ambient_shading(point_light& light, hit_record& hr) {
    return hr.m->ka * light.ambient;
}