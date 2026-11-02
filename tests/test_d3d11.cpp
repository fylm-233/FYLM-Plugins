#include "fylm/backend.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
int main(){try{retro::RenderRequest r;r.size={96,64};r.timeScale=24;r.input.assign(96*64*4,.4f);for(std::size_t i=3;i<r.input.size();i+=4)r.input[i]=1;
 auto identity=fylm::crt::render(r);if(!identity.ok||identity.pixels!=r.input)throw std::runtime_error("identity");
 r.controls.retroEnabled=true;r.controls.dither.enabled=0;
 auto a=fylm::crt::render(r);r.timeValue=12;auto b=fylm::crt::render(r);r.timeValue=0;auto replay=fylm::crt::render(r);
 if(!a.ok||!b.ok||a.pixels==b.pixels||a.pixels!=replay.pixels)throw std::runtime_error("D3D11 dynamic/replay");
 r.controls.crt.vhsEnabled=0;auto crt=fylm::crt::render(r);r.controls.crt.noiseAmount=1;auto noVhs=fylm::crt::render(r);if(!crt.ok||!noVhs.ok||crt.pixels!=noVhs.pixels)throw std::runtime_error("CRT-only leaks VHS noise");
 r.controls.crt.crtEnabled=0;r.controls.crt.vhsEnabled=1;auto vhs=fylm::crt::render(r);if(!vhs.ok||vhs.pixels==crt.pixels)throw std::runtime_error("Mode separation");
 // S0-03: brightness, contrast and saturation are display-wide and must survive VHS-only mode.
 r.controls.crt.noiseAmount=0;r.controls.crt.wobbleIntensity=0;r.controls.crt.trackingHeight=0;
 r.controls.crt.streakIntensity=0;r.controls.crt.dropoutIntensity=0;r.controls.crt.scanlineOpacity=0;
 r.controls.crt.chromaShiftPixels=0;r.controls.crt.luminanceBlur=0;r.controls.crt.chromaBleed=0;
 r.controls.crt.brightness=0;r.controls.crt.contrast=1;r.controls.crt.saturation=1;
 auto neutral=fylm::crt::render(r);r.controls.crt.contrast=2;auto graded=fylm::crt::render(r);
 if(!neutral.ok||!graded.ok||neutral.pixels==graded.pixels)throw std::runtime_error("VHS-only ignores display contrast");
 r.controls.crt.contrast=1;r.controls.crt.saturation=0;auto gray=fylm::crt::render(r);
 if(!gray.ok)throw std::runtime_error("VHS-only saturation");
 for(std::size_t i=0;i<gray.pixels.size();i+=4)if(std::abs(gray.pixels[i]-gray.pixels[i+1])>1e-4f||std::abs(gray.pixels[i+1]-gray.pixels[i+2])>1e-4f)throw std::runtime_error("VHS-only ignores saturation");
 r.controls.crt.saturation=1;r.controls.crt.brightness=-.1f;r.controls.crt.contrast=1.25f;
 r.controls.crt.crtEnabled=1;r.controls.crt.noiseAmount=0;r.controls.crt.maskType=3;auto slot=fylm::crt::render(r);r.controls.crt.maskType=1;auto grille=fylm::crt::render(r);if(!slot.ok||!grille.ok||slot.pixels==grille.pixels)throw std::runtime_error("Mask type ineffective");
 r.cancelled=[](){return true;};if(fylm::crt::render(r).error!="retro.error.cancelled")throw std::runtime_error("cancel");
 std::cout<<"Actual independent D3D11: float staging, dynamic time, replay and cancellation passed.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
