# Reproducible additive GPL adapter; the distributed upstream archive stays unchanged.
set(header "${SG_UPSTREAM}/ShaderGC/ShaderGC.h")
file(READ "${header}" contents)
if(NOT contents MATCHES "CompilePreparedSourceShader")
 if(NOT contents MATCHES "private:")
  message(FATAL_ERROR "Unexpected ShaderGC API: cannot apply prepared-source adapter")
 endif()
 string(REPLACE "private:" "    // FYLM additive adapter; implementation delegates to the upstream compiler.\n    static ShaderDef CompilePreparedSourceShader(SourceShaderDef& def, std::ostream& log, bool& warn, const ShaderCache& cache);\n\nprivate:" contents "${contents}")
 file(WRITE "${header}" "${contents}")
endif()
# Shader/preset text is UTF-8; std::filesystem::path(narrow) uses the Windows
# ANSI code page and breaks non-ASCII dependency filenames.
set(shadergc "${SG_UPSTREAM}/ShaderGC/ShaderGC.cpp")
file(READ "${shadergc}" contents)
if(contents MATCHES "filesystem::path\\(incFile\\)")
 string(REPLACE "filesystem::path(incFile)" "filesystem::u8path(incFile)" contents "${contents}")
 file(WRITE "${shadergc}" "${contents}")
endif()
# Pin push constants to b1 even when the vertex/fragment stage has no UBO0.
# Otherwise SPIRV-Cross auto-assigns b0 and a valid shader silently ignores edits.
set(spirv "${SG_UPSTREAM}/ShaderGC/SPIRV.cpp")
file(READ "${spirv}" contents)
if(NOT contents MATCHES "FYLM_PUSH_BINDING")
 string(REPLACE "hlsl.set_hlsl_options(options);" "hlsl.set_hlsl_options(options);\n        // FYLM_PUSH_BINDING: stable UBO0/b0 and push/b1 runtime contract.\n        HLSLResourceBinding push_binding;\n        push_binding.stage = fragment ? spv::ExecutionModelFragment : spv::ExecutionModelVertex;\n        push_binding.desc_set = ResourceBindingPushConstantDescriptorSet;\n        push_binding.binding = ResourceBindingPushConstantBinding;\n        push_binding.cbv.register_binding = 1;\n        hlsl.add_hlsl_resource_binding(push_binding);" contents "${contents}")
 file(WRITE "${spirv}" "${contents}")
endif()
