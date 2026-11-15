#!/usr/bin/env python3

import struct
import zlib
from pathlib import Path

try:
    from PIL import Image
    import numpy as np
except ImportError:
    Image = None
    np = None


PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def _paeth(left, above, upper_left):
    estimate = left + above - upper_left
    distances = (
        abs(estimate - left),
        abs(estimate - above),
        abs(estimate - upper_left),
    )
    return (left, above, upper_left)[distances.index(min(distances))]


def read_rgba_png(path):
    data = Path(path).read_bytes()
    if not data.startswith(PNG_SIGNATURE):
        raise ValueError(f"{path}: invalid PNG signature")

    width = height = bit_depth = color_type = interlace = None
    compressed = bytearray()
    offset = len(PNG_SIGNATURE)
    while offset + 12 <= len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        chunk_type = data[offset + 4 : offset + 8]
        start = offset + 8
        end = start + length
        if end + 4 > len(data):
            raise ValueError(f"{path}: truncated PNG chunk")
        chunk_data = data[start:end]
        expected_crc = struct.unpack_from(">I", data, end)[0]
        actual_crc = zlib.crc32(chunk_type + chunk_data) & 0xFFFFFFFF
        if actual_crc != expected_crc:
            raise ValueError(f"{path}: invalid PNG chunk CRC")

        if chunk_type == b"IHDR":
            if length != 13:
                raise ValueError(f"{path}: invalid IHDR length")
            width, height, bit_depth, color_type, _, _, interlace = (
                struct.unpack(">IIBBBBB", chunk_data)
            )
        elif chunk_type == b"IDAT":
            compressed.extend(chunk_data)
        elif chunk_type == b"IEND":
            break
        offset = end + 4

    if width is None or height is None:
        raise ValueError(f"{path}: missing IHDR")
    if bit_depth != 8 or color_type != 6 or interlace != 0:
        raise ValueError(
            f"{path}: expected non-interlaced RGBA8 PNG, got "
            f"bit_depth={bit_depth}, color_type={color_type}, "
            f"interlace={interlace}"
        )

    stride = width * 4
    raw = zlib.decompress(compressed)
    if len(raw) != height * (stride + 1):
        raise ValueError(f"{path}: unexpected decompressed PNG size")

    rows = []
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset : offset + stride])
        offset += stride
        for index in range(stride):
            left = row[index - 4] if index >= 4 else 0
            above = previous[index]
            upper_left = previous[index - 4] if index >= 4 else 0
            if filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                predictor = _paeth(left, above, upper_left)
            elif filter_type == 0:
                predictor = 0
            else:
                raise ValueError(f"{path}: unsupported PNG filter {filter_type}")
            row[index] = (row[index] + predictor) & 0xFF
        rows.append(bytes(row))
        previous = row
    return width, height, rows


def _png_chunk(chunk_type, payload):
    crc = zlib.crc32(chunk_type + payload) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + chunk_type + payload + struct.pack(">I", crc)


