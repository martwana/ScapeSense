// ScapeSense AWS IoT -> Firebase bridge (Phase 1 stub).
//
// Phase 3 replaces this handler with the real integration:
//   telemetry -> Firebase Realtime Database (throttled/aggregated writes)
//   events    -> Firestore via the authenticated Functions API -> FCM
// Credentials come from the Secrets Manager ARN in FIREBASE_SECRET_ARN;
// nothing secret lives in this file or in Terraform state.

export const handler = async (event) => {
  // IoT Rules injects deviceId (topic segment 2) and receivedAt.
  const deviceId = event?.deviceId;
  if (typeof deviceId !== 'string' || deviceId.length === 0) {
    console.warn(JSON.stringify({ msg: 'iot-bridge: dropping message without deviceId', event }));
    return { statusCode: 400 };
  }
  console.log(JSON.stringify({ msg: 'iot-bridge stub: accepted', deviceId, receivedAt: event?.receivedAt }));
  return { statusCode: 202 };
};
