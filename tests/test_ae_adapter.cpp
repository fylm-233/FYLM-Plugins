// Real adapter under SDK callbacks. This does not replace AE integration tests.
#include "../plugin/ae/DitheringStudio.cpp"
#include <cstring>
#include <iostream>
#include <atomic>
#include <thread>
#include <random>
#include <numeric>
struct Host {
    PF_ParamDef p[Count]{};
    PF_EffectWorld src{}, dst{};
    int checked = 0;
    bool cancel = false;
};
Host &host(PF_ProgPtr r) { return *reinterpret_cast<Host *>(r); }
PF_Err cp(PF_ProgPtr r, PF_ParamIndex i, A_long, A_long, A_u_long, PF_ParamDef *p) {
    *p = host(r).p[i];
    return 0;
}
PF_Err aborter(PF_ProgPtr r) { return host(r).cancel ? PF_Interrupt_CANCEL : 0; }
PF_Err prog(PF_ProgPtr r, A_long, A_long) { return aborter(r); }
PF_Err layer(PF_ProgPtr r, PF_ParamIndex, A_long, const PF_RenderRequest *, A_long, A_long, A_u_long,
             PF_CheckoutResult *o) {
    auto &s = host(r).src;
    o->max_result_rect = {s.origin_x, s.origin_y, s.origin_x + s.width, s.origin_y + s.height};
    o->result_rect = o->max_result_rect;
    return 0;
}
PF_Err pixels(PF_ProgPtr r, A_long, PF_EffectWorld **w) {
    host(r).checked++;
    *w = &host(r).src;
    return 0;
}
PF_Err checkin(PF_ProgPtr r, A_long) {
    host(r).checked--;
    return 0;
}
PF_Err output(PF_ProgPtr r, PF_EffectWorld **w) {
    *w = &host(r).dst;
    return 0;
}
void require(bool v, const char *m) {
    if (!v)
        throw std::runtime_error(m);
}
std::vector<unsigned char> frameResult(int n){
 Host h;h.p[Algorithm].u.pd.value=n%5+1;h.p[Palette].u.pd.value=n%8+1;
 h.p[retro::ae::DitherOn].u.bd.value=1;
 h.p[Threshold].u.sd.value=(n*37)%256;h.p[Resolution].u.sd.value=n%3?11:0;
 h.p[Serpentine].u.bd.value=n%2;h.p[Invert].u.bd.value=n%2;
 h.p[Midtones].u.fs_d.value=.5+(n%10)/10.0;h.p[Contrast].u.fs_d.value=n%100-50;h.p[Highlights].u.fs_d.value=n%100;
 h.p[Alpha].u.pd.value=n%2+1;
 int w=17,height=13,depth=n%2?16:8,bpp=depth==16?8:4,stride=w*bpp+16;
 std::vector<unsigned char> source(stride*height,0),destination(stride*height,0xCD);
 for(int y=0;y<height;y++)for(int x=0;x<w;x++){
  if(depth==8){auto& p=reinterpret_cast<PF_Pixel8*>(source.data()+y*stride)[x];p.red=(x*31+y*7+n*19)%256;p.green=(x*11+y*23+n*3)%256;p.blue=(x*13+y*5+n*29)%256;p.alpha=(x+y+n)%256;}
  else{auto& p=reinterpret_cast<PF_Pixel16*>(source.data()+y*stride)[x];p.red=(x*193+y*257+n*101)%32769;p.green=(x*101+y*997+n*701)%32769;p.blue=(x*211+y*509+n*503)%32769;p.alpha=(x*607+y*137+n*307)%32769;}}
 h.src.width=w;h.src.height=height;h.src.rowbytes=stride;h.src.data=reinterpret_cast<PF_PixelPtr>(source.data());h.dst=h.src;h.dst.data=reinterpret_cast<PF_PixelPtr>(destination.data());
 PF_InData in{};PF_OutData out{};in.effect_ref=reinterpret_cast<PF_ProgPtr>(&h);in.width=w;in.height=height;in.current_time=n*1001;in.time_scale=24000;in.time_step=1001;
 in.inter.checkout_param=cp;in.inter.abort=aborter;in.inter.progress=prog;
 PF_PreRenderCallbacks pcb{};pcb.checkout_layer=layer;PF_PreRenderInput pi{};pi.bitdepth=depth;pi.output_request.rect={0,0,w,height};PF_PreRenderOutput po{};PF_PreRenderExtra pe{&pi,&po,&pcb};
 require(EffectMain(PF_Cmd_SMART_PRE_RENDER,&in,&out,nullptr,nullptr,&pe)==0,"MFR pre");std::unique_ptr<Frame> data(static_cast<Frame*>(po.pre_render_data));
 h.p[Threshold].u.sd.value=255-h.p[Threshold].u.sd.value;in.current_time=-777;
 PF_SmartRenderCallbacks callbacks{pixels,checkin,output};PF_SmartRenderInput si{};si.bitdepth=depth;si.pre_render_data=data.get();PF_SmartRenderExtra se{&si,&callbacks};
 require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&se)==0,"MFR render");require(h.checked==0,"MFR checkout leak");
 return destination;
}
void multiFrame(){
 std::vector<std::vector<unsigned char>> expected(100);for(int n=0;n<100;n++)expected[n]=frameResult(n);
 std::vector<int> order(100);std::iota(order.begin(),order.end(),0);std::mt19937 rng(712);std::shuffle(order.begin(),order.end(),rng);
 std::atomic<int> next{0};std::atomic<bool> failed{false};std::vector<std::thread> workers;
 for(int t=0;t<4;t++)workers.emplace_back([&]{try{for(;;){int k=next.fetch_add(1);if(k>=100)break;int n=order[k];if(frameResult(n)!=expected[n])failed=true;}}catch(...){failed=true;}});
 for(auto& t:workers)t.join();require(!failed,"SDK 100-frame concurrent contamination");
 std::cout<<"SDK 100 sequential vs shuffled 4-thread frames: 8/16bit, tone/palette/alpha snapshots identical.\n";
}
int main() {
    try {
        PF_InData setupIn{};PF_OutData setupOut{};
        require(EffectMain(PF_Cmd_GLOBAL_SETUP,&setupIn,&setupOut,nullptr,nullptr,nullptr)==0,"global setup");
        require((setupOut.out_flags&PF_OutFlag_NON_PARAM_VARY)!=0,"time-driven VHS must invalidate still-frame cache");
        int count = 0;
        for (int depth : {8, 16})
            for (int alg = 1; alg <= 5; alg++)
                for (int pal = 1; pal <= 8; pal++) {
                    Host h;
                    h.p[retro::ae::DitherOn].u.bd.value = 1;
                    h.p[Algorithm].u.pd.value = alg;
                    h.p[Palette].u.pd.value = pal;
                    h.p[Threshold].u.sd.value = 128;
                    h.p[Serpentine].u.bd.value = 1;
                    h.p[Midtones].u.fs_d.value = 1;
                    h.p[Alpha].u.pd.value = 1;
                    const int w = 17, ht = 13, bpp = depth == 16 ? 8 : 4;
                    int stride = w * bpp + 16;
                    std::vector<unsigned char> src(stride * ht), full(stride * ht, 0xCD),
                        roi((7 * bpp + 16) * 5, 0xCD);
                    for (int y = 0; y < ht; y++)
                        for (int x = 0; x < w; x++) {
                            if (depth == 8) {
                                auto &q = reinterpret_cast<PF_Pixel8 *>(src.data() + y * stride)[x];
                                q.red = (x * 13 + y * 31) % 256;
                                q.green = (x * 7 + y * 19) % 256;
                                q.blue = (x * 23 + y * 11) % 256;
                                q.alpha = 255;
                            } else {
                                auto &q = reinterpret_cast<PF_Pixel16 *>(src.data() + y * stride)[x];
                                q.red = (x * 2000 + y * 3000) % 32769;
                                q.green = q.red;
                                q.blue = q.red;
                                q.alpha = PF_MAX_CHAN16;
                            }
                        }
                    h.src.width = w;
                    h.src.height = ht;
                    h.src.rowbytes = stride;
                    h.src.data = reinterpret_cast<PF_PixelPtr>(src.data());
                    h.dst = h.src;
                    h.dst.data = reinterpret_cast<PF_PixelPtr>(full.data());
                    PF_InData in{};
                    PF_OutData out{};
                    in.effect_ref = reinterpret_cast<PF_ProgPtr>(&h);
                    in.width = w;
                    in.height = ht;
                    in.inter.checkout_param = cp;
                    in.inter.abort = aborter;
                    in.inter.progress = prog;
                    PF_PreRenderCallbacks pcb{};
                    pcb.checkout_layer = layer;
                    PF_PreRenderInput pi{};
                    pi.bitdepth = depth;
                    pi.output_request.rect = {0, 0, w, ht};
                    PF_PreRenderOutput po{};
                    PF_PreRenderExtra pe{&pi, &po, &pcb};
                    require(EffectMain(PF_Cmd_SMART_PRE_RENDER, &in, &out, nullptr, nullptr, &pe) == 0,
                            "pre failed");
                    PF_SmartRenderCallbacks scb{pixels, checkin, output};
                    PF_SmartRenderInput si{};
                    si.bitdepth = depth;
                    si.pre_render_data = po.pre_render_data;
                    PF_SmartRenderExtra se{&si, &scb};
                    require(EffectMain(PF_Cmd_SMART_RENDER, &in, &out, nullptr, nullptr, &se) == 0,
                            "render failed");
                    require(h.checked == 0, "checkout leak");
                    h.dst.width = 7;
                    h.dst.height = 5;
                    h.dst.origin_x = 3;
                    h.dst.origin_y = 4;
                    h.dst.rowbytes = 7 * bpp + 16;
                    h.dst.data = reinterpret_cast<PF_PixelPtr>(roi.data());
                    require(EffectMain(PF_Cmd_SMART_RENDER, &in, &out, nullptr, nullptr, &se) == 0,
                            "roi failed");
                    for (int y = 0; y < 5; y++) {
                        require(std::memcmp(roi.data() + y * h.dst.rowbytes,
                                            full.data() + (y + 4) * stride + 3 * bpp, 7 * bpp) == 0,
                                "roi differs");
                        for (int x = 7 * bpp; x < h.dst.rowbytes; x++)
                            require(roi[y * h.dst.rowbytes + x] == 0xCD, "padding overwritten");
                    }
                    si.bitdepth = 32;
                    require(EffectMain(PF_Cmd_SMART_RENDER, &in, &out, nullptr, nullptr, &se) != 0,
                            "32-bit accepted");
                    require(std::string(out.return_msg) ==
                                loc::text(loc::Locale::English, "error.unsupported_format"),
                            "fallback language mismatch");
                    si.bitdepth = depth;
                    h.cancel = true;
                    require(EffectMain(PF_Cmd_SMART_RENDER, &in, &out, nullptr, nullptr, &se) != 0,
                            "cancellation ignored");
                    require(h.checked == 0, "cancel checkout leak");
                    require(std::string(out.return_msg) ==
                                loc::text(loc::Locale::English, "error.cancelled"),
                            "fallback cancel language mismatch");
                    dispose(po.pre_render_data);
                    count++;
                }
        // Both engines disabled must preserve every 16-bit channel, including hidden RGB.
        for (int depth : {8,16}) {
            Host h;
            const int bpp=depth==16?sizeof(PF_Pixel16):sizeof(PF_Pixel8);
            std::vector<unsigned char> source(3*bpp+16,0),destination(3*bpp+16,0xCD);
            if(depth==16){
                auto p=reinterpret_cast<PF_Pixel16*>(source.data());
                p[0]={0,64,12345,32767};p[1]={123,1,8193,32768};p[2]={32768,0,257,10000};
            }else for(int i=0;i<3*bpp;++i)source[i]=static_cast<unsigned char>(i*19);
            h.src.width=3;h.src.height=1;h.src.origin_x=-1;h.src.rowbytes=3*bpp+16;
            h.src.data=reinterpret_cast<PF_PixelPtr>(source.data());
            h.dst=h.src;h.dst.origin_x=-2;h.dst.width=4;h.dst.rowbytes=4*bpp+16;
            destination.assign(h.dst.rowbytes,0xCD);h.dst.data=reinterpret_cast<PF_PixelPtr>(destination.data());
            PF_InData in{};PF_OutData out{};in.effect_ref=reinterpret_cast<PF_ProgPtr>(&h);
            in.inter.abort=aborter;in.inter.progress=prog;
            Frame frame;frame.bounds={-2,0,3,1};frame.retro.dither.enabled=0;
            PF_SmartRenderCallbacks cb{pixels,checkin,output};PF_SmartRenderInput si{};
            si.bitdepth=depth;si.pre_render_data=&frame;PF_SmartRenderExtra extra{&si,&cb};
            require(EffectMain(PF_Cmd_SMART_RENDER,&in,&out,nullptr,nullptr,&extra)==0,"bypass render");
            require(std::memcmp(destination.data()+bpp,source.data(),3*bpp)==0,"disabled engines changed channel values");
            for(int i=0;i<bpp;++i)require(destination[i]==0,"bypass outside source not transparent");
            for(int i=4*bpp;i<h.dst.rowbytes;++i)require(destination[i]==0xCD,"bypass padding overwritten");
            require(h.checked==0,"bypass checkout leak");
        }
        std::cout<<"Disabled engines: exact 8/16-bit RGBA bypass, negative origin, padding and transparent fill passed.\n";
        multiFrame();
        std::cout << count
                  << " SDK adapter cases: 8/16bit, all algorithms/palettes, ROI, padding, cancellation "
                     "passed. Actual AE integration remains separate.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
