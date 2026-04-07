# HyperBird — software and data

This repository contains **hardware control software** and **processing / analysis** code for the HyperBird hyperspectral microscopic imaging robot. It does **not** include the LaTeX manuscript; that is maintained separately.

## Layout


| Path                                     | Role                                                                                                                                                |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------- |
| `[hyperbird-proto/](hyperbird-proto/)`   | Linux C++ application: motion stage, camera (Andor), tray scanning, ENVI output.                                                                    |
| `[hyperbird-studio/](hyperbird-studio/)` | Python/Jupyter: live processing watchers, flat-field and ENVI→Zarr pipeline, GSAM2 segmentation, calibration notebooks, disease-analysis notebooks. |
| `data/`                                  | Optional place for shared datasets or symlinks (empty by default).                                                                                  |
| `assets/`                                | Optional static assets (empty by default).                                                                                                          |


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