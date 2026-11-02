#target aftereffects
// Use the render queue rather than saveFrameToPng, which can return before file creation.
$.evalFile(new File(new File($.fileName).parent.fsName+'/ae_fylm_07_queue.jsx'));
