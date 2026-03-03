#include <iostream>
#include <time.h>
#include <stdio.h>
#include "SDL.h" 
#include "SDL_image.h"
#include "float.h"
#include "vec3.h"
#include "color.h"
#include "ray.h"
#include "geometry.h"
#include "sphere.h"
#include "hittable.h"
#include "hittable_list.h"
#include "instance.h"
#include "mesh.h"
#include "raster.h"
#include "object.h"
#include "camera.h"
#include "texture.h"
#include "quadrilateral.h"


using namespace std;

int init(int width, int height) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		return 1;
	}

	window = SDL_CreateWindow("RayTracing",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		width, height, SDL_WINDOW_SHOWN);
	if (window == nullptr) {
		std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	renderer = SDL_CreateRenderer(window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == nullptr) {
		SDL_DestroyWindow(window);
		std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		return 1;
	}

	return 0;
}

void close() {
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* argv[])
{
	hittable_list world;


#pragma region Light Settings

	point3 light_position(0.0f, 20.0f, 0.0f);
	color light_ambient = color(0.8f, 0.8f, 0.8f);
	color light_diffuse = color(0.8f, 0.8f, 0.8f);
	color light_specular = color(1.0f, 1.0f, 1.0f);
	point_light* worldlight = new point_light(light_position, light_ambient, light_diffuse, light_specular);

#pragma endregion


	srand(10);

#pragma region  Floor and Walls


	//  MATERIALS 

	material* wall_material = new material();

	wall_material->texture = nullptr;
	wall_material->kd = color(0.96f, 0.94f, 0.86f);
	wall_material->ka = color(0.1f, 0.1f, 0.1f);
	wall_material->ks = color(0.05f, 0.05f, 0.05f);
	wall_material->alpha = 8.0f;



	texture* pavimento_texture = new image_texture_no_repeat("../models/parquet.jpg");

	material* pavimento_material = new material();
	pavimento_material->texture = pavimento_texture;
	pavimento_material->ka = color(0.1f, 0.1f, 0.1f);
	pavimento_material->kd = color(0.85f, 0.85f, 0.85f);
	pavimento_material->ks = color(0.05f, 0.05f, 0.05f);
	pavimento_material->alpha = 8.0f;


	float W_floor = 50.0f;


	// Floor
	auto floor_quad = new quadrilateral(
		point3(0.0f, 0.0f, 0.0f),
		vec3(0, 1, 0),
		W_floor, W_floor
	);
	world.add(make_shared<instance>(floor_quad, pavimento_material));


	// right wall
	auto wall1 = new quadrilateral(
		point3(0.0f, 25.0f, -25.0f),
		vec3(0, 0, 1),
		W_floor, W_floor
	);
	world.add(make_shared<instance>(wall1, wall_material));

	// left wall
	auto wall2 = new quadrilateral(
		point3(-25.0f, 25.0f, 0.0f),
		vec3(1.0f, 0, 0),
		W_floor, W_floor
	);
	world.add(make_shared<instance>(wall2, wall_material));

#pragma endregion

#pragma region Table


	mesh* table = new mesh("../models/table.obj", "../models/", false);

	if (table == nullptr) {
		cerr << "ERRORE: Impossibile caricare table.obj!" << endl;
	}

	table->UsaBVH = false;

	// Table Legs material
	table->materials[0]->kd =  color(1.0f, 1.0f, 1.0f);
	table->materials[0]->ka = color(0.01f, 0.01f, 0.01f); 
	table->materials[0]->ks = color(0.3f, 0.3f, 0.3f);    
	table->materials[0]->alpha = 32.0f;

	// Table Upper Surface Material
	table->materials[1]->ka = table->materials[1]->kd * 0.3f;
	table->materials[1]->ks = color(0.3f, 0.3f, 0.3f);
	table->materials[1]->alpha = 64.0f;
	table->materials[1]->kd = color(1.0f, 1.0f, 1.0f);

	if (table->materials[1] != nullptr) {
		table->materials[1]->reflective = 0.7f;
		table->materials[1]->refractive = 0.8f;
	}


	auto table_instance = make_shared<instance>(table);
	table_instance->scale(0.02f, 0.02f, 0.02f);
	table_instance->rotate_y(180.0f);
	table_instance->rotate_x(0.0f);
	table_instance->translate(0.0f, 1.0f, 0.0f);

	world.add(table_instance);

#pragma endregion


	

# pragma region Kitchen Set

	mesh* kitchenSet = new mesh("../models/kitchenSet.obj", "../models/", false);

	kitchenSet->UsaBVH = false;


	for (size_t i = 0; i < kitchenSet->materials.size(); i++) {
		if (kitchenSet->materials[i] != nullptr) {

			kitchenSet->materials[i]->kd = color(1.0f, 1.0f, 1.0f);
			kitchenSet->materials[i]->ka = color(0.2f, 0.15f, 0.1f);  
			kitchenSet->materials[i]->ks = color(0.3f, 0.3f, 0.3f);   
			kitchenSet->materials[i]->alpha = 32.0f;                 

		}
	}

	auto kitchenSet_instance = make_shared<instance>(kitchenSet);
	kitchenSet_instance->scale(8.0f, 8.0f, 8.0f);
	kitchenSet_instance->rotate_y(-90.0f);
	kitchenSet_instance->rotate_x(0.0f);
	kitchenSet_instance->translate(-11.0f, 0.0f, -25.0f);


	world.add(kitchenSet_instance);

#pragma endregion


# pragma region Wardrobe


	mesh* wardrobe = new mesh("../models/Wardrobe.obj", "../models/", false);


	wardrobe->UsaBVH = false;


	for (size_t i = 0; i < wardrobe->materials.size(); i++) {
		if (wardrobe->materials[i] != nullptr) {

			wardrobe->materials[i]->kd = color(1.0f, 1.0f, 1.0f);
			wardrobe->materials[i]->ka = color(0.2f, 0.15f, 0.1f);  
			wardrobe->materials[i]->ks = color(0.3f, 0.3f, 0.3f);  
			wardrobe->materials[i]->alpha = 32.0f;                  

		}
	}

	auto wardrobe_instance = make_shared<instance>(wardrobe);
	wardrobe_instance->scale(8.3f, 8.3f, 8.3f);
	wardrobe_instance->rotate_y(270.0f);
	wardrobe_instance->rotate_x(0.0f);
	wardrobe_instance->translate(1.0f, 0.0f, -20.0f);


	world.add(wardrobe_instance);

#pragma endregion


# pragma region HandChair


	mesh* armchair = new mesh("../models/armchair.obj", "../models/", false);


	armchair->UsaBVH = false;


	for (size_t i = 0; i < armchair->materials.size(); i++) {
		if (armchair->materials[i] != nullptr) {

			armchair->materials[i]->kd = color(1.0f, 1.0f, 1.0f);
			armchair->materials[i]->ka = color(0.1f, 0.1f, 0.1f);
			armchair->materials[i]->ks = color(0.3f, 0.3f, 0.3f);
			armchair->materials[i]->alpha = 32.0f;

		}
	}

	auto armchair_instance = make_shared<instance>(armchair);
	armchair_instance->scale(3.0f, 3.0f, 3.0f);
	armchair_instance->rotate_y(115.0f);
	armchair_instance->rotate_x(0.0f);
	armchair_instance->translate(-15.0f, 0.0f, 3.0f);


	world.add(armchair_instance);

#pragma endregion



# pragma region Chair


	mesh* chair = new mesh("../models/chair.obj", "../models/", false);


	chair->UsaBVH = false;


	for (size_t i = 0; i < chair->materials.size(); i++) {
		if (chair->materials[i] != nullptr) {

			chair->materials[i]->kd = color(1.0f, 1.0f, 1.0f);
			chair->materials[i]->ka = color(0.1f, 0.1f, 0.1f);
			chair->materials[i]->ks = color(0.3f, 0.3f, 0.3f);
			chair->materials[i]->alpha = 32.0f;

		}
	}

	auto chair_instance = make_shared<instance>(chair);
	chair_instance->scale(8.0f, 8.0f, 8.0f);
	chair_instance->rotate_y(-90.0f);
	chair_instance->rotate_x(0.0f);
	chair_instance->translate(0.0f, 0.0f, -5.0f);


	 world.add(chair_instance);

#pragma endregion



# pragma region Plant


	mesh* plant = new mesh("../models/trop_flower_set_01.obj", "../models/", false);

	plant->UsaBVH = false;

	for (size_t i = 0; i < plant->materials.size(); i++) {
		if (plant->materials[i] != nullptr) {

			plant->materials[i]->kd = color(1.0f, 1.0f, 1.0f);
			plant->materials[i]->ka = color(0.1f, 0.1f, 0.1f);
			plant->materials[i]->ks = color(0.3f, 0.3f, 0.3f);
			plant->materials[i]->alpha = 32.0f;
			plant->materials[i]->reflective = 0.0f;
			plant->materials[i]->refractive = 0.0f;
		}
	}

	auto plant_instance = make_shared<instance>(plant);
	plant_instance->scale(5.0f, 5.0f, 5.0f);
	plant_instance->rotate_y(-90.0f);
	plant_instance->rotate_x(0.0f);
	plant_instance->translate(-20.0f, 0.0f, -5.0f);


	world.add(plant_instance);

#pragma endregion



	camera cam;

	cam.lookfrom = point3(12.0f, 12.0f, 12.0f);
	cam.lookat = point3(-25.0f, 0.0f, -25.0f);
	cam.vfov = 45.0f;
	cam.aspect_ratio = 16.0f / 9.0f;
	cam.image_width = 1200;
	cam.samples_per_pixel = 1;

	cam.use_direct_illumination = true;

	cam.initialize();


	if (init(cam.image_width, cam.image_height) == 1) {
		std::cout << " SDL Error!" << endl;
		return 1;
	}

	std::cout << "Resolution: " << cam.image_width << "x" << cam.image_height << endl;
	std::cout << "Samples per pixel: " << cam.samples_per_pixel << endl;


	time_t start, end;
	time(&start);

	cam.parallel_render(world, *worldlight);

	time(&end);
	double dif = difftime(end, start);
	std::cout << "\nRendering completed in " << dif << " seconds" << endl;


	// Event Loop
	SDL_Event event;
	bool quit = false;

	while (SDL_PollEvent(&event) || (!quit)) {
		switch (event.type) {
		case SDL_QUIT:
			quit = true;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				quit = true;
				break;

			}
		}
	}

	close();
	return 0;
}
