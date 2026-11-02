#target aftereffects
// Verifies automatic host-language selection on a migrated project.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/');
 var folder=new Folder(root+'/docs/i18n-smoke');folder.create();
 var log=new File(folder.fsName+'/host.txt');log.open('w');log.close();
 function write(s){log.open('a');log.writeln(s);log.close();}
 function find(node,name){if(node.name===name)return node;if(!node.children)return null;for(var i=0;i<node.children.length;i++){var value=find(node.children[i],name);if(value)return value;}return null;}
 function localeCode(tag){tag=String(tag||'').toLowerCase().replace(/_/g,'-');if(tag.indexOf('zh')===0)return(tag.indexOf('hant')>=0||tag.indexOf('-tw')>=0||tag.indexOf('-hk')>=0||tag.indexOf('-mo')>=0)?'zh-Hant':'zh-Hans';return tag.indexOf('ja')===0?'ja':'en';}
 try{
  app.beginSuppressDialogs();
  if(app.project&&app.project.numItems>0)throw new Error('Use a fresh AE instance; existing project preserved.');
  app.open(new File(root+'/docs/ae-smoke/smoke.aep'));
  var comp=null;for(var i=1;i<=app.project.numItems;i++)if(app.project.item(i).name==='Dithering Studio Smoke')comp=app.project.item(i);
  if(!comp)throw new Error('Old test composition missing');
  comp.openInViewer();var layer=comp.layer(1);layer.selected=true;
  var fx=layer.property('ADBE Effect Parade').property(1);
  if(fx.property(3).value!==128||fx.property(11).value!==1)throw new Error('Old project migration failed');
  var catalog=$.evalFile(new File(root+'/build/panel/DitheringStudio-i18n.jsxinc'));
  var panel=$.evalFile(new File(root+'/build/panel/Dithering Studio.jsx'));
  if(!panel||find(panel,'language'))throw new Error('Manual language control or panel failure');
  var code=localeCode(app.isoLanguage),r=catalog[code];
  if(fx.property(1).name!==r['param.algorithm'])throw new Error('Native label did not follow AE language: '+fx.property(1).name);
  if(find(panel,'1').items[3].text!==r['algorithm.15'])throw new Error('ScriptUI text did not follow AE language');
  find(panel,'3').text='-1';find(panel,'apply').onClick();
  if(find(panel,'status').text!==r['panel.invalid'])throw new Error('Validation did not follow AE language');
  find(panel,'3').text='128';comp.saveFrameToPng(0,new File(folder.fsName+'/'+code+'.png'));
  write('AE '+app.version+'; '+code+' followed automatically; no per-instance language is persisted.');
  write('I18N_SMOKE_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
 finally{app.endSuppressDialogs(false);log.close();}
}());

