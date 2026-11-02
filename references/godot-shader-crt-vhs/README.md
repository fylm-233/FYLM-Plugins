# CRT and VHS shaders for Godot

Separate CRT and VHS shaders for the Godot Engine, written in Godot's shading language GDShader.

## Why separate shaders for CRT and VHS?

These shaders were originally created for a project that renders virtual VHS tapes as dedicated scenes. In this project, the tape content runs at 25 frames per second, while the virtual TV (including its OSD) renders at 50 frames per second.

I chose to separate the effects so that VHS artifacts (like tape wobble) don't incorrectly affect the TV's OSD. By applying the CRT shader to the final screen, it encapsulates both the VHS content and the TV UI. Since CRT and VHS effects simulate different physical phenomena, keeping them decoupled offers more flexibility than combining them into a single shader, as is often (incorrectly, in my opinion) done.

My specific scene works as follows:

* scene (Node3D)
  * tv (MeshInstance3D, material slot for screen renders "crt" SubViewport as emission texture)
  * content (SubViewport, "Own World 3D" enabled, limited to 25fps)
    * content (scene, the VHS tape's content)
  * vhs (SubViewport, limited to 50fps)
    * vhs (TextureRect, rendering the "content" SubViewport, **apllying VHS shader**)
    * osd (scene, the TV's OSD)
  * crt (SubViewport, limited to 50fps)
    * crt (TextureRect, rendering the "vhs" SubViewport, **applying CRT shader**)

_Note: Node tree is simplified, leaving out irrelevant nodes like camera, environment and such._

Despite the nested SubViewports, the performance impact is negligible due to the low resolutions and capped frame rates involved.

Separating the shaders and capping the frame rates significantly improves authenticity and immersion. This modular approach also allows the virtual TV to display arbitrary content (not just VHS tapes) while seamlessly maintaining the CRT aesthetic.

Because these effects are applied to SubViewports, both shaders are of type `canvas_item`.

The frame rate limiting is not handled within the shaders; it must be handled directly on the SubViewports. Here's a naive implementation you can use:

```gdscript
extends SubViewport

@export var target_fps: float = 60.0

var time: float = 0.0
var interval: float = 0.1

func _ready() -> void:
	interval = 1.0 / max(target_fps, 0.01)
	render_target_update_mode = SubViewport.UPDATE_DISABLED

func _process(delta: float) -> void:
	time += delta

	if time >= interval:
		time -= interval
		render_target_update_mode = SubViewport.UPDATE_ONCE
```

## Why no CRT curvature effect?

Unlike many other CRT shaders, this one does not include a built-in screen curvature effect. For my use case, the 2D viewport is mapped onto a curved 3D mesh (a TV screen), making a 2D curvature warp unnecessary.

![example screenshot](assets/example.png)

*Note: The effect is much more pronounced [in motion](https://www.youtube.com/watch?v=RaBjrnXN_zM) than in a still image.*

## VHS shader effects

* scanlines
* luminance and chroma blur (color bleed and chromatic aberration)
* tracking distortion and noise (bottom of the screen)
* tape wobble (horizontal offsets)
* noise (generic static noise)
* chroma streaks (color shifted horizontal lines)
* dropouts (wide horizontally clustered noise)

## CRT shader effects

* RGB mask
* convergence (vertical RGB color offsets)
* halation (exposure/energy around the center)
* "virtual TV controls"
  * brightness
  * contrast
  * saturation
* vignette
* glass tube's black border with rounded corners
