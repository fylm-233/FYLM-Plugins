#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/docs/0.7-host');base.create();var folder=new Folder(base.fsName+'/queue-'+new Date().getTime());folder.create();var log=new File(folder.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write '+log.fsName);log.writeln(s);log.close();}
 function render(comp,name){
  var item=app.project.renderQueue.items.add(comp);item.timeSpanStart=0;item.timeSpanDuration=1/comp.frameRate;
  var om=item.outputModule(1),templates=om.templates,chosen=null;for(var i=0;i<templates.length;i++)if(/png/i.test(templates[i]))chosen=templates[i];
  write('OUTPUT_TEMPLATES '+templates.join(' | '));
  if(!chosen)for(var k=0;k<templates.length;k++)if(/tiff/i.test(templates[k])){chosen=templates[k];break;}
  if(!chosen){item.remove();throw Error('No PNG or TIFF output module template.');}
  om.applyTemplate(chosen);var extension=/png/i.test(chosen)?'.png':'.tif';om.file=new File(folder.fsName+'/'+name+'_[#####]'+extension);item.render=true;write('RENDER_BEGIN '+name+' bitDepth='+app.project.bitsPerChannel+' template='+chosen);
  app.project.renderQueue.render();write('RENDER_STATUS '+name+' '+item.status+' expected='+RQItemStatus.DONE);if(item.status!=RQItemStatus.DONE)throw Error('Render did not finish: '+name);item.remove();
  var files=[];for(var attempt=0;attempt<40;attempt++){files=folder.getFiles(function(f){return f instanceof File&&f.name.indexOf(name)===0&&f.length>100;});if(files.length)break;$.sleep(50);}if(!files.length)throw Error('No rendered image for '+name);for(var j=0;j<files.length;j++)write('IMAGE '+files[j].name+' bytes='+files[j].length);
 }
 try{
  write('ENTER ae='+app.version);var comp=null;for(var i=1;i<=app.project.numItems;i++){var it=app.project.item(i);if(it instanceof CompItem&&it.name==='FYLM PC98 0.7')comp=it;}
  if(!comp){if(app.project.numItems)throw Error('Use an empty project. Existing project preserved.');var input=new File(root+'/docs/pixel-art/0.7-samples/input.png');if(!input.exists)throw Error('Missing diagnostic input');var footage=app.project.importFile(new ImportOptions(input));comp=app.project.items.addComp('FYLM PC98 0.7',384,256,1,1,24);comp.openInViewer();comp.layers.add(footage).property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');}
  // Avoid accidentally rendering pre-existing user queue items; preserve their state.
  var queue=app.project.renderQueue,saved=[];for(var i=1;i<=queue.numItems;i++){saved.push(queue.item(i).render);if(queue.item(i).render)queue.item(i).render=false;}
  try{
   var fx=comp.layer(1).property('ADBE Effect Parade').property(1);if(fx.matchName!=='FYLM_DitheringStudio')throw Error('Wrong effect');fx.property(2).setValue(7);fx.property(19).setValue(1);fx.property(17).setValue(65);
   app.project.bitsPerChannel=8;app.project.save(new File(folder.fsName+'/FYLM-PC98-0.7.aep'));write('CHECKPOINT_SAVED');render(comp,'pc98-8');render(comp,'pc98-8-repeat');fx.property(17).setValue(0);render(comp,'nearest-8');fx.property(17).setValue(65);
   app.project.bitsPerChannel=16;render(comp,'pc98-16');app.project.bitsPerChannel=32;render(comp,'pc98-32');
   fx.property(19).setValue(3);fx.property(20).setValue(.25);render(comp,'fixed-reference-32');fx.property(19).setValue(1);app.project.bitsPerChannel=8;
   app.project.save(new File(folder.fsName+'/FYLM-PC98-0.7.aep'));write('PROJECT_SAVED');
   write('FYLM_07_QUEUE_SAMPLES_SAVED');
  }finally{for(var i=0;i<saved.length&&i<queue.numItems;i++)if(queue.item(i+1).render!==saved[i])queue.item(i+1).render=saved[i];}
 }catch(e){try{write('ERROR '+e.toString()+' line '+e.line);}catch(ignore){}alert('FYLM queue test: '+e.toString()+'\n'+log.fsName);}
}());
