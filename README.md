# HyperBird Microscopic Imaging Robot

HyperBird is a high-throughput hyperspectral microscopic imaging platform for plant phenotyping, combining robotic sample scanning with automated spectral-image processing and analysis.

▶️ **Watch HyperBird in action (MP4)**

## Contents


| Folder                                   | Description                                                                                               |
| ---------------------------------------- | --------------------------------------------------------------------------------------------------------- |
| `[hyperbird-proto/](hyperbird-proto/)`   | Hardware system control software (Linux C++, camera-motion synchronization, tray scanning, ENVI output).  |
| `[hyperbird-studio/](hyperbird-studio/)` | Processing, calibration, and data analysis pipeline (watchers, segmentation, notebooks, model workflows). |
| `[data_examples/](data_examples/)`       | Example processed sample outputs for visualization and demos.                                             |
| `[assets/](assets/)`                     | README media assets.                                                                                      |


## Sampel Images

Leaf samples (generated RGB images from Hypercube):


|                   |                   |                   |
| ----------------- | ----------------- | ----------------- |
| **002-white**     | **003-DMTSLeaf1** | **004-DMTSLeaf1** |
| **005-DMTSLeaf1** | **006-DMTSLeaf2** | **007-DMTSLeaf2** |
| **008-DMTSLeaf2** | **009-DMTSLeaf2** | **010-DMTSLeaf3** |


### Masks

Segmentation overlays on RGB:


|                   |                   |                   |
| ----------------- | ----------------- | ----------------- |
| **002-white**     | **003-DMTSLeaf1** | **004-DMTSLeaf1** |
| **005-DMTSLeaf1** | **006-DMTSLeaf2** | **007-DMTSLeaf2** |
| **008-DMTSLeaf2** | **009-DMTSLeaf2** | **010-DMTSLeaf3** |


### Mean Spectra

Per-sample ROI mean ± std:


|                   |                   |                   |
| ----------------- | ----------------- | ----------------- |
| **002-white**     | **003-DMTSLeaf1** | **004-DMTSLeaf1** |
| **005-DMTSLeaf1** | **006-DMTSLeaf2** | **007-DMTSLeaf2** |
| **008-DMTSLeaf2** | **009-DMTSLeaf2** | **010-DMTSLeaf3** |


## Author

- **Jinhong Yu**
- Cornell AgriTech, Cornell University
- Contact: `jy773@cornell.edu`

## Publication and Citation

If you use this repository, please cite the HyperBird manuscript.

### Citation (plain text)

Yu, J., Brewer, A., Pippi, L., Hosseinzadeh, S., Moreno, J., Martinez, D., Chen, C., Gold, K. M., Cadle-Davidson, L., and Jiang, Y. HyperBird: A hyperspectral microscopic imaging robot for high-throughput plant phenotyping.

### Citation (BibTeX)

```bibtex
@article{yu_hyperbird,
  title   = {HyperBird: A Hyperspectral Microscopic Imaging Robot for High Throughput Plant Phenotyping},
  author  = {Yu, Jinhong and Brewer, Aliyah and Pippi, Lorenzo and Hosseinzadeh, Saeed and Moreno, Javier and Martinez, Dani and Chen, Chang and Gold, Kaitlin M. and Cadle-Davidson, Lance and Jiang, Yu},
  journal = {TBD},
  year    = {TBD},
  volume  = {TBD},
  number  = {TBD},
  pages   = {TBD},
  doi     = {TBD}
}
```

