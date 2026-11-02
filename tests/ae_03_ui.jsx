#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),folder=new Folder(root+'/docs/0.3-ui-verified');folder.create();
 var log=new File(folder.fsName+'/host.txt');log.open('w');log.close();
 function write(s){log.open('a');log.writeln(s);log.close();}
 function find(n,name){if(n.name===name)return n;if(n.children)for(var i=0;i<n.children.length;i++){var c=find(n.children[i],name);if(c)return c;}return null;}
 var panel=null;
 try{
  write('START');
  app.beginSuppressDialogs();if(app.project&&app.project.numItems)throw Error('Use fresh AE; existing project preserved.');if(!app.project)app.newProject();
  write('PROJECT_READY');
  var comp=app.project.items.addComp('Unified 0.3 UI',96,64,1,1,24);comp.openInViewer();
  write('COMP_READY');
  var layer=comp.layers.addSolid([.6,.4,.2],'Source',96,64,1,1);layer.selected=true;
  var fx=layer.property('ADBE Effect Parade').addProperty('OSLO_DitheringStudio');
  write('EFFECT_READY');
  var catalog=$.evalFile(new File(root+'/build/panel/DitheringStudio-i18n.jsxinc'));
  write('CATALOG_READY');
  panel=$.evalFile(new File(root+'/build/panel/Dithering Studio.jsx'));
  write('PANEL_READY');
  if(!panel)throw Error('Panel unavailable');if(find(panel,'language'))throw Error('Manual language menu still present');find(panel,'bind').onClick();
  write('AE='+app.version+'; native ScriptUI + installed 0.3 effect.');
  function localeCode(tag){tag=String(tag||'').toLowerCase().replace(/_/g,'-');if(tag.indexOf('zh')===0)return(tag.indexOf('hant')>=0||tag.indexOf('-tw')>=0||tag.indexOf('-hk')>=0||tag.indexOf('-mo')>=0)?'zh-Hant':'zh-Hans';if(tag.indexOf('ja')===0)return'ja';return'en';}
  var code=localeCode(app.isoLanguage),r=catalog[code];
  if(find(panel,'15').items[1].text!==r['retro.order.retro_first'])throw Error('Automatic pipeline translation mismatch');
  if(find(panel,'24').helpTip!==r['hint.retro'])throw Error('Retro tooltip mismatch');
  find(panel,'18').text='101';find(panel,'apply').onClick();
  if(find(panel,'status').text!==r['panel.invalid'])throw Error('Validation translation mismatch');
  find(panel,'18').text='4';find(panel,'14').value=true;find(panel,'15').selection=0;find(panel,'1').selection=1;
  find(panel,'retroPreset').onClick();find(panel,'30').value=false;find(panel,'apply').onClick();
  if(find(panel,'status').text!==r['panel.applied']||fx.property(14).value!==1||fx.property(30).value!==0||fx.property(31).value!==40||fx.property(33).value!==60){
   for(var d=1;d<=39;d++){if(d===11||d===12||d===13)continue;try{fx.property(d).setValue(fx.property(d).value);}catch(probe){write('SET probe '+d+': '+probe.toString());}}
   throw Error('Independent apply failed: status='+find(panel,'status').text+' dither='+fx.property(30).value+' tracking='+fx.property(31).value);
  }
  find(panel,'18').text='0';find(panel,'refresh').onClick();if(Number(find(panel,'18').text)!==4)throw Error('Retro read failed');
  // Keep both samples within the one-second comp and source; replay checks determinism.
  comp.saveFrameToPng(0,new File(folder.fsName+'/'+code+'-frame0.png'));
  comp.saveFrameToPng(.5,new File(folder.fsName+'/'+code+'-frame12.png'));
  comp.saveFrameToPng(0,new File(folder.fsName+'/'+code+'-frame0-repeat.png'));
  write(code+': automatic language, independent dither off/CRT on, source motion preset and apply/read passed; in-range samples saved at 0 and 0.5 seconds, plus replay. Pixel checks run separately.');
  app.project.save(new File(folder.fsName+'/ui.aep'));write('UI_03_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
 finally{if(panel)try{panel.close();}catch(ignore){}app.endSuppressDialogs(false);}
}());
