#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/docs/0.7-host');base.create();var folder=new Folder(base.fsName+'/projects-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write '+log.fsName);log.writeln(s);log.close();}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Fresh empty instance required');if(!app.project)app.newProject();
  var input=new File(root+'/docs/pixel-art/0.7-samples/input.png');var footage=app.project.importFile(new ImportOptions(input));var comp=app.project.items.addComp('FYLM PC98 0.7',384,256,1,1,24);var fx=comp.layers.add(footage).property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');fx.property(2).setValue(7);
  function save(name,bits,source,strength){app.project.bitsPerChannel=bits;fx.property(19).setValue(source);fx.property(20).setValue(.25);fx.property(17).setValue(strength);app.project.save(new File(folder.fsName+'/'+name+'.aep'));write('PROJECT '+name+' bits='+bits+' source='+source+' strength='+strength);}
  save('pc98-8',8,1,65);save('nearest-8',8,1,0);save('pc98-16',16,1,65);save('pc98-32',32,1,65);save('fixed-reference-32',32,3,65);write('FYLM_07_PROJECTS_PREPARED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
}());
