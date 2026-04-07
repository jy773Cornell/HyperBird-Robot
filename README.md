# HyperBird Microscopic Imaging Robot

HyperBird is a high-throughput hyperspectral microscopic imaging platform for plant phenotyping, combining robotic sample scanning with automated spectral-image processing and analysis.

<p align="center">
  <img src="assets/hyperbird_system.png" alt="HyperBird system" width="80%">
</p>

<p align="center">
  <a href="assets/hyperbird_in_action.mp4">
    <img src="assets/hyperbird_system.png" alt="Watch HyperBird in action (MP4)" width="80%">
  </a>
</p>

<p align="center">
  ▶️ <a href="assets/hyperbird_in_action.mp4"><strong>Watch HyperBird in action (MP4)</strong></a>
</p>

## Contents

| Folder | Description |
| ------ | ----------- |
| [`hyperbird-proto/`](hyperbird-proto/) | Hardware system control software (Linux C++, camera-motion synchronization, tray scanning, ENVI output). |
| [`hyperbird-studio/`](hyperbird-studio/) | Processing, calibration, and data analysis pipeline (watchers, segmentation, notebooks, model workflows). |
| [`data_examples/`](data_examples/) | Example processed sample outputs for visualization and demos. |
| [`assets/`](assets/) | README media assets. |
## Sampel Images

Leaf RGB examples from [`data_examples/leaf_samples_processed/`](data_examples/leaf_samples_processed/):

<table>
  <tr>
    <td align="center"><b>002-white</b><br><img src="data_examples/leaf_samples_processed/002-white/002-white_rgb.png" width="220"></td>
    <td align="center"><b>003-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/003-DMTSLeaf1/003-DMTSLeaf1_rgb.png" width="220"></td>
    <td align="center"><b>004-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/004-DMTSLeaf1/004-DMTSLeaf1_rgb.png" width="220"></td>
  </tr>
  <tr>
    <td align="center"><b>005-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/005-DMTSLeaf1/005-DMTSLeaf1_rgb.png" width="220"></td>
    <td align="center"><b>006-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/006-DMTSLeaf2/006-DMTSLeaf2_rgb.png" width="220"></td>
    <td align="center"><b>007-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/007-DMTSLeaf2/007-DMTSLeaf2_rgb.png" width="220"></td>
  </tr>
  <tr>
    <td align="center"><b>008-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/008-DMTSLeaf2/008-DMTSLeaf2_rgb.png" width="220"></td>
    <td align="center"><b>009-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/009-DMTSLeaf2/009-DMTSLeaf2_rgb.png" width="220"></td>
    <td align="center"><b>010-DMTSLeaf3</b><br><img src="data_examples/leaf_samples_processed/010-DMTSLeaf3/010-DMTSLeaf3_rgb.png" width="220"></td>
  </tr>
</table>

## Author

- **Jinhong Yu**
- Cornell AgriTech, Cornell University
- Contact: `yujiang@cornell.edu`

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