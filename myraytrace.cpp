#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "lodepng.h"

#ifdef INFINITY
	#undef INFINITY
#endif
#define INFINITY 1e8
#define MAXDEPTH 5

using namespace std;
using namespace lodepng;
void encodeTwoSteps(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height) {
	std::vector<unsigned char> png;
	unsigned error = lodepng::encode(png, image, width, height);
	if(!error) lodepng::save_file(png, filename);
	if(error) std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
}
struct RGB {
	int r;
	int g;
	int b;
};
RGB createRGB(int r, int g, int b) {
	RGB c;
	c.r = r;
	c.g = g;
	c.b = b;
	return c;
}
RGB RED, GREEN, BLUE, BLACK, WHITE;
struct vec {
	double x, y, z;
};
vec createVec(double x, double y, double z) {
	vec c;
	c.x = x;
	c.y = y;
	c.z = z;
	return c;
}
struct Sphere {
	vec centre;
	double radius;
	RGB colour;
	float lambert;
	float specular;
	float ambient;
};
struct Plane {
	vec point;
	vec normal;
	RGB colour;
	float lambert, specular, ambient;
};
Plane createPlane(vec point, vec normal, RGB colour, float lambert, float specular, float ambient) {
	Plane p;
	p.point = point;
	p.normal = normal;
	p.colour = colour;
	p.lambert = lambert;
	p.specular = specular;
	p.ambient = ambient;
	return p;
};

//DECS
std::vector<Plane> planes;
std::vector<Sphere> spheres;


