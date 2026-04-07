# HyperBird Studio

Processing pipeline for HyperBird hyperspectral data: calibration notebooks, **live watchers** that process ENVI (`.hdr`/`.raw`) files as the scanner writes them, and **disease / modeling** notebooks.

Parent overview: [`../README.md`](../README.md) (repository root).

## Setup

1. Install dependencies (`pip install -e .` from this directory, or use the Dockerfile / devcontainer).
2. Download **SAM 2** checkpoints into `sam2/checkpoints/` if they are not already present (`sam2/checkpoints/download_ckpts.sh`).

## Running the watchers

**Run the watcher notebooks before starting the HyperBird scanning program.**

### `hyperbird_watcher1` & `hyperbird_watcher2`

Both [`processing/hyperbird_watcher1.ipynb`](processing/hyperbird_watcher1.ipynb) and [`processing/hyperbird_watcher2.ipynb`](processing/hyperbird_watcher2.ipynb) behave similarly:

1. Open the notebook in Jupyter.
2. Run all cells up to the configuration section.
3. Set `watch_dir` to the directory where HyperBird writes data (e.g. `/media/hyperbird/...`).
4. Run the watcher cell to start monitoring.

The watcher will:

- Watch for new scan subfolders under `watch_dir`.
- Process new ENVI pairs (`.hdr` + `.raw`) when they appear.
- Apply flat-field correction using white/black references.
- Convert to RGB and run GSAM2 segmentation.
- Extract ROI mean spectra and save CSVs.
- Write outputs under `_processed` folders.

### Configuration

Key settings in the notebooks:

- `watch_dir`: scanner output directory.
- `text_prompt`: GSAM2 prompt (e.g. `"green, leaf, circle"`).
- `pipeline_cfg`: processing parameters (temp paths, illuminants, etc.).

### Output

Each processed run gets a `_processed` folder with RGB, masks, overlays, ROI spectra CSVs, and reference plots as implemented in the notebook.

## Other folders

- **`calibration/`** — spectral, spatial, and depth-of-field calibration notebooks.
- **`disease_detection/`** — disease masking, progression, and mixed-effects analysis notebooks.
- **`processing/src/`** — Python modules (ENVI/Zarr, flat-field, HSI→RGB, ROI spectra).
- **`models/`** — saved sklearn / pipeline artifacts when used by notebooks.
