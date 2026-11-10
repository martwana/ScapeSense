export async function fetchJson(path, signal) {
  const response = await fetch(path, { cache: 'no-store', signal });
  if (!response.ok) throw new Error(`Request failed: ${response.status}`);
  return response.json();
}

export function subscribeToDevice(onData, onConnection, EventSourceClass = globalThis.EventSource) {
  const source = new EventSourceClass('/api/events');
  let lastStatus = 0;
  const timer = setInterval(() => {
    if (Date.now() - lastStatus > 10000) onConnection(false);
  }, 2000);
  for (const type of ['status', 'config', 'daily-stats', 'load-cell', 'settings-result']) {
    source.addEventListener(type, (event) => {
      let data;
      try {
        data = JSON.parse(event.data);
      } catch {
        return;
      }
      if (!data || typeof data !== 'object' || Array.isArray(data)) return;
      if (type === 'status') {
        if (!Number.isFinite(data.total)) return;
        lastStatus = Date.now();
        onConnection(true);
      }
      onData(type, data);
    });
  }
  source.onerror = () => onConnection(false);
  return () => {
    clearInterval(timer);
    source.close();
  };
}

export function toFormBody(fields) {
  const body = new URLSearchParams();
  Object.entries(fields).forEach(([key, value]) => body.set(key, String(value)));
  return body.toString();
}

export async function postForm(path, fields = {}) {
  const response = await fetch(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: toFormBody(fields),
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload.error || `Request failed: ${response.status}`);
  return payload;
}
