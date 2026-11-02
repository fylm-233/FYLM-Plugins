const fs=require('fs'),path=require('path');
const entries={
 'sg.uniform.ResourceLOD':['查找纹理层级','查找紋理層級','Lookup mip level'],
 'sg.uniform.BlurSize':['输入图像层级','輸入影像層級','Source mip level'],
 'sg.style_controls':['显示与输入','顯示與輸入','Display and input'],
 'sg.source_actions':['导入与高级参数','匯入與進階參數','Import and advanced controls'],
 'sg.performance_heavy':['复杂处理：预览开销较高，可降低输入网格高度。','複雜處理：預覽開銷較高，可降低輸入網格高度。','Complex processing: higher preview cost. A lower input grid can reduce work.'],
 'sg.import_source':['导入 .slangp 源预设','匯入 .slangp 源預設','Import .slangp source'],
 'sg.import_cancel':['取消后台导入','取消背景匯入','Cancel background import'],
 'sg.import_status':['查看导入状态','查看匯入狀態','Check import status'],
 'sg.import_idle':['未开始导入','尚未開始匯入','No import task'],
 'sg.import_running':['正在后台编译，可继续操作','正在背景編譯，可繼續操作','Compiling in background'],
 'sg.import_done':['导入完成，已载入工程','匯入完成，已載入工程','Import applied to project'],
 'sg.import_cancelled':['导入已取消，原效果保留','匯入已取消，原效果保留','Import cancelled; effect retained'],
 'sg.advanced':['高级参数／创建自定义副本','進階參數／建立自訂副本','Advanced parameters / custom copy'],
 'sg.editor_help':['编辑全部参数。需要关键帧时，将参数绑定到 A–P 槽。应用会建立内嵌副本，不改内置预设；取消不写入工程。','編輯全部參數。需要關鍵影格時，將參數綁定到 A–P 槽。套用會建立內嵌副本，不改內建預設；取消不寫入工程。','Edit any parameter. Bind it to A-P to animate it. Apply creates an embedded copy and leaves built-ins unchanged; Cancel discards edits.'],
 'sg.keep_slots':['保持现有映射','保留現有對應','Keep current slot map'],
 'sg.bind_slot':['将当前参数绑定到所选槽（替换）','將目前參數綁定到所選槽（取代）','Bind current parameter to slot (replace)'],
 'sg.export':['导出当前 .fylmsg 预设','匯出目前 .fylmsg 預設','Export current .fylmsg preset'],
 'sg.editor_apply':['应用为自定义副本','套用為自訂副本','Apply custom copy'],
 'sg.editor_cancel':['取消','取消','Cancel'],
 'sg.error.editor_range':['请输入该参数范围内的有效数字。','請輸入該參數範圍內的有效數字。','Enter a finite number within this parameter range.'],
 'sg.error.editor':['无法打开高级参数编辑器。','無法開啟進階參數編輯器。','Cannot open advanced parameter editor.'],
 'sg.error.validation':['预设GPU验证失败，原效果已保留。请查看导入日志。','預設GPU驗證失敗，原效果已保留。請查看匯入記錄。','Preset GPU validation failed; previous settings were retained. Check the import log.'],
 'sg.error.import_busy':['已有后台导入任务，请等待或取消。','已有背景匯入工作，請等待或取消。','An import is running. Wait or cancel it first.'],
 'sg.error.import_stale':['编译期间目标效果已改变或移除，未覆盖当前工程。请重新导入。','編譯期間目標效果已變更或移除，未覆寫目前工程。請重新匯入。','The target changed or was removed during compilation. Current settings were retained; import again.'],
 'sg.error.import_commit':['导入提交失败，已尝试恢复原参数。请查看导入日志。','匯入提交失敗，已嘗試恢復原參數。請查看匯入記錄。','Import commit failed; restoration was attempted. Check the import log.'],
 'sg.error.temporal':['此预设需要真实历史帧或反馈，当前无历史渲染路径不支持。','此預設需要真實歷史影格或回饋，目前無歷史渲染路徑不支援。','This preset requires true frame history or feedback, which this stateless renderer does not support.'],
 'sg.source_help':['源预设后台导入、取消和高级参数入口位于AE原生效果控件底部。状态按钮可刷新任务结果。','源預設背景匯入、取消及進階參數入口位於AE原生效果控制項底部。狀態按鈕可更新工作結果。','Source import, cancellation and advanced parameters are at the bottom of native Effect Controls. Use Check Import Status to refresh the result.']
};
for(const [lang,index] of [['zh-Hans',0],['zh-Hant',1],['en',2],['ja',2]]){
 const file=path.join(__dirname,'../i18n',lang+'.json'),j=JSON.parse(fs.readFileSync(file,'utf8'));
 for(const [key,values]of Object.entries(entries))j[key]=values[index];fs.writeFileSync(file,JSON.stringify(j,null,2)+'\n');
}
