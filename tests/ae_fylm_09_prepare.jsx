#target aftereffects
// Native Apply must be clicked in Effect Controls. This script does not simulate it.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),base=new Folder(root+'/build/p1-private-qa');base.create();
 var log=new File(base.fsName+'/ae-host.txt');
 function write(s){if(!log.open('a'))throw Error('Cannot write host log');log.writeln(new Date().toString()+' '+s);log.close();}
 try{
  write('ENTER ae='+app.version);if(app.project&&app.project.numItems)throw Error('Fresh empty project required; existing project preserved.');if(!app.project)app.newProject();
  var input=new File(base.fsName+'/143385443_p0/input.png');if(!input.exists)input=new File(root+'/docs/0.8.1-host/input-1440.png');if(!input.exists)throw Error('Run private preview tool or generate diagnostic input first.');
  var footage=app.project.importFile(new ImportOptions(input));var comp=app.project.items.addComp('FYLM PA-P1 native apply',footage.width,footage.height,1,1,24);var layer=comp.layers.add(footage),fx=layer.property('ADBE Effect Parade').addProperty('FYLM_DitheringStudio');
  if(fx.numProperties<51)throw Error('Expected 0.9 candidate with 51 controls; got '+fx.numProperties);
  fx.property(49).setValue(2);fx.property(50).setValue(1);layer.selected=true;fx.selected=true;comp.openInViewer();app.project.save(new File(base.fsName+'/P1-native.aep'));
  function quote(s){return '"'+String(s).replace(/\\/g,'\\\\').replace(/"/g,'\\"').replace(/\r/g,'\\r').replace(/\n/g,'\\n').replace(/\t/g,'\\t')+'"';}
  function snapshot(tag){if(!/^[A-Za-z0-9_-]+$/.test(tag))throw Error('Invalid tag');var f=new File(base.fsName+'/'+tag+'.json'),values=[];for(var i=1;i<=51;i++){var p=fx.property(i);if(i===27||i===28||i===29||i===30||i===51)continue;values.push('{"index":'+i+',"value":'+p.value+',"keys":'+p.numKeys+',"expression":'+quote(p.canSetExpression?p.expression:'')+'}');}if(!f.open('w'))throw Error('Cannot write snapshot');f.write('{"ae":'+quote(app.version)+',"values":['+values.join(',')+']}');f.close();write('SNAPSHOT '+tag+' algorithm='+fx.property(2).value+' source='+fx.property(19).value);}
  snapshot('before');
  $.global.FYLM_P1_poll=function(){try{
   // A tiny fixed TSV protocol avoids relying on JSON being installed in ExtendScript.
   var commandFile=new File(base.fsName+'/ae-command.txt');if(commandFile.exists){if(!commandFile.open('r'))throw Error('Cannot read command');var fields=commandFile.read().replace(/\s+$/,'').split('\t');commandFile.close();commandFile.remove();var command={action:fields[0],tag:fields[1]||'sample',bits:Number(fields[2]||8)};if(!/^[A-Za-z0-9_-]+$/.test(command.tag)||!(command.bits===8||command.bits===16||command.bits===32))throw Error('Invalid command fields');
    if(command.action==='snapshot')snapshot(command.tag);
    else if(command.action==='save'){app.project.save(new File(base.fsName+'/P1-applied.aep'));write('SAVED_APPLIED');}
    else if(command.action==='keyframe'){fx.property(17).setValueAtTime(0,45);fx.property(17).setValueAtTime(.5,55);write('KEYFRAMES_READY');}
    else if(command.action==='expression'){while(fx.property(17).numKeys)fx.property(17).removeKey(1);fx.property(17).expression='45';write('EXPRESSION_READY');}
    else if(command.action==='clear-animation'){while(fx.property(17).numKeys)fx.property(17).removeKey(1);fx.property(17).expression='';write('ANIMATION_CLEARED');}
    else if(command.action==='render'){
     app.project.bitsPerChannel=command.bits||8;var folder=new Folder(base.fsName+'/'+command.tag);folder.create();var item=app.project.renderQueue.items.add(comp);item.timeSpanStart=0;item.timeSpanDuration=1/24;var om=item.outputModule(1),template=null;
     for(var t=0;t<om.templates.length;t++)if(/tiff/i.test(om.templates[t])&&/alpha/i.test(om.templates[t])){template=om.templates[t];break;}if(!template)throw Error('No TIFF Alpha template');om.applyTemplate(template);om.file=new File(folder.fsName+'/frame_[#####].tif');var started=new Date().getTime();app.project.renderQueue.render();if(item.status!==RQItemStatus.DONE)throw Error('Render incomplete');item.remove();write('RENDER_DONE '+command.tag+' ms='+(new Date().getTime()-started));
    }else if(command.action==='stop'){write('WATCH_STOPPED');return;}else throw Error('Unknown command');
   }
   app.scheduleTask('FYLM_P1_poll()',500,false);
  }catch(e){write('WATCH_ERROR '+e.toString()+' line '+e.line);}};
  app.scheduleTask('FYLM_P1_poll()',500,false);write('READY_NATIVE_APPLY');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);alert('FYLM PA-P1 test: '+e.toString()+'\n'+log.fsName);}
}());
