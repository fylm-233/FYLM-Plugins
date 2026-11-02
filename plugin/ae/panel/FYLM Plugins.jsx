#target aftereffects
(function(parent){
 var home=new File($.fileName).parent,catalog,schemas;
 try{catalog=$.evalFile(new File(home.fsName+'/FYLM-i18n.jsxinc'));schemas=[$.evalFile(new File(home.fsName+'/FYLM_DitheringStudio.jsxinc')),$.evalFile(new File(home.fsName+'/FYLM_CRTVHS.jsxinc')),$.evalFile(new File(home.fsName+'/FYLM_ShaderGlass.jsxinc'))];}
 catch(e){alert('FYLM Plugins: missing panel resources.');return null;}
 function locale(){var tag=String(app.isoLanguage||'en').toLowerCase().replace(/_/g,'-');if(tag.indexOf('zh')===0)return /hant|tw|hk|mo/.test(tag)?'zh-Hant':'zh-Hans';return tag.indexOf('ja')===0?'ja':'en';}
 function t(key){return(catalog[locale()]||{})[key]||catalog.en[key]||key;}
 function selectedIndex(c){var items=c.items,item=c.selection;if(typeof item==='number')return item;if(!item)return -1;for(var i=0;i<items.length;i++)if(items[i]===item||items[i].text===item.text)return i;return -1;}
 function choices(key){if(key==='custom.count')return ['2','3','4','5','6','7','8'];if(key==='halftone.mode'||key==='param.bayer_size')return t(key+'.items').split('|');if(key==='retro.style')return t('retro.style.items').split('|');if(key==='retro.signal_model')return t('retro.signal_model.items').split('|');if(key==='pa.spec')return t('pa.spec.items').split('|');if(key==='pa.dac')return t('pa.dac.items').split('|');if(key==='gba.sampling'||key==='modern.sampling')return['pa.sample_nearest','pa.sample_area','pa.sample_feature'];if(key==='modern.debug')return['modern.final','modern.mask'];if(key==='pa.matrix')return['pa.none','pa.bayer2','pa.bayer4','pa.bayer8'];if(key==='pa.palette')return['pa.builtin','pa.imported','pa.fixed'];if(key==='pa.encoding')return['pa.srgb','pa.linear'];if(key==='pa.alpha')return['pa.coverage','pa.binary'];if(key==='retro.mode')return['mode.crt','mode.vhs','mode.combined'];if(key==='retro.mask_type')return['mask.grille','mask.shadow','mask.slot'];if(key==='sg.preset')return t('sg.preset.items').split('|');if(key==='param.algorithm')return['algorithm.1','algorithm.2','algorithm.8','algorithm.15','algorithm.17','algorithm.31','algorithm.1001','algorithm.1002','algorithm.1003'];if(key==='param.palette')return['palette.none','palette.bw','palette.gb','palette.pico8','palette.db16','palette.db32','palette.cga16','palette.ega16','palette.amber','palette.plasma','palette.custom'];return['alpha.opaque','alpha.preserve'];}
 var w=parent instanceof Panel?parent:new Window('palette','FYLM Plugins',undefined,{resizeable:true});w.orientation='column';w.alignChildren=['fill','top'];
 var products=w.add('tabbedpanel');products.preferredSize=[480,760];var views=[];
 function build(schema,product){
  var rows=[];for(var ri=0;ri<schema.rows.length;ri++){var row=schema.rows[ri];if(product===0&&(row.key==='pa.spec'||row.key==='pa.indexed_colors'||row.key==='pa.dac'))continue;rows.push(row);}

  // Visual order is independent of serialized AE parameter identities.
  var first=product===1?['effect.enabled','retro.mode','retro.vhs','retro.crt','retro.style','retro.signal_model','retro.vhs_strength']:product===2?['effect.enabled','sg.preset','sg.strength','sg.use_slots','sg.follow_preset','sg.virtual_height']:[];
  rows.sort(function(a,b){var x=first.length+a.index,y=first.length+b.index;for(var n=0;n<first.length;n++){if(a.key===first[n])x=n;if(b.key===first[n])y=n;}return x-y;});
  var tab=products.add('tab',undefined,schema.name);tab.orientation='column';tab.alignChildren=['fill','top'];
  var workflow=tab.add('statictext',undefined,t('panel.workflow'),{multiline:true});workflow.preferredSize=[410,42];
  var tabs=tab.add('tabbedpanel');tabs.alignChildren=['fill','top'];tabs.preferredSize=[420,450];
  var page1=tabs.add('tab',undefined,product===0?t('panel.dither_tab'):t('panel.mode_style'));page1.orientation='column';page1.alignChildren=['fill','top'];
  var page2=tabs.add('tab',undefined,product===0?t('algorithm.31'):t('panel.signal_motion'));page2.orientation='column';page2.alignChildren=['fill','top'];tabs.selection=page1;
  if(product===2){page1.text=t('sg.style_controls');page2.text=t('sg.source_actions');var sourceHelp=page2.add('statictext',undefined,t('sg.source_help'),{multiline:true});sourceHelp.preferredSize=[390,110];}
  var page3=product===1?tabs.add('tab',undefined,t('retro.display')):product===0?tabs.add('tab',undefined,'PC-98'):null;if(page3){page3.orientation='column';page3.alignChildren=['fill','top'];}
  var page4=product===0?tabs.add('tab',undefined,'GBA'):null,page5=product===0?tabs.add('tab',undefined,t('algorithm.1003')):null,shared=product===0?tabs.add('tab',undefined,t('pa.common')):null;for(var q=0;q<3;q++){var pg=[page4,page5,shared][q];if(pg){pg.orientation='column';pg.alignChildren=['fill','top'];}}
  var sgTime=null,sgMainSlots=null,sgExtraSlots=null;if(product===2){sgTime=tabs.add('tab',undefined,t('sg.motion'));sgMainSlots=tabs.add('tab',undefined,t('sg.main_slots'));sgExtraSlots=tabs.add('tab',undefined,t('sg.extra_slots'));var pages=[sgTime,sgMainSlots,sgExtraSlots];for(var st=0;st<pages.length;st++){pages[st].orientation='column';pages[st].alignChildren=['fill','top'];}}
  var fields={};
  for(var i=0;i<rows.length;i++){var r=rows[i],advanced=product===0?r.index>=12:r.index>=8;
   var destination=advanced?page2:page1;if(product===0&&r.index>=15)destination=page3;if(product===0&&r.index>=19&&r.index<=26&&r.index!==21)destination=shared;if(product===0&&r.index>=31&&r.index<=35)destination=page4;if(product===0&&r.index>=36)destination=page5;if(product===0&&r.index>=61)destination=r.key.indexOf('halftone.')===0?page2:page1;if(product===1&&r.index>25)destination=page3;if(r.key==='retro.mode')destination=page1;if(product===0&&(r.key==='pa.spec'||r.key==='pa.indexed_colors'||r.key==='pa.dac'))destination=page3;
   if(product===1&&(r.key==='retro.style'||r.key==='retro.signal_model'||r.key==='retro.vhs_strength'))destination=page1;
   if(product===2){if(r.key.indexOf('sg.slot_')===0)destination=r.key.charAt(8)>='i'?sgExtraSlots:sgMainSlots;else if(r.key==='sg.extended_slots')destination=sgExtraSlots;else if(r.key==='sg.animate'||r.key==='sg.time_rate'||r.key==='sg.time_offset'||r.key==='sg.frame_offset')destination=sgTime;else destination=page1;}
   var group=destination.add('group');var label=group.add('statictext',undefined,t(r.key));label.preferredSize.width=190;
   var c=group.add(r.kind===0?'checkbox':r.kind===3?'dropdownlist':'edittext',undefined,r.kind===3?[]:String(r.value));c.name=String(r.index);c.preferredSize.width=180;
   if(r.kind===0)c.value=!!r.value;
   if(r.kind===3){var keys=choices(r.key);for(var k=0;k<keys.length;k++)c.add('item',t(keys[k]));c.selection=r.value-1;}
   fields[r.key]={spec:r,control:c,label:label,group:group};
   if(r.key==='custom.count')c.helpTip=t('custom.help');
   if(r.kind!==0&&r.kind!==3)c.helpTip=t('panel.range')+': '+r.low+' – '+r.high;
   if(product===2&&r.key==='sg.virtual_height')c.onChange=function(){if(fields['sg.follow_preset'])fields['sg.follow_preset'].control.value=false;};
   if(product===2&&r.key==='sg.extended_slots')c.onClick=function(){if(this.value&&fields['sg.use_slots'])fields['sg.use_slots'].control.value=true;};
   if(product===2){var tip=r.key.indexOf('sg.slot_')===0?'sg.tip.slot':r.key==='sg.virtual_height'||r.key==='sg.follow_preset'?'sg.tip.grid':r.key==='sg.use_slots'?'sg.tip.overrides':r.key==='sg.extended_slots'?'sg.tip.extra':r.key==='sg.animate'||r.key.indexOf('sg.time_')===0||r.key==='sg.frame_offset'?'sg.tip.time':null;if(tip){c.helpTip=t(tip);label.helpTip=t(tip);}}
  }
  function localValue(key){var f=fields[key];return !f?0:f.spec.kind===0?(f.control.value?1:0):f.spec.kind===3?selectedIndex(f.control)+1:Number(f.control.text);}
  function updateControls(){
   var algorithm=localValue('param.algorithm'),mode=localValue('retro.mode');
   for(var ui=0;ui<rows.length;ui++){var f=fields[rows[ui].key],key=f.spec.key,visible=true,enabled=true;
    if(product===0){var i=f.spec.index;visible=i<=2||(algorithm<6&&i>=3&&i<=11)||(algorithm===6&&i>=12&&i<=14)||(algorithm>=7&&i>=19&&i<=26&&i!==21)||(algorithm===7&&(i>=15&&i<=18||i===21))||(algorithm===8&&i>=31&&i<=35)||(algorithm===9&&i>=36&&i<=48);if(key==='custom.count')visible=algorithm<=5&&localValue('param.palette')===11;if(key==='param.bayer_size')visible=algorithm===2||algorithm===3;if(key.indexOf('halftone.')===0&&i>=70)visible=algorithm===6&&(key==='halftone.mode'||localValue('halftone.mode')===2);if(key==='param.serpentine')visible=algorithm===1;if(key==='pa.reference')visible=visible&&localValue('pa.palette')===3;if(key==='pa.alpha_threshold')visible=visible&&localValue('pa.alpha')===2;if(key==='gba.feature')visible=visible&&localValue('gba.sampling')===3;if(key==='modern.feature')visible=visible&&localValue('modern.sampling')===3;if(key==='modern.cluster_area')visible=visible&&localValue('modern.merge')>0;}
    if(product===1){if(key==='retro.style'||key==='retro.signal_model'||key==='retro.vhs_strength')enabled=mode!==1&&!!localValue('retro.vhs');if(key==='retro.crt'||key==='retro.curvature')enabled=mode!==2;if(key==='retro.vhs')enabled=mode!==1;if(key==='retro.time_rate')enabled=!!localValue('retro.animate');}
    if(product===2){if(key.indexOf('sg.slot_')===0)enabled=!!localValue('sg.use_slots')&&(key.charAt(8)<'i'||!!localValue('sg.extended_slots'));if(key==='sg.time_rate')enabled=!!localValue('sg.animate');}
    f.group.visible=visible;f.group.maximumSize=visible?[10000,10000]:[0,0];f.control.enabled=enabled;f.label.enabled=enabled;
   }
   if(product===0){page2.enabled=algorithm===6;page3.enabled=algorithm===7;page4.enabled=algorithm===8;page5.enabled=algorithm===9;shared.enabled=algorithm>=7;if(tabs.selection&&tabs.selection.enabled===false)tabs.selection=page1;}
   if(preset)preset.enabled=mode!==1&&!!localValue('retro.vhs');
   if(tab.layout.layout)tab.layout.layout(true);
  }
  // Preserve the input-grid and I–P dependency handlers above.
  for(var hi=0;hi<rows.length;hi++)(function(f){var event=f.spec.kind===0?'onClick':'onChange',previous=f.control[event];f.control[event]=function(){if(previous)previous.call(this);updateControls();say('panel.pending');};})(fields[rows[hi].key]);
  if(product===0){var libraryHelp=tab.add('statictext',undefined,t('library.help'),{multiline:true});libraryHelp.preferredSize=[410,48];}
  var actions=tab.add('group'),bind=actions.add('button',undefined,t('panel.bind')),read=actions.add('button',undefined,t('panel.refresh'));
  var apply=tab.add('button',undefined,t('panel.apply'));
  var preset=product===1?tab.add('button',undefined,t('retro.preset')):null;
  var status=tab.add('statictext',undefined,'',{multiline:true});status.preferredSize=[410,46];status.name='status';
  function effect(create){var comp=app.project&&app.project.activeItem;if(!(comp instanceof CompItem)||comp.selectedLayers.length!==1)throw 'panel.no_layer';
   var layer=comp.selectedLayers[0],parade=layer.property('ADBE Effect Parade'),found=null,count=0,chosen=null,selected=layer.selectedProperties||[];
   for(var s=0;s<selected.length;s++){var p=selected[s];while(p){if(p.matchName===schema.match){if(chosen&&chosen.propertyIndex!==p.propertyIndex)throw 'panel.ambiguous';chosen=p;break;}p=p.parentProperty;}}
   for(var i=1;i<=parade.numProperties;i++){var fx=parade.property(i);if(fx.matchName===schema.match){found=fx;count++;}}
   if(chosen)found=chosen;else if(count>1)throw 'panel.ambiguous';
   if(!found&&create)found=parade.addProperty(schema.match);if(!found)throw 'fylm.no_effect';return{fx:found,comp:comp};}
  function say(e){status.text=t(typeof e==='string'?e:'panel.failure');}
  function refresh(){var target=effect(false);for(var i=0;i<rows.length;i++){var r=rows[i],c=fields[r.key].control,v=target.fx.property(r.index).value;if(r.kind===0)c.value=!!v;else if(r.kind===3)c.selection=v-1;else c.text=String(v);}if(product===2&&schema.library){var meta=null;for(var mi=0;mi<schema.library.length;mi++)if(schema.library[mi].menu===target.fx.property(2).value)meta=schema.library[mi];if(meta&&meta.menu>=4&&fields['sg.follow_preset'].control.value)fields['sg.virtual_height'].control.text=String(meta.virtual_height);if(meta&&meta.slots)for(var si=0;si<16;si++){var key='sg.slot_'+String.fromCharCode(97+si),field=fields[key];if(!field)continue;var name=meta.slots[si],translated=name?t('sg.uniform.'+name):'';if(translated==='sg.uniform.'+name)translated=name;field.label.text=meta.menu===3?(target.fx.property(field.spec.index).name||t(field.spec.key)):String.fromCharCode(65+si)+': '+(translated||t('sg.unused'))+' (%)';}}updateControls();say('panel.bound');}
  bind.onClick=function(){var undo=false;try{app.beginUndoGroup(schema.name);undo=true;effect(true);refresh();}catch(e){say(e);}finally{if(undo)app.endUndoGroup();}};
  read.onClick=function(){try{refresh();}catch(e){say(e);}};
  apply.onClick=function(){var undo=false;try{var target=effect(false),values=[];
   var activeRows=[];for(var i=0;i<rows.length;i++){var r=rows[i],c=fields[r.key].control,v;if(product===0&&!fields[r.key].group.visible)continue;activeRows.push(r);
    if(r.kind===0)v=c.value?1:0;else if(r.kind===3)v=selectedIndex(c)+1;else{if(!String(c.text).replace(/\s/g,''))throw 'panel.invalid';v=Number(c.text);}
    if(!isFinite(v)||v<r.low||v>r.high||(r.kind!==2&&v!==Math.round(v)))throw 'panel.invalid';if(target.fx.property(r.index).expressionEnabled)throw 'panel.failure';values.push(v);}
   app.beginUndoGroup(schema.name);undo=true;for(var j=0;j<activeRows.length;j++){var p=target.fx.property(activeRows[j].index);if(p.numKeys)p.setValueAtTime(target.comp.time,values[j]);else p.setValue(values[j]);}say('panel.applied');
  }catch(e){say(e);}finally{if(undo)app.endUndoGroup();}};
  if(preset)preset.onClick=function(){var style=fields['retro.style'];if(style&&schema.recipes&&selectedIndex(style.control)>0){var recipe=schema.recipes[selectedIndex(style.control)-1];for(var key in recipe.settings){var field=fields[key];if(!field)continue;var value=recipe.settings[key];if(field.spec.kind===0)field.control.value=!!value;else if(field.spec.kind===3)field.control.selection=value-1;else field.control.text=String(value);}updateControls();say('panel.preset_loaded');return;}for(var i=0;i<rows.length;i++){var r=rows[i],c=fields[r.key].control;if(r.key==='retro.crt'||r.key==='retro.curvature'||r.key==='retro.mode')continue;if(r.kind===0)c.value=!!r.value;else if(r.kind===3)c.selection=r.value-1;else c.text=String(r.value);}updateControls();say('panel.preset_loaded');};
  var browser=null;
  if(product===2&&schema.library){
   var libraryTab=tabs.add('tab',undefined,t('sg.browser'));libraryTab.orientation='column';libraryTab.alignChildren=['fill','top'];
   var search=libraryTab.add('edittext',undefined,''),category=libraryTab.add('dropdownlist',undefined,[]),only=libraryTab.add('checkbox',undefined,t('sg.favorites_only'));
   var categories=['All','Art','CRT','LCD','Signal','Scaling','Colour'];for(var ci=0;ci<categories.length;ci++)category.add('item',t('sg.category.'+categories[ci]));category.selection=0;
   var list=libraryTab.add('dropdownlist',undefined,[]),note=libraryTab.add('statictext',undefined,'',{multiline:true});note.preferredSize=[390,110];
   var preview=libraryTab.add('image');preview.preferredSize=[192,128];
   var buttons=libraryTab.add('group'),choose=buttons.add('button',undefined,t('sg.choose')),favorite=buttons.add('button',undefined,t('sg.favorite'));
   var hint=libraryTab.add('statictext',undefined,t('sg.library_hint'),{multiline:true});hint.preferredSize=[390,55];
   var fav={},shown=[];try{if(app.settings&&app.settings.haveSetting('FYLM ShaderGlass','favourites.v1')){var ids=app.settings.getSetting('FYLM ShaderGlass','favourites.v1').split('|');for(var fi=0;fi<ids.length;fi++)fav[ids[fi]]=true;}}catch(e){}
   function displayName(e){return locale()==='zh-Hant'?(e.zh_hant||e.zh):locale().indexOf('zh')===0?e.zh:locale()==='ja'?e.ja:e.name;}
   function selectedPreset(){return shown[selectedIndex(list)];}
   function describeSelection(){var e=selectedPreset();note.text=e?(displayName(e)+'\n'+(locale()==='zh-Hant'?(e.use_hant||e.use):locale().indexOf('zh')===0?e.use:e.name)+'\n'+(e.slots.length?'':t('sg.no_parameters')+'\n')+(e.category==='Scaling'&&!e.pixel_grid_profile?t('sg.native_notice')+'\n':'')+(e.performance.indexOf('heavy')>=0?t('sg.performance_heavy')+'\n':'')+t('sg.source_algorithm')+e.name):'';choose.enabled=!!e;favorite.enabled=!!e;try{var thumbnail=e?new File(home.parent.parent.fsName+'/Plug-ins/FYLM Plugins/Assets/ShaderGlass/Library/thumbnails/'+e.slug+'.png'):null;preview.image=thumbnail&&thumbnail.exists&&typeof ScriptUI!=='undefined'?ScriptUI.newImage(thumbnail):null;}catch(error){preview.image=null;}}
   function populate(){var old=selectedPreset(),keep=old?old.id:'',query=String(search.text||'').toLowerCase(),cat=categories[selectedIndex(category)];shown=[];list.removeAll();
    var ordered=[];if(cat==='Scaling'){for(var oi=0;oi<schema.library.length;oi++)if(schema.library[oi].pixel_grid_profile)ordered.push(schema.library[oi]);for(var oi=0;oi<schema.library.length;oi++)if(!schema.library[oi].pixel_grid_profile)ordered.push(schema.library[oi]);}else ordered=schema.library;
    for(var li=0;li<ordered.length;li++){var e=ordered[li];if(e.menu===3||(cat!=='All'&&e.category!==cat)||(only.value&&!fav[e.id]))continue;var hay=(e.id+' '+e.name+' '+e.zh+' '+e.zh_hant+' '+e.ja+' '+e.category).toLowerCase();if(query&&hay.indexOf(query)<0)continue;shown.push(e);list.add('item',(fav[e.id]?'* ':'')+displayName(e));}
    var selected=0;for(var si=0;si<shown.length;si++)if(shown[si].id===keep)selected=si;list.selection=shown.length?selected:null;describeSelection();}
   search.onChanging=category.onChange=only.onClick=populate;list.onChange=describeSelection;
   favorite.onClick=function(){var e=selectedPreset();if(!e)return;fav[e.id]=!fav[e.id];var saved=[];for(var id in fav)if(fav[id])saved.push(id);try{if(app.settings)app.settings.saveSetting('FYLM ShaderGlass','favourites.v1',saved.join('|'));}catch(error){say('panel.failure');}populate();};
   choose.onClick=function(){var undo=false;try{var e=selectedPreset();if(!e)return;var target=effect(false),indices=[2,4,16,30],values=[e.menu,0,1,0],previous=[];
    for(var ai=0;ai<indices.length;ai++){var p=target.fx.property(indices[ai]);if(p.numKeys||p.expressionEnabled||p.expression)throw 'sg.error.preset_guard';previous.push(p.value);}
    app.beginUndoGroup(schema.name);undo=true;try{for(var ai=0;ai<indices.length;ai++)target.fx.property(indices[ai]).setValue(values[ai]);}catch(error){for(var ai=0;ai<indices.length;ai++)target.fx.property(indices[ai]).setValue(previous[ai]);throw error;}refresh();say('panel.applied');
   }catch(error){say(error);}finally{if(undo)app.endUndoGroup();}};
   browser={search:search,category:category,only:only,list:list,note:note,choose:choose,favorite:favorite};populate();
  }
  updateControls();
  return{schema:schema,fields:fields,bind:bind,read:read,apply:apply,preset:preset,status:status,browser:browser};
 }
 for(var i=0;i<schemas.length;i++)views.push(build(schemas[i],i));products.selection=products.children[0];
 w.onResizing=w.onResize=function(){this.layout.resize();};w.fylmViews=views;
 if(w instanceof Window){w.center();w.show();}return w;
}(this));
