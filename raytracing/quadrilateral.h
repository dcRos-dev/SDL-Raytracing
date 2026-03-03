#pragma once
#ifndef QUADRILATERAL_H
#define QUADRILATERAL_H

#include "hittable.h"
#include "material.h"
#include "vec3.h"

class quadrilateral : public hittable {
public:
    quadrilateral() {}

    quadrilateral(const point3& p, const vec3& n, float w, float h)
        : point(p), normal(n), width(w), height(h) {
        // Normalizza la normale se non è già unitaria
        normal = unit_vector(normal);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        // Calcola il denominatore dell'intersezione
        float denom = dot(r.direction(), normal);
        if (abs(denom) > 1e-6) { // Verifica che il raggio non sia parallelo al piano
            // Calcola l'intersezione del raggio con il piano
            float t = dot(point - r.origin(), normal) / denom;

            if (ray_t.surrounds(t)) { // Verifica i limiti t_min e t_max
                // Punto di intersezione
                point3 hit_point = r.at(t);
                vec3 to_hit = hit_point - point;

                // Crea due vettori ortogonali per definire un sistema di coordinate locale
                vec3 right = unit_vector(cross(normal, vec3(0, 1, 0))); // Vettore lungo la larghezza
                if (abs(dot(normal, vec3(0, 1, 0))) > 0.9) {
                    right = unit_vector(cross(normal, vec3(1, 0, 0))); // Se la normale è quasi verticale, scegli un altro vettore per 'right'
                }
                vec3 up = cross(normal, right); // Vettore lungo l'altezza

                // Proiezione del punto di intersezione sugli assi locali (right, up)
                float proj_x = dot(to_hit, right); // Proiezione sull'asse della larghezza
                float proj_y = dot(to_hit, up);    // Proiezione sull'asse dell'altezza

                // Verifica se il punto di intersezione è all'interno del quadrilatero
                if (abs(proj_x) <= (width * 0.5f) && abs(proj_y) <= (height * 0.5f)) {
                    rec.t = t;
                    rec.p = hit_point;
                    rec.normal = normal; // La normale è già normalizzata

                    // === COORDINATE UV - QUESTA È LA PARTE MANCANTE! ===
                    // Converte da [-width/2, width/2] a [0, 1]
                    rec.u = (proj_x + width * 0.5f) / width;
                    rec.v = (proj_y + height * 0.5f) / height;
                    // ===================================================
                    return true;
                }
            }
        }
        return false;
    }



    bool hit_shadow(const ray& r, interval ray_t) const override {
        float denom = dot(r.direction(), normal);
        if (abs(denom) > 1e-6) {
            float t = dot(point - r.origin(), normal) / denom;

            if (ray_t.surrounds(t)) {
                point3 hit_point = r.at(t);
                vec3 to_hit = hit_point - point;

                // === USA GLI STESSI ASSI LOCALI DI hit() ===
                vec3 right = unit_vector(cross(normal, vec3(0, 1, 0)));
                if (abs(dot(normal, vec3(0, 1, 0))) > 0.9) {
                    right = unit_vector(cross(normal, vec3(1, 0, 0)));
                }
                vec3 up = cross(normal, right);

                float proj_x = dot(to_hit, right);
                float proj_y = dot(to_hit, up);
                // ============================================

                if (abs(proj_x) <= (width * 0.5f) && abs(proj_y) <= (height * 0.5f)) {
                    return true;
                }
            }
        }
        return false;
    }


private:
    point3 point;   // Punto centrale del quadrilatero
    vec3 normal;    // Normale del quadrilatero
    float width;    // Larghezza del quadrilatero
    float height;   // Altezza del quadrilatero
};

#endif
