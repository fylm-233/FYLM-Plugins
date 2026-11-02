#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/docs/0.8-host');
 var projects=base.getFiles(function(f){return f instanceof Folder&&/^projects-/.test(f.name);});projects.sort(function(a,b){return a.name<b.name?1:-1;});
 var output=new Folder(base.fsName+'/queue-'+new Date().getTime());output.create();var log=new File(output.fsName+'/host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write log');log.writeln(s);log.close();}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Fresh empty instance required; existing project preserved.');
  if(!projects.length)throw Error('Run ae_fylm_08_prepare.jsx first.');
  var names=['pc98-8','gba-8','gba-16','gba-32','gba-area-8','modern-8','modern-16','modern-32','structure-final','structure-baseline','structure-mask','gba-8-repeat'];
  for(var n=0;n<names.length;n++){
   var name=names[n],projectName=name==='gba-8-repeat'?'gba-8':name;
   if(app.project)app.project.close(CloseOptions.DO_NOT_SAVE_CHANGES);
   app.open(new File(projects[0].fsName+'/'+projectName+'.aep'));
   var comp=null;for(var i=1;i<=app.project.numItems;i++)if(app.project.item(i) instanceof CompItem&&app.project.item(i).name==='FYLM Pixel Art 0.8')comp=app.project.item(i);
   if(!comp)throw Error('Missing test composition '+name);
   var folder=new Folder(output.fsName+'/'+name);folder.create();var item=app.project.renderQueue.items.add(comp);item.timeSpanStart=0;item.timeSpanDuration=1/comp.frameRate;
   var om=item.outputModule(1),chosen=null;for(var t=0;t<om.templates.length;t++)if(/tiff/i.test(om.templates[t])&&/alpha/i.test(om.templates[t])){chosen=om.templates[t];break;}
   if(!chosen)throw Error('No TIFF with Alpha template');om.applyTemplate(chosen);om.file=new File(folder.fsName+'/frame_[#####].tif');
   write('RENDER_BEGIN '+name+' bits='+app.project.bitsPerChannel+' template='+chosen);app.project.renderQueue.render();
   if(item.status!==RQItemStatus.DONE)throw Error('Render not complete '+name);
   var images=folder.getFiles(function(f){return f instanceof File&&/^frame_.*\.tif$/i.test(f.name)&&f.length>100;});if(!images.length)throw Error('Missing TIFF '+name);
   write('RENDER_DONE '+name+' bytes='+images[0].length);item.remove();
  }
  write('FYLM_08_QUEUE_SAMPLES_SAVED');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);alert('FYLM 0.8 test: '+e.toString()+'\n'+log.fsName);}
}());
