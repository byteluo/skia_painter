#!/usr/bin/env python3

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
    for index in range(4):
        if abs(color[index] - expected[index]) > tolerance:
            raise AssertionError(
                f"{label}: expected approx {expected}, got {color}"
            )


def expect_region_contains_color(rows, left, top, right, bottom, expected, tolerance, label):
    for y in range(top, bottom + 1):
        for x in range(left, right + 1):
            color = pixel(rows, x, y)
            if all(abs(color[index] - expected[index]) <= tolerance for index in range(4)):
                return

    raise AssertionError(
        f"{label}: expected approx {expected} within region "
        f"({left}, {top})-({right}, {bottom})"
    )


def verify_sunburst(path):
    width, height, rows = read_png(path)
    expect_equal((width, height), (2880, 1620), "sunburst size")

    background = (255, 250, 240, 255)
    orange = (251, 146, 60, 255)
    blue = (59, 130, 246, 255)
    teal = (20, 184, 166, 255)

    expect_color_range(pixel(rows, 20, 20), background, 3, "sunburst background")
    expect_color_range(pixel(rows, 1440, 1032), background, 3, "sunburst center hole")
    expect_color_range(pixel(rows, 1440, 1314), orange, 10, "sunburst sales ring")
    expect_color_range(pixel(rows, 1872, 864), blue, 10, "sunburst search ring")
    expect_color_range(pixel(rows, 1200, 792), teal, 10, "sunburst retention ring")


def verify_timeline(path):
    width, height, rows = read_png(path)
    expect_equal((width, height), (2880, 1620), "timeline size")

    background = (248, 250, 252, 255)
    orange = (249, 115, 22, 255)
    blue = (101, 120, 186, 255)

    expect_color_range(pixel(rows, 20, 20), background, 3, "timeline background")
    expect_region_contains_color(
        rows,
        2536,
        156,
        2546,
        165,
        blue,
        8,
        "timeline active checkpoint",
    )
    expect_color_range(pixel(rows, 2300, 1280), orange, 8, "timeline q3 search bar")
    expect_color_range(pixel(rows, 2650, 1280), background, 3, "timeline right background")


def main():
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: verify_echarts_regression.py <sunburst-png> <timeline-png>"
        )

    verify_sunburst(sys.argv[1])
    verify_timeline(sys.argv[2])
    print("echarts_regression ok")


if __name__ == "__main__":
    main()
