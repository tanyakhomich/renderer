#include "tracer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

Vec3 Vec3::operator+(Vec3 b) const { return {x + b.x, y + b.y, z + b.z}; }
Vec3 Vec3::operator-(Vec3 b) const { return {x - b.x, y - b.y, z - b.z}; }
Vec3 Vec3::operator*(double k) const { return {x * k, y * k, z * k}; }
Vec3 Vec3::operator/(double k) const { return {x / k, y / k, z / k}; }

double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

Vec3 unit(Vec3 v) {
    double length = std::sqrt(dot(v, v));
    return length == 0 ? Vec3{} : v / length;
}

Vec3 reflected(Vec3 direction, Vec3 normal) {
    return direction - normal * (2 * dot(direction, normal));
}

bool hit_sphere(const Ray& ray, const Sphere& sphere,
                double t_min, double t_max, Hit& hit) {
    Vec3 offset = ray.origin - sphere.center;
    double a = dot(ray.direction, ray.direction);
    double half_b = dot(offset, ray.direction);
    double c = dot(offset, offset) - sphere.radius * sphere.radius;
    double discriminant = half_b * half_b - a * c;
    if (a == 0 || discriminant < 0) return false;

    double root = (-half_b - std::sqrt(discriminant)) / a;
    if (root < t_min || root > t_max) {
        root = (-half_b + std::sqrt(discriminant)) / a;
        if (root < t_min || root > t_max) return false;
    }
    hit.t = root;
    hit.point = ray.origin + ray.direction * root;
    hit.normal = (hit.point - sphere.center) / sphere.radius;
    hit.material = sphere.material;
    return true;
}

bool hit_ground(const Ray& ray, double t_min, double t_max, Hit& hit) {
    if (std::abs(ray.direction.y) < 1e-10) return false;
    double t = -ray.origin.y / ray.direction.y;
    if (t < t_min || t > t_max) return false;

    hit.t = t;
    hit.point = ray.origin + ray.direction * t;
    hit.normal = {0, 1, 0};
    // fmod avoids converting faraway floor coordinates to an integer.
    double square = std::floor(hit.point.x) + std::floor(hit.point.z);
    bool dark = std::abs(std::fmod(square, 2.0)) > 0.5;
    hit.material = {dark ? Vec3{0.25, 0.29, 0.34} : Vec3{0.67, 0.71, 0.74}, 0.10};
    return true;
}

bool hit_scene(const Ray& ray, const Scene& scene,
               double t_min, double t_max, Hit& hit) {
    bool found = false;
    Hit candidate;
    for (const Sphere& sphere : scene.spheres) {
        if (hit_sphere(ray, sphere, t_min, t_max, candidate)) {
            found = true;
            t_max = candidate.t;
            hit = candidate;
        }
    }
    if (hit_ground(ray, t_min, t_max, candidate)) {
        found = true;
        hit = candidate;
    }
    return found;
}

Scene make_scene() {
    return {{
        {{-1.4, 0.90, -0.8}, 0.90, {{0.78, 0.12, 0.08}, 0.18}},
        {{ 1.0, 0.70,  0.2}, 0.70, {{0.07, 0.28, 0.74}, 0.36}},
        {{ 0.5, 1.10, -2.5}, 1.10, {{0.85, 0.59, 0.11}, 0.12}}
    }, {-3.5, 6, 4}};
}

Vec3 trace(const Ray& ray, const Scene& scene, int depth) {
    constexpr double epsilon = 1e-5;
    const double far_away = std::numeric_limits<double>::infinity();
    Hit hit;
    if (!hit_scene(ray, scene, epsilon, far_away, hit)) {
        double amount = (unit(ray.direction).y + 1) * 0.5;
        return Vec3{0.88, 0.92, 0.97} * (1 - amount)
             + Vec3{0.30, 0.49, 0.76} * amount;
    }

    Vec3 color = hit.material.color * 0.12;
    Vec3 shadow_start = hit.point + hit.normal * epsilon;
    Vec3 to_light = scene.light - shadow_start;
    double light_distance = std::sqrt(dot(to_light, to_light));
    Vec3 light_direction = unit(to_light);
    double brightness = std::max(0.0, dot(hit.normal, light_direction));
    Hit blocker;
    if (brightness > 0 &&
        !hit_scene({shadow_start, light_direction}, scene, epsilon,
                   light_distance - epsilon, blocker)) {
        color = color + hit.material.color * (0.88 * brightness);
        Vec3 halfway = unit(light_direction - unit(ray.direction));
        double highlight = std::pow(std::max(0.0, dot(hit.normal, halfway)), 48);
        color = color + Vec3{1, 1, 1} * (0.22 * highlight);
    }

    if (depth > 0 && hit.material.reflection > 0) {
        Ray bounce{shadow_start, reflected(unit(ray.direction), hit.normal)};
        double r = hit.material.reflection;
        color = color * (1 - r) + trace(bounce, scene, depth - 1) * r;
    }
    return color;
}

std::vector<Vec3> render(const Scene& scene, int width, int height, int samples) {
    if (width < 1 || width > 2000 || height < 1 || height > 2000)
        throw std::invalid_argument("width and height must be between 1 and 2000");
    if (samples != 1 && samples != 4)
        throw std::invalid_argument("samples must be 1 or 4");

    const Vec3 camera{0, 1.9, 6.3};
    Vec3 forward = unit(Vec3{0, 0.85, -1} - camera);
    Vec3 right = unit(cross(forward, {0, 1, 0}));
    Vec3 up = cross(right, forward);
    const double half_height = std::tan(45.0 * std::acos(-1.0) / 360);
    double half_width = half_height * width / height;
    int grid = samples == 4 ? 2 : 1;
    std::vector<Vec3> pixels(static_cast<std::size_t>(width) * height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Vec3 color;
            for (int sy = 0; sy < grid; ++sy) {
                for (int sx = 0; sx < grid; ++sx) {
                    double u = (x + (sx + 0.5) / grid) / width;
                    double v = (y + (sy + 0.5) / grid) / height;
                    Vec3 direction = forward + right * ((2 * u - 1) * half_width)
                                             + up * ((1 - 2 * v) * half_height);
                    color = color + trace({camera, unit(direction)}, scene, 3);
                }
            }
            pixels[static_cast<std::size_t>(y) * width + x] = color / samples;
        }
    }
    return pixels;
}

unsigned char color_byte(double value) {
    value = std::sqrt(std::clamp(value, 0.0, 1.0));
    return static_cast<unsigned char>(255 * value + 0.5);
}
