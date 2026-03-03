#pragma once
#include "color.h"
#include "light.h"
#include "math.h"
#include "common.h"
#include "texture.h"

struct hit_record;

struct material {
    color ka, kd, ks;
    float alpha; 

    
    material() {
        ka = color(0.3f, 0.3f, 0.3f);  
        kd = color(0.6f, 0.6f, 0.6f);  
        ks = color(0.9f, 0.9f, 0.9f);  
        alpha = 20 + random_float() * 200; 
    }

    material(color ambient, color diffuse, color specular, float a) : ka(ambient), kd(diffuse), ks(specular), alpha(a) {};

    texture* texture; 

    float reflective = 0.0; 
    float refractive = 0.0; 
};

