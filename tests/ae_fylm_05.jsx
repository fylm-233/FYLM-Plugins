#target aftereffects
// Real-host verification. A stage marker is only written after the PNG exists, has a valid
// PNG header and the expected frame size; content hashes guard the temporal VHS check.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),folder=new Folder(root+'/docs/0.5-host');folder.create();
 var log=new File(folder.fsName+'/host.txt');log.open('w');log.close();
 function write(s){log.open('a');log.writeln(s);log.close();}
 function hashOf(name){
  var f=new File(folder.fsName+'/'+name);if(!f.exists)throw Error('Missing image '+name);
  f.encoding='BINARY';f.open('r');var s=f.read();f.close();
  var h=5381;for(var i=0;i<s.length;i++)h=((h*33)^(s.charCodeAt(i)&0xFF))>>>0;return h;
 }
 function checkPng(name,width,height){
  var f=new File(folder.fsName+'/'+name);
  if(!f.exists)throw Error('Missing image '+name);
  if(f.length<256)throw Error('Image too small '+name+' ('+f.length+' bytes)');
  f.encoding='BINARY';f.open('r');var head=f.read(24);f.close();
  if(head.length<24||(head.charCodeAt(0)&0xFF)!==0x89||(head.charCodeAt(1)&0xFF)!==0x50||(head.charCodeAt(2)&0xFF)!==0x4E||(head.charCodeAt(3)&0xFF)!==0x47)throw Error('Not a PNG: '+name);
  var w=((head.charCodeAt(16)&0xFF)<<24)|((head.charCodeAt(17)&0xFF)<<16)|((head.charCodeAt(18)&0xFF)<<8)|(head.charCodeAt(19)&0xFF);
  var h=((head.charCodeAt(20)&0xFF)<<24)|((head.charCodeAt(21)&0xFF)<<16)|((head.charCodeAt(22)&0xFF)<<8)|(head.charCodeAt(23)&0xFF);
  if(w!==width||h!==height)throw Error('Unexpected size '+name+': '+w+'x'+h);
  var hash=hashOf(name);write('IMAGE_OK '+name+' '+w+'x'+h+' bytes='+f.length+' hash='+hash);
  return hash;
 }
 try{
  write('SCRIPT_ENTERED');
  write('ENV ae='+app.version+' build='+app.buildNumber+' project='+(app.project?app.project.file?app.project.file.fsName:'<unsaved>':'<none>'));
  if(app.project&&app.project.numItems)throw Error('Fresh AE process required; existing project preserved.');
  if(!app.project)app.newProject();
  var effects=app.effects,found=0;
  for(var i=0;i<effects.length;i++){var e=effects[i];if(e.matchName==='FYLM_DitheringStudio'||e.matchName==='FYLM_CRTVHS'||e.matchName==='FYLM_ShaderGlass'){
   if(e.category!=='FYLM Plugins')throw Error('Wrong category for '+e.matchName);
   write('EFFECT '+e.matchName+' display="'+e.displayName+'" category="'+e.category+'"');found++;
  }}if(found!==3)throw Error('Expected three FYLM effects, found '+found);
  var comp=app.project.items.addComp('FYLM 0.5 verification',320,240,1,1,24);comp.openInViewer();
  write('PARAMS bitDepth='+comp.bitsPerChannel+' width='+comp.width+' height='+comp.height);
  var layer=comp.layers.addSolid([.6,.4,.2],'Static source',320,240,1,1);layer.selected=true;
  var parade=layer.property('ADBE Effect Parade');
  var d=parade.addProperty('FYLM_DitheringStudio');d.property(2).setValue(6);
  write('PARAMS dither.algorithm='+d.property(2).value);
  comp.saveFrameToPng(0,new File(folder.fsName+'/halftone.png'));checkPng('halftone.png',320,240);write('HALFTONE_OK');
  d.enabled=false;
  var c=parade.addProperty('FYLM_CRTVHS');c.property(25).setValue(1);
  write('PARAMS crt.mode='+c.property(25).value);
  comp.saveFrameToPng(0,new File(folder.fsName+'/crt.png'));checkPng('crt.png',320,240);write('CRT_MODE_OK');
  c.property(25).setValue(2);
  write('PARAMS vhs.mode='+c.property(25).value+' brightness='+c.property(26).value+' contrast='+c.property(27).value+' saturation='+c.property(28).value);
  comp.saveFrameToPng(0,new File(folder.fsName+'/vhs0.png'));var vhsA=checkPng('vhs0.png',320,240);
  comp.saveFrameToPng(.5,new File(folder.fsName+'/vhs12.png'));var vhsB=checkPng('vhs12.png',320,240);
  comp.saveFrameToPng(0,new File(folder.fsName+'/vhs0-repeat.png'));var vhsC=checkPng('vhs0-repeat.png',320,240);
  if(vhsA!==vhsC)throw Error('VHS same-time repeat is not deterministic');
  if(vhsA===vhsB)throw Error('VHS temporal frames are identical; animation not applied');
  write('VHS_MODE_OK');
  c.enabled=false;
  var s=parade.addProperty('FYLM_ShaderGlass');
  comp.saveFrameToPng(0,new File(folder.fsName+'/geom.png'));checkPng('geom.png',320,240);write('SHADERGLASS_GEOM_OK');
  s.property(2).setValue(2);comp.saveFrameToPng(0,new File(folder.fsName+'/easymode.png'));checkPng('easymode.png',320,240);write('SHADERGLASS_EASYMODE_OK');
  s.property(4).setValue(1);s.property(5).setValue(95);comp.saveFrameToPng(0,new File(folder.fsName+'/easymode-override.png'));checkPng('easymode-override.png',320,240);write('SHADERGLASS_OVERRIDE_OK');
  app.project.save(new File(folder.fsName+'/FYLM-0.5.aep'));write('PROJECT_SAVED');
  app.project.close(CloseOptions.DO_NOT_SAVE_CHANGES);
  app.open(new File(folder.fsName+'/FYLM-0.5.aep'));
  app.project.item(1).saveFrameToPng(0,new File(folder.fsName+'/reopened.png'));checkPng('reopened.png',320,240);write('PROJECT_REOPEN_OK');
  write('FYLM_05_HOST_SAMPLES_SAVED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
}());
