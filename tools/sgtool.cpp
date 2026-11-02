#include "preset.hpp"
#include "tool_process.hpp"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <commdlg.h>
#include <cwctype>
using namespace fylm::sg;
static std::wstring executable(){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);return path;}
int wmain(int argc,wchar_t** argv){try{
 if(argc==2&&std::wstring(argv[1])==L"--worker-hang"){Sleep(600000);return 0;}
 if(argc==3&&std::wstring(argv[1])==L"--inspect"){std::cout<<describe(*load(argv[2]))<<'\n';return 0;}
 if(argc==4&&std::wstring(argv[1])==L"--worker-compile"){auto p=compile(argv[2]);save(p,argv[3]);std::cout<<p.name<<": "<<p.passes.size()<<" passes, "<<p.controls.size()<<" parameters\n";for(std::size_t i=0;i<p.controls.size();i++){auto& u=p.controls[i];std::cout<<i<<" "<<u.name<<" ["<<u.low<<","<<u.high<<"] default="<<u.value<<"\n";}return 0;}
 if(argc==4&&std::wstring(argv[1])==L"--worker-scan"){auto text=scan(argv[2]);std::ofstream out(std::filesystem::path(argv[3]),std::ios::binary);out<<text;if(!out)throw std::runtime_error("sg.error.write");return 0;}
 if(argc==2&&std::wstring(argv[1])==L"--test-timeout"){ToolProcess child(executable(),{L"--worker-hang"},100);while(!child.poll())Sleep(5);if(!child.timedOut)return 1;std::cout<<"worker timeout and job cleanup passed\n";return 0;}
 std::filesystem::path source,dest;bool scanning=false,importing=false;DWORD timeout=120000;
 if(argc==1){wchar_t input[32768]{},output[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.lpstrFile=input;dialog.nMaxFile=32768;dialog.lpstrFilter=L"RetroArch preset (*.slangp)\0*.slangp\0\0";dialog.Flags=OFN_FILEMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&dialog))return 0;source=input;auto suggestion=source;suggestion.replace_extension(L".fylmsg");wcscpy_s(output,suggestion.c_str());dialog.lpstrFile=output;dialog.lpstrFilter=L"FYLM compiled preset (*.fylmsg)\0*.fylmsg\0\0";dialog.lpstrDefExt=L"fylmsg";dialog.Flags=OFN_OVERWRITEPROMPT|OFN_NOCHANGEDIR;if(!GetSaveFileNameW(&dialog))return 0;dest=output;}
 else if(argc==3){source=argv[1];dest=argv[2];}
 else if((argc==4||argc==5)&&(std::wstring(argv[1])==L"scan"||std::wstring(argv[1])==L"compile"||std::wstring(argv[1])==L"import")){scanning=std::wstring(argv[1])==L"scan";importing=std::wstring(argv[1])==L"import";source=argv[2];dest=argv[3];if(argc==5)timeout=std::stoul(argv[4]);}
 else{std::cerr<<"Usage: FYLM-sgtool [scan|compile] input.slangp output [timeout_ms]\n       FYLM-sgtool --inspect compiled.fylmsg\n";return 2;}
 auto temporary=dest;temporary+=L".partial-"+std::to_wstring(GetCurrentProcessId());struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);std::filesystem::remove(p.wstring()+L".probe.rgba",e);std::filesystem::remove(p.wstring()+L".probe.json",e);}} cleanup{temporary};
 auto extension=source.extension().wstring();std::transform(extension.begin(),extension.end(),extension.begin(),[](wchar_t c){return std::towlower(c);});
 if(importing&&extension==L".fylmsg"){auto preset=load(source);requireRuntimeSupport(*preset);save(*preset,temporary);}
 else{ToolProcess worker(executable(),{scanning?L"--worker-scan":L"--worker-compile",source.wstring(),temporary.wstring()},timeout);while(!worker.poll())Sleep(5);if(worker.timedOut)throw std::runtime_error("sg.error.timeout");if(worker.exitCode){std::cerr<<worker.output;throw std::runtime_error("sg.error.compiler");}std::cout<<worker.output;}
 if(!scanning)load(temporary);
 if(importing){auto preset=load(temporary);requireRuntimeSupport(*preset);auto probe=std::filesystem::path(executable()).parent_path()/L"FYLM-sgprobe.exe";ToolProcess validation(probe.wstring(),{temporary.wstring(),L"192",L"128",L"64",L"0",temporary.wstring()+L".probe.rgba",temporary.wstring()+L".probe.json",L"--validate"},timeout);while(!validation.poll())Sleep(5);if(validation.timedOut)throw std::runtime_error("sg.error.timeout");if(validation.exitCode){std::cerr<<validation.output;throw std::runtime_error("sg.error.validation");}}
 if(!MoveFileExW(temporary.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("sg.error.write");
 if(argc==1)MessageBoxW(nullptr,L"Preset compiled. Load the .fylmsg file in FYLM ShaderGlass.",L"FYLM ShaderGlass",MB_OK);return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';if(argc==1)MessageBoxA(nullptr,e.what(),"FYLM ShaderGlass import failed",MB_OK|MB_ICONERROR);return 1;}}
