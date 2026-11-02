#include "library.hpp"
#include <iostream>
int wmain(int argc,wchar_t** argv){try{
 if(argc==2&&std::wstring(argv[1])==L"--list"){std::cout<<fylm::sg::LibraryPanelJson<<'\n';return 0;}
 if(argc==4&&std::wstring(argv[1])==L"--export"){fylm::sg::save(*fylm::sg::builtinPreset(std::stoi(argv[2])),argv[3]);return 0;}
 std::cerr<<"Usage: FYLM-sglibrary --list | --export menu preset.fylmsg\n";return 2;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
