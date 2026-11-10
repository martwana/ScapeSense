# ScapeSense Firebase Notifications

This folder contains a Firebase-hosted push notification service and installable
web app. It complements the local ESP32 dashboard; the device keeps measuring if
the remote service or Internet is unavailable.

## Build and test

Use Node.js 22.22.0 or a later Node 22 release:

```bash
cd Firebase/functions
npm ci
npm run test:coverage
npm audit
cd ../web
npm ci
npm run test:coverage
npm audit
npm run build
```

The authenticated API, Firestore transactions, Firebase trigger wiring,
notification app and service worker have local unit tests. Tests use in-memory
Firestore and mocked Firebase SDKs; they do not create cloud resources or send
real notifications. `web/build.mjs` copies the public ScapeSense logo to its
build directory; `web/dist/` is generated and must not be edited by hand.

## Firebase project setup

No Firebase project or cloud resources are created by these local builds.
Before deployment:

1. Create a ScapeSense Firebase project in a supported region. Cloud Functions
   require the Blaze plan. Review Google Cloud budgets and billing alerts first;
   budgets warn but do not cap spending.
2. Enable Email/Password authentication, email verification and the project's
   default `web.app` authorized domain.
3. Create the default Firestore database in `europe-west2`. Deploy the provided
   deny-all client rules, indexes and `expiresAt` TTL override.
4. In Firebase Cloud Messaging, generate a Web Push VAPID key. Put its **public**
   key in `web/public/push-config.json`; do not put service-account credentials
   or private VAPID material in the web app.
5. Build `web` and deploy Hosting, Functions and Firestore with the Firebase CLI
   after selecting the intended Firebase project. Never deploy until the
   project, region and billing account are confirmed.
6. Grant the Functions runtime service account only the permissions it needs:
   Firestore data access, Firebase Authentication user lookup (revocation
   checking), and Firebase Cloud Messaging send. Use Google's Application
   Default Credentials; do not download or check in a service-account key.
7. Register a verified account in the hosted app, pair the ESP32, copy the
   one-time device key into local Settings, and explicitly enable browser push.
   On iOS/iPadOS, first add the HTTPS app to the Home Screen.

## Security and delivery

- The browser authenticates with Firebase Auth; device requests use an
  independent, revocable per-device secret. Device secrets are hashed in
  Firestore and never sent to Firebase clients or included in JSON exports.
- Firestore client rules deny all direct access. The Functions server verifies
  email-verified ID tokens and checks ownership on every management operation.
- The ESP32 validates HTTPS using the ESP-IDF trust bundle and reports only
  confirmed flow transitions; continuous local operation never awaits network
  requests.
- The ESP32 outbox is bounded (8 events), RAM-only, retried with jittered
  backoff, and expires events after 24 hours. A power loss can discard queued
  notifications; this is best-effort alerting, not an alarm or safety system.
- Flow starts are confirmed after 12 bubbles in 60 seconds; stops after 300
  seconds without a qualifying one-minute window. Push requires phone/browser
  permission and Internet connectivity; delivery latency is not guaranteed.
- API limits and `maxInstances` reduce accidental load, but are not a hard
  billing cap. Set Google Cloud budgets and monitor Firebase usage.
