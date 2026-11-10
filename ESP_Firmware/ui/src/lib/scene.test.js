import test from 'node:test';
import assert from 'node:assert/strict';
import {
  FISH_EYE,
  FISH_LENGTH,
  FISH_PATHS,
  bubblePalette,
  createFish,
  createRng,
  fishPose,
  layoutPlants,
  leafShape,
  leafSway,
  plantScale,
  substrateBands,
  substrateHeight,
} from './scene.js';

test('bubble palettes preserve dark mode and give light mode darker, stronger bubbles', () => {
  const dark = bubblePalette('dark');
  const light = bubblePalette('light');
  assert.deepEqual(dark, { tints: ['#9ef2dd', '#7ad9f0', '#d9f6f0'], opacity: 1, highlight: '#ffffff' });
  assert.equal(bubblePalette(undefined), dark);
  assert.equal(bubblePalette('unknown'), dark);
  assert.equal(light.tints.length, dark.tints.length);
  assert.ok(light.opacity > dark.opacity);
  assert.ok(0.38 * light.opacity <= 1);
  const brightness = (hex) => [1, 3, 5].reduce((sum, offset) => sum + parseInt(hex.slice(offset, offset + 2), 16), 0);
  for (let i = 0; i < light.tints.length; i++) assert.ok(brightness(light.tints[i]) < brightness(dark.tints[i]));
  assert.ok(brightness(light.highlight) < brightness(dark.highlight));
});

test('seeded random numbers are repeatable and within [0, 1)', () => {
  const a = createRng(42);
  const b = createRng(42);
  const values = Array.from({ length: 200 }, () => a());
  assert.deepEqual(values, Array.from({ length: 200 }, () => b()));
  assert.ok(values.every((value) => value >= 0 && value < 1));
  assert.notDeepEqual(values.slice(0, 5), Array.from({ length: 5 }, createRng(43)));
});

test('default seed produces values', () => {
  assert.equal(typeof createRng()(), 'number');
});

test('plant scale follows viewport height within limits', () => {
  assert.equal(plantScale(100), 150);
  assert.equal(plantScale(600), 252);
  assert.equal(plantScale(4000), 420);
});

test('narrow screens only get the bottom-left cluster', () => {
  const leaves = layoutPlants(390, 800, createRng(1));
  assert.equal(leaves.length, 7);
  assert.ok(leaves.every((leaf) => leaf.x < 390 / 2));
});

test('wide screens add a smaller bottom-right cluster', () => {
  const leaves = layoutPlants(1440, 900, createRng(1));
  assert.equal(leaves.length, 11);
  const left = leaves.slice(0, 7);
  const right = leaves.slice(7);
  assert.ok(left.every((leaf) => leaf.x < 300));
  assert.ok(right.every((leaf) => leaf.x > 1440 - 300));
  const maxLeft = Math.max(...left.map((leaf) => leaf.stem + leaf.length));
  const maxRight = Math.max(...right.map((leaf) => leaf.stem + leaf.length));
  assert.ok(maxRight < maxLeft);
});

test('leaves sit on the substrate and fan out from left to right', () => {
  const height = 900;
  const leaves = layoutPlants(1440, height, createRng(3)).slice(0, 7);
  for (const leaf of leaves) {
    assert.ok(leaf.y < height && leaf.y > height - substrateHeight(height));
    assert.ok(leaf.stem > 0 && leaf.length > 0 && leaf.width > 0);
    assert.ok(Math.abs(leaf.bend) <= 0.5);
    assert.ok(leaf.width / leaf.length > 0.2, 'leaves are broad like the logo');
  }
  assert.ok(leaves[0].angle < leaves[leaves.length - 1].angle);
});

test('plant layout uses a default seed', () => {
  assert.deepEqual(layoutPlants(390, 800), layoutPlants(390, 800));
});

test('plant layout is deterministic for a seed', () => {
  assert.deepEqual(layoutPlants(1200, 800, createRng(9)), layoutPlants(1200, 800, createRng(9)));
});

