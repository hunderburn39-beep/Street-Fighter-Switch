# Street-Fighter-Switch

MAME 0.276 ITech32 baseline for Street Fighter: The Movie (sftm).

## Build
[Run GitHub Actions](https://github.com/hunderburn39-beep/Street-Fighter-Switch/actions/workflows/build.yml). Select Run workflow and download the sftm-linux artifact after success.

## Linux validation
Supply your own legally obtained sftm.zip in roms/. No commercial ROMs are distributed here.

```sh
chmod +x sftm
./sftm -rompath roms -verifyroms sftm
./sftm -rompath roms sftm -window
./sftm -rompath roms sftm -bench 60
```

Linux baseline compiled successfully and driver enumeration verified in [run #4](https://github.com/hunderburn39-beep/Street-Fighter-Switch/actions/runs/45425010292). ROM verification, gameplay and benchmark remain pending. Nintendo Switch NRO, libnx and deko3d integration remain pending. Upstream MAME licensing applies.
