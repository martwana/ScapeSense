import { ApiError } from './core.js';

export function firestoreStore(db, timestamp) {
  const devices = db.collection('devices');
  const deviceRef = (id) => devices.doc(id);
  const events = (id) => deviceRef(id).collection('events');
  const subscriptions = (uid) => db.collection('users').doc(uid).collection('subscriptions');
  const value = (snapshot) => snapshot.exists ? snapshot.data() : null;
  return {
    async getDevice(id) { return value(await deviceRef(id).get()); },
    async listDevices(uid) {
      const result = await devices.where('ownerUid', '==', uid).limit(25).get();
      return result.docs.map((doc) => {
        const { name, enabled, createdAt } = doc.data();
        return { deviceId: doc.id, name, enabled, createdAt };
      });
    },
    async createDevice(id, data) {
      await db.runTransaction(async (tx) => {
        const existing = await tx.get(devices.where('ownerUid', '==', data.ownerUid).limit(25));
        if (existing.size >= 25) throw new ApiError(429, 'Device registration limit reached');
        tx.create(deviceRef(id), data);
      });
    },
    async revokeDevice(uid, id) {
      await db.runTransaction(async (tx) => {
        const doc = value(await tx.get(deviceRef(id)));
        if (doc?.ownerUid !== uid) throw new ApiError(404, 'Device not found');
        tx.update(deviceRef(id), { enabled: false });
      });
    },
    async acceptEvent(id, event, now) {
      return db.runTransaction(async (tx) => {
        const device = value(await tx.get(deviceRef(id)));
        const previous = value(await tx.get(events(id).doc(event.eventId)));
        if (!device?.enabled) throw new ApiError(401, 'Device revoked');
        if (previous) {
          if (['kind', 'occurredAt', 'total', 'bps'].some((key) => previous[key] !== event[key])) throw new ApiError(409, 'Event ID already used');
          return true;
        }
        const day = Math.floor(now / 86400000);
        const count = device.eventDay === day ? device.eventCount : 0;
        if (count >= 120) throw new ApiError(429, 'Daily event limit reached');
        tx.create(events(id).doc(event.eventId), {
          ...event, ownerUid: device.ownerUid, deviceName: device.name, receivedAt: now,
          expiresAt: timestamp(now + 30 * 86400000), state: 'pending', leaseUntil: 0, sentTo: [],
        });
        tx.update(deviceRef(id), { eventDay: day, eventCount: count + 1, lastSeenAt: now });
        return false;
      });
    },
    async listEvents(id) {
      const result = await events(id).orderBy('occurredAt', 'desc').limit(20).get();
      return result.docs.map((doc) => {
        const { eventId, kind, occurredAt, total, bps, state } = doc.data();
        return { eventId, kind, occurredAt, total, bps, state };
      });
    },
    async saveSubscription(uid, id, data) {
      await db.runTransaction(async (tx) => {
        const existing = await tx.get(subscriptions(uid).limit(20));
        const registration = db.collection('registrations').doc(id);
        const previous = value(await tx.get(registration));
        if (existing.size >= 20 && !existing.docs.some((doc) => doc.id === id)) throw new ApiError(429, 'Push registration limit reached');
        if (previous && previous.ownerUid !== uid) tx.delete(subscriptions(previous.ownerUid).doc(id));
        tx.set(registration, { ownerUid: uid });
        tx.set(subscriptions(uid).doc(id), data);
      });
    },
    async removeSubscription(uid, id) {
      await db.runTransaction(async (tx) => {
        const registration = db.collection('registrations').doc(id);
        const owner = value(await tx.get(registration));
        tx.delete(subscriptions(uid).doc(id));
        if (owner?.ownerUid === uid) tx.delete(registration);
      });
    },
    async recipients(uid) {
      const result = await subscriptions(uid).limit(20).get();
      return result.docs.map((doc) => ({ ...doc.data(), id: doc.id }));
    },
    async claimDelivery(id, eventId, now) {
      return db.runTransaction(async (tx) => {
        const ref = events(id).doc(eventId);
        const event = value(await tx.get(ref));
        if (!event || event.state === 'done') return null;
        if (event.leaseUntil > now) throw new Error('Delivery lease active');
        tx.update(ref, { state: 'sending', leaseUntil: now + 120000 });
        return event;
      });
    },
    async finishDelivery(id, eventId, sent, complete) {
      await db.runTransaction(async (tx) => {
        const ref = events(id).doc(eventId);
        const event = value(await tx.get(ref));
        if (!event) return;
        tx.update(ref, { sentTo: [...new Set([...event.sentTo, ...sent])], state: complete ? 'done' : 'pending', leaseUntil: 0 });
      });
    },
    async releaseDelivery(id, eventId) {
      await events(id).doc(eventId).update({ leaseUntil: 0 });
    },
  };
}
