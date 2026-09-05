#!/usr/bin/env python3
"""Regenerate the reference images in test/data/.

The unit tests compare libuvc's converters against these files rather than
against hashes of libuvc's own past output, so the expected pixels have to
come from somewhere independent: ImageMagick performs the YCbCr to RGB
conversion here.

Requires ImageMagick 7 (`magick`). Output is deterministic -- ImageMagick's
-seed fixes the noise generator -- so rerunning this reproduces the committed
files byte for byte, and `git diff` shows whether anything moved.

Files written, all for one 64x32 frame:

    plasma_64x32.yuyv   source frame, packed 4:2:2 YUYV  [Y0 Cb Y1 Cr]
    plasma_64x32.uyvy   the same pixels packed as UYVY   [Cb Y0 Cr Y1]
    plasma_64x32.rgb    what it decodes to, per ImageMagick
    plasma_64x32.bgr    the same, channels swapped
    plasma_64x32.y      the luma plane
    plasma_64x32.uv     the interleaved chroma bytes

and, for the MJPEG decoder:

    plasma_64x32.jpg        a 64x32 JPEG of the same image
    plasma_64x32.jpg.rgb    what it decodes to, per ImageMagick
    plasma_64x32.jpg.gray   the same, as GRAY8
    oversize_128x64.jpg     a JPEG whose real size exceeds the frame's
    oversize_64x256.jpg     geometry, for the bounds tests

JPEG is lossy, so the .jpg.rgb reference cannot be compared byte for byte
against the YUYV path; it is only meaningful against the same decoder.
"""

import pathlib
import subprocess

WIDTH = 64
HEIGHT = 32
SEED = 42
BASE = f"plasma_{WIDTH}x{HEIGHT}"

HERE = pathlib.Path(__file__).resolve().parent


def magick(args, stdin=None):
    """Run ImageMagick and return its stdout."""
    proc = subprocess.run(["magick", *args], input=stdin,
                          stdout=subprocess.PIPE, check=True)
    return proc.stdout


def make_source_ycbcr():
    """A plasma frame, one Y, Cb and Cr per pixel."""
    data = magick([
        "-size", f"{WIDTH}x{HEIGHT}", "-seed", str(SEED), "plasma:fractal",
        "-colorspace", "Rec601YCbCr", "-depth", "8", "rgb:-",
    ])
    assert len(data) == WIDTH * HEIGHT * 3, len(data)
    return data


def pack_yuyv(ycbcr):
    """Subsample to 4:2:2 and pack as YUYV.

    A camera averages the chroma of each horizontal pixel pair into one
    Cb/Cr; do the same so the fixture is shaped like real camera output.
    """
    out = bytearray()
    for y in range(HEIGHT):
        for x in range(0, WIDTH, 2):
            i0 = (y * WIDTH + x) * 3
            i1 = (y * WIDTH + x + 1) * 3
            out += bytes([
                ycbcr[i0],                              # Y0
                (ycbcr[i0 + 1] + ycbcr[i1 + 1]) // 2,   # Cb, averaged
                ycbcr[i1],                              # Y1
                (ycbcr[i0 + 2] + ycbcr[i1 + 2]) // 2,   # Cr, averaged
            ])
    return bytes(out)


def pack_uyvy(yuyv):
    """The same pixels in UYVY order: [Cb Y0 Cr Y1]."""
    out = bytearray()
    for i in range(0, len(yuyv), 4):
        y0, cb, y1, cr = yuyv[i:i + 4]
        out += bytes([cb, y0, cr, y1])
    return bytes(out)


def expand_chroma(yuyv):
    """Undo the 4:2:2 packing, replicating each Cb/Cr across its pixel pair.

    This is what the converters do internally, so ImageMagick decodes exactly
    the values libuvc sees -- otherwise the reference would be measuring the
    subsampling rather than the conversion.
    """
    out = bytearray()
    for i in range(0, len(yuyv), 4):
        y0, cb, y1, cr = yuyv[i:i + 4]
        out += bytes([y0, cb, cr, y1, cb, cr])
    return bytes(out)


