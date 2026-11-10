import { createHash, randomBytes, timingSafeEqual } from 'node:crypto';

export class ApiError extends Error {
  constructor(status, message) { super(message); this.status = status; }
}

export const digest = (value) => createHash('sha256').update(value).digest('hex');
export const validDeviceId = (value) => typeof value === 'string' && /^[a-f0-9]{32}$/.test(value);
const fail = (condition, status, message) => { if (condition) throw new ApiError(status, message); };
const bearer = (headers) => {
  const value = headers?.authorization;
  fail(typeof value !== 'string' || !value.startsWith('Bearer '), 401, 'Authentication required');
  return value.slice(7);
};

export function validateEvent(body, now) {
  fail(!body || typeof body !== 'object' || Array.isArray(body), 400, 'Invalid event');
  const { eventId, kind, occurredAt, total, bps } = body;
  fail(typeof eventId !== 'string' || !/^[a-f0-9]{16,64}$/.test(eventId), 400, 'Invalid event ID');
  fail(!['started', 'stopped'].includes(kind), 400, 'Invalid event type');
  fail(!Number.isInteger(occurredAt) || occurredAt < Math.floor(now / 1000) - 86400 || occurredAt > Math.floor(now / 1000) + 300, 400, 'Event timestamp outside delivery window');
  fail(!Number.isInteger(total) || total < 0 || total > 4294967295, 400, 'Invalid bubble count');
  fail(!Number.isFinite(bps) || bps < 0 || bps > 10000, 400, 'Invalid bubble rate');
  return { eventId, kind, occurredAt, total, bps };
}

export function validateSubscription(body) {
  fail(!body || typeof body.token !== 'string' || body.token.length < 20 || body.token.length > 4096 || /\s/.test(body.token), 400, 'Invalid push registration');
  fail(!Array.isArray(body.deviceIds) || body.deviceIds.length > 25 || body.deviceIds.some((id) => !validDeviceId(id)), 400, 'Invalid device selection');
  fail(typeof body.started !== 'boolean' || typeof body.stopped !== 'boolean', 400, 'Invalid notification preferences');
  return { token: body.token, deviceIds: [...new Set(body.deviceIds)], started: body.started, stopped: body.stopped };
}

export function createApi({ store, verifyUser, now = Date.now, random = randomBytes }) {
  const owner = async (headers) => {
    let user;
    try { user = await verifyUser(bearer(headers)); } catch { throw new ApiError(401, 'Sign in again'); }
    fail(!user?.uid, 401, 'Sign in again');
    fail(user.email_verified !== true, 403, 'Verify your email before pairing devices');
    return user.uid;
  };
  return async ({ method, path, headers, body, size = 0 }) => {
    try {
      fail(size > 8192, 413, 'Request too large');
      if (method === 'POST' && path === '/api/device-events') {
        const credential = bearer(headers);
        fail(!/^[a-f0-9]{32}\.[A-Za-z0-9_-]{43}$/.test(credential), 401, 'Invalid device credential');
        const [deviceId, secret] = credential.split('.');
        const device = await store.getDevice(deviceId);
        const expected = Buffer.from(device?.credentialHash || '0'.repeat(64), 'hex');
        fail(!device?.enabled || expected.length !== 32 || !timingSafeEqual(expected, Buffer.from(digest(secret), 'hex')), 401, 'Invalid device credential');
        const event = validateEvent(body, now());
        const duplicate = await store.acceptEvent(deviceId, event, now());
        return { status: 202, body: { accepted: true, duplicate } };
      }
      const uid = await owner(headers);
      if (path === '/api/devices' && method === 'GET') {
        return { status: 200, body: { devices: await store.listDevices(uid) } };
      }
      if (path === '/api/devices' && method === 'POST') {
        fail(typeof body?.name !== 'string' || !body.name.trim() || body.name.trim().length > 64, 400, 'Device name must be 1-64 characters');
        const deviceId = random(16).toString('hex');
        const secret = random(32).toString('base64url');
        await store.createDevice(deviceId, { ownerUid: uid, name: body.name.trim(), credentialHash: digest(secret), enabled: true, createdAt: now() });
        return { status: 201, body: { deviceId, deviceKey: secret, name: body.name.trim() } };
      }
      const match = /^\/api\/devices\/([a-f0-9]{32})(\/events)?$/.exec(path);
      if (match && method === 'DELETE' && !match[2]) {
        await store.revokeDevice(uid, match[1]);
        return { status: 200, body: { revoked: true } };
      }
      if (match && method === 'GET' && match[2]) {
        const device = await store.getDevice(match[1]);
        fail(device?.ownerUid !== uid, 404, 'Device not found');
        return { status: 200, body: { events: await store.listEvents(match[1]) } };
      }
      if (path === '/api/subscription' && method === 'PUT') {
        const sub = validateSubscription(body);
        for (const id of sub.deviceIds) {
          const device = await store.getDevice(id);
          fail(device?.ownerUid !== uid || !device.enabled, 404, 'Device not found');
        }
        await store.saveSubscription(uid, digest(sub.token), { ...sub, updatedAt: now() });
        return { status: 200, body: { subscribed: true } };
      }
      if (path === '/api/subscription' && method === 'DELETE') {
        const sub = validateSubscription({ ...body, deviceIds: [], started: false, stopped: false });
        await store.removeSubscription(uid, digest(sub.token));
        return { status: 200, body: { subscribed: false } };
      }
      return { status: 404, body: { error: 'Endpoint not found' } };
    } catch (error) {
      return { status: error instanceof ApiError ? error.status : 500, body: { error: error instanceof ApiError ? error.message : 'Service temporarily unavailable' } };
    }
  };
}

export function notificationFor(deviceId, event) {
  const time = new Intl.DateTimeFormat('en-GB', { timeZone: 'Europe/London', hour: '2-digit', minute: '2-digit' }).format(new Date(event.occurredAt * 1000));
  return {
    title: `${event.deviceName}: CO₂ ${event.kind}`,
    body: event.kind === 'started' ? `Flow detected at ${time}.` : `Flow stopped at ${time}. ${event.total.toLocaleString('en-GB')} bubbles recorded today.`,
    tag: `scapesense-${deviceId}-${event.eventId}`,
    deviceId,
  };
}

export function createDelivery({ store, send, now = Date.now }) {
  return async (deviceId, eventId) => {
    const event = await store.claimDelivery(deviceId, eventId, now());
    if (!event) return;
    try {
      const device = await store.getDevice(deviceId);
      if (!device?.enabled || device.ownerUid !== event.ownerUid || now() - event.receivedAt > 86400000) {
        await store.finishDelivery(deviceId, eventId, [], true);
        return;
      }
      const recipients = (await store.recipients(event.ownerUid)).filter((sub) => sub.deviceIds.includes(deviceId) && sub[event.kind] && !(event.sentTo || []).includes(sub.id));
      const sent = [];
      let retry = false;
      if (recipients.length) {
        const results = await send(recipients.map((sub) => sub.token), notificationFor(deviceId, event));
        for (let i = 0; i < recipients.length; i++) {
          if (results[i].success) sent.push(recipients[i].id);
          else if (['messaging/registration-token-not-registered', 'messaging/invalid-registration-token'].includes(results[i].code)) {
            await store.removeSubscription(event.ownerUid, recipients[i].id);
          } else retry = true;
        }
      }
      await store.finishDelivery(deviceId, eventId, sent, !retry);
      if (retry) throw new Error('Push delivery needs retry');
    } catch {
      await store.releaseDelivery(deviceId, eventId);
      throw new Error('Push delivery needs retry');
    }
  };
}
