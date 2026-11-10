import test from 'node:test';
import assert from 'node:assert/strict';
import { build } from 'esbuild';

const bundle = await build({
  stdin: { contents: `
    import React from 'react';
    import { renderToStaticMarkup } from 'react-dom/server.browser';
    import App from './src/App.jsx';
    import Dashboard from './src/components/Dashboard.jsx';
    import Settings from './src/components/Settings.jsx';
    import AdcChart from './src/components/AdcChart.jsx';
    import { DailyChart, DailyTable, DisplayControls } from './src/components/ui.jsx';
    export const render = (name, props) => renderToStaticMarkup(React.createElement({App, Dashboard, Settings, AdcChart, DailyChart, DailyTable, DisplayControls}[name], props));
    export const controlButtons = (props) => {
      const walk = (node) => Array.isArray(node) ? node.flatMap(walk) : !node?.props ? [] : node.type === 'button' ? [node.props] : walk(node.props.children);
      return walk(DisplayControls(props));
    };
  `, resolveDir: new URL('../..', import.meta.url).pathname },
  bundle: true, write: false, format: 'esm', platform: 'browser', loader: { '.svg': 'dataurl' },
});
const { render, controlButtons } = await import(`data:text/javascript;base64,${Buffer.from(bundle.outputFiles[0].text).toString('base64')}`);
const status = { total: 100, bps: 1, bpm: 60, weightG: 5100, weightValid: true, loadCellCalibrated: true, gasConfigured: true, gasRemainingPercent: 75, gasRemainingG: 1500, co2CapacityG: 2000, emptyCylinderG: 3600, loadCellRaw: 10, baseline: 2000, raw: 1950, uptime: 10 };

test('dashboard prioritises percentage and provides a guarded main-page tare control', () => {
  const html = render('Dashboard', { status, connected: true });
  assert.match(html, /CO₂ capacity \(weight\)/);
  assert.match(html, /75.0%/);
  assert.match(html, /1.5 kg CO₂ remaining/);
  assert.match(html, />TARE<\/button>/);
  assert.doesNotMatch(html, /disabled=""/);
  for (const props of [{ connected: false }, { busy: 'Calibration' }, { status: { ...status, loadCellBusy: true } }, { status: { ...status, weightValid: false } }]) {
    assert.match(render('Dashboard', { status, connected: true, ...props }), /disabled=""/);
  }
  assert.doesNotMatch(render('Dashboard', { status: { ...status, gasConfigured: false } }), /75.0%/);
});

test('dashboard system logos are double size and softly faded', () => {
  const html = render('Dashboard', { status, connected: true });
  assert.equal((html.match(/system-icon h-16 w-16 opacity-60/g) || []).length, 3);
});

test('temperature card shows only valid connected sensor readings', () => {
  const temperature = { ...status, temperatureC: 24.6, temperatureValid: true };
  const html = render('Dashboard', { status: temperature, connected: true });
  assert.match(html, /Temperature/);
  assert.match(html, /24.6 °C/);
  assert.doesNotMatch(html, /DS18B20|GPIO1/);
  assert.match(render('Dashboard', { status: temperature, connected: true, advanced: true }), /DS18B20 · GPIO1/);
  for (const props of [{ connected: false }, { status: { ...temperature, temperatureValid: false } }, { status: { ...temperature, temperatureC: NaN } }, { status }]) {
    const unavailable = render('Dashboard', { status: temperature, connected: true, ...props });
    assert.doesNotMatch(unavailable, /24.6 °C|NaN °C/);
    assert.match(unavailable, /Reading unavailable|Disconnected/);
  }
});

