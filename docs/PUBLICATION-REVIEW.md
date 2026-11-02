# Publication preparation — 0.13.2

This is a local source candidate, not a published release or an installed update.

## UI review

| Effect / surface | Finding | Change or retained behaviour |
|---|---|---|
| All / optional panel | Apply did not describe its scope; loading a recipe could be mistaken for an immediate preview. | Explicit Add / Connect, Read Effect Settings and Apply Panel Settings actions; persistent workflow explanation and pending-edit feedback. Apply occupies its own row to reduce label crowding in translations. |
| Pixel Art / optional panel | Other algorithms' controls remained visible. | Hide inactive rows and collapse their space. Switching algorithms and dependent selectors updates visibility immediately. Applying skips hidden rows so it preserves other modes' values and ignores invalid text in inactive controls. Mode-specific preset application stays in native controls. |
| CRT VHS / optional panel | Mode followed the detailed controls; VHS button retained the original single-recipe name. | Mode now follows Enable, with VHS style/model/strength near the top. Button explicitly loads a VHS style into the panel; applying remains a separate step. Mode gates its VHS style controls. |
| ShaderGlass / optional panel | Main override toggle was separated from the style controls; inactive slots looked editable. | Main toggle is on the style page. Slot banks reflect their override switches. Checking I–P still enables the main override in one action. Input-grid edits still disable recommended resolution. |
| ShaderGlass / optional panel | Imported presets could show empty built-in slot labels. | Read the imported slot names from the effect. Add Traditional Chinese names to browser search. |
| Native Effect Controls | Saved projects depend on original parameter IDs and stream order. | Keep identities and native order. Existing Pixel Art active-only visibility and ShaderGlass I–P auto-enable remain unchanged. This revision improves the optional panel, not the physical native stream order. |

Native controls remain the primary interface. Source import, status, cancel, advanced parameters and compiled-preset export use native buttons; the optional panel explains their location instead of adding duplicate nonfunctional buttons. Native recipe buttons apply directly; the optional panel loads editable values before applying.

Automated panel tests cover four AE language tags, visual ordering, active-mode visibility, inactive-mode preservation, I–P dependencies and effect identity. They simulate ScriptUI; they do not establish actual AE layout, DPI scaling or native dialog acceptance. No AE launch, installation or UAC attempt is part of this revision. Real AE checks are deferred while the user is away.

## Repository presentation

- Default English README, linked Simplified Chinese and Japanese editions, equivalent product scope and quick-start instructions.
- Three previews use original FYLM procedural diagnostic artwork from existing test assets. No private test artwork is copied.
- Old cumulative README retained locally in `docs/history/README-before-publication.md`; it is not the public landing page or part of the curated source candidate.
- Build notes and dependency fetch script keep setup details outside the landing page.
- Curated source preparation copies code, assets, tests and notices. It excludes local builds, installation evidence, SDKs, old ZIPs and private derived images. It produces an inventory; it does not create or push a Git repository.

## Outstanding release gates

1. Test the new optional-panel layout at normal and high DPI in real AE, including mode switching, imported slot names and translated labels. Verify 0.13 native source-import/advanced/export flows separately.
2. Complete the per-shader attribution and redistribution review already required by NOTICE. Confirm the licence/grant for the locally supplied reimuplayer halftone adaptation. Preserve GPL components' corresponding source and dependency notices. Do not describe the whole suite as Apache-only.
3. Verify a clean-machine dependency fetch and source build. The current local build uses already provisioned dependencies; it does not prove that the fetch script has run on a clean machine.
4. Publish an accurately versioned binary package only after its files and current installer are checked. Existing 0.13.0 ZIPs are historical and do not contain this revision.

Latest automated results and the source inventory are recorded locally under `build/0132-*` and `build/github-source-0.13.2.inventory.json`. No public repository or release has been created by this task.
