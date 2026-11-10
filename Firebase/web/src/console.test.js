import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { parseHTML } from 'linkedom';
import { createConsole } from './console.js';

const markup = await readFile(new URL('../public/index.html', import.meta.url), 'utf8');
const deviceId = 'a'.repeat(32);

function fixture({ confirm = true } = {}) {
  const { document } = parseHTML(markup);
  document.getElementById('started').checked = true;
  document.getElementById('stopped').checked = true;
  const handlers = new WeakMap();
  const instrument = (element) => {
    if (!element) return element;
    handlers.set(element, new Map());
    const add = element.addEventListener.bind(element);
    element.addEventListener = (type, handler, options) => { handlers.get(element).set(type, handler); add(type, handler, options); };
    return element;
  };
  for (const element of document.querySelectorAll('*')) instrument(element);
  const createElement = document.createElement.bind(document);
  document.createElement = (tag) => instrument(createElement(tag));
  const trigger = async (element, type) => {
    const handler = handlers.get(element)?.get(type);
    assert.ok(handler, `missing ${type} handler for ${element.id || element.tagName}`);
    await handler({ preventDefault() {}, currentTarget: element });
  };
  const calls = [];
  let user = null;
  let watcher = () => {};
  let devices = [];
  const auth = {
    user: () => user,
    watch: (callback) => { watcher = callback; callback(user); return () => {}; },
    signIn: async (email) => { user = { uid: 'owner', email, emailVerified: true }; watcher(user); },
    signUp: async (email) => { user = { uid: 'owner', email, emailVerified: false }; watcher(user); },
    refresh: async () => { user.emailVerified = true; watcher(user); },
    verify: async () => {},
    signOut: async () => { user = null; watcher(user); },
  };
  const api = async (path, method = 'GET', body) => {
    calls.push({ path, method, body });
    if (path === '/api/devices' && method === 'GET') return { devices };
    if (path === '/api/devices' && method === 'POST') {
      const paired = { deviceId, deviceKey: 's'.repeat(43), name: body.name };
      devices = [...devices, { ...paired, enabled: true }];
      return paired;
    }
    if (path.endsWith('/events')) return { events: [{ kind: 'started', occurredAt: 1762689540, total: 12, state: 'done' }] };
    if (path.endsWith(deviceId) && method === 'DELETE') { devices = devices.map((item) => item.deviceId === deviceId ? { ...item, enabled: false } : item); return { revoked: true }; }
    if (path === '/api/subscription') return { subscribed: method === 'PUT' };
    throw new Error(`Unexpected API path: ${path}`);
  };
  const pushCalls = [];
  const push = {
    enable: async () => { pushCalls.push('enable'); return 'private-push-token-123456789'; },
    disable: async (remove) => { pushCalls.push('disable'); await remove('private-push-token-123456789'); },
  };
  const application = createConsole({ document, auth, api, push, projectId: 'tank-project', confirm: () => confirm });
  return { document, handlers, trigger, calls, pushCalls, auth, application, devices: () => devices };
}

test('signed-out and unverified accounts cannot enumerate or pair devices', async () => {
  const f = fixture();
  assert.equal(f.document.getElementById('signed-out').hidden, false);
  assert.equal(f.document.getElementById('signed-in').hidden, true);
  f.document.getElementById('email').value = 'owner@example.com';
  f.document.getElementById('password').value = 'long-secure-password';
  await f.trigger(f.document.getElementById('create-account'), 'click');
  assert.equal(f.document.getElementById('verify').hidden, false);
  assert.equal(f.calls.length, 0);
  await f.trigger(f.document.getElementById('refresh-verification'), 'click');
  assert.equal(f.document.getElementById('signed-in').hidden, false);
  assert.deepEqual(f.calls.map((call) => call.path), ['/api/devices', '/api/devices']);
});

