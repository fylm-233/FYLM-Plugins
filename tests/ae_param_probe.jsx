#target aftereffects
(function(){
 var root=new File($.fileName).parent.parent.fsName.replace(/\\/g,'/'),log=new File(root+'/docs/0.3-ui/param-probe.txt');log.open('w');
 function write(s){log.writeln(s);log.close();log.open('a');}
 try{
  app.beginSuppressDialogs();if(app.project&&app.project.numItems)throw Error('Fresh AE required');if(!app.project)app.newProject();
  var comp=app.project.items.addComp('Param probe',64,48,1,1,24),layer=comp.layers.addSolid([.5,.3,.2],'Source',64,48,1,1);
  var fx=layer.property('ADBE Effect Parade').addProperty('OSLO_DitheringStudio');
  var ids=[1,2,3,4,5,6,7,8,9,10,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29];
  for(var i=0;i<ids.length;i++){var p=fx.property(ids[i]);try{write(ids[i]+' value='+p.value+' expr='+p.expressionEnabled+' keys='+p.numKeys+' exprType='+typeof p.expressionEnabled);if(p.expressionEnabled)throw Error('Expression enabled');}catch(e){write('GET ERROR '+e.toString());}}
  fx.property(1).setValue(2);
  for(var i=0;i<ids.length;i++){var p=fx.property(ids[i]);try{p.setValue(p.value);write('SET '+ids[i]+' OK');}catch(e){write('SET ERROR '+ids[i]+' '+e.toString());}}
  write('PARAM_PROBE_COMPLETE');
 }catch(e){write('ERROR '+e.toString()+' line '+e.line);}finally{app.endSuppressDialogs(false);log.close();}
}());
