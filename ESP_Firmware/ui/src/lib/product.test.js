import test from 'node:test';
import assert from 'node:assert/strict';
import { bubbleTarget, downloadSnapshot, exportSnapshot, readPreference, savePreference, temperatureLimitsValid, widgetStates } from './product.js';

const healthy = { status: 'ok', reliable: true, bps: 1, weightG: 4500, weightValid: true, loadCellCalibrated: true, gasConfigured: true, gasRemainingPercent: 50, temperatureValid: true, temperatureC: 24, temperatureTargetC: 24, temperatureToleranceC: 2, rssi: -60 };

test('optional bubble target compares the rate against a ten percent band', () => {
  for (const target of [0, -1, NaN, undefined, 21]) assert.equal(bubbleTarget(1, target), null);
  assert.equal(bubbleTarget(NaN, 1), null);
  assert.deepEqual(bubbleTarget(1, 1), { label: 'On target', position: 50 });
  for (const rate of [0.9, 1.1]) assert.equal(bubbleTarget(rate, 1).label, 'On target');
  assert.equal(bubbleTarget(0.89, 1).label, 'Below target');
  assert.equal(bubbleTarget(1.11, 1).label, 'Above target');
  assert.equal(bubbleTarget(5, 1).position, 100);
  assert.equal(bubbleTarget(-1, 1).position, 0);
});

test('preferences validate stored values and survive blocked storage', () => {
  const values = new Map();
  const storage = { getItem: (key) => values.get(key), setItem: (key, value) => values.set(key, value) };
  assert.equal(readPreference(storage, 'view', ['basic', 'advanced'], 'basic'), 'basic');
  savePreference(storage, 'view', 'advanced');
  assert.equal(readPreference(storage, 'view', ['basic', 'advanced'], 'basic'), 'advanced');
  savePreference(storage, 'view', 'expert');
  assert.equal(readPreference(storage, 'view', ['basic', 'advanced'], 'basic'), 'basic');
  const blocked = { getItem() { throw new Error('blocked'); }, setItem() { throw new Error('blocked'); } };
  assert.equal(readPreference(blocked, 'theme', ['dark', 'light'], 'dark'), 'dark');
  assert.doesNotThrow(() => savePreference(blocked, 'theme', 'light'));
});

test('temperature settings require finite aquarium-range values and a positive tolerance', () => {
  for (const pair of [[0, 0.1], [40, 10], [24, 2]]) assert.ok(temperatureLimitsValid(...pair));
  for (const pair of [[NaN, 2], [Infinity, 2], [-1, 2], [41, 2], [24, NaN], [24, 0], [24, 10.1]]) assert.equal(temperatureLimitsValid(...pair), false);
});

test('normal status is healthy and offline data is never healthy', () => {
  const normal = widgetStates(healthy, true);
  for (const key of ['bubbles', 'gas', 'temperature', 'network']) assert.equal(normal[key].tone, 'healthy');
  assert.equal(normal.bubbles.label, 'Flowing');
  const offline = widgetStates(healthy, false);
  for (const key of ['bubbles', 'gas', 'temperature', 'network']) assert.deepEqual(offline[key], { tone: 'warning', label: 'Disconnected' });
  assert.equal(offline.gasValid, false);
  assert.equal(offline.tempValid, false);
  assert.equal(widgetStates({ ...healthy, bps: 0 }, true).bubbles.label, 'Idle');
});

test('bubble faults and excessive rates trigger red borders', () => {
  assert.equal(widgetStates({ ...healthy, status: 'saturated' }, true).bubbles.tone, 'danger');
  assert.equal(widgetStates({ ...healthy, reliable: false }, true).bubbles.label, 'Bubble rate too high');
  assert.equal(widgetStates({ ...healthy, bps: 11 }, true).bubbles.tone, 'danger');
  assert.equal(widgetStates({ ...healthy, bps: 3 }, true, 2).bubbles.tone, 'danger');
  assert.equal(widgetStates({ ...healthy, bps: 2 }, true, 2).bubbles.tone, 'healthy');
});

test('CO₂ bands use strict 20, 10 and 5 percent boundaries with flashing only at empty', () => {
  for (const [percent, severity, label] of [
    [20, undefined, 'Healthy'], [19.9, 'low', 'CO₂ low'], [10, 'low', 'CO₂ low'],
    [9.9, 'very-low', 'CO₂ very low'], [5, 'very-low', 'CO₂ very low'],
    [4.9, 'critical', 'CO₂ critically low'], [0.1, 'critical', 'CO₂ critically low'],
    [0, 'empty', 'CO₂ empty'], [-1, 'empty', 'CO₂ empty'],
  ]) {
    const state = widgetStates({ ...healthy, gasRemainingPercent: percent }, true).gas;
    assert.equal(state.severity, severity);
    assert.equal(state.label, label);
    assert.equal(state.tone, percent < 20 ? 'danger' : 'healthy');
  }
  for (const values of [{ weightValid: false }, { weightG: NaN }, { loadCellCalibrated: false }, { gasConfigured: false }, { gasRemainingPercent: NaN }]) assert.equal(widgetStates({ ...healthy, ...values }, true).gas.label, 'Reading unavailable');
});