test('sign-in pairs a device and exposes its generated secret once for local pairing', async () => {
  const f = fixture();
  f.document.getElementById('email').value = 'owner@example.com';
  f.document.getElementById('password').value = 'long-secure-password';
  await f.trigger(f.document.getElementById('sign-in'), 'submit');
  f.document.getElementById('device-name').value = '<script>Tank</script>';
  await f.trigger(f.document.getElementById('create-device'), 'submit');
  const pairing = f.document.getElementById('pairing-details');
  const secret = JSON.parse(pairing.value);
  assert.deepEqual(secret, { projectId: 'tank-project', deviceId, deviceKey: 's'.repeat(43) });
  assert.equal(f.document.getElementById('pairing').hidden, false);
  assert.equal(f.document.querySelector('#devices script'), null);
  assert.match(f.document.querySelector('#devices').textContent, /<script>Tank<\/script>/);
  await f.trigger(f.document.getElementById('hide-pairing'), 'click');
  assert.equal(pairing.value, '');
  assert.equal(f.document.getElementById('pairing').hidden, true);
  assert.equal(JSON.stringify(f.calls).includes('deviceKey'), false);
});

test('notification preferences require a selected device and save token ownership securely', async () => {
  const f = fixture();
  f.document.getElementById('email').value = 'owner@example.com';
  f.document.getElementById('password').value = 'long-secure-password';
  await f.trigger(f.document.getElementById('sign-in'), 'submit');
  await f.trigger(f.document.getElementById('enable-push'), 'click');
  assert.match(f.document.getElementById('notice').textContent, /Select at least one/);
  await f.trigger(f.document.getElementById('create-device'), 'submit');
  const checkbox = f.document.querySelector(`#devices input[value="${deviceId}"]`);
  checkbox.checked = true;
  if (!f.document.querySelector(`#devices input:checked`)) checkbox.setAttribute('checked', '');
  await f.trigger(f.document.getElementById('enable-push'), 'click');
  assert.equal(f.pushCalls[0], 'enable', f.document.getElementById('notice').textContent);
  const saved = f.calls.find((call) => call.path === '/api/subscription');
  assert.equal(saved.method, 'PUT');
  assert.deepEqual(saved.body, { token: 'private-push-token-123456789', deviceIds: [deviceId], started: true, stopped: true });
  assert.equal(JSON.stringify(f.calls.filter((call) => call.path.startsWith('/api/devices'))).includes('deviceKey'), false);
});

test('push preferences can be disabled and sign-out clears local key and ownership', async () => {
  const f = fixture();
  f.document.getElementById('email').value = 'owner@example.com';
  f.document.getElementById('password').value = 'long-secure-password';
  await f.trigger(f.document.getElementById('sign-in'), 'submit');
  await f.trigger(f.document.getElementById('disable-push'), 'click');
  assert.deepEqual(f.calls.at(-1), { path: '/api/subscription', method: 'DELETE', body: { token: 'private-push-token-123456789' } });
  const secret = f.document.getElementById('pairing-details');
  secret.value = 'local sensitive key';
  f.document.getElementById('pairing').hidden = false;
  await f.trigger(f.document.getElementById('sign-out'), 'click');
  assert.equal(secret.value, '');
  assert.equal(f.document.getElementById('signed-out').hidden, false);
  assert.ok(f.pushCalls.includes('disable'));
});

test('device event history is rendered as text and revocation requires confirmation', async () => {
  const yes = fixture();
  yes.document.getElementById('email').value = 'owner@example.com';
  yes.document.getElementById('password').value = 'long-secure-password';
  await yes.trigger(yes.document.getElementById('sign-in'), 'submit');
  await yes.trigger(yes.document.getElementById('create-device'), 'submit');
  await yes.trigger(yes.document.querySelector('#devices button'), 'click');
  assert.equal(yes.document.getElementById('history').hidden, false);
  assert.match(yes.document.getElementById('event-list').textContent, /CO₂ started/);
  await yes.trigger(yes.document.querySelectorAll('#devices button')[1], 'click');
  assert.equal(yes.devices()[0].enabled, false);
  assert.ok(yes.calls.some((call) => call.path === `/api/devices/${deviceId}` && call.method === 'DELETE'));
  const no = fixture({ confirm: false });
  no.document.getElementById('email').value = 'owner@example.com';
  no.document.getElementById('password').value = 'long-secure-password';
  await no.trigger(no.document.getElementById('sign-in'), 'submit');
  await no.trigger(no.document.getElementById('create-device'), 'submit');
  await no.trigger(no.document.querySelectorAll('#devices button')[1], 'click');
  assert.equal(no.devices()[0].enabled, true);
  assert.equal(no.calls.some((call) => call.method === 'DELETE' && call.path.includes(deviceId)), false);
});