struct scene {
	std::vector<Sphere> spheres;
	std::vector<Plane> planes;
};
struct Ray {
	vec orig;
	vec dir;
};
double dot(vec a, vec b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec subtract(vec a, vec b) {
	vec c;
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
	return c;
}
vec add(vec a, vec b) {
	vec c;
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
	return c;
}
double vecmodulus(vec a) {
	return pow((a.x * a.x + a.y * a.y + a.z * a.z), 0.5);
}
vec normalise(vec a) {
	//c used for answer despite only 1 arg
	vec c;
	double m = vecmodulus(a);
	c.x = a.x / m;
	c.y = a.y / m;
	c.z = a.z / m;
	return c;
}
vec multiply(vec a, double scalar) {
	vec c;
	c.x = a.x * scalar;
	c.y = a.y * scalar;
	c.z = a.z * scalar;
	return c;
}
RGB multiply(RGB a, double scalar) {
	RGB c;
	c.r = a.r * scalar;
	c.g = a.g * scalar;
	c.b = a.b * scalar;
	return c;
}
RGB add(RGB a, RGB b) {
	RGB c;
	c.r = a.r + b.r;
	c.g = a.g + b.g;
	c.b = a.b + b.b;
	return c;
}
bool intersectSphere(Ray ray, Sphere sphere, double &t1/*, double &t2*/) {
	/*vec L = subtract(sphere.centre, ray.orig);
	double tc = dot(L, ray.dir);
	double Lmod = vecmodulus(L);
	if (tc < 0)
		return false;
	double d2 = (tc*tc) - (Lmod*Lmod);
	
	double radius2 = sphere.radius*sphere.radius;
	if (d2 > radius2)
		return false;
	double t1c = sqrt(radius2 - d2);
	t1 = tc - t1c;
	t2 = tc + t1c;*/
	double radius2 = sphere.radius * sphere.radius;
	vec l = subtract(sphere.centre, ray.orig);
    double tca = dot(l, ray.dir);
    if (tca < 0)
		return false;
	double d2 = dot(l, l) - tca * tca;
	if (d2 > radius2)
		return false;
	double thc = sqrt(radius2 - d2);
	t1 = tca - thc;
	//t2 = tca + thc;
	return true;
}
bool intersectPlane(Ray r, Plane p, double &p1) {
	double denom = dot(r.dir, p.normal);
	if (denom == 0)
		return false;
	double num = dot(subtract(p.point, r.orig), p.normal);
	p1 = num / denom;
	return true;
}


Sphere createSphere(double x, double y, double z, double radius, 
		RGB colour, float lambert, float specular, float ambient) {
	Sphere s;
	s.centre.x = x;
	s.centre.y = y;
	s.centre.z = z;
	s.radius = radius;
	s.colour = colour;
	s.lambert = lambert;
	s.specular = specular;
	s.ambient = ambient;
	return s;
}
vec createLight(double x, double y, double z) {
	vec l;
	l.x = x;
	l.y = y;
	l.z = z;
	return l;
}
std::vector<vec> lights;

bool isLightVisible(vec pos, vec light){
	//setup ray in dir of light
	Ray shadowray;
	shadowray.orig = pos;
	double t1; /*t2, tnear*;
	tnear = INFINITY;*/
	shadowray.dir = normalise(subtract(light, pos));
	//test if something in the way
	//spheres
	for (int i = 0; i < int(spheres.size()); i++) {
		if (intersectSphere(shadowray, spheres[i], t1))
			return false;
	}
	//planes
	for (int i = 0; i < int(planes.size()); i++) {
		if (intersectPlane(shadowray, planes[i], t1))
			return false;
	}
	return true;
}

int w, h;
RGB trace(Ray r, int depth) {
	float bias = 1e-4;
	RGB finalColour, lColour, refColour;
	finalColour.r = finalColour.g = finalColour.b = 0;
	if (depth > MAXDEPTH) {
		return finalColour;
	}
	//find colour at pixel
	RGB BLACK;
	BLACK.r = BLACK.g = BLACK.b = 0;
	double t1, p1/*, t2*/;
	//sphere counter, holds sphere that's hit
	int sctr = INFINITY, pctr = INFINITY;
	double tnear = INFINITY, pnear = INFINITY;
	 //flat projection
	//conical
	/*r.orig.x = 0;//w / 2;
	r.orig.y = 0;//h / 2;
	r.orig.z = 0;
	r.dir.x = x - w/2;
	r.dir.y = y - h/2;
	r.dir.z = sqrt(w*w + h*h);*/
	
	r.dir = normalise(r.dir);
	//intersect spheres
	for (int i = 0; i < int(spheres.size()); i++) {
		if (intersectSphere(r, spheres[i], t1)) {
			//closest sphere yet ?
			if (t1 < tnear) {
				tnear = t1;
				sctr = i;
			}	
		}
	}
	for (int i = 0; i < int(planes.size()); i++) {
		if (intersectPlane(r, planes[i], p1)) {
			if (p1 < pnear) {
				pnear = p1;
				pctr = i;
			}
		}
	}
	//spheres
	if ((sctr != INFINITY) && (tnear < pnear)) { //collided?
		lColour = spheres[sctr].colour;
		//collision point
		vec pos = add(r.orig, multiply(r.dir, tnear-bias));
		vec normal = normalise(subtract(pos, spheres[sctr].centre));
		Ray reflected;
		reflected.orig = pos;
		reflected.dir = normalise(subtract(r.dir, multiply(normal, 2*dot(normal, r.dir))));
		
		float lambertVal = 0;
		
		//SHAWDOWRAYS
		for (int j = 0; j < int(lights.size()); j++) {
			if (!isLightVisible(pos, lights[j]))
				continue;
			else {
				//LAMBERT
				float contribution = dot(normalise(subtract(lights[j], pos)), normal);
				if (contribution > 0)
					lambertVal += contribution;
			}
		}
		if (lambertVal > 1)
			lambertVal = 1;
		lColour = multiply(lColour, lambertVal * spheres[sctr].lambert);
		
		refColour = multiply(trace(reflected, ++depth), spheres[sctr].specular);
		
		finalColour = add(lColour, refColour);
		//ambient lighting
		finalColour = add(finalColour, multiply(spheres[sctr].colour, spheres[sctr].ambient));
	}
	//planes
	else if ((pctr != INFINITY) && (pnear < tnear)) {
		lColour = planes[pctr].colour;
		//collision point
		vec pos = add(r.orig, multiply(r.dir, pnear-bias));
		vec normal = normalise(planes[pctr].normal);
		Ray reflected;
		reflected.orig = pos;
		reflected.dir = subtract(r.dir, multiply(normal, 2*dot(normal, r.dir)));
		
		float lambertVal = 0;
		
		//SHAWDOWRAYS
		for (int j = 0; j < int(lights.size()); j++) {
			if (!isLightVisible(pos, lights[j]))
				continue;
			else {
				//LAMBERT
				float contribution = dot(normalise(subtract(lights[j], pos)), normal);
				if (contribution > 0)
					lambertVal += contribution;
			}
		}
		if (lambertVal > 1)
			lambertVal = 1;
		lColour = multiply(lColour, lambertVal * planes[pctr].lambert);
		
		refColour = multiply(trace(reflected, ++depth), planes[pctr].specular);
		
		finalColour = add(lColour, refColour);
		//ambient lighting
		finalColour = add(finalColour, multiply(planes[pctr].colour, planes[pctr].ambient));
	}
	else
		finalColour = BLACK;
	return finalColour;
}

int main() {
	//all declared outside for scope
	lights.push_back(createLight(500, 240, 20));
	//lights.push_back(createLight(0, 300, 0));
	
	RED = createRGB(255, 0, 0);
	GREEN.g = 255;
	GREEN.r = GREEN.b = 0;
	BLUE.b = 255;
	BLUE.r = BLUE.g = 0;
	BLACK.r = BLACK.g = BLACK.b = 0;
	WHITE.r = WHITE.g = WHITE.b = 255;
	
	//createPlane(vec point, vec normal, RGB colour, float lambert, float specular, float ambient)
	//planes.push_back(createPlane(normalise(createVec(1, 2, 5000)), createVec(0, 0, 1), BLUE, 1, 1, 1));
	
	printf("calc\n");
	//lodepng takes unsigned
	unsigned width = 640, height = 480;
	//declared outside for scope
	w = width, h = height;
	const char* filename = "raytrace.png";
	spheres.push_back(createSphere(320, 240, 20, 50, RED, 0.9, 0.05, 0.1));
	spheres.push_back(createSphere(320 + 50, 240+150, 130, 80, WHITE, 0.7, 0.05, 0.1));
	spheres.push_back(createSphere(320, -9710, 1600, 10000, GREEN, 0.8, 0.3, 0.2));
	spheres.push_back(createSphere(900, 1000, 100, 90, WHITE, 0, 0, 1));
	std::vector<unsigned char> image;
	image.resize(width * height * 4);
	RGB colour;
	
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < w; x++) {
			Ray r;
			r.orig.x = x, r.orig.y = y, r.orig.z = 0;
			r.dir.x = 0, r.dir.y = 0, r.dir.z = 1;
			colour = trace(r, 0);
			//otherwise image upside down
			int iY = height - y - 1;
			image[4 * width * iY + 4 * x + 0] = min(255, colour.r);
			image[4 * width * iY + 4 * x + 1] = min(255, colour.g);
			image[4 * width * iY + 4 * x + 2] = min(255, colour.b);
			image[4 * width * iY + 4 * x + 3] = 255;
		}
	}
	printf("done\nencoding\n");
	encodeTwoSteps(filename, image, width, height);
	printf("done\n");
	return 0;	
}


