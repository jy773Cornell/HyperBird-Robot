"""ENVI to Zarr conversion utilities.

Author: Jinhong Yu
Institution: Cornell University, CALS
Lab: Postharvest Technology Lab & CAIR Lab
Email: jy773@cornell.edu
"""

import os
import zarr
import spectral
import numpy as np
import dask.array as da
from typing import Optional, Iterable, Tuple, Literal

spectral.settings.envi_support_nonlowercase_params = True 

# ENVI data type mapping
_ENVI_DT_MAP = {
    np.uint8: 1,
    np.int16: 2,
    np.int32: 3,
    np.float32: 4,
    np.float64: 5,
    np.uint16: 12,
    np.uint32: 13,
    np.int64: 14,
}


def _get_envi_dtype_code(dtype: np.dtype) -> int:
    """Convert NumPy dtype to ENVI data type code."""
    base = np.dtype(dtype).type
    if base not in _ENVI_DT_MAP:
        raise ValueError(f"Unsupported ENVI dtype: {dtype}")
    return _ENVI_DT_MAP[base]


def _write_envi_header(
    raw_path: str,
    rows: int,
    cols: int,
    bands: int,
    dtype: np.dtype,
    *,
    interleave: str,
    wavelengths: Optional[Iterable[float]] = None,
    wavelength_units: str = "nm",
    byte_order: int = 0,
) -> None:
    """Write ENVI header file with metadata."""
    hdr_path = os.path.splitext(raw_path)[0] + ".hdr"
    with open(hdr_path, "w") as f:
        f.write("ENVI\n")
        f.write("file type = ENVI Standard\n")
        f.write(f"lines   = {rows}\n")
        f.write(f"samples = {cols}\n")
        f.write(f"bands   = {bands}\n")
        f.write("header offset = 0\n")
        f.write(f"data type = {_get_envi_dtype_code(dtype)}\n")
        f.write(f"interleave = {interleave}\n")
        f.write(f"byte order = {byte_order}\n")
        f.write(f"\nwavelength units = {wavelength_units}\n")
        if wavelengths is not None:
            f.write("wavelength = {\n")
            f.write(",\n".join(f"{float(w):.6f}" for w in wavelengths))
            f.write("\n}\n")


# Interleave format metadata
_INTERLEAVE_META = {
    "bip": {"row_axis": 0, "order": "rcb"},
    "bil": {"row_axis": 0, "order": "rbc"},
    "bsq": {"row_axis": 1, "order": "brc"},
}


def _get_interleave_info(
    arr: zarr.Array, interleave: Optional[str]
) -> Tuple[str, int]:
    """Get interleave format and row axis from Zarr array or parameter."""
    iv = interleave or arr.attrs.get("interleave", None)
    if iv is None:
        raise ValueError(
            "Interleave not provided and not found in Zarr attrs. "
            "Pass interleave= ('bip'|'bil'|'bsq') or set arr.attrs['interleave']."
        )
    iv = iv.lower()
    if iv not in _INTERLEAVE_META:
        raise ValueError(f"Unsupported interleave '{iv}'. Use 'bip', 'bil', or 'bsq'.")
    row_axis = _INTERLEAVE_META[iv]["row_axis"]
    return iv, row_axis


def _get_rcb_permutation(interleave: str) -> Tuple[int, int, int]:
    """Convert interleave format to (rows, cols, bands) permutation."""
    iv = str(interleave).strip().lower()
    if iv == "bip":  # (rows, cols, bands)
        return (0, 1, 2)
    if iv == "bil":  # (rows, bands, cols) -> (rows, cols, bands)
        return (0, 2, 1)
    if iv == "bsq":  # (bands, rows, cols) -> (rows, cols, bands)
        return (1, 2, 0)
    raise ValueError(
        f"Unsupported interleave '{interleave}' (expected 'bip'|'bil'|'bsq')."
    )


def _fix_bands_axis(
    arr_shape: Tuple[int, int, int], bands_len: int, permuted_axes: Tuple[int, int, int]
) -> Tuple[int, int, int]:
    """Ensure bands axis is last after permutation."""
    permuted_shape = tuple(arr_shape[i] for i in permuted_axes)
    if permuted_shape[2] == bands_len:
        return (0, 1, 2)

    candidates = [ax for ax, sz in enumerate(permuted_shape) if sz == bands_len]
    if not candidates:
        raise ValueError(
            f"Cannot find bands axis == {bands_len} in shape {permuted_shape}. "
            "Interleave may be wrong or wavelengths length mismatches the data."
        )
    band_axis = candidates[0]
    new_order = [ax for ax in (0, 1, 2) if ax != band_axis] + [band_axis]
    return tuple(new_order)


