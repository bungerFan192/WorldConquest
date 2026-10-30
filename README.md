# World Conquest

A grand-strategy conquest game for Nintendo 3DS, written in C++ with
**libctru** and **citro3D**.

## Current milestone

- [x] Native Nintendo 3DS C++ project structure
- [x] citro3D linked and initialized
- [x] GitHub Actions build pipeline
- [ ] Procedural connected country territories
- [ ] Country selection and custom nation creation
- [ ] Automatic wars and territorial conquest
- [ ] AI-controlled nations
- [ ] Save/load
- [ ] StreetPass civilization exchange (native CECD research/integration)

## Build

Requires devkitPro with devkitARM, libctru and citro3D.

```sh
make
```

The build produces `WorldConquest.3dsx`, suitable for loading with a
Nintendo 3DS homebrew launcher.

## GitHub Actions

Every push to `main`, pull request, or manual workflow dispatch runs the
devkitPro devkitARM container build. The resulting `.3dsx` and `.smdh`
are uploaded as the `WorldConquest-3DSX` workflow artifact.

## Design direction

Each country will own one contiguous territory. Wars are simulated
automatically after a declaration; the player does not directly control
individual units. StreetPass encounters are planned as a later feature and
will require a native 3DS system-service layer.
