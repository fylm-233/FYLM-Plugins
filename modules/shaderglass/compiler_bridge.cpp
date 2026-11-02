// GPL-3.0: additive bridge for the vendored ShaderGC public API.
#include "ShaderGC.h"
ShaderDef ShaderGC::CompilePreparedSourceShader(SourceShaderDef& source,std::ostream& log,bool& warning,const ShaderCache& cache){return CompileSourceShader(source,log,warning,cache);}
