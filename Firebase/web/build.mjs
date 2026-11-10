import { build } from 'esbuild';
import { cp, mkdir, copyFile } from 'node:fs/promises';

await mkdir('dist', { recursive: true });
await cp('public', 'dist', { recursive: true });
await copyFile('../../Logos/logo_01_aquascape_symbol.svg', 'dist/logo.svg');
await copyFile('../../LogoNoText.png', 'dist/icon.png');
await build({ entryPoints: ['src/main.js'], bundle: true, minify: true, target: 'es2022', format: 'esm', outfile: 'dist/app.js' });
await build({ entryPoints: ['src/worker.js'], bundle: true, minify: true, target: 'es2022', format: 'iife', outfile: 'dist/firebase-messaging-sw.js' });