test('gross readings below 10 g mean not present, not an empty cylinder', () => {
  for (const weightG of [0, 0.3, -0.3, 1, -1, 9.99, -9.99]) {
    const states = widgetStates({ ...healthy, weightG, gasRemainingPercent: 0 }, true);
    assert.deepEqual(states.gas, { tone: 'neutral', label: 'Not present' });
    assert.equal(states.gasAbsent, true);
    assert.equal(states.gasValid, false);
  }
  assert.equal(widgetStates({ ...healthy, weightG: 0, gasConfigured: false }, true).gas.label, 'Not present');
  assert.equal(widgetStates({ ...healthy, weightG: 0, weightValid: false }, true).gas.label, 'Reading unavailable');
  assert.equal(widgetStates({ ...healthy, weightG: 0 }, false).gas.label, 'Disconnected');
  for (const weightG of [3500, 10, -10]) assert.equal(widgetStates({ ...healthy, weightG, gasRemainingPercent: 0 }, true).gas.severity, 'empty');
});

test('configured empty-cylinder weight takes precedence over the 10 g fallback', () => {
  for (const weightG of [-1, 0, 1, 3499.9]) {
    assert.equal(widgetStates({ ...healthy, emptyCylinderG: 3500, weightG }, true).gas.label, 'Not present');
  }
  for (const weightG of [3500, 3500.1]) {
    assert.equal(widgetStates({ ...healthy, emptyCylinderG: 3500, weightG, gasRemainingPercent: 0 }, true).gas.label, 'CO₂ empty');
  }
  assert.equal(widgetStates({ ...healthy, emptyCylinderG: 0, weightG: 9.9 }, true).gas.label, 'Not present');
  assert.equal(widgetStates({ ...healthy, emptyCylinderG: Infinity, weightG: 10 }, true).gasValid, true);
});

test('temperature status covers high, low, boundaries, missing limits and bad data', () => {
  assert.equal(widgetStates({ ...healthy, temperatureC: 26.1 }, true).temperature.label, 'Temperature too high');
  assert.equal(widgetStates({ ...healthy, temperatureC: 21.9 }, true).temperature.label, 'Temperature too low');
  for (const temperatureC of [22, 26]) assert.equal(widgetStates({ ...healthy, temperatureC }, true).temperature.tone, 'healthy');
  assert.equal(widgetStates({ ...healthy, temperatureTargetC: undefined }, true).temperature.label, 'Set temperature limits');
  for (const values of [{ temperatureValid: false }, { temperatureC: NaN }]) assert.equal(widgetStates({ ...healthy, ...values }, true).temperature.label, 'Reading unavailable');
  assert.equal(widgetStates({ ...healthy, rssi: -80 }, true).network.label, 'Weak Wi-Fi signal');
});

test('JSON export whitelists public data and retains every daily record', () => {
  const data = { status: { ...healthy, wifiPassword: 'not-for-export' }, config: { threshold: 100, password: 'not-for-export' }, 'daily-stats': { current: { day: 20260131, count: 10, secret: 'not-for-export' }, history: Array.from({ length: 30 }, (_, i) => ({ day: 20260101 + i, count: i })) } };
  const snapshot = exportSnapshot(data, true, new Date('2026-11-09T12:00:00Z'));
  assert.equal(snapshot.schemaVersion, 1);
  assert.equal(snapshot.dailyStats.history.length, 30);
  assert.equal(snapshot.dailyStats.current.count, 10);
  assert.equal(snapshot.exportedAt, '2026-11-09T12:00:00.000Z');
  assert.equal(snapshot.status.temperatureTargetC, 24);
  assert.deepEqual(snapshot.configuration, { threshold: 100 });
  assert.equal(JSON.stringify(snapshot).includes('not-for-export'), false);
  const empty = exportSnapshot({}, false);
  assert.deepEqual(empty.status, {});
  assert.deepEqual(empty.dailyStats, { current: {}, history: [] });
  assert.equal(empty.connected, false);
});

test('download creates a JSON file and releases its temporary URL', async () => {
  const calls = [];
  let blob;
  let timer;
  const link = { click: () => calls.push('click'), remove: () => calls.push('remove') };
  const browser = {
    URL: { createObjectURL: (value) => { blob = value; return 'blob:test'; }, revokeObjectURL: (url) => calls.push(url) },
    document: { createElement: (tag) => { assert.equal(tag, 'a'); return link; }, body: { appendChild: (element) => assert.equal(element, link) } },
    setTimeout: (callback, delay) => { timer = callback; assert.equal(delay, 1000); },
  };
  const snapshot = exportSnapshot({}, false, new Date('2026-11-09'));
  downloadSnapshot(snapshot, browser);
  assert.equal(blob.type, 'application/json');
  assert.deepEqual(JSON.parse(await blob.text()), snapshot);
  assert.equal(link.download, 'scapesense-2026-11-09.json');
  assert.equal(link.href, 'blob:test');
  timer();
  assert.deepEqual(calls, ['click', 'remove', 'blob:test']);
});
