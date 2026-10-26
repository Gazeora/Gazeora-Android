"""Renders the 360-degree scene the app floats Gazeora's window in when the camera can't be used:
a lake at dusk under the first stars, mountains around it. Equirectangular, seamless, made from
nothing but this script, so it can be tweaked and rendered again:

    pip install -r tools/panorama/requirements.txt
    python tools/panorama/render_panorama.py

The middle of the image is straight ahead (where the window first appears); the top is up.
"""

import argparse
import math
from pathlib import Path

import numpy as np
from PIL import Image

WIDTH = 4096  # power of two: the app wraps it around with GL_REPEAT on OpenGL ES 2
HEIGHT = WIDTH // 2
SEED = 7
OUTPUT = Path(__file__).resolve().parents[2] / "app/src/main/assets/room/panorama.jpg"
JPEG_QUALITY = 92


def srgb(hex_color):
    """A hex sRGB color as linear RGB."""
    c = np.array([int(hex_color[i:i + 2], 16) / 255.0 for i in (1, 3, 5)])
    return np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)


def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / (edge1 - edge0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


class Grid:
    """Each pixel's direction: longitude (0 ahead, positive to the right) and latitude (up)."""

    def __init__(self, width, height):
        self.width, self.height = width, height
        u = (np.arange(width) + 0.5) / width
        v = (np.arange(height) + 0.5) / height
        self.lon_1d = (u - 0.5) * 2.0 * math.pi
        self.lat_1d = (0.5 - v) * math.pi
        self.lon, self.lat = np.meshgrid(self.lon_1d, self.lat_1d)
        self.pixel = math.pi / height  # one pixel's angle
        cl = np.cos(self.lat)
        self.dirs = np.stack([cl * np.sin(self.lon), np.sin(self.lat), -cl * np.cos(self.lon)], -1)


class Noise:
    """Smooth value noise: periodic in 1D (around the horizon) and 3D (on the sphere)."""

    def __init__(self, rng):
        self.table = rng.random(1 << 16)

    def _hash(self, *ints):
        h = np.zeros_like(ints[0], dtype=np.int64)
        for i, p in zip(ints, (73856093, 19349663, 83492791)):
            h ^= (i.astype(np.int64) * p) & 0xFFFFFFFF
        return self.table[h & 0xFFFF]

    def periodic(self, lon, cells, seed):
        """1D noise of longitude, `cells` lattice points around the circle."""
        x = (lon / (2 * math.pi) + 0.5) * cells
        i = np.floor(x).astype(np.int64)
        f = x - i
        f = f * f * (3 - 2 * f)
        a = self._hash(i % cells, np.full_like(i, seed))
        b = self._hash((i + 1) % cells, np.full_like(i, seed))
        return a + (b - a) * f

    def ridged(self, lon, cells, octaves, seed):
        """Sharp-crested fBm for mountain ridges, 0..1."""
        total, amp, norm = 0.0, 1.0, 0.0
        for o in range(octaves):
            n = self.periodic(lon, cells << o, seed + o)
            total = total + amp * (1.0 - np.abs(2.0 * n - 1.0)) ** 2
            norm += amp
            amp *= 0.5
        return total / norm

    def solid(self, p, scale, octaves, seed):
        """3D fBm at points p (..., 3), 0..1."""
        total, amp, norm = 0.0, 1.0, 0.0
        for o in range(octaves):
            q = p * scale * (2 ** o)
            i = np.floor(q).astype(np.int64)
            f = q - i
            f = f * f * f * (f * (f * 6 - 15) + 10)  # quintic: no creases at the lattice
            salt = (seed + o) * 1000003  # a different lattice per octave
            acc = 0.0
            for dx in (0, 1):
                for dy in (0, 1):
                    for dz in (0, 1):
                        w = ((f[..., 0] if dx else 1 - f[..., 0]) *
                             (f[..., 1] if dy else 1 - f[..., 1]) *
                             (f[..., 2] if dz else 1 - f[..., 2]))
                        acc = acc + w * self._hash(i[..., 0] + dx, i[..., 1] + dy,
                                                   i[..., 2] + dz + salt)
            total = total + amp * acc
            norm += amp
            amp *= 0.5
        return total / norm


class Sky:
    """Blue hour: deep indigo overhead, warm where the sun has just set."""

    SUN_LON = math.radians(38.0)
    SUN_LAT = math.radians(-5.0)

    def __init__(self):
        self.zenith = srgb("#070b24")
        self.upper = srgb("#141d4c")
        self.middle = srgb("#323a78")
        self.low = srgb("#6f5f96")
        self.horizon_cool = srgb("#8a7fae")
        self.horizon_warm = srgb("#f39a62")
        self.glow = srgb("#ffd59a")
        sl = math.cos(self.SUN_LAT)
        self.sun = np.array([sl * math.sin(self.SUN_LON), math.sin(self.SUN_LAT),
                             -sl * math.cos(self.SUN_LON)])

    def warmth(self, dirs, lat):
        """How much the sunset tints the sky here, 0..1."""
        toward = np.clip(dirs @ self.sun, -1, 1)
        return np.exp(-(1 - toward) / 0.35) * np.exp(-np.maximum(lat, 0) / 0.22)

    def color(self, dirs, lat):
        e = np.clip(lat / (math.pi / 2), 0, 1)[..., None]
        stops = [(0.0, self.horizon_cool), (0.06, self.low), (0.22, self.middle),
                 (0.5, self.upper), (1.0, self.zenith)]
        out = np.zeros(lat.shape + (3,))
        for (e0, c0), (e1, c1) in zip(stops, stops[1:]):
            t = np.clip((e - e0) / (e1 - e0), 0, 1)
            inside = (e >= e0) & (e <= e1)
            out = np.where(inside, c0 + (c1 - c0) * smoothstep(0, 1, t), out)
        w = self.warmth(dirs, lat)[..., None]
        out = out + (self.horizon_warm - out) * np.clip(w * 1.1, 0, 1)
        toward = np.clip(dirs @ self.sun, -1, 1)
        glow = np.exp(-(1 - toward) / 0.04) * np.exp(-np.maximum(lat, 0) / 0.06)
        return out + self.glow * glow[..., None] * 0.9


class Stars:
    """Stars, brighter higher up, faded where the sky is still bright; a faint Milky Way."""

    COUNT = 3000

    def __init__(self, rng, noise):
        self.rng, self.noise = rng, noise
        tilt = math.radians(62)
        self.band_normal = np.array([math.sin(tilt) * 0.6, math.cos(tilt), math.sin(tilt) * 0.8])
        self.band_normal /= np.linalg.norm(self.band_normal)

    def milky_way(self, grid, sky):
        across = grid.dirs @ self.band_normal
        band = np.exp(-(across ** 2) / (2 * 0.16 ** 2))
        clouds = self.noise.solid(grid.dirs, 4.0, 5, 11)
        lanes = smoothstep(0.45, 0.65, self.noise.solid(grid.dirs, 8.0, 4, 23))
        light = band * (0.25 + 0.75 * clouds ** 2) * (1 - 0.6 * lanes * band)
        fade = smoothstep(math.radians(8), math.radians(45), grid.lat)
        fade = fade * (1 - np.clip(sky.warmth(grid.dirs, grid.lat) * 3, 0, 1))
        return (light * fade)[..., None] * srgb("#4a4f78") * 0.45

    def draw(self, image, grid, sky):
        height, width = grid.height, grid.width
        z = self.rng.uniform(math.sin(math.radians(3)), 1, self.COUNT)
        lon = self.rng.uniform(-math.pi, math.pi, self.COUNT)
        lat = np.arcsin(z)
        bright = np.clip(self.rng.pareto(2.6, self.COUNT) * 0.11 + 0.04, 0, 1.4)
        tints = [srgb("#cfd8ff"), srgb("#ffffff"), srgb("#fff0d6"), srgb("#ffd9b0")]
        cl = np.cos(lat)
        dirs = np.stack([cl * np.sin(lon), np.sin(lat), -cl * np.cos(lon)], -1)
        fade = smoothstep(math.radians(4), math.radians(35), lat)
        fade = fade * (1 - np.clip(sky.warmth(dirs, lat) * 4, 0, 1))
        for k in range(self.COUNT):
            if fade[k] * bright[k] < 0.01:
                continue
            x = (lon[k] / (2 * math.pi) + 0.5) * width - 0.5
            y = (0.5 - lat[k] / math.pi) * height - 0.5
            sigma = (0.5 + 0.3 * min(bright[k], 1.0)) * width / WIDTH
            sx = sigma / max(cl[k], 0.05)  # equirect stretches toward the poles
            rx, ry = int(math.ceil(3 * sx)), int(math.ceil(3 * sigma))
            xs = np.arange(int(x) - rx, int(x) + rx + 2)
            ys = np.arange(max(int(y) - ry, 0), min(int(y) + ry + 2, height))
            gx = np.exp(-((xs - x) ** 2) / (2 * sx * sx))
            gy = np.exp(-((ys - y) ** 2) / (2 * sigma * sigma))
            spot = (gy[:, None] * gx[None, :])[..., None]
            color = tints[self.rng.integers(len(tints))] * bright[k] * fade[k] * (WIDTH / width)
            image[np.ix_(ys, xs % width)] += spot * color


class Moon:
    """A thin crescent, lit from the sunset side, with a soft halo."""

    LON = math.radians(-46.0)
    LAT = math.radians(21.0)
    RADIUS = math.radians(0.85)

    def draw(self, image, grid):
        dx = (grid.lon - self.LON) * math.cos(self.LAT)
        dy = grid.lat - self.LAT
        r = np.hypot(dx, dy)
        aa = grid.pixel
        disk = smoothstep(self.RADIUS + aa, self.RADIUS - aa, r)
        # The shadow disk sits away from the sun (to the left, a little up).
        sx, sy = dx + 0.42 * self.RADIUS, dy - 0.12 * self.RADIUS
        shadow = smoothstep(self.RADIUS * 0.97 - aa * 2, self.RADIUS * 0.97 + aa * 2,
                            np.hypot(sx, sy))
        lit = disk * shadow
        halo = np.exp(-r / math.radians(2.2)) * 0.05 + np.exp(-r / math.radians(9)) * 0.015
        moon = srgb("#fff3d8")
        image += (lit * 1.6 + disk * (1 - shadow) * 0.035 + halo)[..., None] * moon


class Mountains:
    """Ranges around the lake, far ones hazy and snow-capped, near ones dark."""

    # base and height in degrees, lattice cells, octaves, color, haze, snow
    LAYERS = [
        (1.5, 15.0, 6, 8, "#3b3462", 0.42, True),
        (1.0, 9.0, 9, 8, "#2a2850", 0.4, False),
        (0.5, 4.5, 14, 7, "#141631", 0.2, False),
    ]

    def __init__(self, noise):
        self.noise = noise

    def draw(self, image, grid, sky):
        lon = grid.lon_1d
        for index, (base, height, cells, octaves, color, haze, snow) in enumerate(self.LAYERS):
            seed = 100 + index * 17
            ridge = self.noise.ridged(lon, cells, octaves, seed)
            ridge = (ridge - ridge.min()) / (ridge.max() - ridge.min())
            # Broad swells decide where the peaks rise and where the valleys open up.
            swell = smoothstep(0.15, 0.85, self.noise.periodic(lon, 5 + index * 2, seed + 50))
            top = np.radians(base + height * ridge ** 1.4 * (0.3 + 0.7 * swell))
            cover = np.clip((top[None, :] - grid.lat) / grid.pixel + 0.5, 0, 1)
            cover = cover * (grid.lat > -grid.pixel)
            depth = np.clip((top[None, :] - grid.lat) / np.maximum(top[None, :], 1e-4), 0, 1)
            rock = srgb(color) * (1.08 - 0.35 * depth[..., None])
            horizon = sky.color(grid.dirs, np.zeros_like(grid.lat))
            rock = rock + (horizon - rock) * haze * (1 - 0.5 * depth[..., None])
            # Ridges facing the afterglow catch a little of it.
            rim = np.exp(-depth / 0.05) * sky.warmth(grid.dirs, np.zeros_like(grid.lat))
            rock = rock + srgb("#f2a37a") * (rim * 0.25)[..., None]
            if snow:
                rock = self._snow(rock, grid, top, depth, sky, seed)
            image[:] = image + (rock - image) * cover[..., None]

    def _snow(self, rock, grid, top, depth, sky, seed):
        # On the high peaks only, down from the crest, broken up by gullies.
        line = np.radians(8.0)
        peaks = smoothstep(line, line + np.radians(3), top)[None, :]
        gullies = self.noise.periodic(grid.lon_1d, 1500, seed + 7)
        gullies = gullies + 0.5 * self.noise.periodic(grid.lon_1d, 4000, seed + 8)
        reach = 0.05 + 0.14 * gullies[None, :]
        amount = peaks * smoothstep(reach, reach * 0.4, depth)
        glow = 0.25 + 0.6 * sky.warmth(grid.dirs, np.zeros_like(grid.lat))
        snow = srgb("#d8c3e0") * glow[..., None]
        return rock + (snow - rock) * (amount * 0.6)[..., None]


class Treeline:
    """Pines along the near shore, in clumps, as dark silhouettes."""

    def __init__(self, rng, noise):
        self.rng, self.noise = rng, noise

    def draw(self, image, grid):
        width, height = grid.width, grid.height
        cover = np.zeros((height, width))
        clumps = self.noise.periodic(grid.lon_1d, 24, 300)
        # A low bank of shrubs everywhere the trees grow.
        bank = np.radians(0.3 + 0.5 * smoothstep(0.45, 0.7, clumps))
        bank = bank * (0.8 + 0.4 * self.noise.periodic(grid.lon_1d, 900, 301))
        cover = np.maximum(cover, np.clip((bank[None, :] - grid.lat) / grid.pixel + 0.5, 0, 1)
                           * (grid.lat > -grid.pixel))
        for x in range(0, width, 2):
            density = smoothstep(0.5, 0.75, clumps[x])
            if self.rng.random() > density * 0.55:
                continue
            self._tree(cover, grid, x + self.rng.random() * 2, bank[x])
        silhouette = srgb("#05060d")
        image[:] = image + (silhouette - image) * cover[..., None]

    def _tree(self, cover, grid, x, foot):
        size = self.rng.uniform(1.0, 3.0)
        tall = math.radians(size) / grid.pixel  # pixels
        half = tall * self.rng.uniform(0.16, 0.22)
        tiers = self.rng.integers(5, 9)
        y_foot = (0.5 - foot / math.pi) * grid.height
        x0, x1 = int(x - half - 2), int(x + half + 3)
        y0, y1 = max(int(y_foot - tall - 2), 0), int(y_foot + 1)
        ss = 4
        xs = np.arange(x0 * ss, x1 * ss) / ss + 0.5 / ss
        ys = np.arange(y0 * ss, y1 * ss) / ss + 0.5 / ss
        X, Y = np.meshgrid(xs, ys)
        up = np.clip((y_foot - Y) / tall, 0, 1)  # 0 at the foot, 1 at the tip
        tier = (up * tiers) % 1.0
        reach = half * (1 - up) * (0.55 + 0.45 * (1 - tier)) + 0.35
        inside = (np.abs(X - x) < reach) & (Y <= y_foot) & (Y >= y_foot - tall)
        trunk = (np.abs(X - x) < 0.6) & (Y <= y_foot) & (Y >= y_foot - tall * 0.15)
        fine = (inside | trunk).astype(float)
        coarse = fine.reshape(y1 - y0, ss, x1 - x0, ss).mean(axis=(1, 3))
        cols = np.arange(x0, x1) % grid.width
        rows = np.arange(y0, y1)
        cover[np.ix_(rows, cols)] = np.maximum(cover[np.ix_(rows, cols)], coarse)


class Lake:
    """Still water below the horizon: the scene above, mirrored and broken up by ripples."""

    EYE_HEIGHT = 1.6  # meters above the water

    def __init__(self, noise):
        self.noise = noise

    def draw(self, image, grid):
        water = grid.lat < 0
        depression = np.maximum(-grid.lat, 1e-4)
        distance = self.EYE_HEIGHT / np.tan(depression)
        across = distance * grid.lon
        p = np.stack([across / 1.8, distance / 0.35, np.zeros_like(distance)], -1)
        ripple = self.noise.solid(p, 1.0, 3, 77) - 0.5
        wobble = self.noise.solid(p * np.array([0.25, 0.25, 1]), 1.0, 2, 78) - 0.5
        dy = (ripple * 0.9 + wobble * 0.4) * depression * 0.35
        dx = wobble * depression * 0.05
        src_lat = np.clip(depression + dy, 0, math.pi / 2 - 1e-3)
        src_lon = grid.lon + dx
        mirrored = self._sample(image, grid, src_lon, src_lat)
        cos_incidence = np.sin(depression)
        fresnel = 0.02 + 0.98 * (1 - cos_incidence) ** 5
        body = srgb("#03070d")
        lake = body + (mirrored * 0.88 - body) * fresnel[..., None]
        image[water] = lake[water]

    @staticmethod
    def _sample(image, grid, lon, lat):
        """Bilinear lookup in the equirectangular image, wrapping around in longitude."""
        x = (lon / (2 * math.pi) + 0.5) * grid.width - 0.5
        y = np.clip((0.5 - lat / math.pi) * grid.height - 0.5, 0, grid.height - 1.001)
        x0, y0 = np.floor(x).astype(np.int64), np.floor(y).astype(np.int64)
        fx, fy = (x - x0)[..., None], (y - y0)[..., None]
        x0 %= grid.width
        x1 = (x0 + 1) % grid.width
        top = image[y0, x0] * (1 - fx) + image[y0, x1] * fx
        bottom = image[y0 + 1, x0] * (1 - fx) + image[y0 + 1, x1] * fx
        return top * (1 - fy) + bottom * fy


class PanoramaRenderer:
    """Draws the layers back to front, then encodes the image."""

    def __init__(self, width, height, seed):
        self.grid = Grid(width, height)
        self.rng = np.random.default_rng(seed)
        self.noise = Noise(self.rng)
        self.sky = Sky()

    def render(self):
        grid = self.grid
        image = self.sky.color(grid.dirs, grid.lat)
        image += Stars(self.rng, self.noise).milky_way(grid, self.sky)
        Stars(self.rng, self.noise).draw(image, grid, self.sky)
        Moon().draw(image, grid)
        Mountains(self.noise).draw(image, grid, self.sky)
        Treeline(self.rng, self.noise).draw(image, grid)
        Lake(self.noise).draw(image, grid)
        return self._encode(image)

    def _encode(self, linear):
        linear = linear / (1 + 0.15 * linear)  # roll off the brightest bits gently
        c = np.clip(linear, 0, 1)
        out = np.where(c <= 0.0031308, c * 12.92, 1.055 * c ** (1 / 2.4) - 0.055) * 255
        out += self.rng.uniform(-0.5, 0.5, out.shape)  # dither: no banding in the sky
        return Image.fromarray(np.clip(np.rint(out), 0, 255).astype(np.uint8), "RGB")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--width", type=int, default=WIDTH)
    parser.add_argument("--output", type=Path, default=OUTPUT)
    args = parser.parse_args()
    image = PanoramaRenderer(args.width, args.width // 2, SEED).render()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output, quality=JPEG_QUALITY, subsampling=0, optimize=True)
    print(f"{args.output} ({args.output.stat().st_size // 1024} KB)")


if __name__ == "__main__":
    main()
