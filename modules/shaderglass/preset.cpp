#include "preset.hpp"
#include "json.hpp"
#include <cmath>
#include <fstream>
#include <set>
using nlohmann::json;
namespace fylm::sg {
// Bound CBOR nesting before building a DOM inside the host. File size alone
// cannot stop a tiny, deeply nested malicious preset from exhausting the stack.
struct PackageBounds: nlohmann::json_sax<json>{
 int depth=0;std::size_t nodes=0;
 bool item(){return ++nodes<=1000000;}bool null()override{return item();}bool boolean(bool)override{return item();}bool number_integer(number_integer_t)override{return item();}bool number_unsigned(number_unsigned_t)override{return item();}bool number_float(number_float_t,const string_t&)override{return item();}bool string(string_t&)override{return item();}bool binary(binary_t&)override{return item();}bool key(string_t&)override{return item();}
 bool start_object(std::size_t)override{return item()&&++depth<=64;}bool end_object()override{--depth;return true;}bool start_array(std::size_t)override{return item()&&++depth<=64;}bool end_array()override{--depth;return true;}bool parse_error(std::size_t,const std::string&,const nlohmann::detail::exception&)override{return false;}
};
static json uniformJson(const Uniform& u){return {u.name,u.description,u.buffer,u.offset,u.size,u.low,u.high,u.value};}
static Uniform uniformValue(const json& a){if(!a.is_array()||a.size()!=8)throw std::runtime_error("sg.error.schema");return {a.at(0),a.at(1),a.at(2),a.at(3),a.at(4),a.at(5),a.at(6),a.at(7)};}
std::string describe(const Preset& p){json j={{"name",p.name},{"id",p.id},{"mix",p.mix},{"slots",p.slots},{"controls",json::array()},{"passes",json::array()},{"resources",json::array()}};for(auto& u:p.controls)j["controls"].push_back({{"name",u.name},{"description",u.description},{"low",u.low},{"high",u.high},{"value",u.value}});for(auto& pass:p.passes){json a={{"format",pass.format},{"settings",pass.settings},{"samplers",json::array()},{"uniforms",json::array()}};for(auto& s:pass.samplers)a["samplers"].push_back({{"name",s.name},{"binding",s.binding}});for(auto& u:pass.uniforms)a["uniforms"].push_back(uniformJson(u));j["passes"].push_back(a);}for(auto& r:p.resources)j["resources"].push_back({{"name",r.name},{"sha256",r.hash},{"width",r.width},{"height",r.height},{"encoded_bytes",r.encoded.size()},{"settings",r.settings}});return j.dump(2);}
std::vector<unsigned char> encode(const Preset& p){json j={{"schema","fylm.shaderglass/2"},{"name",p.name},{"id",p.id},{"mix",p.mix},{"slots",p.slots},{"passes",json::array()},{"controls",json::array()},{"resources",json::array()}};
 for(auto& c:p.controls)j["controls"].push_back(uniformJson(c));
 for(auto& s:p.passes){json a={{"vertex",json::binary(s.vertex)},{"fragment",json::binary(s.fragment)},{"format",s.format},{"settings",s.settings},{"uniforms",json::array()},{"samplers",json::array()}};for(auto& u:s.uniforms)a["uniforms"].push_back(uniformJson(u));for(auto& t:s.samplers)a["samplers"].push_back({t.name,t.binding});j["passes"].push_back(a);}
 for(auto& r:p.resources)j["resources"].push_back({{"name",r.name},{"sha256",r.hash},{"width",r.width},{"height",r.height},{"encoded",json::binary(r.encoded)},{"settings",r.settings}});
 auto bytes=json::to_cbor(j);if(bytes.size()>32*1024*1024)throw std::runtime_error("sg.error.size");return bytes;
}
void save(const Preset& p,const std::filesystem::path& file){auto bytes=encode(p);std::ofstream out(file,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());if(!out)throw std::runtime_error("sg.error.write");}
std::shared_ptr<const Preset> load(const std::filesystem::path& file){if(std::filesystem::file_size(file)>32*1024*1024)throw std::runtime_error("sg.error.size");std::ifstream in(file,std::ios::binary);if(!in)throw std::runtime_error("sg.error.read");std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(in),{}};return loadBytes(bytes);}
std::shared_ptr<const Preset> loadBytes(const std::vector<unsigned char>& bytes){
 if(bytes.empty()||bytes.size()>32*1024*1024)throw std::runtime_error("sg.error.size");PackageBounds bounds;if(!json::sax_parse(bytes,&bounds,json::input_format_t::cbor))throw std::runtime_error("sg.error.schema");auto j=json::from_cbor(bytes);auto schema=j.at("schema").get<std::string>();if(schema!="fylm.shaderglass/1"&&schema!="fylm.shaderglass/2")throw std::runtime_error("sg.error.schema");
 if(!j.at("passes").is_array()||j.at("passes").empty()||j.at("passes").size()>32||!j.at("controls").is_array()||j.at("controls").size()>1024)throw std::runtime_error("sg.error.schema");
 auto p=std::make_shared<Preset>();p->name=j.at("name");p->mix=j.value("mix",1.f);if(!std::isfinite(p->mix)||p->mix<0||p->mix>1)throw std::runtime_error("sg.error.uniform");p->id="sha256:"+digest(bytes);if(p->name.size()>4096)throw std::runtime_error("sg.error.schema");
 std::set<std::string> controls;for(auto& c:j.at("controls")){auto u=uniformValue(c);if(u.name.empty()||u.name.size()>256||u.description.size()>4096||!controls.insert(u.name).second||!std::isfinite(u.low)||!std::isfinite(u.high)||!std::isfinite(u.value)||u.low>u.high||u.value<u.low||u.value>u.high)throw std::runtime_error("sg.error.uniform");p->controls.push_back(std::move(u));}
 if(j.contains("slots")){p->slots=j.at("slots").get<std::vector<std::string>>();if(p->slots.size()>16)throw std::runtime_error("sg.error.uniform");std::set<std::string> used;for(auto& s:p->slots)if(!s.empty()&&(!controls.count(s)||!used.insert(s).second))throw std::runtime_error("sg.error.uniform");}
 for(auto& a:j.at("passes")){Pass s;s.vertex=a.at("vertex").get_binary();s.fragment=a.at("fragment").get_binary();s.format=a.value("format",std::string());s.settings=a.at("settings").get<std::map<std::string,std::string>>();if(a.at("uniforms").size()>2048||a.at("samplers").size()>16||!supportedFormat(s.format))throw std::runtime_error("sg.error.schema");for(auto& u:a.at("uniforms"))s.uniforms.push_back(uniformValue(u));for(auto& t:a.at("samplers"))s.samplers.push_back({t.at(0),t.at(1)});p->passes.push_back(std::move(s));}
 for(auto& pass:p->passes){if(pass.vertex.empty()||pass.fragment.empty())throw std::runtime_error("sg.error.bytecode");for(auto& u:pass.uniforms)if((u.buffer!=0&&u.buffer!=-1)||u.offset<0||u.size<=0||u.offset>65536-u.size||!std::isfinite(u.value))throw std::runtime_error("sg.error.uniform");for(auto& s:pass.samplers)if(s.binding<0||s.binding>=16)throw std::runtime_error("sg.error.texture");}
 if(j.contains("resources")){if(!j.at("resources").is_array()||j.at("resources").size()>16)throw std::runtime_error("sg.error.resource");std::set<std::string> names;std::size_t total=0;for(auto& a:j.at("resources")){Resource r;r.name=a.at("name");r.hash=a.at("sha256");r.width=a.at("width");r.height=a.at("height");r.settings=a.at("settings").get<std::map<std::string,std::string>>();r.encoded=a.at("encoded").get_binary();if(r.name.empty()||r.name=="Original"||r.name=="Source"||r.name.starts_with("PassOutput")||!names.insert(r.name).second)throw std::runtime_error("sg.error.resource");decodeResource(r);total+=r.rgba.size();if(total>128*1024*1024)throw std::runtime_error("sg.error.resource_budget");p->resources.push_back(std::move(r));}}
 return p;
}
}
