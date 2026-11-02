#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),folder=new Folder(root+'/docs/0.4-host');folder.create();
 var log=new File(folder.fsName+'/host.txt');log.open('w');log.close();
 function write(s){log.open('a');log.writeln(s);log.close();}
 try{
  write('SCRIPT_ENTERED');
  if(app.project&&app.project.numItems)throw Error('Run in a fresh AE process; existing project preserved.');
  if(!app.project)app.newProject();
  var found=0;
  write('ENUMERATE_EFFECTS_BEGIN');var effects=app.effects;write('ENUMERATE_EFFECTS_END count='+effects.length);
  for(var i=0;i<effects.length;i++){var e=effects[i];if(e.matchName==='FYLM_DitheringStudio'||e.matchName==='FYLM_CRTVHS'){
   if(e.category!=='FYLM Plugins')throw Error('Wrong category: '+e.category);write(e.displayName+' / '+e.category);found++;
  }}
  if(found!==2)throw Error('Install both FYLM 0.4 plugins and restart AE.');
  write('CREATE_COMP');var comp=app.project.items.addComp('FYLM 0.4',96,96,1,1,24);comp.openInViewer();
  var layer=comp.layers.addSolid([.6,.4,.2],'Static source',96,96,1,1);layer.selected=true;
  write('ADD_DITHER');var d=layer.property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');d.property(2).setValue(6);
  write('RENDER_HALFTONE_BEGIN');
  comp.saveFrameToPng(0,new File(folder.fsName+'/halftone.png'));
  write('RENDER_HALFTONE_END');
  write('ADD_CRT');
  layer.property('ADBE Effect Parade').addProperty('FYLM_CRTVHS');
  write('RENDER_CRT_BEGIN');
  comp.saveFrameToPng(0,new File(folder.fsName+'/motion0.png'));
  comp.saveFrameToPng(.5,new File(folder.fsName+'/motion12.png'));
  comp.saveFrameToPng(0,new File(folder.fsName+'/motion0-repeat.png'));
  write('RENDER_CRT_END');
  app.project.save(new File(folder.fsName+'/FYLM-0.4.aep'));write('FYLM_04_HOST_SAMPLES_SAVED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
}());