def write_rgba_png(path, width, height, rows):
    image_data = b"".join(b"\0" + row for row in rows)
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    data = (
        PNG_SIGNATURE
        + _png_chunk(b"IHDR", header)
        + _png_chunk(b"IDAT", zlib.compress(image_data, 6))
        + _png_chunk(b"IEND", b"")
    )
    output = Path(path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(data)


def _compare_rgba_pngs_python(
    actual_path, golden_path, per_channel_delta, diff_path
):
    actual_width, actual_height, actual_rows = read_rgba_png(actual_path)
    golden_width, golden_height, golden_rows = read_rgba_png(golden_path)
    width = max(actual_width, golden_width)
    height = max(actual_height, golden_height)
    same_size = actual_width == golden_width and actual_height == golden_height
    diff_rows = []
    differing_pixels = 0
    largest_delta = 0

    for y in range(height):
        diff_row = bytearray(width * 4)
        for x in range(width):
            actual_inside = x < actual_width and y < actual_height
            golden_inside = x < golden_width and y < golden_height
            if not actual_inside or not golden_inside:
                deltas = (255, 0, 0, 255)
                differs = True
            else:
                actual_offset = x * 4
                actual_pixel = actual_rows[y][actual_offset : actual_offset + 4]
                golden_pixel = golden_rows[y][actual_offset : actual_offset + 4]
                deltas = tuple(
                    abs(actual_pixel[channel] - golden_pixel[channel])
                    for channel in range(4)
                )
                largest_delta = max(largest_delta, *deltas)
                differs = any(delta > per_channel_delta for delta in deltas)

            if differs:
                differing_pixels += 1
                diff_offset = x * 4
                diff_row[diff_offset : diff_offset + 4] = bytes(
                    (min(deltas[0] * 8, 255),
                     min(deltas[1] * 8, 255),
                     min(deltas[2] * 8, 255),
                     255)
                )
        diff_rows.append(bytes(diff_row))

    if differing_pixels:
        write_rgba_png(diff_path, width, height, diff_rows)
    else:
        Path(diff_path).unlink(missing_ok=True)

    return {
        "same_size": same_size,
        "actual_size": (actual_width, actual_height),
        "golden_size": (golden_width, golden_height),
        "differing_pixels": differing_pixels,
        "largest_channel_delta": largest_delta,
        "total_pixels": max(actual_width * actual_height, golden_width * golden_height),
        "diff_path": str(diff_path),
    }


def _compare_rgba_pngs_numpy(
    actual_path, golden_path, per_channel_delta, diff_path
):
    with Image.open(actual_path) as actual_image:
        actual = np.array(actual_image.convert("RGBA"), dtype=np.uint8, copy=True)
    with Image.open(golden_path) as golden_image:
        golden = np.array(golden_image.convert("RGBA"), dtype=np.uint8, copy=True)

    actual_height, actual_width = actual.shape[:2]
    golden_height, golden_width = golden.shape[:2]
    width = max(actual_width, golden_width)
    height = max(actual_height, golden_height)
    same_size = actual_width == golden_width and actual_height == golden_height
    overlap_height = min(actual_height, golden_height)
    overlap_width = min(actual_width, golden_width)
    diff_image = np.zeros((height, width, 4), dtype=np.uint8)
    differing_pixels = 0
    largest_delta = 0

    if overlap_height and overlap_width:
        actual_overlap = actual[:overlap_height, :overlap_width].astype(np.int16)
        golden_overlap = golden[:overlap_height, :overlap_width].astype(np.int16)
        deltas = np.abs(actual_overlap - golden_overlap)
        largest_delta = int(deltas.max())
        differs = np.any(deltas > per_channel_delta, axis=2)
        differing_pixels = int(np.count_nonzero(differs))
        diff_image[:overlap_height, :overlap_width][differs, :3] = np.minimum(
            deltas[:, :, :3] * 8, 255
        ).astype(np.uint8)[differs]
        diff_image[:overlap_height, :overlap_width, 3][differs] = 255

    missing_pixels = width * height - overlap_height * overlap_width
    if missing_pixels:
        missing_pixel = np.array((255, 0, 0, 255), dtype=np.uint8)
        diff_image[overlap_height:, :] = missing_pixel
        diff_image[:overlap_height, overlap_width:] = missing_pixel
        differing_pixels += missing_pixels

    if differing_pixels:
        write_rgba_png(
            diff_path,
            width,
            height,
            [diff_image[row].tobytes() for row in range(height)],
        )
    else:
        Path(diff_path).unlink(missing_ok=True)

    return {
        "same_size": same_size,
        "actual_size": (actual_width, actual_height),
        "golden_size": (golden_width, golden_height),
        "differing_pixels": differing_pixels,
        "largest_channel_delta": largest_delta,
        "total_pixels": max(actual_width * actual_height, golden_width * golden_height),
        "diff_path": str(diff_path),
    }


def compare_rgba_pngs(actual_path, golden_path, per_channel_delta, diff_path):
    if Image is not None and np is not None:
        return _compare_rgba_pngs_numpy(
            actual_path, golden_path, per_channel_delta, diff_path
        )
    return _compare_rgba_pngs_python(
        actual_path, golden_path, per_channel_delta, diff_path
    )
