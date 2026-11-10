export function createTransport({ fetcher, getToken }) {
  return async (path, method = 'GET', body) => {
    const response = await fetcher(path, {
      method, headers: { Authorization: `Bearer ${await getToken()}`, 'Content-Type': 'application/json' },
      ...(body === undefined ? {} : { body: JSON.stringify(body) }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.error || 'Request failed');
    return data;
  };
}

export function workerURL(config) {
  const fields = ['apiKey', 'authDomain', 'projectId', 'storageBucket', 'messagingSenderId', 'appId'];
  return `/firebase-messaging-sw.js?config=${encodeURIComponent(JSON.stringify(Object.fromEntries(fields.filter((key) => config[key] !== undefined).map((key) => [key, config[key]]))))}`;
}

export function pushMessage(data) {
  if (typeof data?.title !== 'string' || typeof data.body !== 'string' || data.title.length > 160 || data.body.length > 500 || !/^[a-f0-9]{32}$/.test(data.deviceId) || data.tag !== `scapesense-${data.deviceId}-${String(data.tag).split('-').at(-1)}` || !/^[a-f0-9]{16,64}$/.test(String(data.tag).split('-').at(-1))) return null;
  return { title: data.title, options: { body: data.body, tag: data.tag, icon: '/icon.png', data: { deviceId: data.deviceId } } };
}

export async function openNotification(clients, origin, data) {
  const url = new URL('/', origin);
  if (/^[a-f0-9]{32}$/.test(data?.deviceId)) url.searchParams.set('device', data.deviceId);
  const windows = await clients.matchAll({ type: 'window', includeUncontrolled: true });
  const existing = windows.find((client) => new URL(client.url).origin === origin);
  if (existing) {
    await existing.navigate(url.href);
    return existing.focus();
  }
  return clients.openWindow(url.href);
}

export function createPush({ browser, supported, requestToken, removeToken, config, vapidKey }) {
  let token = null;
  return {
    async enable() {
      if (!vapidKey) throw new Error('Configure the public Firebase Web Push VAPID key before enabling notifications');
      if (!browser.Notification || !browser.navigator.serviceWorker) throw new Error('Push is not supported in this browser');
      const permission = await browser.Notification.requestPermission();
      if (permission !== 'granted') throw new Error('Notification permission was not granted. On iPhone, open the Home Screen app first.');
      if (!await supported()) throw new Error('Push is not supported in this browser');
      await browser.navigator.serviceWorker.register(workerURL(config));
      const registration = await browser.navigator.serviceWorker.ready;
      token = await requestToken({ vapidKey, serviceWorkerRegistration: registration });
      return token;
    },
    async disable(unsubscribe) {
      if (!browser.Notification || !browser.navigator.serviceWorker || !await supported()) return;
      if (browser.Notification.permission !== 'granted') return;
      const registration = await browser.navigator.serviceWorker.getRegistration();
      if (!registration?.active) return;
      if (!token) token = await requestToken({ vapidKey, serviceWorkerRegistration: registration });
      await unsubscribe(token);
      await removeToken();
      token = null;
    },
  };
}
