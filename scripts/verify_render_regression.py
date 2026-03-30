#!/usr/bin/env python3

import struct
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
        raise ValueError(f"{path}: unsupported PNG format {bit_depth=}, {color_type=}")

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


def pixel(rows, x, y):
    offset = x * 4
    return tuple(rows[y][offset : offset + 4])


def expect_equal(actual, expected, label):
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected}, got {actual}")


def expect_color_range(color, expected, tolerance, label):
    for index, component in enumerate(("r", "g", "b", "a")):
        if abs(color[index] - expected[index]) > tolerance:
            raise AssertionError(
                f"{label}: expected approx {expected}, got {color}"
            )


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: verify_render_regression.py <png-path>")

    width, height, rows = read_png(sys.argv[1])
    expect_equal((width, height), (660, 360), "output size")

    background = (226, 232, 240, 255)
    dark = (15, 23, 42, 255)
    orange = (249, 115, 22, 255)
    red = (239, 68, 68, 255)
    teal = (20, 184, 166, 255)
    purple = (139, 92, 246, 255)
    green = (34, 197, 94, 255)
    sky = (14, 165, 233, 255)

    expect_color_range(pixel(rows, 10, 10), background, 2, "background corner")
    expect_color_range(pixel(rows, 60, 60), dark, 4, "first image dark region")
    expect_color_range(pixel(rows, 240, 240), orange, 8, "first image circle region")
    expect_color_range(pixel(rows, 432, 192), orange, 8, "scaled image center")
    expect_color_range(pixel(rows, 534, 126), dark, 6, "cropped image region")
    expect_color_range(pixel(rows, 240, 48), dark, 6, "image data blit")
    expect_color_range(pixel(rows, 132, 264), red, 10, "quadratic curve stroke")
    expect_color_range(pixel(rows, 330, 282), teal, 8, "ellipse fill")
    expect_color_range(pixel(rows, 564, 276), purple, 8, "arcTo fill")
    expect_color_range(pixel(rows, 552, 18), green, 8, "requestAnimationFrame queue")
    expect_color_range(pixel(rows, 600, 18), sky, 8, "setTimeout queue")

    print("render_regression ok")


if __name__ == "__main__":
    main()
