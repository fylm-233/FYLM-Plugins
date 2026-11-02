#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),dir=new Folder(root+'/docs/0.3-smoke');dir.create();
 var log=new File(dir.fsName+'/gpu-host.txt');log.open('w');log.close();function write(s){log.open('a');log.writeln(s);log.close();}
 function walk(group,depth){for(var i=1;i<=group.numProperties;i++){var p=group.property(i);write(depth+' '+i+' '+p.name+' '+p.matchName+' type='+p.propertyType);if(p.numProperties)walk(p,depth+1);}}
 function param(group,id){var match='OSLO_DitheringStudio-'+('0000'+id).slice(-4);for(var i=1;i<=group.numProperties;i++){var p=group.property(i);if(p.matchName===match)return p;if(p.numProperties){var found=param(p,id);if(found)return found;}}return null;}
 try{app.beginSuppressDialogs();if(app.project&&app.project.numItems)throw Error('Use fresh AE');if(!app.project)app.newProject();
  write('AE '+app.version);var comp=app.project.items.addComp('GPU smoke',64,48,1,1,24);comp.openInViewer();var layer=comp.layers.addSolid([.6,.35,.15],'Source',64,48,1,1);layer.selected=true;
  var fx=layer.property('ADBE Effect Parade').addProperty('OSLO_DitheringStudio');walk(fx,0);
  var enable=param(fx,14);if(!enable)throw Error('Retro parameter missing');param(fx,1).setValue(2);enable.setValue(1);param(fx,4).setValue(32);param(fx,18).setValue(4);
  comp.saveFrameToPng(0,new File(dir.fsName+'/gpu.png'));write('GPU_FRAME_SAVED');
  app.project.save(new File(dir.fsName+'/gpu-smoke.aep'));write('GPU_SMOKE_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}finally{app.endSuppressDialogs(false);}
}());
