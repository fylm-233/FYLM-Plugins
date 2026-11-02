#include "preset.hpp"
#include <windows.h>
#include <bcrypt.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
namespace fylm::sg {
std::string digest(const std::vector<unsigned char>& bytes){BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::vector<unsigned char> object;struct Guard{BCRYPT_ALG_HANDLE& a;BCRYPT_HASH_HANDLE& h;~Guard(){if(h)BCryptDestroyHash(h);if(a)BCryptCloseAlgorithmProvider(a,0);}}guard{algorithm,hash};
 if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)throw std::runtime_error("sg.error.hash");DWORD length=0,size=0;if(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&length),sizeof(length),&size,0)<0)throw std::runtime_error("sg.error.hash");object.resize(length);if(BCryptCreateHash(algorithm,&hash,object.data(),length,nullptr,0,0)<0||BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),ULONG(bytes.size()),0)<0)throw std::runtime_error("sg.error.hash");unsigned char result[32];if(BCryptFinishHash(hash,result,32,0)<0)throw std::runtime_error("sg.error.hash");std::ostringstream text;text<<std::hex<<std::setfill('0');for(auto value:result)text<<std::setw(2)<<int(value);return text.str();
}
bool supportedFormat(const std::string& s){return s.empty()||s=="R8G8B8A8_UNORM"||s=="R8G8B8A8_SRGB"||s=="R8_UNORM"||s=="R8G8_UNORM"||s=="R16_SFLOAT"||s=="R16G16_SFLOAT"||s=="R16G16B16A16_SFLOAT"||s=="R32_SFLOAT"||s=="R32G32_SFLOAT"||s=="R32G32B32A32_SFLOAT";}
void decodeResource(Resource& r){
 if(r.encoded.empty()||r.encoded.size()>32*1024*1024)throw std::runtime_error("sg.error.resource");auto hash=digest(r.encoded);if(!r.hash.empty()&&r.hash!=hash)throw std::runtime_error("sg.error.hash");r.hash=hash;
 HRESULT initialized=CoInitializeEx(nullptr,COINIT_MULTITHREADED);struct Apartment{HRESULT result;~Apartment(){if(SUCCEEDED(result))CoUninitialize();}} apartment{initialized};
 ComPtr<IWICImagingFactory> factory;ComPtr<IStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;ComPtr<IWICFormatConverter> converter;
 auto check=[](HRESULT h){if(FAILED(h))throw std::runtime_error("sg.error.resource");};check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)));
 auto memory=GlobalAlloc(GMEM_MOVEABLE,r.encoded.size());if(!memory)throw std::bad_alloc();auto bytes=GlobalLock(memory);if(!bytes){GlobalFree(memory);throw std::bad_alloc();}std::memcpy(bytes,r.encoded.data(),r.encoded.size());GlobalUnlock(memory);auto result=CreateStreamOnHGlobal(memory,TRUE,&stream);if(FAILED(result)){GlobalFree(memory);check(result);}
 check(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder));check(decoder->GetFrame(0,&frame));unsigned width=0,height=0;check(frame->GetSize(&width,&height));if(!width||!height||width>16384||height>16384||std::uint64_t(width)*height*4>64*1024*1024)throw std::runtime_error("sg.error.resource_budget");if((r.width&&r.width!=width)||(r.height&&r.height!=height))throw std::runtime_error("sg.error.resource");r.width=width;r.height=height;
 check(factory->CreateFormatConverter(&converter));check(converter->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));r.rgba.resize(std::size_t(width)*height*4);check(converter->CopyPixels(nullptr,width*4,UINT(r.rgba.size()),r.rgba.data()));
}
const Uniform* slotControl(const Preset& p,std::size_t slot){if(slot>=16)return nullptr;if(p.slots.empty())return slot<8&&slot<p.controls.size()?&p.controls[slot]:nullptr;if(slot>=p.slots.size()||p.slots[slot].empty())return nullptr;for(auto& c:p.controls)if(c.name==p.slots[slot])return &c;return nullptr;}
void requireRuntimeSupport(const Preset& p){
 std::map<std::string,bool> available{{"Source",true},{"Original",true},{"OriginalHistory0",true}};
 for(auto& r:p.resources){if(r.rgba.size()!=std::size_t(r.width)*r.height*4||!r.width||!r.height)throw std::runtime_error("sg.error.resource");available[r.name]=true;}
 int index=0;
 for(auto& pass:p.passes){auto enabled=[&](const char* key){auto it=pass.settings.find(key);return it!=pass.settings.end()&&it->second=="true";};
  if(!supportedFormat(pass.format))throw std::runtime_error("sg.error.format_unsupported");
  for(auto& sampler:pass.samplers)if(!available.count(sampler.name))throw std::runtime_error(sampler.name.find("History")!=std::string::npos||sampler.name.find("Feedback")!=std::string::npos?"sg.error.temporal":"sg.error.texture");
  available["PassOutput"+std::to_string(index++)]=true;
  if(auto it=pass.settings.find("alias");it!=pass.settings.end()&&!it->second.empty()){
   if(available.count(it->second)||it->second=="Source"||it->second.starts_with("PassOutput"))throw std::runtime_error("sg.error.texture");
   available[it->second]=true;
  }
 }
}
}
