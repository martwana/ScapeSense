import test from 'node:test';
import assert from 'node:assert/strict';
import { FakeFirestore } from './fake-firestore.js';

test('Firebase trigger wrappers configure bounded regions, authenticate requests and send FCM', async (t) => {
  const db = new FakeFirestore();
  let sends = 0;
  t.mock.module('firebase-admin/app', { namedExports: { initializeApp: () => {} } });
  t.mock.module('firebase-admin/auth', { namedExports: { getAuth: () => ({ verifyIdToken: async (token) => ({ uid: token, email_verified: true }) }) } });
  t.mock.module('firebase-admin/firestore', { namedExports: { getFirestore: () => db, Timestamp: { fromMillis: (value) => value } } });
  t.mock.module('firebase-admin/messaging', { namedExports: { getMessaging: () => ({ sendEachForMulticast: async ({ tokens, data }) => {
    assert.deepEqual(tokens, ['push-registration-token-12345']);
    assert.match(data.title, /Tank: CO₂ started/);
    sends += 1;
    return { responses: tokens.map(() => ({ success: true })) };
  } }) } });
  t.mock.module('firebase-functions/v2/https', { namedExports: { onRequest: (options, handler) => { assert.equal(options.region, 'europe-west2'); assert.equal(options.maxInstances, 2); return handler; } } });
  let documentOptions;
  t.mock.module('firebase-functions/v2/firestore', { namedExports: { onDocumentCreated: (options, handler) => { documentOptions = options; return handler; } } });
  const { api, notifyFlow } = await import('../src/index.js');
  assert.equal(documentOptions.retry, true);
  assert.equal(documentOptions.timeoutSeconds, 60);
  const invoke = async (method, path, body, authorization = 'Bearer owner') => {
    let status = 0, response;
    await api({ method, path, body, headers: { authorization }, rawBody: Buffer.from(JSON.stringify(body || {})) }, {
      set() { return this; }, status(value) { status = value; return this; }, json(value) { response = value; },
    });
    return { status, body: response };
  };
  const device = (await invoke('POST', '/api/devices', { name: 'Tank' })).body;
  assert.ok(device.deviceKey);
  await invoke('PUT', '/api/subscription', { token: 'push-registration-token-12345', deviceIds: [device.deviceId], started: true, stopped: true });
  const event = { eventId: '000000016aa00000', kind: 'started', occurredAt: Math.floor(Date.now() / 1000) - 1, total: 12, bps: 0.2 };
  assert.equal((await invoke('POST', '/api/device-events', event, `Bearer ${device.deviceId}.${device.deviceKey}`)).status, 202);
  await notifyFlow({ params: { deviceId: device.deviceId, eventId: event.eventId } });
  assert.equal(sends, 1);
});
