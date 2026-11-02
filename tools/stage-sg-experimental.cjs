const fs=require('fs'),p=require('path'),crypto=require('crypto');
const root=p.resolve(__dirname,'..'),dest=p.resolve(process.argv[2]),out=p.join(dest,'Assets/ShaderGlass/Experimental');
fs.mkdirSync(out,{recursive:true});
const read=x=>JSON.parse(fs.readFileSync(p.join(root,x),'utf8').replace(/^\uFEFF/,''));
const small=read('build/sg-candidates/probe-results.json'),large=read('build/sg-candidates/probe-1080-results.json'),audit=read('docs/shaderglass/audit/candidates.json');
const source=p.join(root,'third_party/slang-source/slang-shaders-'+audit.commit),records=[];
for(const r of large.results){if(r.probe?.status!=='d3d-execution-passed'||!small.results.some(x=>x.slug===r.slug&&x.probe?.status==='d3d-execution-passed'))continue;
 const a=audit.candidates.find(x=>x.slug===r.slug);if(!a)throw Error('Missing source audit');
 fs.copyFileSync(p.join(root,'build/sg-candidates',r.slug+'.fylmsg'),p.join(out,r.slug+'.fylmsg'));
 for(const f of a.files){const src=p.resolve(source,f.path),target=p.resolve(out,'Sources',f.path);if(!src.startsWith(source+p.sep)||!target.startsWith(p.join(out,'Sources')+p.sep))throw Error('Invalid source path');const bytes=fs.readFileSync(src);if(crypto.createHash('sha256').update(bytes).digest('hex')!==f.sha256)throw Error('Source hash mismatch');fs.mkdirSync(p.dirname(target),{recursive:true});fs.writeFileSync(target,bytes);}
 records.push({...a,ae:'pending',visual_reference:'pending',execution_1080:r.probe});
}
if(records.length!==23)throw Error('Expected23, got '+records.length);
for(const f of fs.readdirSync(source)){if(/^(readme|license|copying)/i.test(f)&&fs.statSync(p.join(source,f)).isFile())fs.copyFileSync(p.join(source,f),p.join(out,'Sources',f));}
fs.writeFileSync(p.join(out,'catalog.json'),JSON.stringify({experimental:true,repository:audit.repository,commit:audit.commit,presets:records},null,2));
fs.writeFileSync(p.join(out,'README.md'),'# 0.10 ShaderGlass 实验预设\n\n23个预编译文件通过独立D3D11小图及1080p执行检查。请使用FYLM ShaderGlass加载已编译预设，选择同目录.fylmsg。原生菜单仍为Geom/EasyMode/导入，未提供新浏览器。尚待AE及上游视觉对照验收。资源纹理/mipmap/未支持格式/历史反馈候选未纳入。Sources保留原文件及许可声明，catalog.json记录来源及审查状态。\n');
console.log('Staged '+records.length+' experimental presets');