test('leaf shape grows from the stem tip with the vein inside the blade', () => {
  const leaf = { angle: 0.3, stem: 100, length: 120, width: 40, bend: 0.4 };
  const shape = leafShape(leaf);
  assert.equal(shape.mirrored, false);
  assert.deepEqual(shape.stem.tip, { x: 14, y: -100 });
  assert.equal(shape.blade.length, 2);
  assert.deepEqual(shape.blade.at(-1).slice(-2), [0, 0], 'blade closes at its base');
  assert.ok(Math.abs(shape.blade[0][5] + 120) < 1e-9, 'blade tip reaches full length');
  for (const point of [shape.vein.from, shape.vein.control, shape.vein.to]) {
    assert.ok(point.x > 0 && point.x < 40 && point.y < 0 && point.y > -120);
  }
  assert.ok(Math.abs(shape.bladeRotation - 0.24) < 1e-9);
});

test('left-leaning leaves are mirrored and line widths have minimums', () => {
  const shape = leafShape({ angle: -0.4, stem: 10, length: 10, width: 4, bend: 0 });
  assert.equal(shape.mirrored, true);
  assert.equal(shape.stem.lineWidth, 2);
  assert.equal(shape.vein.lineWidth, 1.2);
});

test('leaf sway stays gentle', () => {
  const leaf = { speed: 0.6, phase: 1 };
  for (let t = 0; t < 30; t += 0.5) assert.ok(Math.abs(leafSway(leaf, t)) <= 0.035);
  assert.notEqual(leafSway(leaf, 0), leafSway(leaf, 2));
});

test('substrate height is clamped', () => {
  assert.equal(substrateHeight(200), 36);
  assert.ok(Math.abs(substrateHeight(800) - 56) < 1e-9);
  assert.equal(substrateHeight(3000), 80);
});

test('substrate bands span the width with the ribbon above the hill', () => {
  const { hill, ribbon, thickness } = substrateBands(1000, 800, 20);
  assert.equal(hill.length, 21);
  assert.equal(hill[0].x, 0);
  assert.equal(hill.at(-1).x, 1000);
  assert.ok(thickness > 0);
  hill.forEach((point, index) => {
    assert.ok(point.y < 800 && point.y > 800 - substrateHeight(800));
    assert.ok(ribbon[index].y + thickness < point.y, 'gap between ribbon and hill');
  });
});

test('substrate uses default resolution', () => {
  assert.equal(substrateBands(500, 500).hill.length, 49);
});

test('fish are spread over different depths', () => {
  const fish = createFish(createRng(2), 3);
  assert.equal(fish.length, 3);
  assert.ok(fish[0].depth < fish[1].depth && fish[1].depth < fish[2].depth);
  for (const one of fish) {
    assert.ok(one.size >= 0.55 && one.size <= 1.1);
    assert.ok(one.speed >= 14 && one.speed <= 30);
  }
  assert.equal(createFish().length, 3);
  assert.equal(createFish(createRng(1), 1).length, 1);
});

test('fish swim right, turn off-screen and swim back left', () => {
  const fish = { size: 1, depth: 0.5, speed: 20, offset: 0, bob: 10, phase: 0 };
  const width = 1000;
  const span = width + FISH_LENGTH * 2;
  const start = fishPose(fish, 0, width, 800);
  assert.equal(start.direction, 1);
  assert.equal(start.x, -FISH_LENGTH);
  const later = fishPose(fish, 10, width, 800);
  assert.equal(later.direction, 1);
  assert.equal(later.x, -FISH_LENGTH + 200);
  const back = fishPose(fish, (span + 100) / 20, width, 800);
  assert.equal(back.direction, -1);
  assert.equal(back.x, width + FISH_LENGTH - 100);
  const turn = fishPose(fish, span / 20 - 0.01, width, 800);
  assert.ok(turn.x > width, 'fish turns while off-screen');
});

test('fish bob around their depth with a slight tilt', () => {
  const fish = { size: 0.8, depth: 0.4, speed: 25, offset: 300, bob: 12, phase: 2 };
  for (let t = 0; t < 60; t += 1.7) {
    const pose = fishPose(fish, t, 1200, 900);
    assert.ok(Math.abs(pose.y - 360) <= 12);
    assert.ok(Math.abs(pose.tilt) <= 0.06);
    assert.ok(pose.x >= -80 && pose.x <= 1280);
    assert.equal(pose.scale, 0.8);
  }
});

test('fish silhouette includes body, fins and an eye inside the body', () => {
  assert.equal(FISH_PATHS.length, 4);
  assert.ok(FISH_PATHS.every((path) => path.startsWith('M') && path.endsWith('Z')));
  assert.ok(FISH_EYE.x > 0 && FISH_EYE.x < FISH_LENGTH / 2);
});
