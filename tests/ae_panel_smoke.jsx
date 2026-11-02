#target aftereffects
// Validates native ScriptUI without replacing any installed effect plugin.
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),folder=new Folder(root+'/docs/i18n-smoke');folder.create();
 var log=new File(folder.fsName+'/panel-host.txt');log.open('w');
 function write(s){log.writeln(s);log.close();log.open('a');}
 function find(n,name){if(n.name===name)return n;if(!n.children)return null;for(var i=0;i<n.children.length;i++){var c=find(n.children[i],name);if(c)return c;}return null;}
 var panel=null;
 try{
  app.beginSuppressDialogs();
  if(app.project&&app.project.numItems>0)throw new Error('Use a fresh AE instance; existing project preserved.');
  if(!app.project)app.newProject();
  var comp=app.project.items.addComp('I18n panel test',64,64,1,1,24);comp.openInViewer();
  var catalog=$.evalFile(new File(root+'/build/panel/DitheringStudio-i18n.jsxinc'));
  panel=$.evalFile(new File(root+'/build/panel/Dithering Studio.jsx'));
  if(!panel)throw new Error('ScriptUI panel unavailable');
  if(find(panel,'language'))throw new Error('Manual language switch still present.');
  function localeCode(tag){tag=String(tag||'').toLowerCase().replace(/_/g,'-');if(tag.indexOf('zh')===0)return(tag.indexOf('hant')>=0||tag.indexOf('-tw')>=0||tag.indexOf('-hk')>=0||tag.indexOf('-mo')>=0)?'zh-Hant':'zh-Hans';return tag.indexOf('ja')===0?'ja':'en';}
  var code=localeCode(app.isoLanguage),r=catalog[code],algorithm=find(panel,'1'),threshold=find(panel,'3');
  if(algorithm.items[3].text!==r['algorithm.15'])throw new Error('Automatic menu language mismatch');
  if(threshold.helpTip!==r['hint.range'])throw new Error('Automatic tooltip language mismatch');
  find(panel,'apply').onClick();if(find(panel,'status').text!==r['panel.no_layer'])throw new Error('Automatic error language mismatch');
  write('AE '+app.version+'; automatic '+code+' ScriptUI language and error text passed.');write('PANEL_SMOKE_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}
 finally{if(panel)try{panel.close();}catch(ignore){}app.endSuppressDialogs(false);log.close();}
}());