def convert_envi_to_zarr(
    hdr_path: str,
    out_path: str,
    chunks=(256, 256, 256),
    overwrite=True,
):
    """Convert ENVI hyperspectral file to Zarr format with metadata preservation."""
    img = spectral.open_image(hdr_path)
    interleave = img.metadata.get("interleave", "bip").lower()
    arr = img.open_memmap(interleave="source")

    # print(
    #     f"[envi_to_zarr] {hdr_path}: shape={arr.shape}, dtype={arr.dtype}, interleave={interleave}"
    # )

    darr = da.from_array(arr, chunks=chunks)
    darr.to_zarr(out_path, overwrite=overwrite)

    # Attach metadata
    za = zarr.open(out_path, mode="r+")
    if isinstance(za, zarr.Array):
        za.attrs["interleave"] = interleave
        if "Wavelength" in img.metadata:
            za.attrs["wavelength"] = [float(w) for w in img.metadata["Wavelength"]]
    else:
        keys = [k for k in za.array_keys()]
        if keys:
            a = za[keys[0]]
            a.attrs["interleave"] = interleave
            if "Wavelength" in img.metadata:
                a.attrs["wavelength"] = [float(w) for w in img.metadata["Wavelength"]]


def convert_zarr_to_envi(
    zarr_path: str,
    out_raw_path: str,
    *,
    interleave: Optional[Literal["bip", "bil", "bsq"]] = None,
    out_dtype: Optional[np.dtype] = None,
    wavelengths: Optional[Iterable[float]] = None,
    wavelength_units: str = "nm",
    rows_per_block: int = 256,
    byte_order: int = 0,
) -> str:
    """Convert Zarr array to ENVI format with streaming for large datasets."""
    za = zarr.open(zarr_path, mode="r")
    if not isinstance(za, zarr.Array):
        keys = [k for k in za.array_keys()]
        if not keys:
            raise ValueError(f"No arrays found in {zarr_path}")
        za = za[keys[0]]

    if za.ndim != 3:
        raise ValueError(f"Expected 3D array; got {za.shape}")

    iv, row_axis = _get_interleave_info(za, interleave)
    dtype = np.dtype(out_dtype or za.dtype)

    if wavelengths is None:
        attrs = za.attrs.asdict() if hasattr(za.attrs, "asdict") else dict(za.attrs)
        wavelengths = attrs.get("wavelength", None)

    # Determine dimensions for header
    if iv == "bip":
        rows, cols, bands = za.shape
    elif iv == "bil":
        rows, bands, cols = za.shape
    else:  # "bsq"
        bands, rows, cols = za.shape

    _write_envi_header(
        out_raw_path,
        rows=rows,
        cols=cols,
        bands=bands,
        dtype=dtype,
        interleave=iv,
        wavelengths=wavelengths,
        wavelength_units=wavelength_units,
        byte_order=byte_order,
    )

    # Align block size to row-axis chunk
    try:
        rchunk = za.chunks[row_axis]
        if rchunk and rows_per_block % rchunk != 0:
            rows_per_block = max(rchunk, (rows_per_block // rchunk) * rchunk)
    except Exception:
        pass

    # Stream data according to interleave layout
    if iv == "bip":
        out_mm = np.memmap(
            out_raw_path, mode="w+", dtype=dtype, shape=(rows, cols, bands)
        )
        for r0 in range(0, rows, rows_per_block):
            r1 = min(r0 + rows_per_block, rows)
            block = za[r0:r1, :, :]
            block = np.ascontiguousarray(block, dtype=dtype)
            out_mm[r0:r1, :, :] = block
        out_mm.flush()
        del out_mm

    elif iv == "bil":
        out_mm = np.memmap(
            out_raw_path, mode="w+", dtype=dtype, shape=(rows, bands, cols)
        )
        for r0 in range(0, rows, rows_per_block):
            r1 = min(r0 + rows_per_block, rows)
            block = za[r0:r1, :, :]
            block = np.ascontiguousarray(block, dtype=dtype)
            out_mm[r0:r1, :, :] = block
        out_mm.flush()
        del out_mm

    else:  # "bsq"
        out_mm = np.memmap(
            out_raw_path, mode="w+", dtype=dtype, shape=(bands, rows, cols)
        )
        for b in range(bands):
            for r0 in range(0, rows, rows_per_block):
                r1 = min(r0 + rows_per_block, rows)
                plane = za[b, r0:r1, :]
                plane = np.ascontiguousarray(plane, dtype=dtype)
                out_mm[b, r0:r1, :] = plane
        out_mm.flush()
        del out_mm