test('history table exposes full records and graph retains all 31 days', () => {
  const stats = { history: [{ day: 20260101, count: 86400, start: 1767258000, stop: 1767286800 }], current: { day: 20260102, count: 12, active: true } };
  const html = render('DailyTable', { stats });
  for (const label of ['Date', 'Start time', 'End time', 'Total bubbles', 'Avg BPS', '01/01/2026', '~3.000', 'Flowing', 'Estimated from older start/stop data']) assert.ok(html.includes(label));
  assert.match(render('DailyTable', {}), /No daily history yet/);
  assert.match(render('DailyChart', {}), /No daily history yet/);
  stats.history = Array.from({ length: 30 }, (_, i) => ({ day: 20260101 + i, count: i + 1 }));
  stats.current = { day: 20260131, count: 31 };
  const chart = render('DailyChart', { stats });
  assert.equal((chart.match(/bubbles"/g) || []).length, 31);
});

test('ADC chart shows traces, waiting and disconnected states in settings', () => {
  assert.match(render('AdcChart', { samples: [], connected: true }), /Waiting for live samples/);
  assert.match(render('AdcChart', { samples: [], connected: false }), /trace paused/);
  const samples = [status, { ...status, raw: 1000 }];
  assert.match(render('AdcChart', { samples, connected: true }), /Last 2 status samples/);
  const settings = render('Settings', { status, adcSamples: samples, connected: true, advanced: true });
  assert.match(settings, /Live ADC signal/);
  assert.match(settings, /data:image\/svg\+xml/);
});

test('Basic hides diagnostics and Advanced reveals them without separating total and rate', () => {
  const basic = render('Dashboard', { status, connected: true });
  assert.doesNotMatch(basic, /Optical diagnostics|ADC raw|Raw counts|HX711|DS18B20/);
  const bubbleWidget = basic.slice(0, basic.indexOf('</section>'));
  assert.match(bubbleWidget, /BPS/);
  assert.match(bubbleWidget, /total bubbles today/);
  assert.match(basic, /Connection &amp; uptime/);
  const advanced = render('Dashboard', { status, connected: true, advanced: true });
  assert.match(advanced, /Optical diagnostics/);
  assert.match(advanced, /ADC raw/);
  assert.match(advanced, /Raw counts/);
});

test('Basic settings contain product controls but no tuning or calibration', () => {
  const props = { status, connected: true, temperature: { targetC: 24, toleranceC: 2 }, cylinder: { emptyCylinderG: 3000, co2CapacityG: 2000 } };
  const basic = render('Settings', props);
  for (const label of ['Temperature target', 'CO₂ bottle details', 'Export data as JSON', 'Reset Wi-Fi', 'Erase device memory', 'Type ERASE', 'Type RESET WIFI']) assert.ok(basic.includes(label));
  assert.doesNotMatch(basic, /Bubble sensor tuning|Load-cell calibration|Known mass|Live ADC signal/);
  assert.match(basic, /disabled="">Erase device memory/);
  assert.match(basic, /disabled="">Reset Wi-Fi/);
  assert.match(basic, /permanently removes/);
  const advanced = render('Settings', { ...props, advanced: true, adcSamples: [] });
  assert.match(advanced, /Bubble sensor tuning/);
  assert.match(advanced, /Load-cell calibration/);
});

test('app shell renders branding, footer and view/theme selectors safely before connection', () => {
  const previous = globalThis.window;
  try {
    globalThis.window = { location: { hash: '' }, localStorage: { getItem: () => null }, matchMedia: () => ({ matches: false }) };
    const html = render('App', {});
    assert.match(html, /ScapeSense aquascape logo/);
    assert.match(html, /system-icon h-20 w-20 sm:h-24 sm:w-24/);
    assert.doesNotMatch(html, /brightness-0 invert/);
    assert.match(html, /Made for the life beneath the surface/);
    const header = html.slice(html.indexOf('<header'), html.indexOf('</header>'));
    assert.match(header, /aria-label="Advanced view"/);
    assert.match(header, /aria-label="Light theme"/);
    assert.match(header, /aria-label="Dark theme"/);
    assert.match(header, /aria-label="Colour theme"/);
    assert.doesNotMatch(html, /<select/);
    assert.doesNotMatch(html, /Optical diagnostics/);
    globalThis.window.location.hash = '#settings';
    globalThis.window.localStorage.getItem = (key) => key === 'scapesense-mode' ? 'advanced' : 'light';
    assert.match(render('App', {}), /Live ADC signal/);
  } finally {
    if (previous === undefined) delete globalThis.window;
    else globalThis.window = previous;
  }
});

test('header toggles select themes and switch Basic/Advanced in both directions', () => {
  let theme = 'dark';
  let mode = 'basic';
  const props = () => ({ theme, mode, setTheme: (value) => { theme = value; }, setMode: (value) => { mode = value; } });
  const button = (label) => controlButtons(props()).find((entry) => entry['aria-label'] === label);
  assert.equal(button('Dark theme')['aria-pressed'], true);
  assert.equal(button('Light theme')['aria-pressed'], false);
  button('Light theme').onClick();
  assert.equal(theme, 'light');
  assert.equal(button('Light theme')['aria-pressed'], true);
  button('Dark theme').onClick();
  assert.equal(theme, 'dark');
  assert.equal(button('Advanced view')['aria-pressed'], false);
  button('Advanced view').onClick();
  assert.equal(mode, 'advanced');
  assert.equal(button('Advanced view')['aria-pressed'], true);
  assert.match(render('DisplayControls', props()), /Switch to Basic view/);
  button('Advanced view').onClick();
  assert.equal(mode, 'basic');
  assert.match(render('DisplayControls', props()), /Switch to Advanced view/);
});

test('optional BPS target renders its meter and hides stale or faulty comparisons', () => {
  const props = { status: { ...status, status: 'ok', reliable: true, bubbleTargetBps: 1 }, connected: true };
  assert.match(render('Dashboard', props), /On target · target 1.00 BPS/);
  assert.match(render('Dashboard', props), /role="meter"/);
  assert.match(render('Dashboard', { ...props, status: { ...props.status, bps: 0.5 } }), /Below target/);
  assert.match(render('Dashboard', { ...props, status: { ...props.status, bps: 1.5 } }), /Above target/);
  for (const patch of [{ connected: false }, { status: { ...props.status, bubbleTargetBps: 0 } }, { status: { ...props.status, reliable: false } }]) assert.doesNotMatch(render('Dashboard', { ...props, ...patch }), /role="meter"/);
  assert.match(render('Settings', { status, connected: true, bubbleTargetBps: '' }), /Bubble rate target/);
});

test('CO₂ widget exposes severity for flashing and distinguishes bottle absence', () => {
  for (const [percent, severity] of [[19, 'low'], [9, 'very-low'], [4, 'critical'], [0, 'empty']]) {
    const html = render('Dashboard', { status: { ...status, gasRemainingPercent: percent }, connected: true });
    assert.match(html, new RegExp(`data-alert="${severity}"`));
    assert.match(html, /co2-level/);
  }
  const absent = render('Dashboard', { status: { ...status, weightG: 0, gasRemainingPercent: 0 }, connected: true });
  assert.match(absent, /Not present/);
  assert.match(absent, /Place your CO₂ bottle on the scale/);
  assert.match(absent, /data-tone="neutral"/);
  assert.doesNotMatch(absent, /CO₂ empty|data-alert="empty"/);
  const stale = render('Dashboard', { status: { ...status, gasRemainingPercent: 0 }, connected: false });
  assert.doesNotMatch(stale, /data-alert="empty"/);
});

test('widgets show accessible status labels alongside coloured outlines', () => {
  const html = render('Dashboard', { status: { ...status, status: 'ok', reliable: true, temperatureC: 30, temperatureValid: true, temperatureTargetC: 24, temperatureToleranceC: 2, gasRemainingPercent: 0 }, connected: true });
  assert.match(html, /data-tone="danger"/);
  assert.match(html, /Temperature too high/);
  assert.match(html, /CO₂ empty/);
});
