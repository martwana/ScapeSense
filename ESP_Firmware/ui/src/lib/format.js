export function formatWeightG(value, valid = true) {
  if (!valid || !Number.isFinite(value)) return '--';
  if (Math.abs(value) <= 500) return `${Math.round(value)} g`;
  return `${Number((value / 1000).toFixed(2))} kg`;
}

export function formatDuration(seconds) {
  if (!Number.isFinite(seconds)) return '--';
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor((seconds % 86400) / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  if (days > 0) return `${days}d ${hours}h`;
  if (hours > 0) return `${hours}h ${minutes}m`;
  return `${minutes}m ${Math.floor(seconds % 60)}s`;
}

export function formatDayKey(dayKey) {
  const text = String(dayKey ?? '');
  if (!/^\d{8}$/.test(text)) return '--';
  return `${text.slice(6, 8)}/${text.slice(4, 6)}`;
}

export function formatDateTime(epochSeconds) {
  if (!epochSeconds) return '--';
  return new Date(epochSeconds * 1000).toLocaleString([], {
    day: '2-digit',
    month: 'short',
    hour: '2-digit',
    minute: '2-digit',
  });
}

export function dailyRecords(stats) {
  return [...(stats?.history || []), ...(stats?.current?.day ? [stats.current] : [])]
    .sort((a, b) => a.day - b.day);
}

export function dailyAverageBps(record, now = Date.now() / 1000) {
  const key = String(record.day);
  if (!/^\d{8}$/.test(key)) return null;
  const midnight = Date.UTC(Number(key.slice(0, 4)), Number(key.slice(4, 6)) - 1, Number(key.slice(6)));
  if (new Date(midnight).toISOString().slice(0, 10).replaceAll('-', '') !== key) return null;
  let flowingSeconds = record.activeSeconds;
  if (!Number.isFinite(flowingSeconds)) {
    const start = record.start;
    const end = record.active ? now : record.stop;
    if (!Number.isFinite(start) || !Number.isFinite(end)) return null;
    flowingSeconds = end - start;
  }
  return flowingSeconds > 0 ? (record.count || 0) / flowingSeconds : null;
}

export function adcPoints(samples, field) {
  return samples.map((sample, index) => `${samples.length > 1 ? index * 100 / (samples.length - 1) : 0},${100 - Math.max(0, Math.min(4095, sample[field])) * 100 / 4095}`).join(' ');
}

export function appendAdcSample(samples, status, connected) {
  if (!connected || !Number.isFinite(status.raw) || !Number.isFinite(status.baseline)) return samples;
  const previous = samples.at(-1);
  if (previous && status.uptime < previous.uptime) return [status];
  return [...samples, status].slice(-120);
}

export function gasTone(percent) {
  if (!Number.isFinite(percent)) return 'bg-white/20';
  if (percent < 20) return 'bg-coral';
  return 'bg-aqua';
}
