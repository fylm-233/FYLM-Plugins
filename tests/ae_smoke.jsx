// Run in a fresh AE instance. Creates only this project's smoke-test project and renders.
(function () {
    var root = new File($.fileName).parent.parent.fsName.replace(/\\/g, '/');
    var folder = new Folder(root + '/docs/ae-smoke'); folder.create();
    var log = new File(folder.fsName + '/host.txt'); log.open('w');
    function write(s) { log.writeln(s); log.close(); log.open('a'); }
    try {
        app.beginSuppressDialogs();
        if (app.project && app.project.numItems > 0) throw new Error('Use a fresh AE instance; existing project was preserved.');
        if (!app.project) app.newProject();
        write('AE version: ' + app.version);
        app.project.bitsPerChannel = 8;
        var comp = app.project.items.addComp('Dithering Studio Smoke', 64, 64, 1, 1, 24);
        var layer = comp.layers.addSolid([0.5,0.5,0.5], 'Gray', 64, 64, 1, 1);
        var fx = layer.property('ADBE Effect Parade').addProperty('OSLO_DitheringStudio');
        if (!fx) throw new Error('Effect not loaded');
        write('Effect loaded: ' + fx.matchName + '; parameters: ' + fx.numProperties);
        for (var i=1; i<=fx.numProperties; i++) write(i + ': ' + fx.property(i).name);
        fx.property(4).setValue(0);
        comp.openInViewer();
        // SaveFrameToPng exercises preview sampling; render queue below exercises output rendering.
        for(var depth=8;depth<=16;depth+=8){
            app.project.bitsPerChannel=depth;
            for(var alg=1;alg<=5;alg++){
                fx.property(1).setValue(alg);
                var image = new File(folder.fsName + '/preview-' + depth + '-' + alg + '.png');
                comp.saveFrameToPng(0, image);
                write('Preview frame saved: '+image.name);
            }
        }
        app.project.bitsPerChannel=8;
        fx.property(1).setValue(1);
        var rq=app.project.renderQueue.items.add(comp);rq.timeSpanDuration=1/24;
        var om=rq.outputModule(1);write('Output templates: '+om.templates.join('|'));
        var found=false;
        for(var t=0;t<om.templates.length;t++)if(/TIFF/i.test(om.templates[t])){om.applyTemplate(om.templates[t]);found=true;break;}
        if(!found)throw new Error('No TIFF output template');
        om.file=new File(folder.fsName+'/queue-[#####].tif');
        app.project.save(new File(folder.fsName+'/smoke.aep'));
        app.project.renderQueue.render();
        write('Render queue status: '+rq.status);
        var files=folder.getFiles('queue-*.tif'); if(!files.length)throw new Error('Queue produced no TIFF'); var footage=app.project.importFile(new ImportOptions(files[0])); var check=app.project.items.addComp('Queue Check',64,64,1,1,24); check.layers.add(footage); check.saveFrameToPng(0,new File(folder.fsName+'/queue-check.png')); write('SMOKE_COMPLETE');
    } catch(e) {write('ERROR: '+e.toString()+' line '+e.line);}
    finally {app.endSuppressDialogs(false);log.close();}
}());


