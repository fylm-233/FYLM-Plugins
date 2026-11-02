#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/docs/0.8-host');base.create();var folder=new Folder(base.fsName+'/projects-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write '+log.fsName);log.writeln(s);log.close();}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Fresh empty instance required; existing project preserved.');if(!app.project)app.newProject();
  var source=app.project.importFile(new ImportOptions(new File(root+'/docs/pixel-art/0.8-samples/input.png')));var comp=app.project.items.addComp('FYLM Pixel Art 0.8',384,256,1,1,24);var layer=comp.layers.add(source);var fx=layer.property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');
  function save(name,mode,bits){fx.property(2).setValue(mode);app.project.bitsPerChannel=bits;app.project.save(new File(folder.fsName+'/'+name+'.aep'));write('PROJECT '+name+' mode='+mode+' bits='+bits);}
  save('pc98-8',7,8);save('gba-8',8,8);save('gba-16',8,16);save('gba-32',8,32);
  fx.property(33).setValue(2);save('gba-area-8',8,8);fx.property(33).setValue(3);
  save('modern-8',9,8);save('modern-16',9,16);save('modern-32',9,32);
  // Core cleanup evidence uses exact palette colors on a logical-cell-aligned fixture.
  var fixture=app.project.importFile(new ImportOptions(new File(root+'/docs/pixel-art/0.8-samples/structure-input.png')));layer.replaceSource(fixture,false);comp.width=81;comp.height=81;layer.property('ADBE Transform Group').property('ADBE Anchor Point').setValue([40.5,40.5]);layer.property('ADBE Transform Group').property('ADBE Position').setValue([40.5,40.5]);
  save('structure-final',9,8);fx.property(39).setValue(0);fx.property(41).setValue(0);fx.property(42).setValue(0);fx.property(43).setValue(0);save('structure-baseline',9,8);
  fx.property(39).setValue(35);fx.property(41).setValue(1);fx.property(42).setValue(1);fx.property(46).setValue(2);save('structure-mask',9,8);
  write('FYLM_08_PROJECTS_PREPARED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
}());
