import { initializeApp } from '@firebase/app';
import { getAuth, onAuthStateChanged, signInWithEmailAndPassword, createUserWithEmailAndPassword, sendEmailVerification, signOut } from '@firebase/auth';
import { getMessaging, getToken, deleteToken, isSupported, onMessage } from '@firebase/messaging';
import { createConsole } from './console.js';
import { createPush, createTransport, pushMessage } from './client.js';

async function start() {
  const [configResponse, pushResponse] = await Promise.all([fetch('/__/firebase/init.json'), fetch('/push-config.json')]);
  if (!configResponse.ok || !pushResponse.ok) throw new Error('Deploy this app to your Firebase Hosting project and configure its public VAPID key.');
  const config = await configResponse.json();
  const { vapidKey } = await pushResponse.json();
  const app = initializeApp(config);
  const auth = getAuth(app);
  const api = createTransport({ fetcher: fetch, getToken: () => auth.currentUser.getIdToken() });
  const push = createPush({ browser: window, supported: isSupported, config, vapidKey,
    requestToken: (options) => getToken(getMessaging(app), options), removeToken: () => deleteToken(getMessaging(app)),
  });
  createConsole({ document, push, api, projectId: config.projectId, confirm: window.confirm.bind(window), auth: {
    user: () => auth.currentUser,
    watch: (callback) => onAuthStateChanged(auth, callback),
    signIn: (email, password) => signInWithEmailAndPassword(auth, email, password),
    signUp: async (email, password) => { const result = await createUserWithEmailAndPassword(auth, email, password); await sendEmailVerification(result.user); },
    verify: () => sendEmailVerification(auth.currentUser),
    refresh: async () => { await auth.currentUser.reload(); await auth.currentUser.getIdToken(true); },
    signOut: () => signOut(auth),
  } });
  if (await isSupported()) onMessage(getMessaging(app), async (payload) => {
    const message = pushMessage(payload.data);
    const registration = await navigator.serviceWorker.getRegistration();
    if (message && registration) await registration.showNotification(message.title, message.options);
  });
}

start().catch(() => {
  document.getElementById('notice').textContent = 'Notification app setup is incomplete or unavailable. Check Firebase Hosting configuration and connectivity.';
  document.getElementById('notice').dataset.error = 'true';
});
