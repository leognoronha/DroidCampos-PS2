"""Build the menu gallery from the game's meshes with Blender and Pillow.

python tools/build_blueprint_gallery.py --blender PATH --logo PATH --interior PATH
"""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile


def render_meshes(resources, target):
    import bpy
    from mathutils import Vector

    shots = [
        ("droid", [("droid_preview/actor.obj", 0)], (0, -1, 0)),
        ("cerberus", [("enemies/leader/enemy.obj", 0)], (0.3, -1, 0.19)),
    ]
    for name, models, direction in shots:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        scene = bpy.context.scene
        scene.render.engine = "CYCLES"
        scene.cycles.samples = 12
        scene.cycles.use_denoising = True
        scene.render.resolution_x = scene.render.resolution_y = 640
        scene.render.resolution_percentage = 100
        scene.render.film_transparent = True
        scene.render.image_settings.color_mode = "RGBA"
        scene.view_settings.view_transform = "Standard"

        models_objects = []
        for filename, offset in models:
            before = set(bpy.data.objects)
            bpy.ops.wm.obj_import(filepath=str(resources / filename))
            for obj in set(bpy.data.objects) - before:
                if obj.type == "MESH":
                    obj.location.x += offset
                    models_objects.append(obj)
        bpy.context.view_layer.update()
        camera_data = bpy.data.cameras.new("Blueprint camera")
        camera = bpy.data.objects.new("Blueprint camera", camera_data)
        scene.collection.objects.link(camera)
        scene.camera = camera
        camera_data.type = "ORTHO"
        direction = Vector(direction).normalized()
        camera.rotation_euler = (-direction).to_track_quat("-Z", "Y").to_euler()
        basis = camera.rotation_euler.to_matrix()
        local_points = [basis.transposed() @ (obj.matrix_world @ Vector(corner))
                        for obj in models_objects for corner in obj.bound_box]
        if name == "droid":
            local_points = [
                basis.transposed() @ (obj.matrix_world @ obj.data.vertices[index].co)
                for obj in models_objects
                for face in obj.data.polygons
                if obj.data.materials[face.material_index].name == "head"
                for index in face.vertices
                if (obj.matrix_world @ obj.data.vertices[index].co).z >= 2.65
            ]
            if not local_points:
                raise RuntimeError("Droid head material not found")
        lower = Vector(tuple(min(v[i] for v in local_points) for i in range(3)))
        upper = Vector(tuple(max(v[i] for v in local_points) for i in range(3)))
        center = basis @ ((lower + upper) / 2)
        span = max(upper - lower)
        camera_data.ortho_scale = span * (1.20 if name == "droid" else 1.06)
        if name == "droid":
            center.z -= span * 0.05
        camera.location = center + direction * span * 2

        world = bpy.data.worlds.new("Ambient")
        world.color = (0.08, 0.08, 0.08)
        scene.world = world
        for label, position, strength in [
            ("Key", (-1, -1.5, 2), 120),
            ("Rim", (1.8, 0.8, 1.2), 50),
        ]:
            data = bpy.data.lights.new(label, "AREA")
            data.energy = strength * span * span
            data.shape = "DISK"
            data.size = span * 1.6
            light = bpy.data.objects.new(label, data)
            scene.collection.objects.link(light)
            light.location = center + Vector(position) * span
            light.rotation_euler = (center - light.location).to_track_quat("-Z", "Y").to_euler()
        scene.render.filepath = str(target / (name + ".png"))
        bpy.ops.render.render(write_still=True)


def create_atlas(render_dir, logo, interior, output):
    from PIL import Image, ImageChops, ImageOps

    atlas = Image.new("RGBA", (512, 512))
    for index, (name, path) in enumerate((
        ("droid", render_dir / "droid.png"),
        ("cerberus", render_dir / "cerberus.png"),
        ("interior", interior),
        ("logo", logo),
    )):
        with Image.open(path) as src:
            art = src.convert("RGBA")
        alpha = art.getchannel("A")
        if name == "logo" and alpha.getextrema() == (255, 255):
            alpha = ImageOps.grayscale(art).point(lambda value: min(255, value * 3))
            art.putalpha(alpha)
        if name != "droid":
            art = art.crop(alpha.getbbox())
        art.thumbnail((198, 218) if name != "logo" else (168, 168), Image.Resampling.LANCZOS)
        alpha = art.getchannel("A")
        # Grade the original textures rather than replacing them with a flat tint.
        red, green, blue = art.convert("RGB").split()
        luma = ImageOps.grayscale(art)
        surface = Image.merge("RGB", (
            ImageChops.add(red.point(lambda v: v * 0.38),
                           ImageChops.subtract(red, green).point(lambda v: max(0, v - 35) * 0.80)),
            ImageChops.add(green.point(lambda v: v * 0.80), luma.point(lambda v: v * 0.29)),
            ImageChops.add(blue.point(lambda v: v * 0.81), luma.point(lambda v: v * 0.22)),
        ))
        surface.putalpha(alpha)
        tile = Image.new("RGBA", (208, 240))
        tile.alpha_composite(surface, ((208 - art.width) // 2, (240 - art.height) // 2))
        atlas.alpha_composite(tile, (index % 2 * 256 + 24, index // 2 * 256 + 8))
    atlas.quantize(colors=256, method=Image.Quantize.FASTOCTREE,
                   dither=Image.Dither.NONE).save(output, bits=8, optimize=True)


if __name__ == "__main__":
    if "--" in sys.argv:
        resources, target = map(Path, sys.argv[sys.argv.index("--") + 1:])
        render_meshes(resources, target)
    else:
        parser = argparse.ArgumentParser(description=__doc__)
        parser.add_argument("--blender", required=True, type=Path)
        parser.add_argument("--logo", required=True, type=Path)
        parser.add_argument("--interior", required=True, type=Path)
        parser.add_argument("--resources", type=Path,
                            default=Path(__file__).resolve().parents[1] / "game/res")
        parser.add_argument("--output", type=Path)
        args = parser.parse_args()
        output = (args.output or args.resources / "ui_blueprint_gallery.png").resolve()
        with tempfile.TemporaryDirectory(prefix="blueprint-gallery-", dir=output.parent) as scratch:
            subprocess.run([str(args.blender), "-b", "--factory-startup", "--python-exit-code",
                            "1", "--python", str(Path(__file__).resolve()), "--",
                            str(args.resources.resolve()), scratch], check=True)
            create_atlas(Path(scratch), args.logo, args.interior,
                         output)
