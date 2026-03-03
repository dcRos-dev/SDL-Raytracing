#ifndef AMBIENT_OCCLUSION_H
#define AMBIENT_OCCLUSION_H

#include "vec3.h"
#include "ray.h"
#include "hittable_list.h"
#include <cstdlib>  // Per rand()
#include <cmath>    // Per sqrt e altri metodi matematici

class AmbientOcclusion {
public:
    // Calcola il fattore di occlusione ambientale
    static float calculate(const point3& point, const vec3& normal, const hittable_list& world, int sample_count = 16) {
        int occluded = 0;
        for (int i = 0; i < sample_count; i++) {
            vec3 sample_dir = random_in_hemisphere(normal); // Genera una direzione casuale nell'emisfero
            ray sample_ray(point + normal * 0.001f, sample_dir); // Evita self-intersection con un offset
            if (world.hit_shadow(sample_ray, interval(0.001f, FLT_MAX))) { // Usa la funzione hit_shadow
                occluded++;
            }
        }
        return 1.0f - static_cast<float>(occluded) / sample_count; // Fattore AO: meno occlusione significa valore più alto
        // (Quindi si fa 1 - il risultato di occluded / sample_count)
    }

private:
    // Genera raggi AO solo sopra la superficie
    static vec3 random_in_hemisphere(const vec3& normal) {
        vec3 random_vec = random_unit_vector(); // Genera un vettore casuale sulla sfera unitaria
        if (dot(random_vec, normal) > 0.0f) {  // Mantieni solo i vettori sopra la superficie
            return random_vec;
        }
        return -random_vec; // Altrimenti, inverti il vettore
    }

    // Genera un vettore casuale uniforme sulla sfera unitaria(per lanciare i raggi in direzione casuale)
    static vec3 random_unit_vector() {
        float theta = random_float(0.0f, 2.0f * M_PI); // Angolo orizzontale
        float z = random_float(-1.0f, 1.0f);           // Altezza (asse Z)
        float r = sqrt(1.0f - z * z);                 // Raggio sulla circonferenza

        return vec3(r * cos(theta), r * sin(theta), z);
    }

    // Genera un valore casuale uniforme tra min e max
    static float random_float(float min, float max) {
        return min + (max - min) * (rand() / (RAND_MAX + 1.0f));
    }
};

#endif
