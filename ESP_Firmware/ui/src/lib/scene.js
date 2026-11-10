// Pure geometry and motion for the aquarium background, styled after the
// ScapeSense logo: lance-shaped leaves with an off-centre vein cut-out, a wavy
// substrate and a silhouetted fish with an eye cut-out.

export const FISH_PATHS = [
  'M50 0C46-13 30-22 10-21C-6-20-20-12-30-4L-54-20C-46-7-46 7-54 20L-30 4C-20 12-6 18 10 18C30 18 46 12 50 0Z',
  'M-4-19L-10-34C4-33 18-29 24-20Z',
  'M-10 15C-15 25-8 30-1 28C-2 22-4 18-10 15Z',
  'M16 16C14 25 21 28 28 23C25 20 22 17 16 16Z',
];
export const FISH_EYE = { x: 36, y: -5, r: 3.4 };
export const FISH_LENGTH = 100;

const BUBBLE_PALETTES = {
  dark: { tints: ['#9ef2dd', '#7ad9f0', '#d9f6f0'], opacity: 1, highlight: '#ffffff' },
  light: { tints: ['#246c61', '#2e6e85', '#55733a'], opacity: 1.6, highlight: '#153f37' },
};

export function bubblePalette(theme) {
  return BUBBLE_PALETTES[theme === 'light' ? 'light' : 'dark'];
}

export function createRng(seed = 1) {
  let state = seed >>> 0;
  return () => {
    state = (state + 0x6d2b79f5) >>> 0;
    let value = state;
    value = Math.imul(value ^ (value >>> 15), value | 1);
    value ^= value + Math.imul(value ^ (value >>> 7), value | 61);
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
}

const clamp = (value, min, max) => Math.min(max, Math.max(min, value));
const between = (rng, min, max) => min + rng() * (max - min);

export function plantScale(height) {
  return clamp(height * 0.42, 150, 420);
}

function cluster(rng, { x, ground, scale, count, from, to }) {
  return Array.from({ length: count }, (_, index) => {
    const spread = index / (count - 1);
    const centre = 1 - Math.abs(spread - 0.5) * 2;
    return {
      x: x + between(rng, -0.06, 0.06) * scale,
      y: ground,
      angle: from + (to - from) * spread + between(rng, -0.08, 0.08),
      stem: scale * (0.18 + centre * 0.28 + between(rng, 0, 0.08)),
      length: scale * (0.36 + centre * 0.22 + between(rng, 0, 0.08)),
      width: scale * between(rng, 0.16, 0.21),
      bend: between(rng, -0.5, 0.5),
      phase: between(rng, 0, Math.PI * 2),
      speed: between(rng, 0.45, 0.8),
    };
  });
}

export function layoutPlants(width, height, rng = createRng(7)) {
  const scale = plantScale(height);
  const ground = height - substrateHeight(height) * 0.35;
  const left = cluster(rng, {
    x: clamp(Math.max(width * 0.1, scale * 0.55), 90, 240),
    ground,
    scale,
    count: 7,
    from: -0.55,
    to: 0.65,
  });
  if (width < 640) return left;
  const right = cluster(rng, {
    x: width - clamp(Math.max(width * 0.06, scale * 0.4), 60, 200),
    ground,
    scale: scale * 0.62,
    count: 4,
    from: -0.5,
    to: 0.85,
  });
  return [...left, ...right];
}

export function leafSway(leaf, t) {
  return Math.sin(t * leaf.speed + leaf.phase) * 0.035;
}

// Local geometry for one leaf, with its base at the origin pointing up.
// Leaves leaning left are mirrored so the blade always curls outwards.
// The stem is an S-curve; the blade is a curled lance with an off-centre vein.
export function leafShape(leaf) {
  const { stem, length: L, width: w, bend } = leaf;
  const tip = { x: bend * stem * 0.35, y: -stem };
  return {
    mirrored: leaf.angle < 0,
    stem: {
      control: { x: -tip.x * 0.6, y: -stem * 0.55 },
      tip,
      lineWidth: Math.max(2, w * 0.13),
    },
    bladeRotation: bend * 0.6,
    blade: [
      [w * 1.0, -L * 0.25, w * 0.9, -L * 0.7, w * 0.45, -L],
      [w * 0.05, -L * 0.8, -w * 0.75, -L * 0.45, 0, 0],
    ],
    vein: {
      from: { x: w * 0.16, y: -L * 0.1 },
      control: { x: w * 0.62, y: -L * 0.5 },
      to: { x: w * 0.42, y: -L * 0.88 },
      lineWidth: Math.max(1.2, w * 0.07),
    },
  };
}

export function substrateHeight(height) {
  return clamp(height * 0.07, 36, 80);
}

// Two bands like the logo: a solid rolling hill and a thinner wave ribbon above
// it, separated by a gap. Points run left to right across the full width.
export function substrateBands(width, height, steps = 48) {
  const depth = substrateHeight(height);
  const hill = [];
  const ribbon = [];
  for (let i = 0; i <= steps; i += 1) {
    const x = (width * i) / steps;
    const u = i / steps;
    const swell = Math.sin(u * Math.PI * 2.6 + 0.9);
    hill.push({ x, y: height - depth * (0.42 + 0.3 * swell) });
    ribbon.push({ x, y: height - depth * (1.05 + 0.35 * Math.sin(u * Math.PI * 2.6 + 0.2)) });
  }
  return { hill, ribbon, thickness: depth * 0.16 };
}

export function createFish(rng = createRng(11), count = 3) {
  return Array.from({ length: count }, (_, index) => ({
    size: between(rng, 0.55, 1.1),
    depth: 0.22 + (0.5 * index) / Math.max(1, count - 1) + between(rng, -0.04, 0.04),
    speed: between(rng, 14, 30),
    offset: between(rng, 0, 4000),
    bob: between(rng, 6, 16),
    phase: between(rng, 0, Math.PI * 2),
  }));
}

// Fish swim the full width, turn while off-screen and swim back.
export function fishPose(fish, t, width, height) {
  const length = FISH_LENGTH * fish.size;
  const margin = length;
  const span = width + margin * 2;
  const travelled = (t * fish.speed + fish.offset) % (span * 2);
  const outbound = travelled < span;
  const x = outbound ? travelled - margin : width + margin - (travelled - span);
  const wave = t * 0.55 + fish.phase;
  return {
    x,
    y: fish.depth * height + Math.sin(wave) * fish.bob,
    direction: outbound ? 1 : -1,
    scale: fish.size,
    tilt: Math.cos(wave) * 0.06,
  };
}
