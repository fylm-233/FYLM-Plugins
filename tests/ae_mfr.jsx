#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),folder=new Folder($.getenv('DITHER_MFR_RUN')||root+'/docs/0.3-mfr');folder.create();
 var log=new File(folder.fsName+'/host.txt');log.open('w');log.close();function write(s){log.open('a');log.writeln(s);log.close();}
 function param(fx,id){return fx.property('OSLO_DitheringStudio-'+('0000'+id).slice(-4));}
 try{app.beginSuppressDialogs();if(app.project&&app.project.numItems)throw Error('Use fresh AE');if(!app.project)app.newProject();
  if(typeof app.setMultiFrameRenderingConfig!=='function')throw Error('MFR API unavailable');
  write('AE='+app.version+'; 100 frames per render; GPU internal + SmartFX MFR.');
  var comp=app.project.items.addComp('Unified MFR 100',320,180,1,100/24,24);
  var layer=comp.layers.addSolid([.62,.37,.15],'Source',320,180,1,100/24),fx=layer.property('ADBE Effect Parade').addProperty('OSLO_DitheringStudio');
  param(fx,14).setValue(1);param(fx,4).setValue(160);param(fx,18).setValue(5);param(fx,10).setValue(2);
  // Unique across launches, identical within all four phases: avoid persistent disk-cache reuse.
  var runOffset=(new Date().getTime()%1000000)/1000;param(fx,27).setValue(runOffset);write('VHS test time offset='+runOffset);
  param(fx,3).setValueAtTime(0,70);param(fx,3).setValueAtTime(99/24,200);
  var cases=[{name:'bayer8',depth:8,algorithm:2,order:1},{name:'fs16',depth:16,algorithm:1,order:2}];
  for(var c=0;c<cases.length;c++){
   var test=cases[c];app.project.bitsPerChannel=test.depth;param(fx,1).setValue(test.algorithm);param(fx,15).setValue(test.order);
   for(var m=0;m<2;m++){
    var label=test.name+'-'+(m?'mfr':'serial'),output=new Folder(folder.fsName+'/'+label);output.create();
    // Locale is pixel-neutral but invalidates the old effect state so AE cannot reuse the first render.
    param(fx,11).setValue(m+1+c*2);app.setMultiFrameRenderingConfig(m===1,100);
    var item=app.project.renderQueue.items.add(comp);item.timeSpanStart=0;item.timeSpanDuration=100/24;
    var om=item.outputModule(1),found=false;for(var t=0;t<om.templates.length;t++)if(/TIFF/i.test(om.templates[t])){om.applyTemplate(om.templates[t]);found=true;break;}
    if(!found)throw Error('TIFF template unavailable');om.file=new File(output.fsName+'/frame-[#####].tif');
    var before=new Date().getTime();write('START '+label+' '+before);app.project.renderQueue.render();
    write('END '+label+' status='+item.status+' ms='+(new Date().getTime()-before)+' files='+output.getFiles('*.tif').length);
    if(item.status!==RQItemStatus.DONE||output.getFiles('*.tif').length!==100)throw Error('Render incomplete '+label);
   }
  }
  app.project.save(new File(folder.fsName+'/mfr-test.aep'));write('MFR_HOST_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}finally{app.endSuppressDialogs(false);}
}());

