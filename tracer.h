#ifndef TRACER_H
#define TRACER_H

#include <vector>

struct Vec3 {
    double x = 0;
    double y = 0;
    double z = 0;

    Vec3 operator+(Vec3 b) const;
    Vec3 operator-(Vec3 b) const;
    Vec3 operator*(double k) const;
    Vec3 operator/(double k) const;
};

double dot(Vec3 a, Vec3 b);
Vec3 cross(Vec3 a, Vec3 b);
Vec3 unit(Vec3 v);
Vec3 reflected(Vec3 direction, Vec3 normal);

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct Material {
    Vec3 color;
    double reflection;
};

struct Sphere {
    Vec3 center;
    double radius;
    Material material;
};

struct Hit {
    double t;
    Vec3 point;
    Vec3 normal;
    Material material;
};

struct Scene {
    std::vector<Sphere> spheres;
    Vec3 light;
};

bool hit_sphere(const Ray& ray, const Sphere& sphere,
                double t_min, double t_max, Hit& hit);
bool hit_ground(const Ray& ray, double t_min, double t_max, Hit& hit);
bool hit_scene(const Ray& ray, const Scene& scene,
               double t_min, double t_max, Hit& hit);
Scene make_scene();
Vec3 trace(const Ray& ray, const Scene& scene, int depth);
std::vector<Vec3> render(const Scene& scene, int width, int height, int samples);
unsigned char color_byte(double value);

#endif
