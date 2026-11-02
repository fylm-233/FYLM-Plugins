const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert');
const root=path.resolve(__dirname,'..'),build=path.resolve(process.argv[2]);
function Control(type,text){this.type=type;this.text=typeof text==='string'?text:'';this.children=[];this.items=[];this.preferredSize={};this.layout={resize(){}};}
Control.prototype.add=function(type,a,value){const c=new Control(type,value);c.parent=this;this.children.push(c);if(type==='dropdownlist')for(const s of value||[])c.add('item',s);if(type==='item'){c.text=a;c.index=-1;this.items.push(c);}return c;};
Object.defineProperty(Control.prototype,'selection',{get(){return this._selection;},set(v){this._selection=typeof v==='number'?this.items[v]||null:v;}});
Control.prototype.removeAll=function(){this.items=[];this.children=[];this._selection=null;};
Control.prototype.show=Control.prototype.center=function(){};
function Window(){Control.call(this,'window','');}Window.prototype=Object.create(Control.prototype);Window.prototype.constructor=Window;
function Panel(){}function File(f){this.fsName=f;this.parent={fsName:path.dirname(f)};}
let undo=0;
for(const language of ['en_US','zh_CN','zh_TW','ja_JP']){
 const active={selectedLayers:[],selectedProperties:[],time:0};function CompItem(){}Object.setPrototypeOf(active,CompItem.prototype);
 const effects=[];const parade={get numProperties(){return effects.length;},property(i){return effects[i-1];},addProperty(match){const schema=panel.fylmViews.find(v=>v.schema.match===match).schema;
  const props=[null];for(const r of schema.rows)props[r.index]={value:r.value,numKeys:0,setValue(v){this.value=v;},setValueAtTime(t,v){this.value=v;}};
  const fx={matchName:match,propertyIndex:effects.length+1,property(i){return props[i];}};effects.push(fx);return fx;}};
 active.selectedLayers=[{selectedProperties:[],property(){return parade;}}];
 const settings={};const context={Window,Panel,File,CompItem,alert(){},isFinite,app:{settings:{haveSetting(a,b){return settings[a+b]!==undefined;},getSetting(a,b){return settings[a+b];},saveSetting(a,b,v){settings[a+b]=v;}},isoLanguage:language,project:{activeItem:active},beginUndoGroup(){undo++;},endUndoGroup(){undo--;}}};
 context.$={fileName:path.join(build,'panel/FYLM Plugins.jsx'),evalFile(f){return vm.runInNewContext(fs.readFileSync(f.fsName,'utf8'));}};
 const source=fs.readFileSync(path.join(root,'plugin/ae/panel/FYLM Plugins.jsx'),'utf8').replace(/^#target.*$/m,'');
 const panel=vm.runInNewContext(source,context);assert.strictEqual(panel.fylmViews.length,3);
 const [dither,crt]=panel.fylmViews;assert.strictEqual(dither.schema.name,'FYLM Pixel Art');assert.strictEqual(dither.schema.match,'FYLM_DitheringStudio');assert.strictEqual(crt.schema.name,'FYLM CRT VHS');
 assert.ok(!dither.fields['retro.vhs']&&!crt.fields['param.algorithm']);
 assert.ok(!dither.fields['pa.spec']&&!dither.fields['pa.indexed_colors']&&!dither.fields['pa.dac'],'retired PC98 controls visible');
 for(const view of panel.fylmViews){view.bind.onClick();assert.strictEqual(effects.at(-1).matchName,view.schema.match);view.apply.onClick();assert.strictEqual(undo,0);}
 dither.fields['param.algorithm'].control.selection=5;dither.fields['param.algorithm'].control.onChange();dither.apply.onClick();assert.strictEqual(effects[0].property(2).value,6);
 assert.strictEqual(dither.fields['pa.block'].group.visible,false);assert.strictEqual(dither.fields['halftone.pitch'].group.visible,true);
 // Hidden mode values must neither fail validation nor overwrite the other mode.
 dither.fields['gba.block'].control.text='invalid';dither.apply.onClick();assert.strictEqual(effects[0].property(31).value,6);dither.fields['gba.block'].control.text='6';
 const crtPage=crt.fields['retro.mode'].group.parent;assert.strictEqual(crtPage.children.indexOf(crt.fields['retro.mode'].group),1,'mode must follow enable');
 assert.strictEqual(crt.fields['retro.mode'].spec.index,25,'layout must preserve serialized identity');
 dither.fields['halftone.pitch'].control.text='0';dither.apply.onClick();assert.strictEqual(effects[0].property(13).value,2.5);
 crt.fields['retro.noise'].control.text='99';crt.fields['retro.crt'].control.value=false;crt.preset.onClick();crt.apply.onClick();
 assert.strictEqual(effects[1].property(10).value,10);assert.strictEqual(effects[1].property(3).value,0);assert.strictEqual(effects[0].property(2).value,6);
 const sg=panel.fylmViews[2],browser=sg.browser;assert.ok(browser);assert.strictEqual(sg.fields['sg.preset'].control.items.length,39);assert.strictEqual(browser.list.items.length,38);
 assert.strictEqual(sg.fields['sg.slot_i'].control.enabled,false,'inactive extension must look inactive');
 browser.search.text='SG01';browser.search.onChanging();assert.strictEqual(browser.list.items.length,1);browser.choose.onClick();assert.strictEqual(effects[2].property(2).value,4);assert.strictEqual(effects[2].property(4).value,0);assert.strictEqual(effects[2].property(16).value,1);assert.strictEqual(effects[0].property(2).value,6);
 browser.favorite.onClick();browser.only.value=true;browser.only.onClick();assert.strictEqual(browser.list.items.length,1);assert.ok(settings['FYLM ShaderGlassfavourites.v1']);browser.favorite.onClick();assert.strictEqual(browser.list.items.length,0);
 browser.only.value=false;browser.search.text='';browser.category.selection=4;browser.category.onChange();assert.strictEqual(browser.list.items.length,8);
 browser.category.selection=5;browser.category.onChange();assert.strictEqual(browser.list.items.length,8);browser.choose.onClick();assert.strictEqual(effects[2].property(2).value,35);assert.ok(sg.fields['sg.virtual_height'].control.helpTip);sg.fields['sg.virtual_height'].control.text='360';sg.fields['sg.virtual_height'].control.onChange();sg.apply.onClick();assert.strictEqual(effects[2].property(3).value,360);assert.strictEqual(effects[2].property(16).value,0);
 sg.fields['sg.preset'].control.selection=30;sg.fields['sg.use_slots'].control.value=false;
 sg.fields['sg.extended_slots'].control.value=true;sg.fields['sg.extended_slots'].control.onClick();
 assert.strictEqual(sg.fields['sg.use_slots'].control.value,true,'I-P click must enable custom parameters');
 assert.strictEqual(sg.fields['sg.slot_i'].control.enabled,true,'single I-P click must enable editing');
 sg.apply.onClick();assert.strictEqual(effects[2].property(4).value,1);assert.strictEqual(effects[2].property(30).value,1);
 sg.fields['sg.extended_slots'].control.value=false;sg.fields['sg.extended_slots'].control.onClick();sg.apply.onClick();
 assert.strictEqual(effects[2].property(4).value,1,'I-P off must preserve A-H');assert.strictEqual(effects[2].property(30).value,0);
 effects[2].property(4).numKeys=1;const before=effects[2].property(2).value;browser.choose.onClick();assert.strictEqual(effects[2].property(2).value,before);effects[2].property(4).numKeys=0;
 assert.strictEqual(undo,0);
}
console.log('FYLM panel: four host languages, isolated effect identities, sixth halftone choice, preset isolation and atomic validation passed.');
