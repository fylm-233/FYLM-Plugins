#target aftereffects
(function(){
 var folder=new Folder(new File($.fileName).parent.parent.fsName+'/docs/0.4-host');folder.create();
 var log=new File(folder.fsName+'/entry-probe.txt');
 if(log.open('w')){log.writeln('SCRIPT_ENTERED '+new Date().toString());log.close();}
}());
