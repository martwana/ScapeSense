export class FakeFirestore {
  records = new Map();
  collection(path) { return new Query(this, path); }
  async runTransaction(action) {
    const writes = [];
    let writing = false;
    const tx = {
      get: async (ref) => { if (writing) throw new Error('Reads must precede writes'); return ref.get(); },
      create: (ref, data) => { writing = true; writes.push(() => { if (this.records.has(ref.path)) throw new Error('Already exists'); this.records.set(ref.path, structuredClone(data)); }); },
      set: (ref, data) => { writing = true; writes.push(() => this.records.set(ref.path, structuredClone(data))); },
      update: (ref, data) => { writing = true; writes.push(() => { if (!this.records.has(ref.path)) throw new Error('Not found'); this.records.set(ref.path, { ...this.records.get(ref.path), ...structuredClone(data) }); }); },
      delete: (ref) => { writing = true; writes.push(() => this.records.delete(ref.path)); },
    };
    const result = await action(tx);
    for (const write of writes) write();
    return result;
  }
}

class Doc {
  constructor(db, path) { this.db = db; this.path = path; this.id = path.split('/').at(-1); }
  collection(path) { return this.db.collection(`${this.path}/${path}`); }
  async get() {
    const data = this.db.records.get(this.path);
    return { id: this.id, exists: data !== undefined, data: () => structuredClone(data) };
  }
  async delete() { this.db.records.delete(this.path); }
  async update(data) {
    if (!this.db.records.has(this.path)) throw new Error('Not found');
    this.db.records.set(this.path, { ...this.db.records.get(this.path), ...structuredClone(data) });
  }
}

class Query {
  constructor(db, path, filters = [], count = Infinity, order = null) { Object.assign(this, { db, path, filters, count, order }); }
  doc(id) { return new Doc(this.db, `${this.path}/${id}`); }
  where(field, operator, value) {
    if (operator !== '==') throw new Error('Unsupported query');
    return new Query(this.db, this.path, [...this.filters, [field, value]], this.count, this.order);
  }
  limit(count) { return new Query(this.db, this.path, this.filters, count, this.order); }
  orderBy(field, direction) { return new Query(this.db, this.path, this.filters, this.count, [field, direction]); }
  async get() {
    let values = [...this.db.records.entries()].filter(([path, data]) => path.startsWith(`${this.path}/`) && !path.slice(this.path.length + 1).includes('/') && this.filters.every(([field, value]) => data[field] === value));
    if (this.order) values.sort((a, b) => (a[1][this.order[0]] - b[1][this.order[0]]) * (this.order[1] === 'desc' ? -1 : 1));
    const docs = await Promise.all(values.slice(0, this.count).map(([path]) => new Doc(this.db, path).get()));
    return { docs, size: docs.length };
  }
}
