# The Last Emberwing — Unreal starter project

This folder contains the first playable Unreal Engine foundation for the game concept in [`../../UNREAL_GAME_CONCEPT.md`](../../UNREAL_GAME_CONCEPT.md).

## Current prototype

The project is intentionally asset-light so it can be opened before final art is ready:

- UE5 game descriptor targeting Windows and Android
- C++ gameplay module with Blueprint-friendly classes
- Third-person mantis placeholder character
- Run, jump, camera look, light attack, and glide input
- Three simple chasing enemies with health and contact damage
- Runtime-generated platform route and ember lantern
- Landscape-friendly virtual joystick configuration
- Mobile-conscious renderer defaults

The prototype arena is generated at runtime using Unreal's built-in primitive meshes. It is not the final visual direction; it exists to make movement and combat testable immediately.

## Requirements

- Unreal Engine 5.4 or a compatible newer UE5 version
- Visual Studio 2022 with **Game development with C++** on Windows
- Android Studio, Android SDK/NDK, and OpenJDK for Android packaging

The sandbox used to prepare this repository does not include Unreal Engine, so the project must be compiled and opened on a machine with UE5 installed.

## Open the project

1. Install the required Unreal Engine version.
2. Open `Emberwing.uproject` with Unreal Editor.
3. If Unreal asks to rebuild the `Emberwing` module, choose **Yes**.
4. Press **Play**. The prototype route and enemies are generated when the game starts.
5. Test keyboard/mouse first, then configure Android packaging and deploy to a phone.

If the engine does not provide `/Engine/Maps/Entry` in your installation, create a new **Empty Level**, save it as `Content/Maps/Prototype.umap`, and change both map entries in `Config/DefaultEngine.ini` to `/Game/Maps/Prototype`.

## Controls

| Action | Keyboard / mouse | Gamepad / touch |
|---|---|---|
| Move | WASD | Left stick / virtual joystick |
| Look | Mouse | Right stick |
| Jump | Space | Face button bottom |
| Attack | Left mouse button | Right trigger |
| Glide | Left Shift | Face button left |

The final mobile HUD will replace the engine virtual joystick during the UI pass.

## Next implementation pass

1. Replace primitive meshes with Ari's mantis character and animations.
2. Add health UI, pause menu, shrine checkpoint, and save data.
3. Add the moonlight leaf puzzle and the Water Strider boss.
4. Create the Drowned Root level as authored Unreal content rather than runtime geometry.
5. Add stylized materials, foliage, Niagara effects, sound, and story cinematics.
6. Profile on real Android devices before increasing visual complexity.
