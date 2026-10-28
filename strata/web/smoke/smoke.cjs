// Smoke test for env-lib-cpp's browser build in headless Chromium (Playwright).
//
//   node strata/web/smoke/smoke.cjs <url> <out-dir> [--phone]
//
// Checks: the page starts with no page errors; both worlds open from their
// buttons, tick and draw (the canvas is not one flat colour); Pause stops
// the ticks and Step advances one; a real click (or tap) on the canvas goes
// through SDL and Strata's ray cast and picks a location, which the page
// lists. Writes a PNG per world and one of the whole page.
'use strict';
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const { chromium, devices } = require('playwright');

const [url, outDir] = process.argv.slice(2);
const phone = process.argv.includes('--phone');
if (!url || !outDir) {
  console.error('usage: smoke.cjs <url> <out-dir> [--phone]');
  process.exit(2);
}
fs.mkdirSync(outDir, { recursive: true });

// Minimal PNG decoder (8-bit RGB/RGBA, non-interlaced: what Chromium writes).
function decodePng(buf) {
  let pos = 8, width = 0, height = 0, channels = 0;
  const idat = [];
  while (pos < buf.length) {
    const len = buf.readUInt32BE(pos);
    const type = buf.toString('ascii', pos + 4, pos + 8);
    const data = buf.subarray(pos + 8, pos + 8 + len);
    if (type === 'IHDR') {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      channels = { 2: 3, 6: 4 }[data[9]];
      if (data[8] !== 8 || !channels || data[12] !== 0) throw new Error('unsupported PNG');
    } else if (type === 'IDAT') idat.push(data);
    pos += 12 + len;
  }
  const raw = zlib.inflateSync(Buffer.concat(idat));
  const stride = width * channels;
  const out = Buffer.alloc(height * stride);
  for (let y = 0; y < height; y++) {
    const f = raw[y * (stride + 1)];
    for (let x = 0; x < stride; x++) {
      const v = raw[y * (stride + 1) + 1 + x];
      const a = x >= channels ? out[y * stride + x - channels] : 0;
      const b = y > 0 ? out[(y - 1) * stride + x] : 0;
      const c = x >= channels && y > 0 ? out[(y - 1) * stride + x - channels] : 0;
      let p;
      if (f === 0) p = 0;
      else if (f === 1) p = a;
      else if (f === 2) p = b;
      else if (f === 3) p = (a + b) >> 1;
      else {
        const pa = Math.abs(b - c), pb = Math.abs(a - c), pc = Math.abs(a + b - 2 * c);
        p = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
      }
      out[y * stride + x] = (v + p) & 255;
    }
  }
  return { width, height, channels, data: out };
}

// Distinct colours (quantised to 4 bits per channel) in a PNG.
function colourCount(png) {
  const img = decodePng(png);
  const seen = new Set();
  for (let i = 0; i < img.data.length; i += img.channels)
    seen.add(((img.data[i] >> 4) << 8) | ((img.data[i + 1] >> 4) << 4) | (img.data[i + 2] >> 4));
  return seen.size;
}

const worlds = [
  { id: 'walkers', environment: 'Earth', grid: 8 },
  { id: 'meadow', environment: 'Meadow', grid: 20 },
];

