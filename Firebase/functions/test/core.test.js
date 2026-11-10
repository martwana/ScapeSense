import test from 'node:test';
import assert from 'node:assert/strict';
import { ApiError, createApi, createDelivery, digest, notificationFor, validateEvent, validateSubscription, validDeviceId } from '../src/core.js';
import { firestoreStore } from '../src/store.js';
import { FakeFirestore } from './fake-firestore.js';

const NOW = Date.parse('2026-11-09T12:00:00Z');
const event = (patch = {}) => ({ eventId: '000000016aa00000', kind: 'started', occurredAt: NOW / 1000 - 60, total: 12, bps: 0.2, ...patch });
function fixture() {
  const db = new FakeFirestore();
  const store = firestoreStore(db, (ms) => ms);
  const api = createApi({ store, now: () => NOW, verifyUser: async (token) => {
    if (token === 'bad') throw new Error('private details');
    return token === 'missing' ? {} : { uid: token, email_verified: token !== 'unverified' };
  } });
  const call = (path, method = 'GET', body, authorization = 'Bearer owner', size = 100) => api({ path, method, body, headers: { authorization }, size });
  const pair = async (uid = 'owner') => (await call('/api/devices', 'POST', { name: 'Living room' }, `Bearer ${uid}`)).body;
  const report = (device, data = event()) => call('/api/device-events', 'POST', data, `Bearer ${device.deviceId}.${device.deviceKey}`);
  return { db, store, api, call, pair, report };
}

test('event validation rejects malformed data, stale/future time and bad metrics', () => {
  assert.deepEqual(validateEvent({ ...event(), secret: 'ignored' }, NOW), event());
  for (const body of [null, [], {}, event({ eventId: 2 }), event({ eventId: 'bad' }), event({ kind: 'reset' }), event({ occurredAt: 0 }), event({ occurredAt: NOW / 1000 + 301 }), event({ occurredAt: 1.2 }), event({ total: -1 }), event({ total: 4294967296 }), event({ bps: NaN }), event({ bps: -1 }), event({ bps: 10001 })]) assert.throws(() => validateEvent(body, NOW), ApiError);
  assert.equal(validDeviceId('a'.repeat(32)), true);
  assert.equal(validDeviceId(undefined), false);
  assert.equal(validDeviceId('A'.repeat(32)), false);
});

test('subscriptions validate registration and ownership selections without trusting extra fields', () => {
  const body = { token: 'x'.repeat(20), deviceIds: ['a'.repeat(32), 'a'.repeat(32)], started: true, stopped: false, ownerUid: 'attacker' };
  assert.deepEqual(validateSubscription(body), { token: body.token, deviceIds: ['a'.repeat(32)], started: true, stopped: false });
  for (const patch of [null, { ...body, token: 'short' }, { ...body, token: 'a '.repeat(20) }, { ...body, token: 'x'.repeat(4097) }, { ...body, deviceIds: null }, { ...body, deviceIds: Array(26).fill('a'.repeat(32)) }, { ...body, deviceIds: ['bad'] }, { ...body, started: 1 }, { ...body, stopped: undefined }]) assert.throws(() => validateSubscription(patch), ApiError);
});

test('API requires verified sign-in, rejects oversized input and sanitises infrastructure failures', async () => {
  const f = fixture();
  for (const token of [undefined, 'Basic x', 'Bearer bad', 'Bearer missing']) assert.equal((await f.call('/api/devices', 'GET', undefined, token)).status, token === undefined ? 200 : 401);
  assert.equal((await f.call('/api/devices', 'GET', undefined, 'Bearer unverified')).status, 403);
  assert.equal((await f.api({ method: 'GET', path: '/api/devices' })).status, 401);
  assert.equal((await f.call('/api/devices', 'POST', {}, undefined, 8193)).status, 413);
  assert.equal((await f.call('/missing')).status, 404);
  const broken = createApi({ store: { listDevices() { throw new Error('private details'); } }, verifyUser: async () => ({ uid: 'owner', email_verified: true }) });
  assert.deepEqual(await broken({ method: 'GET', path: '/api/devices', headers: { authorization: 'Bearer valid' } }), { status: 500, body: { error: 'Service temporarily unavailable' } });
});

