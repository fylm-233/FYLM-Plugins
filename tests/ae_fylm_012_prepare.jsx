#target aftereffects
// Manual preparation only: does not click native Apply, test undo, or run the render queue.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/');
 var build=new Folder(root+'/build');build.create();var base=new Folder(build.fsName+'/host-012');base.create();
 var folder=new Folder(base.fsName+'/run-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write log '+log.fsName);log.writeln(s);log.close();}
 function read(file){if(!file.open('r'))throw Error('Cannot read '+file.fsName);var text=file.read();file.close();return eval('('+text+')');}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Use an empty project. Existing project preserved.');if(!app.project)app.newProject();
  var source=new File(root+'/assets/presets/pixel-art/thumbnails/source.png');if(!source.exists)throw Error('Missing original diagnostic source');
  var footage=app.project.importFile(new ImportOptions(source)),first=null;
  for(var choice=31;choice<=34;choice++){
   var comp=app.project.items.addComp('FYLM SG012 SG0'+(choice-26),768,512,1,2,24);var layer=comp.layers.add(footage),fx=layer.property('ADBE Effect Parade').addProperty('FYLM_ShaderGlass');
   if(!fx||!fx.property(30))throw Error('Install0.12 before testing expanded ShaderGlass controls');
   fx.property(2).setValue(choice);fx.property(4).setValue(0);fx.property(16).setValue(1);fx.property(17).setValue(100);fx.property(18).setValue(1);fx.property(19).setValue(1);fx.property(20).setValue(0);fx.property(21).setValue(0);fx.property(30).setValue(0);
   write('SG_CREATED menu='+choice);if(!first)first=comp;
  }
  var recipes=read(new File(root+'/assets/presets/vhs/catalog-v1.json'));
  var schema=$.evalFile(new File(root+'/build/panel/FYLM_CRTVHS.jsxinc'));var indices={};for(var i=0;i<schema.rows.length;i++)indices[schema.rows[i].key]=schema.rows[i].index;
  for(var i=0;i<recipes.entries.length;i++){
   var recipe=recipes.entries[i],comp=app.project.items.addComp('FYLM VHS012 '+recipe.code,768,512,1,2,24),fx=comp.layers.add(footage).property('ADBE Effect Parade').addProperty('FYLM_CRTVHS');
   if(!fx||!fx.property(45))throw Error('Install0.12 before testing VHS recipes');fx.property(25).setValue(2);
   for(var key in recipe.settings){if(!indices[key])throw Error('Missing parameter '+key);fx.property(indices[key]).setValue(recipe.settings[key]);}fx.property(44).setValue(recipe.choice);
   write('VHS_CREATED '+recipe.code+' direct settings; native Apply still needs manual check');
  }
  app.project.save(new File(folder.fsName+'/FYLM-012-diagnostics.aep'));if(first)first.openInViewer();write('FYLM_012_PREPARED');
  alert('FYLM0.12:12 diagnostic compositions prepared. Review0s/0.5s/1s, then test native Apply/defaults, keyframes, undo and save/reopen manually.\n'+log.fsName);
 }catch(e){try{write('ERROR '+e.toString()+' line '+e.line);}catch(ignore){}alert('FYLM0.12 preparation: '+e.toString()+'\n'+log.fsName);}
}());
