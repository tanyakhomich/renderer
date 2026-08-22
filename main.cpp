#include "tracer.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

int read_number(const std::string& text) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("expected a positive integer");
    return std::stoi(text);
}

void usage(std::ostream& out) {
    out << "Usage: raytracer [output.ppm [width height [samples]]]\n"
        << "Defaults: image.ppm, 480 x 300, 4 samples per pixel\n"
        << "Dimensions: 1 to 2000; samples: 1 or 4\n";
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        usage(std::cout);
        return 0;
    }
    if (argc != 1 && argc != 2 && argc != 4 && argc != 5) {
        usage(std::cerr);
        return 1;
    }

    try {
        std::string path = argc >= 2 ? argv[1] : "image.ppm";
        int width = argc >= 4 ? read_number(argv[2]) : 480;
        int height = argc >= 4 ? read_number(argv[3]) : 300;
        int samples = argc == 5 ? read_number(argv[4]) : 4;
        auto pixels = render(make_scene(), width, height, samples);
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("could not open output file: " + path);
        out << "P6\n" << width << ' ' << height << "\n255\n";
        for (Vec3 color : pixels) {
            unsigned char bytes[]{color_byte(color.x), color_byte(color.y), color_byte(color.z)};
            out.write(reinterpret_cast<const char*>(bytes), 3);
        }
        out.close();
        if (!out) throw std::runtime_error("could not finish writing: " + path);
        std::cout << "Wrote " << path << " (" << width << 'x' << height
                  << ", " << samples << " samples per pixel)\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
