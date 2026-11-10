export function readPreference(storage, key, choices, fallback) {
  try {
    const value = storage.getItem(key);
    return choices.includes(value) ? value : fallback;
  } catch {
    return fallback;
  }
}

export function savePreference(storage, key, value) {
  try { storage.setItem(key, value); } catch {}
}

export function temperatureLimitsValid(target, tolerance) {
  return Number.isFinite(target) && target >= 0 && target <= 40 &&
    Number.isFinite(tolerance) && tolerance >= 0.1 && tolerance <= 10;
}

export function bubbleTarget(rate, target) {
  if (!Number.isFinite(target) || target <= 0 || target > 20 || !Number.isFinite(rate)) return null;
  const ratio = rate / target;
  return { label: ratio < 0.9 - 1e-9 ? 'Below target' : ratio > 1.1 + 1e-9 ? 'Above target' : 'On target', position: Math.max(0, Math.min(100, ratio * 50)) };
}

export function widgetStates(status, connected, maxBps = 10) {
  const unavailable = { tone: 'warning', label: connected ? 'Reading unavailable' : 'Disconnected' };
  const healthy = { tone: 'healthy', label: 'Healthy' };
  const scaleValid = connected && status.weightValid && status.loadCellCalibrated && Number.isFinite(status.weightG);
  const hasEmptyCylinderWeight = Number.isFinite(status.emptyCylinderG) && status.emptyCylinderG > 0;
  const gasAbsent = scaleValid && (hasEmptyCylinderWeight
    ? status.weightG < status.emptyCylinderG
    : Math.abs(status.weightG) < 10);
  const gasValid = scaleValid && !gasAbsent && status.gasConfigured && Number.isFinite(status.gasRemainingPercent);
  const tempValid = connected && status.temperatureValid && Number.isFinite(status.temperatureC);
  const limitsValid = temperatureLimitsValid(status.temperatureTargetC, status.temperatureToleranceC);
  return {
    bubbles: !connected ? unavailable : status.status !== 'ok' ? { tone: 'danger', label: 'Check bubble sensor' } :
      !status.reliable || status.bps > maxBps ? { tone: 'danger', label: 'Bubble rate too high' } : { ...healthy, label: status.bps > 0 ? 'Flowing' : 'Idle' },
    gas: gasAbsent ? { tone: 'neutral', label: 'Not present' } : !gasValid ? unavailable :
      status.gasRemainingPercent <= 0 ? { tone: 'danger', label: 'CO₂ empty', severity: 'empty' } :
      status.gasRemainingPercent < 5 ? { tone: 'danger', label: 'CO₂ critically low', severity: 'critical' } :
      status.gasRemainingPercent < 10 ? { tone: 'danger', label: 'CO₂ very low', severity: 'very-low' } :
      status.gasRemainingPercent < 20 ? { tone: 'danger', label: 'CO₂ low', severity: 'low' } : healthy,
    temperature: !tempValid ? unavailable : !limitsValid ? { tone: 'warning', label: 'Set temperature limits' } :
      status.temperatureC < status.temperatureTargetC - status.temperatureToleranceC ? { tone: 'danger', label: 'Temperature too low' } :
      status.temperatureC > status.temperatureTargetC + status.temperatureToleranceC ? { tone: 'danger', label: 'Temperature too high' } : { ...healthy, label: 'Within target range' },
    network: !connected ? unavailable : status.rssi < -75 ? { tone: 'warning', label: 'Weak Wi-Fi signal' } : { ...healthy, label: 'Connected' },
    gasValid, gasAbsent, tempValid,
  };
}

export function exportSnapshot(data, connected, now = new Date()) {
  const statusKeys = ['total', 'bps', 'bpm', 'bubbleTargetBps', 'baseline', 'raw', 'reliable', 'status', 'temperatureC', 'temperatureValid', 'temperatureTargetC', 'temperatureToleranceC', 'weightG', 'weightValid', 'loadCellRaw', 'loadCellCalibrated', 'loadCellBusy', 'emptyCylinderG', 'co2CapacityG', 'gasRemainingG', 'gasRemainingPercent', 'gasConfigured', 'rssi', 'uptime', 'firmware'];
  const configKeys = ['threshold', 'hysteresis', 'minSeparationMs', 'maxReliableBps', 'polarity'];
  const pick = (source, keys) => Object.fromEntries(keys.filter((key) => source?.[key] !== undefined).map((key) => [key, source[key]]));
  const record = (value) => pick(value, ['day', 'count', 'start', 'stop', 'active']);
  return {
    schemaVersion: 1, product: 'ScapeSense', exportedAt: now.toISOString(), connected,
    status: pick(data.status, statusKeys), configuration: pick(data.config, configKeys),
    dailyStats: { current: record(data['daily-stats']?.current), history: (data['daily-stats']?.history || []).map(record) },
  };
}

export function downloadSnapshot(snapshot, browser = window) {
  const url = browser.URL.createObjectURL(new Blob([JSON.stringify(snapshot, null, 2)], { type: 'application/json' }));
  const link = browser.document.createElement('a');
  link.href = url;
  link.download = `scapesense-${snapshot.exportedAt.slice(0, 10)}.json`;
  browser.document.body.appendChild(link);
  link.click();
  link.remove();
  browser.setTimeout(() => browser.URL.revokeObjectURL(url), 1000);
}