test('pairing returns a one-time key but listing never exposes its hash or another owner', async () => {
  const f = fixture();
  const device = await f.pair();
  assert.ok(validDeviceId(device.deviceId));
  assert.equal(device.deviceKey.length, 43);
  assert.equal((await f.store.getDevice(device.deviceId)).credentialHash, digest(device.deviceKey));
  const list = await f.call('/api/devices');
  assert.equal(list.body.devices[0].name, 'Living room');
  assert.equal(JSON.stringify(list).includes(device.deviceKey), false);
  assert.equal(JSON.stringify(list).includes('credentialHash'), false);
  assert.deepEqual((await f.call('/api/devices', 'GET', undefined, 'Bearer other')).body.devices, []);
  for (const name of [undefined, 1, '', ' ', 'x'.repeat(65)]) assert.equal((await f.call('/api/devices', 'POST', { name })).status, 400);
  for (let i = 0; i < 24; i++) await f.pair();
  assert.equal((await f.call('/api/devices', 'POST', { name: 'extra' })).status, 429);
});

test('device events require the per-device key and deduplicate persisted retries', async () => {
  const f = fixture(), device = await f.pair();
  assert.equal((await f.report(device)).status, 202);
  assert.equal((await f.report(device)).body.duplicate, true);
  assert.equal((await f.store.getDevice(device.deviceId)).eventCount, 1);
  assert.equal((await f.report(device, event({ total: 13 }))).status, 409);
  assert.equal((await f.report({ ...device, deviceKey: 'x'.repeat(43) })).status, 401);
  assert.equal((await f.report({ ...device, deviceId: 'a'.repeat(32) })).status, 401);
  assert.equal((await f.call('/api/device-events', 'POST', event(), 'Bearer owner')).status, 401);
  assert.equal((await f.call('/api/device-events', 'POST', event(), undefined)).status, 401);
  assert.equal((await f.report(device, event({ kind: 'invalid' }))).status, 400);
  const ref = f.db.records.get(`devices/${device.deviceId}`);
  ref.eventCount = 120;
  assert.equal((await f.report(device, event({ eventId: '000000026aa00000', kind: 'stopped' }))).status, 429);
  ref.eventDay--;
  assert.equal((await f.report(device, event({ eventId: '000000026aa00000', kind: 'stopped' }))).status, 202);
  const history = await f.call(`/api/devices/${device.deviceId}/events`);
  assert.equal(history.body.events.length, 2);
  assert.equal(history.body.events[0].state, 'pending');
  assert.equal((await f.call(`/api/devices/${device.deviceId}/events`, 'GET', undefined, 'Bearer other')).status, 404);
});

test('revocation is owner-only and rechecked during event persistence', async () => {
  const f = fixture(), device = await f.pair();
  assert.equal((await f.call(`/api/devices/${device.deviceId}`, 'DELETE', undefined, 'Bearer other')).status, 404);
  assert.equal((await f.call(`/api/devices/${device.deviceId}`, 'DELETE')).status, 200);
  assert.equal((await f.report(device)).status, 401);
  await assert.rejects(f.store.acceptEvent(device.deviceId, event(), NOW), /revoked/);
});

test('subscriptions are scoped to owned devices, capped and transferred between signed-in accounts', async () => {
  const f = fixture(), device = await f.pair(), other = await f.pair('other');
  const body = { token: 'x'.repeat(30), deviceIds: [device.deviceId], started: true, stopped: true };
  assert.equal((await f.call('/api/subscription', 'PUT', { ...body, deviceIds: [other.deviceId] })).status, 404);
  assert.equal((await f.call('/api/subscription', 'PUT', body)).status, 200);
  assert.equal((await f.store.recipients('owner')).length, 1);
  assert.equal((await f.call('/api/subscription', 'PUT', { ...body, deviceIds: [other.deviceId] }, 'Bearer other')).status, 200);
  assert.equal((await f.store.recipients('owner')).length, 0);
  assert.equal((await f.call('/api/subscription', 'DELETE', { token: body.token }, 'Bearer other')).status, 200);
  assert.equal((await f.store.recipients('other')).length, 0);
  for (let i = 0; i < 20; i++) await f.call('/api/subscription', 'PUT', { ...body, token: `registration-${String(i).padStart(20, '0')}` });
  assert.equal((await f.call('/api/subscription', 'PUT', body)).status, 429);
  assert.equal((await f.call('/api/subscription', 'PUT', { ...body, token: `registration-${'0'.repeat(20)}` })).status, 200);
});

