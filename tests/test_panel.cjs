const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert'),cp=require('child_process');
const root=path.resolve(__dirname,'..'),build=path.resolve(process.argv[2]||path.join(root,'build'));
const pipl=fs.readFileSync(path.join(root,'plugin/ae/DitheringStudioPiPL.r'),'utf8');
assert.ok((parseInt(pipl.match(/AE_Effect_Global_OutFlags\s*\{(0x[0-9a-f]+)\}/i)[1],16)&4)!==0,'PiPL must also declare time-dependent rendering');
const locales=['en','zh-Hans','zh-Hant','ja'];
const catalog=Object.fromEntries(locales.map(l=>[l,JSON.parse(fs.readFileSync(path.join(root,'i18n',l+'.json'),'utf8').replace(/^\uFEFF/,''))]));
for(const l of locales)assert.deepStrictEqual(Object.keys(catalog[l]).sort(),Object.keys(catalog.en).sort(),l+' keys');
function Control(type,text){this.type=type;this.text=typeof text==='string'?text:'';this.children=[];this.items=[];this.preferredSize={};this.layout={layout(){},resize(){}};this.enabled=true;this._selection=null;}
// AE may return stale -1 ListItem.index after translated menus are rebuilt.
Control.prototype.add=function(type,a,value){const c=new Control(type,value);this.children.push(c);if(type==='dropdownlist')for(const s of value||[])c.add('item',s);if(type==='item'){c.text=a;c.index=-1;this.items.push(c);}return c;};
Object.defineProperty(Control.prototype,'selection',{get(){return this._selection;},set(v){this._selection=typeof v==='number'?this.items[v]||null:v;}});
Control.prototype.removeAll=function(){this.items=[];this.children=[];this._selection=null;};Control.prototype.show=Control.prototype.center=function(){};
function Window(){Control.call(this,'window','');}Window.prototype=Object.create(Control.prototype);Window.prototype.constructor=Window;
function Panel(){};function File(f){this.fsName=f;this.parent={fsName:path.dirname(f)};}
let undo=0,active=null;
const context={Window,Panel,File,isFinite,PropertyValueType:{OneD:1},CompItem:function(){},alert(){},app:{isoLanguage:'en_US',project:{get activeItem(){return active;}},beginUndoGroup(){undo++;},endUndoGroup(){undo--;}}};
context.$={fileName:path.join(build,'panel/Dithering Studio.jsx'),evalFile(f){return vm.runInNewContext(fs.readFileSync(f.fsName,'utf8'));}};
function find(node,name){if(node.name===name)return node;for(const c of node.children){const r=find(c,name);if(r)return r;}return null;}
const source=fs.readFileSync(path.join(root,'plugin/ae/panel/Dithering Studio.jsx'),'utf8').replace(/^#target.*$/m,'');
function loadPanel(isoLanguage){context.app.isoLanguage=isoLanguage;return vm.runInNewContext(source,context);}
let panel=loadPanel('en_US');
assert.strictEqual(find(panel,'language'),null);
const hostTags=['en_US','zh_CN','zh_TW','ja_JP'];
for(let l=0;l<4;l++){
 panel=loadPanel(hostTags[l]);assert.strictEqual(find(panel,'language'),null);
 assert.strictEqual(find(panel,'1').items[3].text,catalog[locales[l]]['algorithm.15']);
 find(panel,'apply').onClick();assert.strictEqual(find(panel,'status').text,catalog[locales[l]]['panel.no_layer']);
}
panel=loadPanel('zh_CN');
const params=Array.from({length:41},(_,i)=>({value:i===8?1:i===3?128:i===4?512:i===34?2:i===31?40:i===33?60:1,propertyValueType:1,numKeys:0,setValue(v){this.value=v;}}));
const fx={matchName:'OSLO_DitheringStudio',property(i){return params[i];}};
active=new context.CompItem();active.time=0;active.selectedLayers=[{property(){return {numProperties:1,property(){return fx;}};}}];
find(panel,'refresh').onClick();find(panel,'3').text='-1';find(panel,'apply').onClick();assert.strictEqual(find(panel,'status').text,catalog['zh-Hans']['panel.invalid']);assert.strictEqual(params[3].value,128);
find(panel,'30').value=false;find(panel,'17').value=false;find(panel,'19').text='5';find(panel,'20').text='0';find(panel,'21').text='0';
find(panel,'retroPreset').onClick();assert.strictEqual(find(panel,'31').text,'40');assert.strictEqual(params[31].value,40);
assert.strictEqual(find(panel,'30').value,false);assert.strictEqual(find(panel,'17').value,false);
assert.strictEqual(find(panel,'19').text,'288');assert.strictEqual(find(panel,'20').text,'25');assert.strictEqual(find(panel,'21').text,'2');
context.app.isoLanguage='zh_TW';find(panel,'refresh').onClick();find(panel,'3').text='100';find(panel,'apply').onClick();assert.strictEqual(params[3].value,100);assert.strictEqual(find(panel,'30').value,true);assert.strictEqual(params[11].value,1);assert.strictEqual(undo,0);
const effect2={matchName:'OSLO_DitheringStudio',propertyIndex:2,property(i){return params[i];}};fx.propertyIndex=1;
const layer=active.selectedLayers[0];layer.property=()=>({numProperties:2,property(i){return i===1?fx:effect2;}});
find(panel,'apply').onClick();assert.strictEqual(find(panel,'status').text,catalog['zh-Hant']['panel.ambiguous']);
layer.selectedProperties=[effect2];find(panel,'apply').onClick();assert.strictEqual(find(panel,'status').text,catalog['zh-Hant']['panel.applied']);
// CLI localizes errors, while locale must never change rendered bytes.
let first;
for(const l of locales){let exe=process.argv[3]||path.join(build,'Release/dither_cli.exe');if(!fs.existsSync(exe))exe=path.join(build,'dither_cli');
 let err=cp.spawnSync(exe,['--lang',l,'missing.rgba',path.join(build,'unused.rgba'),'1','1'],{encoding:'utf8'});assert.strictEqual(err.status,1);assert.strictEqual(err.stderr.trim(),catalog[l]['error.input_size']);
 let out=path.join(build,'locale-'+l+'.rgba');cp.execFileSync(exe,['--lang',l,path.join(root,'tests/golden/0361.in.rgba'),out,'64','64','1','none','77','0','1','0']);let bytes=fs.readFileSync(out);if(first)assert.deepStrictEqual(bytes,first);first=bytes;
}
console.log('Automatic AE language following, independent dither/retro controls, source motion preset, validation and CLI locale invariance passed.');