(async () => {
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const base = phone ? devices['iPhone 13'] : { viewport: { width: 1280, height: 800 } };
  const context = await browser.newContext({ ...base, userAgent: (base.userAgent || 'Mozilla/5.0') + ' envlib-smoke headless' });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('console', (m) => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
  const fail = async (why) => {
    console.error('FAIL: ' + why);
    await page.screenshot({ path: path.join(outDir, 'failure.png') }).catch(() => {});
    await browser.close();
    process.exit(1);
  };

  // The main loop keeps the load event from ever firing: wait for commit only.
  await page.goto(url, { waitUntil: 'commit' });
  try {
    await page.waitForFunction(() => window.envlib, null, { timeout: 90000 });
  } catch (e) {
    await fail('the viewer never became ready: ' + errors.join(' | '));
  }
  const status = async () => JSON.parse(await page.evaluate(() => window.envlib.status()));
  const canvas = page.locator('#canvas');
  const press = async (sel) => {
    if (phone) await page.locator(sel).tap();
    else await page.locator(sel).click();
    await page.waitForTimeout(400);
  };

  for (let i = 0; i < worlds.length; i++) {
    const want = worlds[i];
    await press(`[data-world="${i}"]`);
    await page.waitForTimeout(2500);
    const s = await status();
    if (s.world !== want.id) await fail(`button ${i} opened ${s.world}, not ${want.id}`);
    if (s.environment !== want.environment || s.grid !== want.grid)
      await fail(`${s.world}: ${s.environment} ${s.grid}x${s.grid}, not ${want.environment} ${want.grid}x${want.grid}`);
    if (s.tick < 1) await fail(`${s.world}: no tick after 2.5 s (tick ${s.tick})`);
    if (s.entities < 1) await fail(`${s.world}: no entities`);
    const shot = await canvas.screenshot({ path: path.join(outDir, `${want.id}.png`), timeout: 180000 });
    const colours = colourCount(shot);
    console.log(`${s.world}: tick ${s.tick}, ${s.entities} entities, ${colours} colours`);
    if (colours < 40) await fail(`${s.world} drew only ${colours} colours`);
  }

  // Pause stops the clock; Step moves it one tick; Run starts it again.
  await press('#pause');
  let s = await status();
  if (!s.paused) await fail('Pause did not pause');
  const held = s.tick;
  await page.waitForTimeout(1200);
  if ((await status()).tick !== held) await fail('the ticks went on while paused');
  await press('#step');
  s = await status();
  if (s.tick !== held + 1) await fail(`Step went from tick ${held} to ${s.tick}`);
  await press('#pause');
  if ((await status()).paused) await fail('Run did not resume');
  console.log(`pause/step: held at ${held}, stepped to ${held + 1}`);

  // A real click or tap on the canvas, through SDL and the ray cast: the
  // middle of the canvas is the middle of the grid.
  await press('#home');
  const box = await canvas.boundingBox();
  const x = box.x + box.width * 0.5, y = box.y + box.height * 0.5;
  if (phone) await page.touchscreen.tap(x, y);
  else await page.mouse.click(x, y);
  await page.waitForTimeout(800);
  s = await status();
  console.log(`pick: (${s.pickX}, ${s.pickY}), ${s.picked} here`);
  if (s.pickX < 0 || s.pickY < 0) await fail('clicking the middle of the canvas picked no location');
  await page.waitForTimeout(400);
  if (!(await page.locator('#picked').isVisible())) await fail('the picked location is not listed on the page');
  // A point away from the middle, through SDL and through the page's own
  // call (CSS pixels, no SDL): the pointer scaling (a phone's device pixel
  // ratio) must agree, to within a cell where the point sits on an edge.
  await press('#home');
  const box2 = await canvas.boundingBox();
  const fx = 0.68, fy = 0.62;
  if (phone) await page.touchscreen.tap(box2.x + box2.width * fx, box2.y + box2.height * fy);
  else await page.mouse.click(box2.x + box2.width * fx, box2.y + box2.height * fy);
  await page.waitForTimeout(600);
  s = await status();
  const direct = await page.evaluate(([cx, cy]) => {
    window.envlib.pick(cx, cy);
    return JSON.parse(window.envlib.status());
  }, [box2.width * fx, box2.height * fy]);
  console.log(`pointer: (${s.pickX}, ${s.pickY}) through SDL, (${direct.pickX}, ${direct.pickY}) direct`);
  if (s.pickX < 0 || direct.pickX < 0 || Math.abs(s.pickX - direct.pickX) > 1 || Math.abs(s.pickY - direct.pickY) > 1)
    await fail(`a ${phone ? 'tap' : 'click'} picked (${s.pickX}, ${s.pickY}), but that point is (${direct.pickX}, ${direct.pickY})`);

  // The loading shell finished; and without WebGL 2 it shows the trailer (or
  // its still) and says why, and the engine never downloads.
  {
    await page.waitForFunction(() => { const r = document.querySelector('.sk-loader'); return r && r.hidden; }, null, { timeout: 10000 }).catch(() => {});
    const shell = await page.evaluate(() => {
      const r = document.querySelector('.sk-loader');
      return { hidden: !!(r && r.hidden), state: window.StrataLoader && StrataLoader.state };
    });
    if (!shell.hidden || shell.state !== 'done') await fail('the loading shell did not finish: ' + JSON.stringify(shell));
    console.log('ok: the loading shell showed and is gone');
    const bare = await browser.newContext({ ...base, userAgent: (base.userAgent || 'Mozilla/5.0') + ' envlib-smoke headless' });
    await bare.addInitScript(() => {
      const get = HTMLCanvasElement.prototype.getContext;
      HTMLCanvasElement.prototype.getContext = function (t, o) { return t === 'webgl2' ? null : get.call(this, t, o); };
    });
    const p2 = await bare.newPage();
    const engine = [];
    p2.on('request', (r) => { if (/envlib-strata\.(wasm|js)$/.test(r.url())) engine.push(r.url()); });
    await p2.goto(url, { waitUntil: 'commit' });
    await p2.waitForFunction(() => window.StrataLoader && StrataLoader.state === 'fallback', null, { timeout: 30000 });
    const fb = await p2.evaluate(() => {
      const r = document.querySelector('.sk-loader');
      return { text: r.innerText, media: !!r.querySelector('video, img'), wide: document.documentElement.scrollWidth > innerWidth };
    });
    await p2.screenshot({ path: path.join(outDir, 'no-webgl2.png') });
    if (!/WebGL 2/.test(fb.text) || fb.wide) await fail('without WebGL 2 the shell did not say why: ' + JSON.stringify(fb));
    if (engine.length) await fail('without WebGL 2 the engine still downloaded: ' + engine.join(', '));
    console.log('ok: without WebGL 2 the shell ' + (fb.media ? 'shows the trailer and ' : '') + 'says why, and nothing of the engine downloads');
    await bare.close();
  }

  if (errors.length) await fail('page errors: ' + errors.join(' | '));
  await page.screenshot({ path: path.join(outDir, 'page.png') });
  await browser.close();
  console.log('ok');
})().catch((e) => { console.error(e); process.exit(1); });
