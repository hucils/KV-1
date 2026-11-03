/*
  The controller page the ESP32 sends to the phone.
  Every 50 ms it sends "throttle,steer,turret,gun" (-511..511) over a WebSocket on
  port 81, plus "mode" or "fire" when those buttons are pressed. The ESP32 answers
  "slow", "fast", "fire" (shot started), "ready" (can fire again) or "busy".
  Leaving the page closes the socket, which stops the tank.
*/
#pragma once

const char PHONE_PAGE[] PROGMEM = R"PAGE(<!doctype html>
<html lang="ro">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="mobile-web-app-capable" content="yes">
<title>Tanc KV-1</title>
<style>
*{box-sizing:border-box;margin:0;-webkit-tap-highlight-color:transparent;-webkit-touch-callout:none;-webkit-user-select:none;user-select:none}
html,body{height:100%;overflow:hidden;overscroll-behavior:none;background:#14171c;color:#eef1f5;font:16px system-ui,sans-serif}
body{display:flex;align-items:center;justify-content:space-between;gap:8px;padding:8px max(12px,env(safe-area-inset-right)) 8px max(12px,env(safe-area-inset-left));touch-action:none}
.stick{min-width:0;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px;touch-action:none}
.v{flex:0 0 96px}
.h{flex:1 1 0}
.track{position:relative;background:#262b33;border-radius:44px;touch-action:none}
.v .track{width:88px;height:min(72vh,300px)}
.h .track{width:min(100%,260px);height:88px}
.pad .track{width:min(58vh,190px);height:min(58vh,190px)}
.knob{position:absolute;left:50%;top:50%;width:76px;height:76px;margin:-38px 0 0 -38px;border-radius:50%;background:#4d9fff;pointer-events:none}
.pad .knob{background:#e0a23c}
.label{font-size:13px;color:#9aa4b2;text-align:center}
.middle{flex:0 0 96px;display:flex;flex-direction:column;align-items:center;gap:8px;text-align:center}
#mode{width:100%;min-height:56px;border:0;border-radius:14px;background:#2f9e5b;color:#fff;font:600 16px system-ui,sans-serif;touch-action:manipulation}
#mode.fast{background:#d6453d}
#mode:disabled{background:#3a404a;color:#9aa4b2}
#status{font-size:13px;line-height:1.3;color:#9aa4b2;min-height:3.9em}
#status.ok{color:#5fd38d}
small{font-size:11px;color:#6d7682}
.gun{flex:0 0 auto;height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:6px}
#fire{width:min(58vh,190px);flex:0 1 72px;min-height:48px;border:0;border-radius:14px;background:#d6453d;color:#fff;font:700 20px system-ui,sans-serif;touch-action:none}
#fire:disabled{background:#3a404a;color:#9aa4b2;font-size:15px}
@media (orientation:portrait){
body{flex-wrap:wrap;align-content:center;justify-content:center;gap:16px}
.middle{order:-1;flex:1 1 100%}
#mode{width:160px}
#status{min-height:1.3em}
.v{flex:0 0 96px}
.h{flex:1 1 50%}
.v .track{height:220px}
.gun{flex:1 1 100%;height:auto}
.pad .track,#fire{width:190px}
.pad .track{height:190px}
#fire{flex:0 0 64px}
}
</style>
</head>
<body>
<div class="stick v"><div class="track" id="throttle"><div class="knob"></div></div><div class="label">Înainte / înapoi</div></div>
<div class="stick h"><div class="track" id="steer"><div class="knob"></div></div><div class="label">Viraj</div></div>
<div class="middle">
<button id="mode" disabled>Încet</button>
<div id="status" role="status">Se conectează…</div>
<small>KV-1</small>
</div>
<div class="gun">
<div class="stick pad"><div class="track" id="aim"><div class="knob"></div></div><div class="label">Turelă și tun</div></div>
<button id="fire" disabled>FOC</button>
</div>
<script>
const MAX = 511;
const statusEl = document.getElementById('status');
const modeEl = document.getElementById('mode');
const fireEl = document.getElementById('fire');
const axes = {throttle: 0, steer: 0, turret: 0, gun: 0};
const sticks = [];
let ws = null;
let busy = false;
let connected = false;
let reloading = false;

function showFire() {
  fireEl.disabled = !connected || reloading;
  fireEl.textContent = reloading ? 'Se încarcă…' : 'FOC';
}

function setStatus(text, ok) {
  statusEl.textContent = text;
  statusEl.className = ok ? 'ok' : '';
  modeEl.disabled = !ok;
  connected = ok;
  showFire();
}

// A stick moves along the axes it is given and springs back to the middle when released
function bindStick(id, xAxis, yAxis) {
  const track = document.getElementById(id);
  const knob = track.firstElementChild;
  let pointer = null;

  function place(x, y) {
    if (xAxis) axes[xAxis] = Math.round(x * MAX);
    if (yAxis) axes[yAxis] = Math.round(y * MAX);
    const roomX = track.clientWidth / 2 - 44;
    const roomY = track.clientHeight / 2 - 44;
    knob.style.transform = `translate(${x * roomX}px,${-y * roomY}px)`;
  }
  function follow(e) {
    const box = track.getBoundingClientRect();
    const roomX = box.width / 2 - 44;
    const roomY = box.height / 2 - 44;
    const x = xAxis ? (e.clientX - box.left - box.width / 2) / roomX : 0;
    const y = yAxis ? (box.top + box.height / 2 - e.clientY) / roomY : 0;
    place(Math.max(-1, Math.min(1, x)), Math.max(-1, Math.min(1, y)));
  }
  function release(e) {
    if (e.pointerId !== pointer) return;
    pointer = null;
    place(0, 0);
  }
  track.addEventListener('pointerdown', e => {
    e.preventDefault();
    pointer = e.pointerId;
    track.setPointerCapture(pointer);
    follow(e);
  });
  track.addEventListener('pointermove', e => {
    if (e.pointerId === pointer) follow(e);
  });
  track.addEventListener('pointerup', release);
  track.addEventListener('pointercancel', release);
  track.addEventListener('lostpointercapture', release);
  sticks.push(() => { pointer = null; place(0, 0); });
}

function centerSticks() {
  sticks.forEach(reset => reset());
}

function connect() {
  if (document.hidden || (ws && ws.readyState <= WebSocket.OPEN)) return;
  ws = new WebSocket(`ws://${location.hostname}:81/`);
  ws.onopen = () => {
    busy = false;
    setStatus('Conectat', true);
  };
  ws.onmessage = e => {
    if (e.data === 'busy') {
      busy = true;
      setStatus('Altcineva conduce tancul', false);
    } else if (e.data === 'fire' || e.data === 'ready') {
      reloading = e.data === 'fire';
      showFire();
    } else {
      modeEl.textContent = e.data === 'fast' ? 'Repede' : 'Încet';
      modeEl.classList.toggle('fast', e.data === 'fast');
    }
  };
  ws.onclose = () => {
    if (!busy) setStatus('Fără legătură. Încerc din nou…', false);
    setTimeout(connect, 1000);
  };
}

function leavePage() {
  centerSticks();
  if (ws) ws.close();
}

bindStick('throttle', null, 'throttle');
bindStick('steer', 'steer', null);
bindStick('aim', 'turret', 'gun');

modeEl.addEventListener('click', () => {
  if (ws && ws.readyState === WebSocket.OPEN) ws.send('mode');
});

// pointerdown, because phones skip "click" while another finger is on a stick
fireEl.addEventListener('pointerdown', e => {
  e.preventDefault();
  if (!fireEl.disabled && ws && ws.readyState === WebSocket.OPEN) ws.send('fire');
});

// Skipped while the network is behind, so old commands never pile up
setInterval(() => {
  if (ws && ws.readyState === WebSocket.OPEN && ws.bufferedAmount === 0) {
    ws.send(`${axes.throttle},${axes.steer},${axes.turret},${axes.gun}`);
  }
}, 50);

window.addEventListener('blur', centerSticks);
window.addEventListener('pagehide', leavePage);
window.addEventListener('pageshow', connect);
document.addEventListener('visibilitychange', () => {
  if (document.hidden) leavePage();
  else connect();
});
document.addEventListener('contextmenu', e => e.preventDefault());
document.addEventListener('gesturestart', e => e.preventDefault());

connect();
</script>
</body>
</html>
)PAGE";
