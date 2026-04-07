# HyperBird — software and data

This repository contains **hardware control software** and **processing / analysis** code for the HyperBird hyperspectral microscopic imaging robot. It does **not** include the LaTeX manuscript; that is maintained separately.

HyperBird combines:
- A Linux C++ scanner controller for camera-motion synchronization and tray-based acquisition.
- A Python/Jupyter processing stack for flat-field correction, ENVI-to-analysis conversion, segmentation, and downstream disease modeling.

## Layout


| Path                                     | Role                                                                                                                                                |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `[hyperbird-proto/](hyperbird-proto/)`   | Linux C++ application: motion stage, camera (Andor), tray scanning, ENVI output.                                                                    |
| `[hyperbird-studio/](hyperbird-studio/)` | Python/Jupyter: live processing watchers, flat-field and ENVI→Zarr pipeline, GSAM2 segmentation, calibration notebooks, disease-analysis notebooks. |
| `data/`                                  | Optional place for shared datasets or symlinks (empty by default).                                                                                  |
| `assets/`                                | Project media assets (system photo and action video).                                                                                               |


## hyperbird-proto (scanner control)

- Builds with `make` → executable `hyperbird` (see `[hyperbird-proto/Makefile](hyperbird-proto/Makefile)`).
- Requires **Andor SDK** (camera) and **xlnt** (Excel tray labels); see `[hyperbird-proto/README.md](hyperbird-proto/README.md)`.
- Typical run: `./hyperbird <labels.xlsx> <cfg/some.cfg>` — output root is configured for your machine (e.g. under `/home/hyperbird/Hyperimages`).

## hyperbird-studio (processing & analysis)

- Install the package in editable mode from `hyperbird-studio/` (`pip install -e .`) or use the provided **Dockerfile** / **devcontainer**.
- **SAM 2** weights are **not** in git: download checkpoints into `hyperbird-studio/sam2/checkpoints/` (see `download_ckpts.sh` there).
- **Watchers** (`processing/hyperbird_watcher1.ipynb`, `hyperbird_watcher2.ipynb`): run *before* a scan to watch a directory for new ENVI pairs and write `_processed` outputs (RGB, masks, ROI spectra). Details: `[hyperbird-studio/README.md](hyperbird-studio/README.md)`.
- **Calibration**: notebooks under `hyperbird-studio/calibration/` (spectral, spatial, depth of field).
- **Disease / stats pipelines**: notebooks under `hyperbird-studio/disease_detection/`.
- **Trained models** (e.g. `*.joblib` under `models/`).

## Git

Single repository at this root. Nested `.git` directories under `hyperbird-proto` and `hyperbird-studio` were removed so everything is tracked here.

Optional: configure [nbstripout](https://github.com/kynan/nbstripout) so `.gitattributes` strips notebook outputs on commit.

## Assets

Media for documentation and repository overview live in [`assets/`](assets/). **PNG and MP4** render on GitHub; **HEIC** is the optional camera-original still image.

### System photo

![HyperBird system](assets/hyperbird_system.png)

| Format | File |
| ------ | ---- |
| **PNG** (preview) | [`assets/hyperbird_system.png`](assets/hyperbird_system.png) |
| HEIC (original) | [`assets/hyperbird_system.heic`](assets/hyperbird_system.heic) |

### In-action video

<video src="assets/hyperbird_in_action.mp4" controls width="100%"></video>

| Format | File |
| ------ | ---- |
| **MP4** (playback in browser) | [`assets/hyperbird_in_action.mp4`](assets/hyperbird_in_action.mp4) |