#target aftereffects
// Manual entry only. Preserves existing projects and does not start a render.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/');
 var build=new Folder(root+'/build');build.create();var base=new Folder(build.fsName+'/sg-host-011');base.create();
 var folder=new Folder(base.fsName+'/run-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(text){if(!log.open('a'))throw Error('Cannot write host log: '+log.fsName);log.writeln(text);log.close();}
 try{
  write('ENTER ae='+app.version);
  if(app.project&&app.project.numItems)throw Error('Use a fresh empty project. Existing project preserved.');
  if(!app.project)app.newProject();
  var input=new File(root+'/assets/presets/pixel-art/thumbnails/source.png');if(!input.exists)throw Error('Missing original diagnostic source');
  var footage=app.project.importFile(new ImportOptions(input));var first=null;
  var choices=[1,2,4,5,6,7,15,26],names=['Geom','EasyMode','SG01','SG02','SG03','SG04','LCD Grid','NTSC 320'];
  // Names and frozen selection numbers are checked against catalog-v1 by the core tests.
  for(var i=0;i<choices.length;i++){
   var comp=app.project.items.addComp('FYLM SG011 '+names[i],768,512,1,1,24);var layer=comp.layers.add(footage),fx=layer.property('ADBE Effect Parade').addProperty('FYLM_ShaderGlass');
   if(!fx||fx.matchName!=='FYLM_ShaderGlass')throw Error('FYLM ShaderGlass unavailable');
   if(!fx.property(17))throw Error('Install 0.11 first; display-strength control missing');
   fx.property(2).setValue(choices[i]);fx.property(4).setValue(0);fx.property(16).setValue(1);fx.property(17).setValue(100);
   write('CREATED '+names[i]+' menu='+choices[i]+' properties='+fx.numProperties);if(i===2)first=comp;
  }
  app.project.save(new File(folder.fsName+'/FYLM-ShaderGlass-011.aep'));if(first)first.openInViewer();
  write('FYLM_011_PREPARED');alert('FYLM 0.11: eight test compositions prepared.\nReview SG01-SG04, undo/defaults and save/reopen manually.\n'+log.fsName);
 }catch(e){try{write('ERROR '+e.toString()+' line '+e.line);}catch(ignore){}alert('FYLM 0.11 preparation: '+e.toString()+'\n'+log.fsName);}
}());
