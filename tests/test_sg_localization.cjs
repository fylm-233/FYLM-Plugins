const fs=require('fs'),path=require('path'),assert=require('assert');
const root=path.resolve(__dirname,'..'),catalog=JSON.parse(fs.readFileSync(path.join(root,'assets/ShaderGlass/Library/catalog-v1.json')));
for(const language of ['zh-Hans','zh-Hant']){
 const labels=JSON.parse(fs.readFileSync(path.join(root,'i18n',language+'.json'),'utf8').replace(/^\uFEFF/,''));
 assert.strictEqual(labels['sg.preset.items'].split('|').length,39);
 for(const item of labels['sg.preset.items'].split('|'))assert.ok(/[\u3400-\u9fff]/.test(item),'Untranslated preset: '+item);
 for(const e of catalog.entries)for(const name of e.display_slots||e.slots||[])assert.ok(/[\u3400-\u9fff]/.test(labels['sg.uniform.'+name]||''),'Untranslated visible uniform: '+name);
 assert.ok(/[\u3400-\u9fff]/.test(labels['sg.tip.grid']));
}
assert.strictEqual(catalog.entries.filter(e=>e.pixel_grid_profile).length,4);
assert.ok(catalog.entries.filter(e=>e.pixel_grid_profile).every(e=>e.menu>=35&&e.virtual_height===240));
console.log('38 Chinese style names, all visible slot labels and explicit pixel-grid recipes checked');
