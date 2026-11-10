export function createConsole({ document, auth, push, api, projectId, confirm }) {
  const element = (id) => document.getElementById(id);
  const notice = (text, error = false) => { element('notice').textContent = text; element('notice').dataset.error = String(error); };
  let busy = false;
  let selected = null;
  const clearPairing = () => { element('pairing-details').value = ''; element('pairing').hidden = true; };
  const run = (action) => async (event) => {
    event.preventDefault();
    if (busy) return;
    busy = true;
    for (const button of document.querySelectorAll('button')) button.disabled = true;
    try { await action(); } catch (error) { notice(error.message, true); }
    finally { busy = false; for (const button of document.querySelectorAll('button')) button.disabled = false; }
  };
  const selectedIds = () => [...element('devices').querySelectorAll('input')].filter((input) => input.checked && !input.disabled).map((input) => input.value);
  const history = async (id) => {
    const result = await api(`/api/devices/${id}/events`);
    element('event-list').replaceChildren();
    for (const event of result.events) {
      const row = document.createElement('li');
      row.textContent = `${new Date(event.occurredAt * 1000).toLocaleString()} · CO₂ ${event.kind} · ${event.total} bubbles · ${event.state}`;
      element('event-list').append(row);
    }
    if (!result.events.length) element('event-list').textContent = 'No events received yet.';
    element('history').hidden = false;
  };
  const loadDevices = async () => {
    const { devices } = await api('/api/devices');
    if (selected === null) selected = new Set(devices.filter((device) => device.enabled).map((device) => device.deviceId));
    element('devices').replaceChildren();
    for (const device of devices) {
      const row = document.createElement('div');
      row.className = 'device';
      const label = document.createElement('label');
      label.className = 'check';
      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox'; checkbox.value = device.deviceId; checkbox.checked = device.enabled && selected.has(device.deviceId); checkbox.disabled = !device.enabled;
      checkbox.addEventListener('change', () => { selected = new Set(selectedIds()); });
      const name = document.createElement('span');
      name.textContent = `${device.name}${device.enabled ? '' : ' (revoked)'}`;
      label.append(checkbox, name); row.append(label);
      const recent = document.createElement('button');
      recent.textContent = 'Recent events'; recent.className = 'secondary';
      recent.addEventListener('click', run(() => history(device.deviceId))); row.append(recent);
      if (device.enabled) {
        const revoke = document.createElement('button');
        revoke.textContent = 'Revoke'; revoke.className = 'secondary';
        revoke.addEventListener('click', run(async () => {
          if (!confirm(`Revoke ${device.name}? It will stop reporting until paired again.`)) return;
          await api(`/api/devices/${device.deviceId}`, 'DELETE');
          await loadDevices(); notice('Device revoked.');
        }));
        row.append(revoke);
      }
      element('devices').append(row);
    }
    if (!devices.length) element('devices').textContent = 'No devices paired yet.';
  };
  const disablePush = () => push.disable((token) => api('/api/subscription', 'DELETE', { token }));
  element('sign-in').addEventListener('submit', run(async () => {
    await auth.signIn(element('email').value, element('password').value);
    element('password').value = '';
  }));
  element('create-account').addEventListener('click', run(async () => {
    if (!element('email').value || element('password').value.length < 12) throw new Error('Enter your email and a password of at least 12 characters.');
    await auth.signUp(element('email').value, element('password').value);
    element('password').value = ''; notice('Account created. Check your verification email.');
  }));
  element('refresh-verification').addEventListener('click', run(async () => { await auth.refresh(); await accountChanged(auth.user()); }));
  element('resend-verification').addEventListener('click', run(async () => { await auth.verify(); notice('Verification email sent.'); }));
  element('create-device').addEventListener('submit', run(async () => {
    const result = await api('/api/devices', 'POST', { name: element('device-name').value });
    selected ||= new Set(); selected.add(result.deviceId);
    await loadDevices();
    element('pairing-details').value = JSON.stringify({ projectId, deviceId: result.deviceId, deviceKey: result.deviceKey }, null, 2);
    element('pairing').hidden = false; element('device-name').value = '';
    notice('Device paired. Copy the one-time key into your local dashboard.');
  }));
  element('hide-pairing').addEventListener('click', clearPairing);
  element('enable-push').addEventListener('click', run(async () => {
    const deviceIds = selectedIds();
    if (!deviceIds.length) throw new Error('Select at least one aquarium.');
    const started = element('started').checked, stopped = element('stopped').checked;
    if (!started && !stopped) throw new Error('Choose at least one notification type.');
    const token = await push.enable();
    await api('/api/subscription', 'PUT', { token, deviceIds, started, stopped });
    notice('Notifications enabled for this browser. Click Enable / update again after changing your selections.');
  }));
  element('disable-push').addEventListener('click', run(async () => { await disablePush(); notice('Notifications disabled for this browser.'); }));
  element('sign-out').addEventListener('click', run(async () => { await disablePush(); clearPairing(); await auth.signOut(); }));
  async function accountChanged(user) {
    element('signed-out').hidden = Boolean(user);
    element('verify').hidden = !user || user.emailVerified;
    element('signed-in').hidden = !user?.emailVerified;
    element('sign-out').hidden = !user;
    clearPairing(); element('history').hidden = true;
    if (!user) { selected = null; notice('Sign in to manage your aquariums.'); }
    else if (!user.emailVerified) notice('Verify your email to continue.');
    else { await loadDevices(); notice(`Signed in as ${user.email}.`); }
  }
  const stop = auth.watch((user) => { accountChanged(user).catch((error) => notice(error.message, true)); });
  return { destroy: stop, refresh: loadDevices };
}
