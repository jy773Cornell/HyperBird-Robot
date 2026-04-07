# HyperBird Proto

Linux-based control program for the HyperBird prototype scanner: motion control, line-scan camera acquisition, and ENVI output.

Default experiment output root (configure as needed on your system):

```
/home/hyperbird/Hyperimages
```

## Usage

Two arguments are required:

1. Excel file with tray sample labels (Blackbird-style).
2. Acquisition configuration file.

Example:

```bash
./hyperbird labels.xlsx cfg/hyperbird.cfg
```

## Repository layout

| Folder | Contents |
|--------|----------|
| `src/` | `.cpp` sources |
| `include/` | Headers |
| `docs/` | Documentation |
| `cfg/` | Acquisition configs |
| `data/` | Auxiliary data shipped with the repo |
| `scripts/` | Python helpers (preview, RGB, etc.) |

## Andor SDK (typical install paths)

- Libraries: `/usr/local/lib`
- Executables: `/usr/local/bin`
- Headers: `/usr/local/include`
- Examples: `/opt/andor/examples`
- Documentation: `/opt/andor/docs`

## Notes

- The **lens aperture** is the only camera setting that must be changed manually on the hardware.
