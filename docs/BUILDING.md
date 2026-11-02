# Building FYLM Plugins

Use Windows x64, CMake 3.21+, Node.js, and Visual Studio with Desktop development with C++ and a Windows SDK. The current development build uses Visual Studio 2026. Obtain the Adobe After Effects SDK separately; it is not included in this repository.

1. Run `./tools/fetch-shaderglass.ps1`. It downloads the pinned ShaderGlass source and its bundled compiler libraries into the ignored `third_party/ShaderGlass-master` directory. The upstream source notices remain with the dependency.
2. Configure and build from the repository root:

```powershell
cmake -S . -B build -DBUILD_AE_PLUGIN=ON -DAE_SDK_ROOT="C:/SDK/AfterEffectsSDK"
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
```

The SDK directory must contain `Examples/Headers/AE_Effect.h`. To build without the AE adapter, omit both `-D` options. The other components still require Windows and the ShaderGlass dependency.

The three suite binaries are `FYLM_PixelArt.aex`, `FYLM_CRTVHS.aex` and `FYLM_ShaderGlass.aex` under `build/plugin/ae/Release`. Ship runtime tools, panels and assets together using a matching package script; copying only the AEX files omits import tools and previews. Historical package scripts create their named versions and must not be used to label a newer build.

No private images or Adobe SDK files belong in a public source tree. Read [PUBLICATION-REVIEW.md](PUBLICATION-REVIEW.md) before uploading. Automated tests do not replace validation in AE.
