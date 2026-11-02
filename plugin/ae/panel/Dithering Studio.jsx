#target aftereffects
// Native host UI only. Image processing always runs inside the C++ effect.
(function (parent) {
    var home = new File($.fileName).parent;
    var catalog;
    try { catalog = $.evalFile(new File(home.fsName + '/DitheringStudio-i18n.jsxinc')); }
    catch (e) { alert('Language resources unavailable / 语言资源不可用 / 語言資源無法使用 / 言語リソースを利用できません'); return null; }
    var codes = ['en', 'zh-Hans', 'zh-Hant', 'ja'], locale = 0, busy = false, statusKey = 'help.text';
    function localeFromTag(tag) {
        var value = String(tag || '').toLowerCase().replace(/_/g, '-');
        if (value.indexOf('zh') === 0) {
            if (value.indexOf('hant') >= 0 || value.indexOf('-tw') >= 0 ||
                value.indexOf('-hk') >= 0 || value.indexOf('-mo') >= 0) return 2;
            return 1;
        }
        return value.indexOf('ja') === 0 ? 3 : 0;
    }
    function detectHostLocale() {
        // app.isoLanguage is AE's active displayed-language setting, not the OS locale.
        try { if (app.isoLanguage) return localeFromTag(app.isoLanguage); } catch (ignore) {}
        try { if (app.locale) return localeFromTag(app.locale); } catch (ignore2) {}
        return 0;
    }
    locale = detectHostLocale();
    function t(key) { return (catalog[codes[locale]] || {})[key] || catalog.en[key] || key; }
    var w = parent instanceof Panel ? parent : new Window('palette', 'Dithering Studio', undefined, {resizeable:true});
    w.orientation = 'column'; w.alignChildren = ['fill','top'];
    var tabs=w.add('tabbedpanel'),ditherTab=tabs.add('tab',undefined,''),retroTab=tabs.add('tab',undefined,'CRT / VHS');
    tabs.preferredSize=[380,720];tabs.selection=ditherTab;
    ditherTab.orientation=retroTab.orientation='column';ditherTab.alignChildren=retroTab.alignChildren=['fill','top'];
    var fieldsParent=ditherTab,rows=[], fields={};
    // Rebuilt ScriptUI lists can expose a stale ListItem.index in AE. Resolve the actual item.
    function selectionIndex(control){
        // Read items before selection so AE finishes refreshing a rebuilt popup.
        var items=control.items,count=items?items.length:0,item=control.selection;
        if(typeof item==='number')return isFinite(item)&&item===Math.round(item)&&item>=0&&item<count?item:-1;
        if(!item||!count)return -1;
        for(var i=0;i<count;i++)if(items[i]===item)return i;
        if(item.text!==undefined)for(var j=0;j<count;j++)if(items[j].text===item.text)return j;
        if(typeof item.index==='number'&&item.index>=0&&item.index<count&&(item.text===undefined||items[item.index].text===item.text))return item.index;
        return -1;
    }
    function row(index,key,type,choices,value,low,high) {
        var group=fieldsParent.add('group');group.alignChildren=['left','center'];
        var label=group.add('statictext',undefined,'');label.preferredSize.width=145;
        var control=group.add(type,undefined,type==='dropdownlist'?[]:String(value));
        control.name=String(index);control.preferredSize.width=200;
        if(type==='dropdownlist')control.selection=0;
        if(type==='checkbox')control.value=!!value;
        var field={index:index,key:key,type:type,choices:choices,control:control,label:label,low:low,high:high};
        rows.push(field);fields[index]=field;return control;
    }
    row(30,'dither.enabled','checkbox',null,true);
    var algorithm=row(1,'param.algorithm','dropdownlist',['algorithm.1','algorithm.2','algorithm.8','algorithm.15','algorithm.17']);
    row(2,'param.palette','dropdownlist',['palette.none','palette.bw','palette.gb','palette.pico8','palette.db16','palette.db32','palette.cga16','palette.ega16']);
    row(3,'param.threshold','edittext',null,128,0,255);row(4,'param.resolution','edittext',null,512,0,8192);
    row(5,'param.invert','checkbox',null,false);var serpentine=row(6,'param.serpentine','checkbox',null,true);
    row(7,'param.contrast','edittext',null,0,-100,100);row(8,'param.midtones','edittext',null,1,0.5,2);row(9,'param.highlights','edittext',null,0,0,100);
    row(10,'param.alpha','dropdownlist',['alpha.opaque','alpha.preserve']);
    fieldsParent=retroTab;
    row(14,'retro.enabled','checkbox',null,false);row(15,'retro.order','dropdownlist',['retro.order.dither_first','retro.order.retro_first']);
    row(16,'retro.vhs','checkbox',null,true);row(17,'retro.crt','checkbox',null,true);
    row(18,'retro.curvature','edittext',null,0,0,100);row(19,'retro.scan_count','edittext',null,288,0,8192);
    row(20,'retro.scan_opacity','edittext',null,25,0,100);row(21,'retro.chroma','edittext',null,2,-20,20);
    row(22,'retro.jitter','edittext',null,.2,0,10);row(23,'retro.jitter_frequency','edittext',null,4,0,100);
    row(24,'retro.noise','edittext',null,10,0,100);row(25,'retro.noise_color','edittext',null,30,0,100);
    row(26,'retro.time_rate','edittext',null,1,-4,4);row(27,'retro.time_offset','edittext',null,0,-3600,3600);
    row(28,'retro.animate','checkbox',null,true);row(29,'retro.clamp','checkbox',null,true);
    row(31,'retro.tracking_height','edittext',null,40,0,100);row(32,'retro.tracking_distortion','edittext',null,1,0,100);
    row(33,'retro.tracking_noise','edittext',null,60,0,100);row(34,'retro.streak_frequency','edittext',null,2,0,100);
    row(35,'retro.streak_intensity','edittext',null,10,0,100);row(36,'retro.streak_height','edittext',null,2,0.1,100);
    row(37,'retro.dropout_frequency','edittext',null,2,0,100);row(38,'retro.dropout_intensity','edittext',null,20,0,100);
    row(39,'retro.dropout_height','edittext',null,20,0.1,100);
    var presetGroup=retroTab.add('group'),dynamicPreset=presetGroup.add('button',undefined,'');dynamicPreset.name='retroPreset';
    var buttons=w.add('group'),bind=buttons.add('button',undefined,''),read=buttons.add('button',undefined,''),apply=buttons.add('button',undefined,''),help=buttons.add('button',undefined,'');
    bind.name='bind';read.name='refresh';apply.name='apply';help.name='help';
    var status=w.add('statictext',undefined,'',{multiline:true});status.name='status';status.preferredSize=[370,95];
    function renderText(){
        busy=true;locale=detectHostLocale();w.text=t('panel.title');ditherTab.text=t('panel.dither_tab');retroTab.text='CRT / VHS';
        for(var i=0;i<rows.length;i++){var r=rows[i],c=r.control;r.label.text=t(r.key);c.helpTip=t(r.index===30?'hint.dither':(r.index>=14?'hint.retro':'hint.range'));
            if(r.choices){var selected=Math.max(0,selectionIndex(c));c.removeAll();for(var j=0;j<r.choices.length;j++)c.add('item',t(r.choices[j]));c.selection=Math.min(selected,r.choices.length-1);}}
        dynamicPreset.text=t('retro.preset');dynamicPreset.helpTip=t('hint.retro');
        bind.text=t('panel.bind');read.text=t('panel.refresh');apply.text=t('panel.apply');help.text=t('param.help');
        status.text=t(statusKey);serpentine.enabled=selectionIndex(algorithm)===0;
        busy=false;w.layout.layout(true);
    }
    function say(key){statusKey=key;status.text=t(key);}
    function selected(create){
        var comp=app.project&&app.project.activeItem;
        if(!(comp instanceof CompItem)||comp.selectedLayers.length!==1)throw 'panel.no_layer';
        var layer=comp.selectedLayers[0],effects=layer.property('ADBE Effect Parade'),fx=null,matches=0;
        var properties=layer.selectedProperties||[],chosen=0;
        for(var s=0;s<properties.length;s++){var p=properties[s];while(p){
            if(p.matchName==='OSLO_DitheringStudio'){if(chosen&&chosen!==p.propertyIndex)throw 'panel.ambiguous';chosen=p.propertyIndex;break;}p=p.parentProperty;}}
        for(var i=1;i<=effects.numProperties;i++)if(effects.property(i).matchName==='OSLO_DitheringStudio'){fx=effects.property(i);matches++;}
        if(chosen)fx=effects.property(chosen);else if(matches>1)throw 'panel.ambiguous';
        if(!fx&&create)fx=effects.addProperty('OSLO_DitheringStudio');
        if(!fx)throw 'panel.no_effect';
        if(!fx.property(39))throw 'panel.unavailable';
        return {fx:fx,comp:comp};
    }
    function fail(e){
        var diagnostic=$.getenv?$.getenv('DITHER_PANEL_DIAGNOSTICS'):'';
        if(diagnostic)try{var log=new File(diagnostic);if(log.open('a')){log.writeln(String(e));log.close();}}catch(ignore){}
        say(typeof e==='string'&&catalog.en[e]?e:'panel.failure');
    }
    function readValues(fx){
        for(var i=0;i<rows.length;i++){var r=rows[i],v=fx.property(r.index).value,c=r.control;
            if(r.choices)c.selection=Math.max(0,Math.min(r.choices.length-1,Math.round(v)-1));else if(r.type==='checkbox')c.value=!!v;else c.text=String(v);}
        renderText();
    }
    algorithm.onChange=function(){if(!busy)serpentine.enabled=selectionIndex(algorithm)===0;};
    function setRow(index,value){var r=fields[index];if(!r)return;var c=r.control;if(r.type==='checkbox')c.value=!!value;else c.text=String(value);}
    dynamicPreset.onClick=function(){
        // Source VHS shader defaults at commit 6ba87f9. These controls are animated from composition time.
        // Restore the full exposed VHS baseline; keep Dither, CRT and pipeline choices independent.
        setRow(19,288);setRow(20,25);setRow(21,2);setRow(29,true);
        setRow(14,true);setRow(16,true);setRow(22,.2);setRow(23,4);setRow(24,10);setRow(25,30);
        setRow(26,1);setRow(27,0);setRow(28,true);setRow(31,40);setRow(32,1);setRow(33,60);
        setRow(34,2);setRow(35,10);setRow(36,2);setRow(37,2);setRow(38,20);setRow(39,20);
        say('panel.preset_loaded');
    };
    bind.onClick=function(){var opened=false;try{app.beginUndoGroup(t('panel.bind'));opened=true;var target=selected(true);readValues(target.fx);say('panel.bound');}catch(e){fail(e);}finally{if(opened)app.endUndoGroup();}};
    read.onClick=function(){try{readValues(selected(false).fx);say('panel.bound');}catch(e){fail(e);}};
    apply.onClick=function(){var opened=false;try{
        var target=selected(false),values=[];
        // Validate every value before changing the host project.
        for(var i=0;i<rows.length;i++){var r=rows[i],c=r.control,choiceIndex=-1,v;
            if(r.choices){choiceIndex=selectionIndex(c);v=choiceIndex+1;}
            else if(r.type==='checkbox')v=c.value?1:0;
            else v=Number(c.text);
            if(!isFinite(v)||(r.choices&&(v<1||v>r.choices.length))||(r.low!==undefined&&(String(c.text).replace(/\s/g,'')===''||v<r.low||v>r.high))||((r.index===3||r.index===4)&&v!==Math.round(v))){
                var diagnostic=$.getenv?$.getenv('DITHER_PANEL_DIAGNOSTICS'):'';
                if(diagnostic)try{var log=new File(diagnostic);if(log.open('a')){log.writeln('panel.invalid index='+r.index+' type='+r.type+' raw='+String(c.text)+' value='+String(v)+' selection='+choiceIndex);log.close();}}catch(ignore){}
                throw 'panel.invalid';
            }
            if(target.fx.property(r.index).expressionEnabled)throw 'panel.failure';values.push(v);}
        app.beginUndoGroup(t('panel.apply'));opened=true;
        for(var j=0;j<rows.length;j++){var p=target.fx.property(rows[j].index);if(p.numKeys>0)p.setValueAtTime(target.comp.time,values[j]);else p.setValue(values[j]);}
        say('panel.applied');
    }catch(e){fail(e);}finally{if(opened)app.endUndoGroup();}};
    help.onClick=function(){alert(t('about.text')+'\n\n'+t('help.text'),t('param.help'));};
    w.onResizing=w.onResize=function(){this.layout.resize();};
    renderText();if(w instanceof Window){w.center();w.show();}return w;
}(this));

