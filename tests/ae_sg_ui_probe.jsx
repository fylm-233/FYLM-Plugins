#target aftereffects
// Read-only: does not add effects, change values, render, or save the project.
(function () {
    var lines = ['FYLM ShaderGlass UI probe v2', 'Time: ' + new Date().toString(), 'AE: ' + app.version];
    try {
        var active = app.project && app.project.activeItem, matches = [];
        function scan(comp, itemIndex) {
            if (!(comp instanceof CompItem)) return;
            for (var layerIndex = 1; layerIndex <= comp.numLayers && matches.length < 16; layerIndex++) {
                var effects = comp.layer(layerIndex).property('ADBE Effect Parade');
                if (!effects) continue;
                for (var i = 1; i <= effects.numProperties && matches.length < 16; i++) {
                    if (effects.property(i).matchName === 'FYLM_ShaderGlass')
                        matches.push({fx: effects.property(i), location: 'item=' + itemIndex + ', layer=' + layerIndex + ', effect=' + i});
                }
            }
        }
        scan(active, 'active');
        if (!matches.length && app.project) {
            for (var itemIndex = 1; itemIndex <= app.project.numItems && matches.length < 16; itemIndex++)
                if (app.project.item(itemIndex) !== active) scan(app.project.item(itemIndex), itemIndex);
        }
        if (!matches.length) throw new Error('当前工程中未找到 FYLM ShaderGlass 效果。请打开有故障效果的工程后运行。');
        var indices = [2, 4, 16, 22, 23, 24, 25, 26, 27, 28, 29, 30];
        for (var matchIndex = 0; matchIndex < matches.length; matchIndex++) {
            var found = matches[matchIndex].fx;
            lines.push('\nInstance: ' + matches[matchIndex].location + ' | Properties: ' + found.numProperties);
            for (var j = 0; j < indices.length; j++) {
                var index = indices[j], p = found.property(index);
                lines.push(index + ' | ' + p.name + ' | value=' + p.value + ' | keys=' + p.numKeys);
            }
        }
        lines.push('FYLM_SG_UI_PROBE_READ_OK');
    } catch (e) { lines.push('ERROR: ' + e.toString()); }
    var result = lines.join('\n');
    try {
        var folder = new Folder(new File($.fileName).parent.parent.fsName + '/build/sg-ui-probe');
        if (!folder.exists && !folder.create()) throw new Error('Cannot create diagnostic folder');
        var file = new File(folder.fsName + '/host.txt'); file.encoding = 'UTF-8';
        if (!file.open('w')) throw new Error('Cannot write diagnostic log');
        try { file.write(result); } finally { file.close(); }
        result += '\n\n日志：' + file.fsName;
    } catch (writeError) { result += '\n\n日志未写入，请复制以上结果。'; }
    alert(result);
}());
