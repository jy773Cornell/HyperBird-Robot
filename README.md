# HyperBird Microscopic Imaging Robot

HyperBird is a high-throughput hyperspectral microscopic imaging platform for plant phenotyping, combining robotic sample scanning with automated spectral-image processing and analysis.

<p align="center">
  <img src="assets/hyperbird_system.png" alt="HyperBird system" width="65%">
</p>

<p align="center">
  <a href="assets/hyperbird_in_action.mp4"><img src="assets/hyperbird_in_action_preview.gif" alt="HyperBird in action (preview)" width="65%"></a>
</p>

<p align="center"><a href="assets/hyperbird_in_action.mp4"><strong>▶ Open full video (MP4)</strong></a></p>

## Contents

| Folder | Description |
| ------ | ----------- |
| [`hyperbird-proto/`](hyperbird-proto/) | Scanner control (Linux C++, motion + camera, ENVI output). |
| [`hyperbird-studio/`](hyperbird-studio/) | Processing, calibration, analysis (watchers, segmentation, notebooks). |
| [`data_examples/`](data_examples/) | Example processed outputs for demos. |
| [`assets/`](assets/) | README media (photos, GIF preview, MP4). |

## Sample images

RGB renderings generated from hypercube.

<table>
  <tr>
    <td align="center"><b>002-white</b><br><img src="data_examples/leaf_samples_processed/002-white/002-white_rgb.png" width="220" alt="002 RGB"></td>
    <td align="center"><b>003-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/003-DMTSLeaf1/003-DMTSLeaf1_rgb.png" width="220" alt="003 RGB"></td>
    <td align="center"><b>004-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/004-DMTSLeaf1/004-DMTSLeaf1_rgb.png" width="220" alt="004 RGB"></td>
  </tr>
  <tr>
    <td align="center"><b>005-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/005-DMTSLeaf1/005-DMTSLeaf1_rgb.png" width="220" alt="005 RGB"></td>
    <td align="center"><b>006-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/006-DMTSLeaf2/006-DMTSLeaf2_rgb.png" width="220" alt="006 RGB"></td>
    <td align="center"><b>007-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/007-DMTSLeaf2/007-DMTSLeaf2_rgb.png" width="220" alt="007 RGB"></td>
  </tr>
  <tr>
    <td align="center"><b>008-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/008-DMTSLeaf2/008-DMTSLeaf2_rgb.png" width="220" alt="008 RGB"></td>
    <td align="center"><b>009-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/009-DMTSLeaf2/009-DMTSLeaf2_rgb.png" width="220" alt="009 RGB"></td>
    <td align="center"><b>010-DMTSLeaf3</b><br><img src="data_examples/leaf_samples_processed/010-DMTSLeaf3/010-DMTSLeaf3_rgb.png" width="220" alt="010 RGB"></td>
  </tr>
</table>

### Masks

Segmentation overlays on RGB.

<table>
  <tr>
    <td align="center"><b>002-white</b> (mask)<br><img src="data_examples/leaf_samples_processed/002-white/002-white_mask.png" width="220" alt="002 mask"></td>
    <td align="center"><b>003-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/003-DMTSLeaf1/003-DMTSLeaf1_mask_overlay.png" width="220" alt="003 overlay"></td>
    <td align="center"><b>004-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/004-DMTSLeaf1/004-DMTSLeaf1_mask_overlay.png" width="220" alt="004 overlay"></td>
  </tr>
  <tr>
    <td align="center"><b>005-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/005-DMTSLeaf1/005-DMTSLeaf1_mask_overlay.png" width="220" alt="005 overlay"></td>
    <td align="center"><b>006-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/006-DMTSLeaf2/006-DMTSLeaf2_mask_overlay.png" width="220" alt="006 overlay"></td>
    <td align="center"><b>007-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/007-DMTSLeaf2/007-DMTSLeaf2_mask_overlay.png" width="220" alt="007 overlay"></td>
  </tr>
  <tr>
    <td align="center"><b>008-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/008-DMTSLeaf2/008-DMTSLeaf2_mask_overlay.png" width="220" alt="008 overlay"></td>
    <td align="center"><b>009-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/009-DMTSLeaf2/009-DMTSLeaf2_mask_overlay.png" width="220" alt="009 overlay"></td>
    <td align="center"><b>010-DMTSLeaf3</b><br><img src="data_examples/leaf_samples_processed/010-DMTSLeaf3/010-DMTSLeaf3_mask_overlay.png" width="220" alt="010 overlay"></td>
  </tr>
</table>

### Mean spectra
 
 ROI mean ± std.

<table>
  <tr>
    <td align="center"><b>002-white</b><br><img src="data_examples/leaf_samples_processed/002-white/002-white_white_ref_plot.svg" width="280" alt="002 spectra"></td>
    <td align="center"><b>003-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/003-DMTSLeaf1/003-DMTSLeaf1_roi_mean_std.svg" width="280" alt="003 spectra"></td>
    <td align="center"><b>004-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/004-DMTSLeaf1/004-DMTSLeaf1_roi_mean_std.svg" width="280" alt="004 spectra"></td>
  </tr>
  <tr>
    <td align="center"><b>005-DMTSLeaf1</b><br><img src="data_examples/leaf_samples_processed/005-DMTSLeaf1/005-DMTSLeaf1_roi_mean_std.svg" width="280" alt="005 spectra"></td>
    <td align="center"><b>006-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/006-DMTSLeaf2/006-DMTSLeaf2_roi_mean_std.svg" width="280" alt="006 spectra"></td>
    <td align="center"><b>007-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/007-DMTSLeaf2/007-DMTSLeaf2_roi_mean_std.svg" width="280" alt="007 spectra"></td>
  </tr>
  <tr>
    <td align="center"><b>008-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/008-DMTSLeaf2/008-DMTSLeaf2_roi_mean_std.svg" width="280" alt="008 spectra"></td>
    <td align="center"><b>009-DMTSLeaf2</b><br><img src="data_examples/leaf_samples_processed/009-DMTSLeaf2/009-DMTSLeaf2_roi_mean_std.svg" width="280" alt="009 spectra"></td>
    <td align="center"><b>010-DMTSLeaf3</b><br><img src="data_examples/leaf_samples_processed/010-DMTSLeaf3/010-DMTSLeaf3_roi_mean_std.svg" width="280" alt="010 spectra"></td>
  </tr>
</table>

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
