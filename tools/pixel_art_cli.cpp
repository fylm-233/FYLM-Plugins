// PA-014 CLI half: runs the pixel-art core on headerless tight RGBA8 so the same code path
// can be exercised outside After Effects. Palette ids come from the existing dither catalog.
#include "dither/core.hpp"
#include "pixel_art/core.hpp"
#include "pixel_art/presets.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

namespace {
int usage() {
    std::cerr << "usage: pixel_art_cli <in.rgba> <out.rgba> <width> <height> [block=4] "
                 "[palette=bw] [binary_alpha=0] [origin_x=0] [origin_y=0] [pixel_aspect=1] "
                 "[sampling=area|nearest|feature] [view=final|mask|baseline] [workers=0(auto)..8]\n"
                 "Input and output are headerless tight RGBA8.\n";
    return 2;
}
} // namespace

int main(int argc, char **argv) {
    try {
        if(argc>=7&&argc<=11&&std::string(argv[1])=="--pc98-spec"){
            const std::map<std::string,pixel_art::PC98ColorSpec> specs={{"art",pixel_art::PC98ColorSpec::Art},{"digital8",pixel_art::PC98ColorSpec::Digital8},{"analog16",pixel_art::PC98ColorSpec::Analog16},{"indexed256",pixel_art::PC98ColorSpec::Indexed256},{"rgb565",pixel_art::PC98ColorSpec::RGB565},{"rgb888",pixel_art::PC98ColorSpec::RGB888}};
            auto found=specs.find(argv[2]);if(found==specs.end())throw std::runtime_error("Unknown PC98 colour specification");int w=std::stoi(argv[5]),h=std::stoi(argv[6]),bits=argc>8?std::stoi(argv[8]):8,count=argc>9?std::stoi(argv[9]):256;if(w<1||h<1||w>32768||h>32768||count<2||count>256||(bits!=6&&bits!=8))return usage();pixel_art::Params p;p.pc98=true;p.pc98ColorSpec=found->second;p.pc98PaletteBits=bits;p.grid.block=argc>7?std::stoi(argv[7]):4;p.workers=argc>10?std::stoi(argv[10]):0;p.palette=pixel_art::pc98HardwarePalette(p.pc98ColorSpec,count,bits);
            std::vector<unsigned char> source(std::size_t(w)*h*4),target(source.size());std::ifstream input(argv[3],std::ios::binary);if(!input.read(reinterpret_cast<char*>(source.data()),std::streamsize(source.size()))||input.peek()!=EOF)throw std::runtime_error("PC98 input size");auto result=pixel_art::render({source.data(),w,h,std::size_t(w)*4,pixel_art::PixelFormat::RGBA8},{target.data(),w,h,std::size_t(w)*4,pixel_art::PixelFormat::RGBA8},p);if(!result.ok)throw std::runtime_error(result.error);std::ofstream output(argv[4],std::ios::binary);output.write(reinterpret_cast<const char*>(target.data()),std::streamsize(target.size()));return output?0:1;
        }
        if(argc==8&&std::string(argv[1])=="--preset"){
            int variant=std::string(argv[3])=="A"?0:std::string(argv[3])=="B"?1:-1,w=std::stoi(argv[6]),h=std::stoi(argv[7]);if(w<1||h<1||w>32768||h>32768)return usage();auto params=pixel_art::artPresetParams(pixel_art::artPreset(argv[2]),variant,w);std::vector<unsigned char> source(std::size_t(w)*h*4),target(source.size());std::ifstream input(argv[4],std::ios::binary);if(!input.read(reinterpret_cast<char*>(source.data()),std::streamsize(source.size()))||input.peek()!=EOF)throw std::runtime_error("preset input size");auto result=pixel_art::render({source.data(),w,h,std::size_t(w)*4,pixel_art::PixelFormat::RGBA8},{target.data(),w,h,std::size_t(w)*4,pixel_art::PixelFormat::RGBA8},params);if(!result.ok)throw std::runtime_error(result.error);std::ofstream output(argv[5],std::ios::binary);output.write(reinterpret_cast<const char*>(target.data()),std::streamsize(target.size()));return output?0:1;
        }
        if(argc==6&&std::string(argv[1])=="--preset-export"){
            int variant=std::string(argv[3])=="A"?0:std::string(argv[3])=="B"?1:-1;auto bytes=pixel_art::artPresetPalette(pixel_art::artPreset(argv[2]),variant,std::stoi(argv[4]));std::ofstream file(argv[5],std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));return file?0:1;
        }
        if(argc==4&&std::string(argv[1])=="--palette"){
            std::string name=argv[2];if(name!="gba"&&name!="modern"&&name!="pc98")return usage();auto colors=pixel_art::stylePalette(name=="gba"?pixel_art::Style::GBA:name=="modern"?pixel_art::Style::Modern:pixel_art::Style::Basic);std::ofstream file(argv[3]);file<<"{\"schema_version\":1,\"name\":\"FYLM "<<name<<"\",\"encoding\":\"srgb\",\"colors\":[";
            for(std::size_t i=0;i<colors.size();i++){auto c=colors[i];if(i)file<<',';file<<"\"#"<<std::hex<<std::uppercase<<std::setfill('0')<<std::setw(2)<<int(c.r)<<std::setw(2)<<int(c.g)<<std::setw(2)<<int(c.b)<<"\"";}file<<"]}\n";return file?0:1;
        }
        if (argc < 5)
            return usage();
        const int width = std::stoi(argv[3]), height = std::stoi(argv[4]);
        if (width <= 0 || height <= 0 || std::size_t(width) > SIZE_MAX / std::size_t(height) / 4) {
            std::cerr << "invalid size\n";
            return 1;
        }
        const std::size_t bytes = std::size_t(width) * height * 4;
        std::vector<unsigned char> source(bytes), target(bytes, 0);
        std::ifstream input(argv[1], std::ios::binary);
        if (!input.read(reinterpret_cast<char *>(source.data()), std::streamsize(bytes)) ||
            input.peek() != EOF) {
            std::cerr << "input must contain exactly width*height*4 bytes\n";
            return 1;
        }
        pixel_art::Params params;
        params.grid.block = argc > 5 ? std::stoi(argv[5]) : 4;
        const std::string paletteId = argc > 6 ? argv[6] : "bw";
        const auto entries = (paletteId=="pc98"||paletteId=="gba"||paletteId=="modern")?std::vector<dither::Rgb8>{}:dither::palette(paletteId);
        if (entries.empty() && paletteId!="pc98" && paletteId!="gba" && paletteId!="modern") {
            std::cerr << "unknown palette: " << paletteId << '\n';
            return 1;
        }
        params.palette.reserve(entries.size());
        for (const auto &entry : entries)
            params.palette.push_back({entry.r, entry.g, entry.b});
        if(paletteId=="pc98"){params.palette=pixel_art::pc98Palette();params.pc98=true;}
        if(paletteId=="gba"||paletteId=="modern"){params.style=paletteId=="gba"?pixel_art::Style::GBA:pixel_art::Style::Modern;params.palette=pixel_art::stylePalette(params.style);params.grid.sampling=pixel_art::Sampling::Feature;params.outlineStrength=paletteId=="gba"?.15:0;params.featureStrength=paletteId=="gba"?.65:.5;}
        params.binaryAlpha = argc > 7 && std::stoi(argv[7]) != 0;
        params.grid.originX = argc > 8 ? std::stoi(argv[8]) : 0;
        params.grid.originY = argc > 9 ? std::stoi(argv[9]) : 0;
        params.grid.pixelAspect = argc > 10 ? std::stod(argv[10]) : 1.0;
        if (argc > 11)
            params.grid.sampling = std::string(argv[11]) == "feature" ? pixel_art::Sampling::Feature : std::string(argv[11]) == "nearest" ? pixel_art::Sampling::Nearest
                                                                      : pixel_art::Sampling::Area;
        if(argc>12){std::string view=argv[12];params.debugChanges=view=="mask";if(view=="baseline"){params.mergeThreshold=0;params.cleanSpeckles=params.cleanSpurs=params.cleanStairs=false;params.outlineStrength=0;}}
        if(argc>13)params.workers=std::stoi(argv[13]);
        auto result = pixel_art::render(
            {source.data(), width, height, std::size_t(width) * 4, pixel_art::PixelFormat::RGBA8},
            {target.data(), width, height, std::size_t(width) * 4, pixel_art::PixelFormat::RGBA8},
            params);
        if (!result.ok) {
            std::cerr << pixel_art::errorKey(result.code) << ": " << result.error << '\n';
            return 1;
        }
        std::ofstream output(argv[2], std::ios::binary);
        if (!output.write(reinterpret_cast<const char *>(target.data()), std::streamsize(bytes))) {
            std::cerr << "cannot write output\n";
            return 1;
        }
        return 0;
    } catch (const std::bad_alloc &) {
        std::cerr << "out of memory\n";
        return 1;
    } catch (const std::exception &e) {
        std::cerr << "invalid arguments: " << e.what() << '\n';
        return 1;
    }
}

