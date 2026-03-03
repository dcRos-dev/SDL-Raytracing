#ifndef CAMERA_H
#define CAMERA_H

#include <windows.h>
#include <ppl.h>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <vector>
#include "color.h"
#include "common.h"
#include "hittable.h"
#include "light.h"
#include "raster.h"
#include "shader.h"

using namespace std;
using namespace concurrency;

const unsigned int MAX_RAY_DEPTH = 5;

class camera {
public:
    float aspect_ratio;
    int   image_width;
    int   image_height;
    int   samples_per_pixel;

    float  vfov = 90.0f;
    point3 lookfrom = point3(0.0f, 0.0f, -1.0f);
    point3 lookat = point3(0.0f, 0.0f, 0.0f);
    vec3   vup = vec3(0.0f, 1.0f, 0.0f);
    point3 camera_center;

    bool use_ambient_occlusion = true;
    bool use_direct_illumination = true;
    bool use_color_ao = true;

    camera() {}

    camera(float _aspect_ratio, int _image_width) {
        aspect_ratio = _aspect_ratio;
        image_width = _image_width;
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;
    }

    void initialize() {
        image_height = static_cast<int>(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        camera_center = lookfrom;

        // Calculate viewport dimensions.
        float focal_length = (lookfrom - lookat).length();
        float theta = degrees_to_radians(vfov);
        float h = tan(theta / 2.0f);
        float viewport_height = 2.0f * h * focal_length;
        float viewport_width = viewport_height * (static_cast<float>(image_width) / image_height);

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;
        vec3 viewport_v = viewport_height * -v;

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        point3 viewport_upper_left = camera_center - (focal_length * w) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5f * (pixel_delta_u + pixel_delta_v);
    }

    // Renderizzatore sequenziale
    void render(hittable_list& world, point_light& worldlight) {
        for (int j = 0; j < image_height; ++j) {
            std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0.0f, 0.0f, 0.0f);

                for (int sample = 0; sample < samples_per_pixel; ++sample) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, world, worldlight, 0,
                        use_ambient_occlusion,
                        use_direct_illumination);
                }

                pixel_color /= samples_per_pixel;
                setColor(pixel_color[0], pixel_color[1], pixel_color[2]);
                setPixel(i, j);
            }
        }
        SDL_RenderPresent(renderer);
    }

    // Parallel Rendering
    void parallel_render(hittable_list& world, point_light& worldlight) {
        vector<color> matrix(image_width * image_height);
        volatile long rows_completed = 0;

        // The image is converted in a h x w  matrix 
        // and the rendering is splitted in rows
        parallel_for(int(0), image_height, [&](int j) {
            
            //for each row we process each pixel at time
            for (int i = 0; i < image_width; ++i) {
                color pixel_color(0.0f, 0.0f, 0.0f);

                for (int sample = 0; sample < samples_per_pixel; ++sample) {

                    ray r = get_ray(i, j);

                    pixel_color += ray_color(r, world, worldlight, 0,
                        use_ambient_occlusion,
                        use_direct_illumination);
                }

                pixel_color /= samples_per_pixel;
                matrix[j * image_width + i] = pixel_color;
            }

            InterlockedIncrement(&rows_completed);
            if (rows_completed % 50 == 0) {
                std::clog << "\rProgres: " << (rows_completed * 100 / image_height) << "% " << std::flush;
            }
            });

        std::clog << "\rRendering final result" << std::flush;

        for (int j = 0; j < image_height; j++) {
            for (int i = 0; i < image_width; i++) {
                color pixel_color = matrix[j * image_width + i];
                setColor(pixel_color[0], pixel_color[1], pixel_color[2]);
                setPixel(i, j);
            }

            if (j % 50 == 0) {
                SDL_RenderPresent(renderer);
                SDL_PumpEvents();
            }
        }
        SDL_RenderPresent(renderer);
        std::clog << "\rRendering completed!                     " << std::endl;
    }

private:
    point3 pixel00_loc;
    vec3   pixel_delta_u;
    vec3   pixel_delta_v;
    vec3   u, v, w;


    ray get_ray(int i, int j) const {
        
        vec3 pixel_center = pixel00_loc + (i * pixel_delta_u) + (j * pixel_delta_v);
        vec3 pixel_sample = pixel_center + pixel_sample_square();

        point3 ray_origin = camera_center;
        vec3 ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 pixel_sample_square() const {
        float px = -0.5 + random_float();
        float py = -0.5 + random_float();
        return (px * pixel_delta_u) + (py * pixel_delta_v);
    }

    color ray_color(const ray& r, hittable_list& world, point_light& worldlight,
        unsigned int depth, bool use_ao, bool use_direct) {
        hit_record rec;
        color reflectColor = color(1.0f, 1.0f, 1.0f);
        color refractColor = color(1.0f, 1.0f, 1.0f);


        if (depth >= MAX_RAY_DEPTH)
            return color(0, 0, 0);

        if (world.hit(r, interval(0.001f, infinity), rec)) {

            color final_color(0, 0, 0);

            if (use_direct) {
                vec3 light_dir = worldlight.position - rec.p;
                float light_distance = light_dir.length();
                light_dir = unit_vector(light_dir);

                ray shadow_ray(rec.p + light_dir * 0.001f, light_dir);

                bool in_shadow = world.hit_shadow(shadow_ray, interval(0.001f, light_distance));

                color direct_color;
                if (in_shadow) {
                    direct_color = ambient_shading(worldlight, rec);
                }
                else {
                    direct_color = phong_shading(worldlight, rec, camera_center);
                }

                final_color += direct_color;
            }
           

            if (depth < MAX_RAY_DEPTH)
            {
                if (rec.m->reflective > 0)
                {
                    ray reflectRay;
                    reflectRay.d = reflect(-unit_vector(r.d), rec.normal);
                    reflectRay.o = rec.p;
                    reflectRay.o = reflectRay.at(0.01f);

                    reflectColor = ray_color(reflectRay, world, worldlight, depth + 1,use_ao, use_direct);
                }


                if (rec.m->refractive > 0)
                {
                    ray refractRay;
                    if (refract(-unit_vector(r.d), rec.normal, 1.51f, refractRay.d))
                    {
                        refractRay.o = rec.p;
                        refractRay.o = refractRay.at(0.01f);

                        refractColor = ray_color(refractRay, world, worldlight, depth + 1, use_ao, use_direct);
                    }
                    else
                        refractColor = color(0.0f, 0.0f, 0.0f);
                }

            }

            return final_color * (1.0f - rec.m->reflective) + reflectColor * (rec.m->reflective);
        }

        vec3 unit_direction = unit_vector(r.direction());
        float t = 0.5f * (unit_direction.y() + 1.0f);
        return (1.0f - t) * color(1.0f, 1.0f, 1.0f) + t * color(0.5f, 0.7f, 1.0f);
    }
};

#endif