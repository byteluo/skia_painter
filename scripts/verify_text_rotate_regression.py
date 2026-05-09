#!/usr/bin/env python3

import math
import sys
import zlib


def read_png(path):
    with open(path, "rb") as file:
        data = file.read()

    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path}: invalid PNG signature")

    pos = 8
    width = 0
    height = 0
    bit_depth = 0
    color_type = 0
    idat_chunks = []

    while pos < len(data):
        length = int.from_bytes(data[pos : pos + 4], "big")
        chunk_type = data[pos + 4 : pos + 8]
        chunk = data[pos + 8 : pos + 8 + length]
        pos += length + 12

        if chunk_type == b"IHDR":
            width = int.from_bytes(chunk[0:4], "big")
            height = int.from_bytes(chunk[4:8], "big")
            bit_depth = chunk[8]
            color_type = chunk[9]
        elif chunk_type == b"IDAT":
            idat_chunks.append(chunk)
        elif chunk_type == b"IEND":
            break

    if bit_depth != 8 or color_type != 6:
        raise ValueError(
            f"{path}: unsupported PNG format "
            f"bit_depth={bit_depth}, color_type={color_type}"
        )

    raw = zlib.decompress(b"".join(idat_chunks))
    stride = width * 4
    rows = []
    offset = 0
    previous = bytearray(stride)

    def paeth(a, b, c):
        p = a + b - c
        pa = abs(p - a)
        pb = abs(p - b)
        pc = abs(p - c)
        if pa <= pb and pa <= pc:
            return a
        if pb <= pc:
            return b
        return c

    for _ in range(height):
        filter_type = raw[offset]
        offset += 1
        row = bytearray(raw[offset : offset + stride])
        offset += stride

        if filter_type == 1:
            for x in range(stride):
                row[x] = (row[x] + (row[x - 4] if x >= 4 else 0)) & 255
        elif filter_type == 2:
            for x in range(stride):
                row[x] = (row[x] + previous[x]) & 255
        elif filter_type == 3:
            for x in range(stride):
                left = row[x - 4] if x >= 4 else 0
                row[x] = (row[x] + ((left + previous[x]) // 2)) & 255
        elif filter_type == 4:
            for x in range(stride):
                left = row[x - 4] if x >= 4 else 0
                up = previous[x]
                up_left = previous[x - 4] if x >= 4 else 0
                row[x] = (row[x] + paeth(left, up, up_left)) & 255
        elif filter_type != 0:
            raise ValueError(f"{path}: unsupported filter type {filter_type}")

        rows.append(bytes(row))
        previous = row

    return width, height, rows


def dark_pixels(rows, width, height):
    pixels = []
    for y in range(height):
        row = rows[y]
        for x in range(width):
            offset = x * 4
            r, g, b, a = row[offset : offset + 4]
            if a > 200 and r < 80 and g < 80 and b < 80:
                pixels.append((x, y))
    return pixels


def principal_angle_degrees(points):
    mean_x = sum(x for x, _ in points) / len(points)
    mean_y = sum(y for _, y in points) / len(points)
    cov_xx = sum((x - mean_x) ** 2 for x, _ in points) / len(points)
    cov_yy = sum((y - mean_y) ** 2 for _, y in points) / len(points)
    cov_xy = sum((x - mean_x) * (y - mean_y) for x, y in points) / len(points)
    angle = 0.5 * math.atan2(2.0 * cov_xy, cov_xx - cov_yy)
    return abs(math.degrees(angle))


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_text_rotate_regression.py <png-path>")

    width, height, rows = read_png(sys.argv[1])
    points = dark_pixels(rows, width, height)
    if len(points) < 200:
        raise AssertionError(f"not enough dark text pixels: {len(points)}")

    xs = [x for x, _ in points]
    ys = [y for _, y in points]
    bbox_width = max(xs) - min(xs) + 1
    bbox_height = max(ys) - min(ys) + 1
    angle = principal_angle_degrees(points)

    if not (15.0 <= angle <= 45.0):
        raise AssertionError(f"text principal angle should be rotated, got {angle:.2f}")
    if bbox_height < 70:
        raise AssertionError(f"rotated text bbox is too short: {bbox_height}")
    if bbox_width < 130:
        raise AssertionError(f"text bbox is too narrow: {bbox_width}")

    print(
        f"text_rotate_regression ok angle={angle:.2f} "
        f"bbox={bbox_width}x{bbox_height}"
    )


if __name__ == "__main__":
    main()
