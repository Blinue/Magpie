#!/usr/bin/env python3
#
# Copyright (C) 2024
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import struct
from enum import IntEnum, IntFlag
from ctypes import c_uint32, sizeof, Structure
from typing import Any

DDS_MAGIC = "DDS "


class DDS_HEADER_FLAGS(IntFlag):
    CAPS = 0x1
    HEIGHT = 0x2
    WIDTH = 0x4
    PITCH = 0x8
    PIXELFORMAT = 0x1000
    MIPMAPCOUNT = 0x20000
    LINEARSIZE = 0x80000
    DEPTH = 0x800000


class DDS_CAPS(IntFlag):
    COMPLEX = 0x8
    TEXTURE = 0x1000
    MIPMAP = 0x400000


class DDS_CAPS2(IntFlag):
    CUBEMAP = 0x200
    CUBEMAP_POSITIVEX = 0x400
    CUBEMAP_NEGATIVEX = 0x800
    CUBEMAP_POSITIVEY = 0x1000
    CUBEMAP_NEGATIVEY = 0x2000
    CUBEMAP_POSITIVEZ = 0x4000
    CUBEMAP_NEGATIVEZ = 0x8000
    VOLUME = 0x200000


class DDS_PIXELFORMAT_FLAGS(IntFlag):
    ALPHAPIXELS = 0x1
    ALPHA = 0x2
    FOURCC = 0x4
    RGB = 0x40
    YUV = 0x200
    LUMINANCE = 0x20000


class DDS_FOURCC(IntEnum):
    R16G16B16A16_FLOAT = 0x71
    R32G32B32A32_FLOAT = 0x74


PIXEL_TYPE_MAP = {
    DDS_FOURCC.R16G16B16A16_FLOAT: "e",
    DDS_FOURCC.R32G32B32A32_FLOAT: "f",
}


class DDS_PIXELFORMAT(Structure):
    _fields_ = [
        ("size", c_uint32),
        ("flags", c_uint32),
        ("fourCC", c_uint32),
        ("RGBBitCount", c_uint32),
        ("RBitMask", c_uint32),
        ("GBitMask", c_uint32),
        ("BBitMask", c_uint32),
        ("ABitMask", c_uint32),
    ]

    def __init__(self, *args: Any, **kw: Any) -> None:
        super().__init__(*args, **kw)

        self.size = sizeof(DDS_PIXELFORMAT)
        assert self.size == 32

        self.flags = DDS_PIXELFORMAT_FLAGS.FOURCC


class DDS_HEADER(Structure):
    _fields_ = [
        ("size", c_uint32),
        ("flags", c_uint32),
        ("height", c_uint32),
        ("width", c_uint32),
        ("pitchOrLinearSize", c_uint32),
        ("depth", c_uint32),
        ("mipMapCount", c_uint32),
        ("reserved1", c_uint32 * 11),
        ("ddspf", DDS_PIXELFORMAT),
        ("caps", c_uint32),
        ("caps2", c_uint32),
        ("caps3", c_uint32),
        ("caps4", c_uint32),
        ("reserved2", c_uint32),
    ]

    def __init__(self, *args: Any, **kw: Any) -> None:
        super().__init__(*args, **kw)

        self.size = sizeof(DDS_HEADER)
        assert self.size == 124

        self.flags = (
            DDS_HEADER_FLAGS.CAPS
            | DDS_HEADER_FLAGS.HEIGHT
            | DDS_HEADER_FLAGS.WIDTH
            | DDS_HEADER_FLAGS.PITCH
            | DDS_HEADER_FLAGS.PIXELFORMAT
            | DDS_HEADER_FLAGS.MIPMAPCOUNT
        )
        self.depth = 1
        self.mipMapCount = 1
        self.ddspf = DDS_PIXELFORMAT()
        self.caps = DDS_CAPS.TEXTURE


class DDSTexture(object):
    _data = bytes()
    magic = bytes(DDS_MAGIC, "ascii")

    def __init__(self) -> None:
        self.header = DDS_HEADER()

    def __bytes__(self) -> bytes:
        ret = self.magic + bytearray(self.header) + self.data
        assert (
            len(self.data)
            == self.header.width * self.header.height * self.bytes_per_pixel
        )
        assert len(ret) == 4 + 124 + len(self.data)
        return ret

    @property
    def bytes_per_pixel(self) -> int:
        return struct.calcsize("<1%s" % self.pack_type) * 4

    @property
    def data(self) -> bytes:
        return self._data

    @data.setter
    def data(self, input: "list[float]") -> None:
        self._data = struct.pack("<%d%s" % (len(input), self.pack_type), *input)

    @property
    def format(self) -> str:
        return DDS_FOURCC(self.header.ddspf.fourCC).name

    @format.setter
    def format(self, input: str) -> None:
        self.header.ddspf.fourCC = DDS_FOURCC[input]

    @property
    def height(self) -> int:
        return self.header.height

    @height.setter
    def height(self, input: int) -> None:
        self.header.height = input

    @property
    def pack_type(self) -> str:
        return PIXEL_TYPE_MAP[self.header.ddspf.fourCC]

    @property
    def width(self) -> int:
        return self.header.width

    @width.setter
    def width(self, input: int) -> None:
        self.header.width = input

        # https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide#:~:text=For%20other%20formats%2C-,compute%20the%20pitch%20as
        bits_per_pixel = self.bytes_per_pixel * 8
        pitch = (self.header.width * bits_per_pixel + 7) // 8
        self.header.pitchOrLinearSize = pitch