def decode_rgb(ycbcr):
    """Have ImageMagick convert per-pixel YCbCr to RGB.

    -set colorspace tags the incoming bytes without touching them; the
    following -colorspace is what actually converts. Using -colorspace for
    both is a no-op and silently returns the input unchanged.
    """
    data = magick([
        "-size", f"{WIDTH}x{HEIGHT}", "-depth", "8", "rgb:-",
        "-set", "colorspace", "Rec601YCbCr",
        "-colorspace", "sRGB", "-depth", "8", "rgb:-",
    ], stdin=ycbcr)
    assert len(data) == WIDTH * HEIGHT * 3, len(data)
    return data


def make_jpeg(size, seed, quality=90):
    """A JPEG of a plasma frame at the given size."""
    return magick([
        "-size", size, "-seed", str(seed), "plasma:fractal",
        "-quality", str(quality), "jpg:-",
    ])


def decode_jpeg(jpeg, colorspace):
    """Decode a JPEG to raw bytes in the given colorspace.

    Grayscale uses -grayscale Rec601Luma, not -colorspace Gray: libjpeg's
    JCS_GRAYSCALE hands back the JPEG's own Y channel, which is Rec601
    luma, whereas -colorspace Gray computes linear-light luminance. The two
    differ by up to 18 levels on this image, which would swamp any
    reasonable tolerance.
    """
    args = ["-depth", "8", "jpg:-"]
    if colorspace == "gray":
        args += ["-grayscale", "Rec601Luma", "-depth", "8", "gray:-"]
    else:
        args += ["-depth", "8", "rgb:-"]
    return magick(args, stdin=jpeg)


def swap_rgb_bgr(rgb):
    out = bytearray()
    for i in range(0, len(rgb), 3):
        out += bytes([rgb[i + 2], rgb[i + 1], rgb[i]])
    return bytes(out)


def build():
    """Return {filename: contents} for every fixture."""
    ycbcr = make_source_ycbcr()
    yuyv = pack_yuyv(ycbcr)
    rgb = decode_rgb(expand_chroma(yuyv))

    return {
        f"{BASE}.yuyv": yuyv,
        f"{BASE}.uyvy": pack_uyvy(yuyv),
        f"{BASE}.rgb": rgb,
        f"{BASE}.bgr": swap_rgb_bgr(rgb),
        # These two converters only demultiplex, so the expected bytes come
        # straight from the source frame: luma is every even byte, chroma
        # every odd one.
        f"{BASE}.y": bytes(yuyv[i] for i in range(0, len(yuyv), 2)),
        f"{BASE}.uv": bytes(yuyv[i + 1] for i in range(0, len(yuyv), 2)),
        **jpeg_files(),
    }


def jpeg_files():
    """Fixtures for the MJPEG decoder.

    The oversized frames exist for the bounds tests: libuvc sizes the output
    buffer from the frame's declared width and height, but libjpeg decodes to
    whatever the JPEG header says, so a frame that understates its size makes
    the decoder write past the end of the buffer.
    """
    jpeg = make_jpeg(f"{WIDTH}x{HEIGHT}", SEED)
    return {
        f"{BASE}.jpg": jpeg,
        f"{BASE}.jpg.rgb": decode_jpeg(jpeg, "rgb"),
        f"{BASE}.jpg.gray": decode_jpeg(jpeg, "gray"),
        "oversize_128x64.jpg": make_jpeg("128x64", 11),
        "oversize_64x256.jpg": make_jpeg("64x256", 7),
    }


def main():
    for name, content in sorted(build().items()):
        (HERE / name).write_bytes(content)
        print(f"wrote {name} ({len(content)} bytes)")


if __name__ == "__main__":
    main()
