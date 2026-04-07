"""Flat field correction for hyperspectral Zarr arrays.

Author: Jinhong Yu
Institution: Cornell University, CALS
Lab: Postharvest Technology Lab & CAIR Lab
Email: jy773@cornell.edu
"""

import numpy as np
import dask.array as da
import zarr
from typing import Optional, Iterable, Tuple, Literal
from .envi_zarr_conversion import _get_interleave_info


def flat_field_correction_zarr(
    raw_zarr_path: str,
    white_zarr_path: str,
    dark_zarr_path: str,
    out_zarr_path: str,
    *,
    interleave: Optional[Literal["bip", "bil", "bsq"]] = None,
    eps: float = 1e-6,
    clamp_min: Optional[float] = 0.0,
    clamp_max: Optional[float] = 1.0,
    dtype_out=np.float32,
    chunks: Optional[Tuple[int, int, int]] = None,
    overwrite: bool = True,
    zarr_version: int = 2,
    compressor: Optional[object] = None,
) -> str:
    """Apply flat field correction using white and dark references to remove illumination artifacts."""
    raw = da.from_zarr(raw_zarr_path)
    white = da.from_zarr(white_zarr_path)
    dark = da.from_zarr(dark_zarr_path)

    if raw.ndim != 3 or white.ndim != 3 or dark.ndim != 3:
        raise ValueError("All inputs must be 3D arrays.")

    # Get interleave info
    za = zarr.open(raw_zarr_path, mode="r")
    if not isinstance(za, zarr.Array):
        keys = [k for k in za.array_keys()]
        if not keys:
            raise ValueError(f"No arrays found in {raw_zarr_path}")
        za = za[keys[0]]
    interleave, row_axis = _get_interleave_info(za, interleave)

    # Compute row-mean references
    white_mean = white.mean(axis=row_axis, dtype=dtype_out)
    dark_mean = dark.mean(axis=row_axis, dtype=dtype_out)

    white_row = da.expand_dims(white_mean.astype(dtype_out, copy=False), axis=row_axis)
    dark_row = da.expand_dims(dark_mean.astype(dtype_out, copy=False), axis=row_axis)

    raw_f32 = raw.astype(dtype_out, copy=False)
    eps_scalar = dtype_out(eps)
    denom = da.maximum(white_row - dark_row, eps_scalar)
    refl = (raw_f32 - dark_row) / denom

    if clamp_min is not None or clamp_max is not None:
        lo = (
            dtype_out(clamp_min)
            if clamp_min is not None
            else np.array(-np.inf, dtype=dtype_out)
        )
        hi = (
            dtype_out(clamp_max)
            if clamp_max is not None
            else np.array(np.inf, dtype=dtype_out)
        )
        refl = da.clip(refl, lo, hi)

    if chunks is not None:
        if len(chunks) != 3:
            raise ValueError("chunks must be length-3 in the stored axis order.")
        refl = refl.rechunk(chunks)

    # Set default compressor for Zarr v2
    if compressor is None and zarr_version == 2:
        try:
            compressor = zarr.Blosc(cname="zstd", clevel=3, shuffle=2)
        except Exception:
            compressor = None

    # Write corrected data
    if zarr_version == 2:
        refl.to_zarr(
            out_zarr_path, overwrite=overwrite, compressor=compressor, zarr_version=2
        )
    else:
        refl.to_zarr(out_zarr_path, overwrite=overwrite)

    # Preserve metadata
    out_za = zarr.open(out_zarr_path, mode="r+")
    if isinstance(out_za, zarr.Array):
        out_za.attrs["interleave"] = interleave
        in_attrs = za.attrs.asdict() if hasattr(za.attrs, "asdict") else dict(za.attrs)
        if "wavelength" in in_attrs:
            out_za.attrs["wavelength"] = in_attrs["wavelength"]
    else:
        keys = [k for k in out_za.array_keys()]
        if keys:
            arr = out_za[keys[0]]
            arr.attrs["interleave"] = interleave
            in_attrs = (
                za.attrs.asdict() if hasattr(za.attrs, "asdict") else dict(za.attrs)
            )
            if "wavelength" in in_attrs:
                arr.attrs["wavelength"] = in_attrs["wavelength"]

    return out_zarr_path
