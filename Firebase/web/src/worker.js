import { initializeApp } from '@firebase/app';
import { getMessaging, onBackgroundMessage } from '@firebase/messaging/sw';
import { openNotification, pushMessage } from './client.js';

self.addEventListener('notificationclick', (event) => {
  event.notification.close();
  event.stopImmediatePropagation();
  event.waitUntil(openNotification(self.clients, self.location.origin, event.notification.data));
});
const config = JSON.parse(new URL(self.location.href).searchParams.get('config'));
const messaging = getMessaging(initializeApp(config));
onBackgroundMessage(messaging, async (payload) => {
  const message = pushMessage(payload.data);
  if (message) await self.registration.showNotification(message.title, message.options);
});
