import { initializeApp } from 'firebase-admin/app';
import { getAuth } from 'firebase-admin/auth';
import { getFirestore, Timestamp } from 'firebase-admin/firestore';
import { getMessaging } from 'firebase-admin/messaging';
import { onRequest } from 'firebase-functions/v2/https';
import { onDocumentCreated } from 'firebase-functions/v2/firestore';
import { createApi, createDelivery } from './core.js';
import { firestoreStore } from './store.js';

initializeApp();
const store = firestoreStore(getFirestore(), (ms) => Timestamp.fromMillis(ms));
const handle = createApi({ store, verifyUser: (token) => getAuth().verifyIdToken(token, true) });
const deliver = createDelivery({ store, send: async (tokens, message) => {
  const result = await getMessaging().sendEachForMulticast({
    tokens,
    data: message,
    webpush: { headers: { TTL: '86400', Urgency: 'normal' } },
  });
  return result.responses.map((response) => ({ success: response.success, code: response.error?.code }));
} });

export const api = onRequest({ region: 'europe-west2', maxInstances: 2, concurrency: 10, timeoutSeconds: 30, memory: '256MiB', cors: false }, async (request, response) => {
  response.set('Cache-Control', 'no-store');
  const result = await handle({ method: request.method, path: request.path, headers: request.headers, body: request.body, size: request.rawBody?.length || 0 });
  response.status(result.status).json(result.body);
});

export const notifyFlow = onDocumentCreated({ document: 'devices/{deviceId}/events/{eventId}', region: 'europe-west2', retry: true, maxInstances: 2, concurrency: 10, timeoutSeconds: 60, memory: '256MiB' }, async (event) => {
  await deliver(event.params.deviceId, event.params.eventId);
});
