import test from 'node:test';
import assert from 'node:assert/strict';
import { adcPoints, appendAdcSample, dailyAverageBps, dailyRecords, formatDateTime, formatDayKey, formatDuration, formatWeightG, gasTone } from './format.js';

test('daily records include all retained days in calendar order without mutation', () => {
  const stats = { history: [{ day: 20260302 }, { day: 20260301 }], current: { day: 20260303 } };
  assert.deepEqual(dailyRecords(stats).map((r) => r.day), [20260301, 20260302, 20260303]);
  assert.equal(stats.history[0].day, 20260302);
  assert.deepEqual(dailyRecords(), []);
  assert.deepEqual(dailyRecords({ current: { day: 0 } }), []);
  assert.equal(dailyRecords({ history: Array.from({ length: 30 }, (_, i) => ({ day: 20260101 + i })), current: { day: 20260201 } }).length, 31);
});

test('daily averages use accumulated flow time, not wall-clock idle time', () => {
  assert.equal(dailyAverageBps({ day: 20260101, count: 600, activeSeconds: 300 }, Date.UTC(2026, 0, 2) / 1000), 2);
  assert.equal(dailyAverageBps({ day: 20260101, count: 100, start: 1000, stop: 1200 }), 0.5);
  assert.equal(dailyAverageBps({ day: 20260101, count: 100, start: 1000, stop: 0, active: true }, 1200), 0.5);
  assert.equal(dailyAverageBps({ day: 20260101, count: 500, activeSeconds: 0 }), null);
  assert.equal(dailyAverageBps({ day: 20260101, count: 500 }), null);
  assert.equal(dailyAverageBps({ day: 20260101, count: 500, activeSeconds: -1 }), null);
  assert.equal(dailyAverageBps({ day: 20260101, count: 500, activeSeconds: NaN }), null);
  assert.equal(dailyAverageBps({ day: 0 }), null);
  assert.equal(dailyAverageBps({ day: 20260230 }), null);
});

test('multiple separate flow sessions contribute only their accumulated duration', () => {
  assert.equal(dailyAverageBps({ day: 20260329, count: 120, activeSeconds: 30 + 30 }), 2);
  assert.equal(dailyAverageBps({ day: 20261025, count: 900, activeSeconds: 300 }), 3);
});

test('ADC trace clamps values and retains a bounded valid stream', () => {
  assert.equal(adcPoints([], 'raw'), '');
  assert.equal(adcPoints([{ raw: 0 }], 'raw'), '0,100');
  assert.equal(adcPoints([{ raw: -1 }, { raw: 4096 }], 'raw'), '0,100 100,0');
  let samples = [];
  for (let i = 0; i < 130; i++) samples = appendAdcSample(samples, { raw: i, baseline: 2000, uptime: i }, true);
  assert.equal(samples.length, 120);
  assert.equal(samples[0].raw, 10);
  assert.equal(appendAdcSample(samples, {}, false), samples);
  assert.equal(appendAdcSample(samples, { raw: NaN, baseline: 1 }, true), samples);
  assert.equal(appendAdcSample(samples, { raw: 1, baseline: NaN }, true), samples);
  assert.equal(appendAdcSample(samples, { raw: 1, baseline: 1, uptime: 0 }, true).length, 1);
});
import { fetchJson, postForm, toFormBody } from './api.js';

test('weight uses whole grams through 500 grams', () => {
  for (const [value, expected] of [[1, '1 g'], [8.44, '8 g'], [500, '500 g'], [-8.44, '-8 g'], [-0.2, '0 g']]) {
    assert.equal(formatWeightG(value), expected);
  }
});

test('weight uses compact kilograms above 500 grams', () => {
  for (const [value, expected] of [[600, '0.6 kg'], [1100, '1.1 kg'], [5120, '5.12 kg'], [1000, '1 kg'], [1248, '1.25 kg'], [-600, '-0.6 kg']]) {
    assert.equal(formatWeightG(value), expected);
  }
});

test('invalid weights remain unavailable', () => {
  assert.equal(formatWeightG(NaN), '--');
  assert.equal(formatWeightG(100, false), '--');
});

test('duration uses compact readable units', () => {
  assert.equal(formatDuration(90061), '1d 1h');
  assert.equal(formatDuration(3720), '1h 2m');
  assert.equal(formatDuration(125), '2m 5s');
  assert.equal(formatDuration(NaN), '--');
});

test('day keys render as day and month', () => {
  assert.equal(formatDayKey(20260314), '14/03');
  assert.equal(formatDayKey(undefined), '--');
  assert.equal(formatDayKey('2026-03'), '--');
});

test('date-times are blank when unset and readable when set', () => {
  assert.equal(formatDateTime(0), '--');
  assert.match(formatDateTime(1773489600), /\d/);
});

test('gas tone reflects remaining percentage', () => {
  assert.equal(gasTone(NaN), 'bg-white/20');
  assert.equal(gasTone(10), 'bg-coral');
  assert.equal(gasTone(19.9), 'bg-coral');
  assert.equal(gasTone(20), 'bg-aqua');
  assert.equal(gasTone(35), 'bg-aqua');
  assert.equal(gasTone(80), 'bg-aqua');
});

test('fetchJson returns JSON and rejects failed responses', async (t) => {
  t.mock.method(globalThis, 'fetch', async (path, options) => {
    assert.equal(options.cache, 'no-store');
    return path === '/ok'
      ? { ok: true, json: async () => ({ total: 1 }) }
      : { ok: false, status: 404 };
  });
  assert.deepEqual(await fetchJson('/ok'), { total: 1 });
  await assert.rejects(fetchJson('/missing'), /Request failed: 404/);
});

test('postForm sends form data and surfaces device errors', async (t) => {
  const calls = [];
  t.mock.method(globalThis, 'fetch', async (path, options) => {
    calls.push([path, options]);
    if (path === '/ok') return { ok: true, json: async () => ({ id: 3 }) };
    if (path === '/busy') return { ok: false, status: 409, json: async () => ({ error: 'Load cell is busy' }) };
    return { ok: false, status: 500, json: async () => { throw new Error('not json'); } };
  });
  assert.deepEqual(await postForm('/ok', { knownMassG: 100 }), { id: 3 });
  assert.equal(calls[0][1].method, 'POST');
  assert.equal(calls[0][1].body, 'knownMassG=100');
  await assert.rejects(postForm('/busy'), /Load cell is busy/);
  await assert.rejects(postForm('/broken'), /Request failed: 500/);
});

test('form body encodes exact values', () => {
  assert.equal(toFormBody({ knownMassG: 165.25, polarity: 1 }), 'knownMassG=165.25&polarity=1');
});
