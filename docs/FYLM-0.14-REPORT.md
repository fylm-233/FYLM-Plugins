# FYLM Plugins 0.14.0 test candidate

Pixel Art adds four-tone **Amber CRT** and **1986 Plasma**, an editable **2–8 colour palette**, explicit **2×2 / 4×4 / 8×8** Bayer grids, and a smooth CMYK halftone option. The two display palettes are original FYLM artistic interpretations of four-level monochrome shading, not historical hardware specifications or CRT geometry simulations.

## Operation

- In native Effect Controls, select Bayer 4×4 or Bayer 8×8, then choose the desired Bayer grid. Follow Algorithm preserves the previous algorithm's grid.
- Choose Amber CRT, 1986 Plasma or Custom Palette from the classic palette menu. For Custom Palette, select the number of colours, then use each native colour swatch or AE eyedropper. Enabled colours are sorted by luminance at render time. Colours are stored as ordinary AE colour parameters, can be animated, and persist with the project. Duplicate swatches are allowed.
- The custom palette applies to the classic dithering modes, including Floyd–Steinberg, Bayer, threshold and blue noise. PC-98/GBA/Modern retain their separate palette workflows. The optional FYLM panel edits palette selection/count and directs colour editing to native controls.
- In Halftone, choose Smooth CMYK Dots, then adjust Screen Rotation and Dot Gain. Try working resolution 384–768 and dot pitch 3–6 for finer printing details. Original Cover Print remains the default for existing projects.

## Rendering and compatibility

Existing parameter IDs 1–60 are retained. Palette indices 1–8 retain their meanings; 9 Amber, 10 Plasma and 11 Custom append. Parameters 61–73 append palette count, eight native colour swatches, halftone mode/rotation/gain and Bayer grid. Hidden swatches do not enter the active palette.

The added halftone separates CMYK from an immutable source copy, uses independently rotated screens, circular dots with analytic edge coverage, adjustable dot gain, exact white/black endpoints, preserved source alpha and row cancellation. The old halftone function remains unchanged. Dot gain affects midtones while preserving endpoints. The classic CPU path retains its existing SDR / encoded-sRGB treatment; AE 32 bpc is not a promise of an HDR print transform.

## Installer

`FYLM-Plugins-0.14.0-Setup.exe` embeds the complete runtime package and checks its SHA-256 before extraction. It detects AE folders under Program Files and Adobe's 32/64-bit registry keys. Users can browse to or type another AE version folder or its Support Files directory. Detection lists candidates and requires choosing a target; it does not install into every version.

Install checks `AfterFX.exe`, rejects running AE/aerender, requests Windows administrator access, and invokes the existing transactional installer. Existing plugin/panel/asset files are backed up. Logs and the extracted package are retained under LocalAppData/FYLM. Each version is installed separately by choosing its directory. `--diagnose <report-file>` verifies embedded payload contents and reports detected paths without installing.

## Verification scope

Core and SDK-mock tests cover original output hashes, new halftone endpoints/alpha/angle/gain, appended menu compatibility, four language catalogs, native colour registration, palette/grid snapshot mapping and new palette/CMYK rendering at AE 8/16/32 bpc. These tests do not replace real AE eyedropper/undo/save/reopen checks or visual acceptance.

Test and package receipts live locally in `build/014-*` and `docs/0.14-host`. Real AE and interactive Setup acceptance remain pending until reported by the user. Passed source changes are mirrored into the requested local Git working tree using `tools/sync-tested-source.ps1`; publishing and committing remain separate operations.

2026-11-02 installation acceptance: user confirms Setup detection and installation normal. Independently verified all275 installed runtime/panel/asset files against the 0.14.0 package; no mismatches. Current installed version is0.14.0. Manual version selection, new palette/eyedropper and halftone visual acceptance are not inferred from this result.
