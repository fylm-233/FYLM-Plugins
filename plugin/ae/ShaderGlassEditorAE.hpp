#pragma once
namespace {
std::wstring sgWide(std::string_view text){if(text.empty())return {};int size=MultiByteToWideChar(CP_UTF8,0,text.data(),int(text.size()),nullptr,0);std::wstring out(size,0);MultiByteToWideChar(CP_UTF8,0,text.data(),int(text.size()),out.data(),size);return out;}
struct SGEditor {
 fylm::sg::Preset preset;dither::i18n::Locale language;HWND list=nullptr,edit=nullptr,range=nullptr,slot=nullptr;int selected=0;
 std::wstring text(const char* key){return sgWide(dither::i18n::text(language,key));}
 bool store(HWND window){wchar_t input[128]{};GetWindowTextW(edit,input,128);wchar_t* end=nullptr;double value=std::wcstod(input,&end);auto& control=preset.controls[selected];if(end==input||*end||!std::isfinite(value)||value<control.low||value>control.high){MessageBoxW(window,text("sg.error.editor_range").c_str(),L"FYLM ShaderGlass",MB_OK|MB_ICONWARNING);return false;}control.value=float(value);return true;}
 void show(){auto& u=preset.controls[selected];wchar_t number[64]{};std::swprintf(number,64,L"%.9g",double(u.value));SetWindowTextW(edit,number);std::swprintf(number,64,L"%.9g  —  %.9g",double(u.low),double(u.high));SetWindowTextW(range,number);}
};
INT_PTR CALLBACK sgEditorDialog(HWND window,UINT message,WPARAM w,LPARAM l){
 auto state=reinterpret_cast<SGEditor*>(GetWindowLongPtrW(window,DWLP_USER));
 if(message==WM_INITDIALOG){state=reinterpret_cast<SGEditor*>(l);SetWindowLongPtrW(window,DWLP_USER,LONG_PTR(state));SetWindowTextW(window,state->text("sg.advanced").c_str());SetWindowPos(window,nullptr,0,0,640,335,SWP_NOMOVE|SWP_NOZORDER);
  auto add=[&](const wchar_t* klass,const wchar_t* label,DWORD style,int x,int y,int width,int height,int id){auto child=CreateWindowExW(0,klass,label,WS_CHILD|WS_VISIBLE|style,x,y,width,height,window,reinterpret_cast<HMENU>(INT_PTR(id)),nullptr,nullptr);SendMessageW(child,WM_SETFONT,WPARAM(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return child;};
  add(L"STATIC",state->text("sg.editor_help").c_str(),0,18,12,590,54,0);
  state->list=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,18,72,590,170,100);
  for(auto& u:state->preset.controls){auto key="sg.uniform."+u.name;auto label=dither::i18n::text(state->language,key);std::string display=label==key?u.description:std::string(label);display+="  ["+u.name+"]";SendMessageW(state->list,CB_ADDSTRING,0,LPARAM(sgWide(display).c_str()));}
  state->edit=add(L"EDIT",L"",WS_BORDER|WS_TABSTOP|ES_AUTOHSCROLL,18,114,210,26,101);state->range=add(L"STATIC",L"",0,245,118,360,24,102);
  state->slot=add(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,18,158,210,200,103);SendMessageW(state->slot,CB_ADDSTRING,0,LPARAM(state->text("sg.keep_slots").c_str()));for(int i=0;i<16;i++){wchar_t label[2]{wchar_t('A'+i),0};SendMessageW(state->slot,CB_ADDSTRING,0,LPARAM(label));}SendMessageW(state->slot,CB_SETCURSEL,0,0);add(L"BUTTON",state->text("sg.bind_slot").c_str(),WS_TABSTOP,245,158,360,28,104);
  add(L"BUTTON",state->text("sg.editor_apply").c_str(),BS_DEFPUSHBUTTON|WS_TABSTOP,260,235,170,30,IDOK);add(L"BUTTON",state->text("sg.editor_cancel").c_str(),WS_TABSTOP,445,235,160,30,IDCANCEL);SendMessageW(state->list,CB_SETCURSEL,0,0);state->show();return TRUE;
 }
 if(!state)return FALSE;
 if(message==WM_COMMAND){if(LOWORD(w)==100&&HIWORD(w)==CBN_SELCHANGE){int selected=int(SendMessageW(state->list,CB_GETCURSEL,0,0));if(selected<0)return TRUE;if(state->store(window)){state->selected=selected;state->show();}else SendMessageW(state->list,CB_SETCURSEL,state->selected,0);return TRUE;}
  if(LOWORD(w)==104){int slot=int(SendMessageW(state->slot,CB_GETCURSEL,0,0))-1;if(slot>=0&&slot<16&&state->store(window)){auto name=state->preset.controls[state->selected].name;if(state->preset.slots.empty())for(std::size_t i=0;i<std::min<std::size_t>(8,state->preset.controls.size());i++)state->preset.slots.push_back(state->preset.controls[i].name);state->preset.slots.resize(16);for(auto& bound:state->preset.slots)if(bound==name)bound.clear();state->preset.slots[slot]=name;wchar_t label[16]{};std::swprintf(label,16,L"%c: ",wchar_t('A'+slot));SetWindowTextW(state->range,(std::wstring(label)+sgWide(name)).c_str());}return TRUE;}
  if(LOWORD(w)==IDOK){if(state->store(window))EndDialog(window,IDOK);return TRUE;}if(LOWORD(w)==IDCANCEL){EndDialog(window,IDCANCEL);return TRUE;}}
 if(message==WM_CLOSE){EndDialog(window,IDCANCEL);return TRUE;}return FALSE;
}
fylm::sg::Preset sgCurrentValues(PF_InData* in,PF_ParamDef* params[]){
 int selection=params[2]->u.pd.value;auto p=sgPreset(selection,selection==3?sgRead(in,params[13]->u.arb_d.value):std::vector<unsigned char>{});
 auto value=*p;
 // Start with the actual visible overrides, so making a copy does not lose the
 // user's A-P adjustments. Advanced values become the copy's saved defaults.
 if(params[4]->u.bd.value)for(int slot=0;slot<(params[30]->u.bd.value?16:8);slot++)if(auto c=fylm::sg::slotControl(*p,slot)){float percent=float(params[slot<8?5+slot:22+slot-8]->u.fs_d.value);for(auto& u:value.controls)if(u.name==c->name)u.value=u.low+(u.high-u.low)*std::clamp(percent,0.f,100.f)/100.f;}
 for(auto& pass:value.passes)for(auto& uniform:pass.uniforms)for(auto& c:value.controls)if(uniform.name==c.name)uniform.value=c.value;
 return value;
}
PF_Err sgEditParameters(PF_InData* in,PF_OutData* out,PF_ParamDef* params[],dither::i18n::Locale language){
 SGEditor state{sgCurrentValues(in,params),language};if(state.preset.controls.empty())return 0;
 struct Template{DLGTEMPLATE dialog;WORD menu=0,klass=0,title=0;} dialog{};dialog.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;dialog.dialog.cx=320;dialog.dialog.cy=140;
 HMODULE module=nullptr;GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&sgEditorDialog),&module);
 auto result=DialogBoxIndirectParamW(module,&dialog.dialog,GetActiveWindow(),sgEditorDialog,LPARAM(&state));if(result==-1)throw std::runtime_error("sg.error.editor");if(result!=IDOK)return 0;
 for(auto& pass:state.preset.passes)for(auto& uniform:pass.uniforms)for(auto& c:state.preset.controls)if(uniform.name==c.name)uniform.value=c.value;
 return sgCommitPreset(in,out,params,fylm::sg::encode(state.preset));
}
PF_Err sgExportPreset(PF_InData* in,PF_ParamDef* params[]){
 auto preset=sgCurrentValues(in,params);wchar_t path[32768]=L"FYLM-ShaderGlass.fylmsg";OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();dialog.lpstrFile=path;dialog.nMaxFile=32768;dialog.lpstrDefExt=L"fylmsg";dialog.lpstrFilter=L"FYLM compiled preset (*.fylmsg)\0*.fylmsg\0\0";dialog.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(!GetSaveFileNameW(&dialog))return 0;
 auto temporary=std::filesystem::path(std::wstring(path)+L".partial-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{temporary};fylm::sg::save(preset,temporary);if(!MoveFileExW(temporary.c_str(),path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("sg.error.write");return 0;
}
}
