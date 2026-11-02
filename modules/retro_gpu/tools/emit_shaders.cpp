#include "retro/retro.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc, char **argv) {
    try {
        if (argc != 2)
            return 2;
        std::filesystem::path dir(argv[1]);
        std::filesystem::create_directories(dir);
        for (int i = 0; i <= 6; ++i) {
            std::ofstream f(dir / ("pass" + std::to_string(i) + ".frag"));
            f << retro::fragmentShader(i);
            if (!f)
                return 1;
        }
        std::ofstream f(dir / "fullscreen.vert");
        f << retro::vertexShader();
        return f ? 0 : 1;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
