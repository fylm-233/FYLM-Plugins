#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/docs/0.8.2-host');base.create();var folder=new Folder(base.fsName+'/queue-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write log');log.writeln(s);log.close();}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Fresh empty instance required; existing project preserved.');if(!app.project)app.newProject();
  var input=new File(root+'/docs/0.8.1-host/input-1440.png');if(!input.exists)throw Error('Missing 2560x1440 diagnostic input');
  var footage=app.project.importFile(new ImportOptions(input));var comp=app.project.items.addComp('FYLM PC98 perf 0.8.2',2560,1440,1,1,24);var layer=comp.layers.add(footage),fx=layer.property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');fx.property(2).setValue(7);
  function render(name,bits,block){
   app.project.bitsPerChannel=bits;fx.property(15).setValue(block);var destination=new Folder(folder.fsName+'/'+name);destination.create();
   var item=app.project.renderQueue.items.add(comp);item.timeSpanStart=0;item.timeSpanDuration=1/24;var om=item.outputModule(1),chosen=null;
   for(var t=0;t<om.templates.length;t++)if(/tiff/i.test(om.templates[t])&&/alpha/i.test(om.templates[t])){chosen=om.templates[t];break;}if(!chosen)throw Error('No TIFF Alpha template');om.applyTemplate(chosen);om.file=new File(destination.fsName+'/frame_[#####].tif');
   write('RENDER_BEGIN '+name+' bits='+bits+' block='+block);var started=new Date().getTime();app.project.renderQueue.render();
   if(item.status!==RQItemStatus.DONE)throw Error('Render incomplete '+name);var images=destination.getFiles(function(f){return f instanceof File&&/^frame_.*\.tif$/i.test(f.name)&&f.length>100;});if(!images.length)throw Error('Missing TIFF '+name);
   write('RENDER_DONE '+name+' elapsed_ms='+(new Date().getTime()-started)+' bytes='+images[0].length);item.remove();
  }
  render('pc98-8-b4',8,4);render('pc98-8-b1',8,1);render('pc98-8-b1-repeat',8,1);render('pc98-16-b4',16,4);render('pc98-32-b4',32,4);
  app.project.bitsPerChannel=8;fx.property(15).setValue(4);comp.openInViewer();app.project.save(new File(folder.fsName+'/FYLM-PC98-0.8.2.aep'));write('FYLM_082_QUEUE_SAMPLES_SAVED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);alert('FYLM 0.8.2 test: '+e.toString()+'\n'+log.fsName);}
}());