test('delivery filters preferences, records successful recipients and retries only failures', async () => {
  const f = fixture(), device = await f.pair();
  await f.report(device);
  for (const [id, changes] of [['one', {}], ['two', {}], ['three', {}], ['off', { started: false }], ['different', { deviceIds: [] }]]) await f.store.saveSubscription('owner', id, { token: id, deviceIds: [device.deviceId], started: true, stopped: true, ...changes });
  const sends = [];
  const deliver = createDelivery({ store: f.store, now: () => NOW, send: async (tokens, message) => {
    sends.push({ tokens, message });
    return tokens.map((token) => token === 'two' ? { success: false, code: 'messaging/registration-token-not-registered' } : token === 'three' && sends.length === 1 ? { success: false, code: 'messaging/internal-error' } : { success: true });
  } });
  await assert.rejects(deliver(device.deviceId, event().eventId), /retry/);
  assert.deepEqual(sends[0].tokens, ['one', 'two', 'three']);
  assert.equal((await f.store.recipients('owner')).some((sub) => sub.id === 'two'), false);
  await deliver(device.deviceId, event().eventId);
  assert.deepEqual(sends[1].tokens, ['three']);
  await deliver(device.deviceId, event().eventId);
  assert.equal(sends.length, 2);
  assert.match(sends[0].message.title, /Living room: CO₂ started/);
  assert.equal((await f.store.listEvents(device.deviceId))[0].state, 'done');
});

test('leases prevent simultaneous delivery; expired, revoked, missing and recipient-free events are safe', async () => {
  for (const variant of ['revoked', 'expired', 'none', 'different-owner']) {
    const f = fixture(), device = await f.pair();
    await f.report(device);
    if (variant === 'revoked') await f.store.revokeDevice('owner', device.deviceId);
    if (variant === 'different-owner') f.db.records.get(`devices/${device.deviceId}`).ownerUid = 'other';
    const deliver = createDelivery({ store: f.store, now: () => variant === 'expired' ? NOW + 86400001 : NOW, send: () => { throw new Error('must not send'); } });
    await deliver(device.deviceId, event().eventId);
    assert.equal((await f.store.listEvents(device.deviceId))[0].state, 'done');
  }
  const f = fixture(), device = await f.pair();
  assert.equal(await f.store.claimDelivery(device.deviceId, event().eventId, NOW), null);
  await f.report(device);
  await f.store.claimDelivery(device.deviceId, event().eventId, NOW);
  await assert.rejects(f.store.claimDelivery(device.deviceId, event().eventId, NOW), /lease/);
  assert.ok(await f.store.claimDelivery(device.deviceId, event().eventId, NOW + 120001));
  await f.store.finishDelivery(device.deviceId, 'missing', [], true);
});

test('failed sends release their lease and notification text uses UK time with stable tags', async () => {
  const f = fixture(), device = await f.pair();
  await f.report(device);
  await f.store.saveSubscription('owner', 'one', { token: 'one', deviceIds: [device.deviceId], started: true });
  const deliver = createDelivery({ store: f.store, now: () => NOW, send: () => { throw new Error('private details'); } });
  await assert.rejects(deliver(device.deviceId, event().eventId), /^Error: Push delivery needs retry$/);
  const stored = f.db.records.get(`devices/${device.deviceId}/events/${event().eventId}`);
  assert.equal(stored.leaseUntil, 0);
  assert.match(notificationFor(device.deviceId, { ...event(), deviceName: 'Tank' }).body, /11:59/);
  assert.match(notificationFor(device.deviceId, { ...event({ kind: 'stopped', total: 1000 }), deviceName: 'Tank' }).body, /1,000 bubbles/);
  assert.match(notificationFor(device.deviceId, { ...event({ occurredAt: Date.parse('2026-07-01T12:00:00Z') / 1000 }), deviceName: 'Tank' }).body, /13:00/);
});
