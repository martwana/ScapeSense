import test from 'node:test';
import assert from 'node:assert/strict';
import { subscribeToDevice } from './api.js';

class FakeEventSource {
  static instance;
  listeners = {};
  closed = false;
  constructor(path) {
    this.path = path;
    FakeEventSource.instance = this;
  }
  addEventListener(type, listener) {
    this.listeners[type] = listener;
  }
  emit(type, data) {
    this.listeners[type]({ data });
  }
  close() {
    this.closed = true;
  }
}

test('SSE receives all snapshots and closes on cleanup', () => {
  const received = [];
  const connections = [];
  const stop = subscribeToDevice((...args) => received.push(args),
    (value) => connections.push(value), FakeEventSource);
  const source = FakeEventSource.instance;
  try {
    assert.equal(source.path, '/api/events');
    source.emit('status', '{"total":12}');
    source.emit('config', '{"threshold":100}');
    source.emit('daily-stats', '{"current":{"count":12}}');
    source.emit('load-cell', '{"id":1,"state":"succeeded"}');
    source.emit('settings-result', '{"operation":"Temperature settings","success":true}');
    assert.deepEqual(received.map(([type]) => type), ['status', 'config', 'daily-stats', 'load-cell', 'settings-result']);
    assert.deepEqual(connections, [true]);
    source.onerror();
    assert.deepEqual(connections, [true, false]);
  } finally {
    stop();
  }
  assert.equal(source.closed, true);
});

test('SSE marks the device disconnected when status updates go stale', (t) => {
  t.mock.timers.enable({ apis: ['setInterval', 'Date'], now: 100000 });
  const connections = [];
  const stop = subscribeToDevice(() => {}, (value) => connections.push(value), FakeEventSource);
  try {
    FakeEventSource.instance.emit('status', '{"total":1}');
    t.mock.timers.tick(8000);
    assert.deepEqual(connections, [true]);
    t.mock.timers.tick(4000);
    assert.equal(connections.at(-1), false);
  } finally {
    stop();
  }
});

test('SSE ignores malformed payloads and empty initial status', () => {
  const received = [];
  const connections = [];
  const stop = subscribeToDevice((...args) => received.push(args),
    (value) => connections.push(value), FakeEventSource);
  try {
    const source = FakeEventSource.instance;
    for (const value of ['broken', 'null', '[]', '{}']) source.emit('status', value);
    assert.deepEqual(received, []);
    assert.deepEqual(connections, []);
  } finally {
    stop();
  }
});
