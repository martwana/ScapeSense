import test from 'node:test';
import assert from 'node:assert/strict';
import { createPush, createTransport, openNotification, pushMessage, workerURL } from './client.js';

const id = 'a'.repeat(32);
const token = 'fcm-registration-token-123456';
const data = { title: 'Tank: CO2 started', body: 'Flow detected', tag: `scapesense-${id}-${'b'.repeat(16)}`, deviceId: id };

test('authenticated transport serializes request bodies and exposes safe server errors', async () => {
  let request;
  const api = createTransport({
    getToken: async () => 'id-token',
    fetcher: async (...args) => {
      request = args;
      return { ok: true, json: async () => ({ accepted: true }) };
    },
  });
  assert.deepEqual(await api('/api/devices', 'POST', { name: 'Tank' }), { accepted: true });
  assert.equal(request[0], '/api/devices');
  assert.deepEqual(request[1], { method: 'POST', headers: { Authorization: 'Bearer id-token', 'Content-Type': 'application/json' }, body: '{"name":"Tank"}' });
  const broken = createTransport({ getToken: async () => 'id', fetcher: async () => ({ ok: false, json: async () => ({ error: 'Verify your email first' }) }) });
  await assert.rejects(broken('/api/devices'), /Verify your email first/);
  const generic = createTransport({ getToken: async () => 'id', fetcher: async () => ({ ok: false, json: async () => ({}) }) });
  await assert.rejects(generic('/api/devices'), /Request failed/);
});

test('service-worker configuration is URL encoded and restricted to public Firebase fields', () => {
  const url = workerURL({ apiKey: 'public test', authDomain: 'tank.firebaseapp.com', projectId: 'tank-project', deviceKey: 'do-not-include' });
  const config = JSON.parse(new URL(url, 'https://tank.web.app').searchParams.get('config'));
  assert.deepEqual(config, { apiKey: 'public test', authDomain: 'tank.firebaseapp.com', projectId: 'tank-project' });
  assert.equal(url.includes('do-not-include'), false);
});

test('notification content is bounded and links cannot leave the installed app', () => {
  const message = pushMessage(data);
  assert.deepEqual(message, {
    title: data.title,
    options: { body: data.body, tag: data.tag, icon: '/icon.png', data: { deviceId: id } },
  });
  for (const bad of [undefined, {}, { ...data, deviceId: '../target' }, { ...data, tag: `scapesense-${'c'.repeat(32)}-${'b'.repeat(16)}` }, { ...data, title: '<script>'.repeat(30) }, { ...data, body: 'x'.repeat(501) }]) assert.equal(pushMessage(bad), null);
});

test('notification clicks focus only this app origin and use a validated device ID', async () => {
  const calls = [];
  const existing = {
    url: 'https://tank.web.app/previous',
    navigate: async (url) => calls.push(['navigate', url]),
    focus: async () => calls.push(['focus']),
  };
  const clients = {
    matchAll: async () => [{ url: 'https://attacker.example' }, existing],
    openWindow: async (url) => calls.push(['open', url]),
  };
  await openNotification(clients, 'https://tank.web.app', { deviceId: id });
  assert.deepEqual(calls, [['navigate', `https://tank.web.app/?device=${id}`], ['focus']]);
  calls.length = 0;
  clients.matchAll = async () => [];
  await openNotification(clients, 'https://tank.web.app', { deviceId: '/evil' });
  assert.deepEqual(calls, [['open', 'https://tank.web.app/']]);
});

test('push permission is user initiated; subscribe and unsubscribe work and setup errors are clear', async () => {
  const calls = [];
  const registration = { active: true };
  const browser = {
    Notification: {
      permission: 'default',
      requestPermission: async () => { calls.push('permission'); return 'granted'; },
    },
    navigator: { serviceWorker: {
      register: async (url) => { calls.push(['register', url]); },
      get ready() { return Promise.resolve(registration); },
      getRegistration: async () => registration,
    } },
  };
  const push = createPush({
    browser, supported: async () => true, config: { projectId: 'tank-project' }, vapidKey: 'public-vapid',
    requestToken: async (options) => { calls.push(['token', options]); return token; },
    removeToken: async () => calls.push('remove'),
  });
  assert.equal(await push.enable(), token);
  assert.deepEqual(calls.slice(0, 2), ['permission', ['register', workerURL({ projectId: 'tank-project' })]]);
  assert.equal(calls[2][0], 'token');
  browser.Notification.permission = 'granted';
  await push.disable(async (value) => { assert.equal(value, token); calls.push('unsubscribe'); });
  assert.deepEqual(calls.slice(-2), ['unsubscribe', 'remove']);
  const missing = createPush({ browser, supported: async () => true, config: {}, vapidKey: '', requestToken: async () => token, removeToken: async () => {} });
  await assert.rejects(missing.enable(), /VAPID/);
  const denied = createPush({ browser: { ...browser, Notification: { requestPermission: async () => 'denied' } }, supported: async () => true, config: {}, vapidKey: 'key', requestToken: async () => token, removeToken: async () => {} });
  await assert.rejects(denied.enable(), /permission/);
  const unsupported = createPush({ browser, supported: async () => false, config: {}, vapidKey: 'key', requestToken: async () => token, removeToken: async () => {} });
  await assert.rejects(unsupported.enable(), /not supported/);
});

test('push cleanup tolerates missing permissions or a browser without an installed worker', async () => {
  let called = false;
  const push = createPush({
    browser: { Notification: { permission: 'default' }, navigator: { serviceWorker: { getRegistration: async () => null } } },
    supported: async () => true, config: {}, vapidKey: '', requestToken: async () => { called = true; }, removeToken: async () => { called = true; },
  });
  await push.disable(async () => { called = true; });
  assert.equal(called, false);
  const browserPush = createPush({
    browser: { Notification: { permission: 'granted' }, navigator: { serviceWorker: { getRegistration: async () => null } } },
    supported: async () => true, config: {}, vapidKey: 'key', requestToken: async () => { called = true; return token; }, removeToken: async () => { called = true; },
  });
  await browserPush.disable(async () => { called = true; });
  assert.equal(called, false);
  const unavailable = createPush({ browser: { Notification: undefined, navigator: {} }, supported: async () => false, config: {}, vapidKey: 'key' });
  await unavailable.disable(async () => { throw new Error('must not run'); });
  await assert.rejects(unavailable.enable(), /not supported/);
  const noWorker = createPush({ browser: { Notification: {}, navigator: { serviceWorker: {} } }, supported: async () => true, config: {}, vapidKey: 'key' });
  await noWorker.disable(async () => { throw new Error('must not run'); });
});
