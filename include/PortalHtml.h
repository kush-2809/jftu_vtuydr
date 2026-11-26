#pragma once
#include <Arduino.h>
const char SMARTTANK_PORTAL_HTML[] PROGMEM = R"stportal(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#081320"><title>SmartTank · Control Center</title>
<style>
:root{font-family:system-ui,-apple-system,"Segoe UI",sans-serif;color-scheme:dark;--bg:#081320;--panel:#111f30;--panel2:#182c42;--border:#2a4055;--text:#edf6fb;--muted:#94aabe;--accent:#19c9b2;--red:#f57886;--amber:#f8bd67}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at top right,#112e42,var(--bg) 55%);color:var(--text);min-height:100vh}main{max-width:1180px;margin:0 auto;padding:24px}
h1{margin:0;font-size:clamp(26px,3vw,36px);letter-spacing:-.04em}h2{font-size:17px;margin:0 0 14px}p{line-height:1.55}button,input,select{font:inherit}button{cursor:pointer;background:var(--accent);color:#052324;border:0;border-radius:10px;padding:11px 16px;font-weight:700}button:hover{filter:brightness(1.13)}button:disabled{opacity:.5;cursor:wait}button.alt{background:#2b4459;color:var(--text)}button.danger{background:#a94855;color:#fff}input,select{display:block;width:100%;padding:11px 12px;margin:6px 0 15px;border-radius:10px;border:1px solid #38556b;background:#091727;color:var(--text);min-width:0}input:focus,select:focus{outline:2px solid var(--accent)}label{font-size:13px;color:#c1d3e0}small,.muted{font-size:13px;color:var(--muted)}.panel{background:var(--panel);border:1px solid var(--border);border-radius:18px;padding:20px;min-width:0}.top{display:flex;align-items:center;justify-content:space-between;gap:14px;margin-bottom:16px}.eyebrow{font-size:11px;font-weight:800;letter-spacing:.17em;color:var(--accent)}.chip{border-radius:99px;border:1px solid #296754;background:#163b37;padding:9px 13px;font-size:12px;color:#8ae7c9}.nav{display:flex;flex-wrap:wrap;gap:8px;margin:0 0 18px}.nav button{background:#162b3d;color:#b8d1df;border:1px solid var(--border)}.nav button.active{color:#082323;background:var(--accent)}.page{display:none}.page.active{display:block}.grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:13px}.grid .span2{grid-column:span 2}.value{font-size:clamp(25px,4vw,36px);font-weight:800;letter-spacing:-.04em;margin:8px 0}.accent{color:var(--accent)}.muted{overflow-wrap:anywhere}.row{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px}.actions{display:flex;flex-wrap:wrap;gap:8px;margin:9px 0}.alert{background:#442131;border-color:#a64b5e}.alert strong{color:#ffb6bd}.count{font-size:28px;font-weight:800;font-variant-numeric:tabular-nums}.network{width:100%;display:flex;align-items:center;justify-content:space-between;gap:15px;margin:5px 0;background:var(--panel2);color:var(--text);text-align:left}.network small{white-space:nowrap}.hidden,[hidden]{display:none!important}.auth{max-width:460px;margin:8vh auto}.message{min-height:20px;margin:8px 0;color:var(--muted);font-size:13px}.help{padding:12px;background:#1b3444;border-radius:9px;font-size:13px;color:#bfd6e0;margin:8px 0}.divider{height:1px;background:var(--border);margin:19px 0}.weather{display:flex;align-items:center;gap:10px}.weather .value{margin:0}footer{font-size:12px;color:var(--muted);margin-top:20px}
@media(max-width:880px){.grid{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:540px){main{padding:10px}.top{align-items:flex-start;padding:16px}.grid{grid-template-columns:minmax(0,1fr)}.grid .span2{grid-column:auto}.row{grid-template-columns:minmax(0,1fr);gap:0}.panel{padding:16px;border-radius:14px}.nav{flex-wrap:nowrap;overflow:auto}.nav button{white-space:nowrap}button{min-height:42px}}
.auth{position:fixed;z-index:20;top:50%;left:50%;transform:translate(-50%,-50%);width:min(94vw,460px);margin:0;box-shadow:0 0 0 200vmax #020a12cc,0 28px 80px #000a}
</style></head><body><main>
<section id="loginBox" class="panel auth hidden" role="dialog" aria-label="Administrator unlock">
<div class="eyebrow">SMART HOME · WATER MANAGEMENT</div><h1>SmartTank</h1>
<p class="muted">Sign in to configure your controller.</p>
<form id="loginForm"><label for="loginPass">Administrator password</label>
<input id="loginPass" type="password" required autocomplete="current-password" minlength="8">
<button type="submit">Unlock control center</button></form>
<div id="loginMsg" class="message"></div><button id="cancelLogin" class="alt" type="button">Back to monitoring</button>
<small>The administrator credential is verified on the ESP32-S3, not in this webpage.</small>
</section>
<section id="changeBox" class="panel auth hidden">
<div class="eyebrow">SECURE INITIAL SETUP</div><h1>Set your password</h1>
<p class="muted">The factory password must be changed on your first login. Choose a new password with at least 10 characters.</p>
<form id="changeForm">
<label>Current password</label><input id="oldPass" type="password" required autocomplete="current-password">
<label>New administrator password</label><input id="newPass" type="password" minlength="10" maxlength="64" autocomplete="new-password" required>
<label>Confirm new password</label><input id="confirmPass" type="password" minlength="10" maxlength="64" autocomplete="new-password" required>
<button type="submit">Save new password</button></form><div id="changeMsg" class="message"></div>
</section>
<div id="app">
<header class="top panel"><div><div class="eyebrow">SMART HOME · WATER MANAGEMENT</div><h1>SmartTank</h1><div class="muted">Local water monitoring & control</div></div>
<div style="text-align:right"><div class="chip" id="net">Connecting…</div><div class="muted" id="clock" style="margin-top:8px">--:--:--</div></div></header>
<nav class="nav" aria-label="Sections">
<button type="button" data-page="overview" class="active">Overview</button>
<button type="button" data-page="display">Display</button>
<button type="button" data-page="settings">Settings</button>
<button type="button" data-page="wifi">Wi-Fi</button>
<button type="button" data-page="network">Network</button>
<button type="button" data-page="location">Time & Weather</button>
<button type="button" data-page="security">Security</button>
<button type="button" data-page="configuration">Configuration <span aria-hidden="true">🔒</span></button>
</nav>
<section id="overview" class="page active">
<div id="retryAlert" class="panel alert hidden" style="margin-bottom:14px">
<strong>Water supply not detected</strong><p>The pump was stopped because the tank level did not rise. SmartTank will retry according to your schedule.</p>
<div id="retryLabel" class="muted">Next retry</div><div id="retryCountdown" class="count">--:--:--</div>
<small>To change the retry time, use System Retry on the TFT display.</small>
</div>
<div class="grid">
<div class="panel"><small>WATER LEVEL</small><div class="value accent" id="level">--%</div><small id="volume">Waiting for sensor</small></div>
<div class="panel"><small>PUMP OUTPUT</small><div class="value" id="pump">--</div><small id="pumpmode">--</small></div>
<div class="panel"><small>LoRa SENSOR</small><div class="value" id="link">--</div><small id="distance">--</small></div>
<div class="panel"><small>AUTOMATIC MODE</small><div class="value" id="auto">--</div><small id="thresholds">--</small></div>
<div class="panel span2"><h2>Pump control</h2><p class="muted">Manual overrides use the controller's existing time limit and safety interlocks.</p><div class="actions"><button id="pumpStart">Start manual / test water</button><button id="pumpStop" class="danger">Stop manual override</button></div><div id="pumpmsg" class="message"></div></div>
<div class="panel span2"><h2>Automatic operation</h2><p class="muted">The existing pump controller remains authoritative.</p><div class="actions"><button id="autoOn">Enable automatic</button><button id="autoOff" class="alt">Disable automatic</button></div></div>
<div class="panel span2"><h2>Tank configuration</h2><p class="muted">Tank count, total litres, height and pump thresholds are managed in the password-protected Configuration tab.</p><div class="actions"><button class="alt" type="button" id="openTankConfig">Open configuration</button></div></div>
<div class="panel span2"><h2>Time & weather</h2><div class="weather"><div class="value" id="temperature">--°C</div><div><div id="weatherText">Weather unavailable</div><small id="weatherPlace">Choose a city in Time & Weather</small></div></div><div class="divider"></div><small>Forecast requires internet. Device clock uses NTP when connected.</small></div>
</div></section>
<section id="settings" class="page">
<div class="panel" style="margin-bottom:14px">
<div class="eyebrow">SMARTTANK · OPERATING SETTINGS</div>
<h2>Automatic filling & manual override</h2>
<p class="muted">Change everyday pump thresholds and manual-run duration here. Changes are saved to the ESP32 and also appear in Management on its TFT screen.</p>
<form id="operatingForm">
<div class="row">
<div><label for="on">Start automatic filling below (%)</label><input id="on" type="number" step=".5" min="0" max="99" required></div>
<div><label for="off">Stop automatic filling at (%)</label><input id="off" type="number" step=".5" min="1" max="100" required></div>
<div><label for="mins">Manual override maximum (minutes)</label><input id="mins" type="number" min="1" max="120" step="1" required></div>
</div>
<div class="help">Safety requirement: OFF must exceed ON by at least 1%. Manual operation still follows the pump controller's safety limits.</div>
<button type="submit">Save operating settings</button>
<div id="operatingMsg" class="message" role="status"></div>
</form>
</div>
</section>
<section id="display" class="page"><div class="panel"><h2>TFT display management</h2><div class="message" id="displaySync">Reading settings from SmartTank...</div><p class="muted">Changes save directly to ESP32; rotation, brightness, timeout, theme, and sleep window are independent of portal appearance.</p><form id="displayForm">
<div class="row">
<div><label>Screen orientation</label><select id="rotation"><option value="0">Portrait · 0°</option><option value="1">Landscape · 90°</option><option value="2">Portrait · 180°</option><option value="3">Landscape · 270°</option></select></div>
<div><label>Theme · Dark/Light</label><select id="theme"><option value="0">Midnight (Dark)</option><option value="1">Slate</option><option value="2">Amber</option><option value="3">Terminal</option><option value="4">Daylight (Light)</option></select></div>
<div><label>Idle timeout (seconds, 0 = off)</label><input id="timeout" type="number" min="0" max="600" required></div>
<div><label>Dim step (%)</label><select id="dim"><option value="10">10%</option><option value="15">15%</option><option value="20">20%</option></select></div>
<div><label>Brightness (%)</label><input id="brightness" type="number" min="40" max="100" step="10" required></div>
<div><label>Buzzer volume</label><select id="buzzer"><option value="0">Off</option><option value="1">Low</option><option value="2">Medium</option><option value="3">High</option></select></div>
<div><label>Sleep window start (HH:MM)</label><input id="sleepStart" type="time" required></div>
<div><label>Sleep window end (HH:MM)</label><input id="sleepEnd" type="time" required></div>
</div><label><input id="sleepEnabled" type="checkbox" style="width:auto;display:inline;margin:10px"> Enable sleep window</label><br>
<div class="actions"><button type="button" id="setDark" class="alt">Dark display</button><button type="button" id="setLight" class="alt">Light display</button></div><button type="submit">Save display settings</button><div id="displayMsg" class="message"></div></form></div></section>
<section id="wifi" class="page"><div class="panel"><h2>Wi-Fi connection</h2><p class="muted" id="wifiinfo">Network details loading...</p><div class="actions"><button id="scanBtn" type="button" class="alt">Scan nearby networks</button></div><div id="scanMsg" class="message">Scan to see networks detected by the ESP32-S3 (2.4 GHz).</div><div id="wifiNetworks"></div><div class="divider"></div>
<form id="wifiForm"><label>Wi-Fi name (SSID)</label><input id="wifiSSID" maxlength="32" placeholder="Select a network or enter manually" required><label>Wi-Fi password</label><input id="wifiPassword" type="password" maxlength="63" autocomplete="new-password" placeholder="Enter network password">
<button type="submit">Save Wi-Fi and connect</button><div id="wifiMsg" class="message"></div></form><div class="help">Setup access point remains a recovery path while switching networks. This dashboard may reconnect at a new IP.</div></div></section>
<section id="network" class="page"><div class="panel"><h2>IP and hostname</h2><p class="muted">DHCP recommended. Changes apply after restarting SmartTank.</p><form id="networkForm"><label>IP assignment</label><select id="netdhcp"><option value="1">Automatic · DHCP</option><option value="0">Static IP</option></select><div id="staticFields" class="row"><div><label>Static IP</label><input id="netip" required></div><div><label>Gateway</label><input id="netgateway" required></div><div><label>Subnet mask</label><input id="netsubnet" required></div><div><label>DNS</label><input id="netdns" required></div></div><label>Hostname (without .local)</label><input id="nethost" maxlength="24" required><div class="row"><div><label>Setup AP IP</label><input id="netapip" required></div><div><label>Setup AP subnet</label><input id="netapsubnet" required></div></div><button type="submit">Save network configuration</button><div id="networkmsg" class="message"></div></form></div></section>
<section id="location" class="page"><div class="panel"><h2>Local time and weather</h2><p class="muted">Choose your city and country. The location is saved on the ESP32; an online weather service supplies conditions, while NTP synchronizes the clock.</p>
<form id="locationForm"><div class="row"><div><label>City</label><input id="city" maxlength="64" required placeholder="Lucknow"></div><div><label>Country</label><input id="country" maxlength="64" required placeholder="India"></div></div><label>NTP server</label><input id="ntp" maxlength="100" value="pool.ntp.org" required>
<button type="submit">Find city and save time settings</button><div id="locationMsg" class="message"></div></form><div class="divider"></div><div class="weather"><div class="value" id="weatherTemperature">--°C</div><div><div id="weatherDescription">Weather unavailable</div><div class="muted" id="locationClock">--:--:--</div><div class="muted" id="weatherDevice">Display weather: checking...</div></div></div><div class="help">Weather needs internet access on the device viewing this webpage. It cannot be fetched via an offline-only setup hotspot. Time-zone offset is saved for the selected location; daylight-saving changes may need re-selection.</div></div></section>
<section id="security" class="page"><div class="panel"><h2>Administrator security</h2><p class="muted">The password verifier and session state live on the ESP32-S3. No admin password is embedded in HTML or JavaScript.</p>
<form id="securityForm"><label>Current admin password</label><input id="securityOld" type="password" required autocomplete="current-password"><label>New password</label><input id="securityNew" type="password" minlength="10" maxlength="64" required autocomplete="new-password"><button type="submit">Change administrator password</button><div id="securityMsg" class="message"></div></form><div class="divider"></div><button id="logout" class="alt" type="button">Log out</button></div></section>
<section id="configuration" class="page">
<div class="panel" style="margin-bottom:14px">
<div class="eyebrow">SMARTTANK · ADMINISTRATION</div><h2>Tank configuration</h2>
<p class="muted">Set installed tank count, total storage and sensor calibration. For pump thresholds and manual override, use the Settings tab.</p>
<div class="help" id="adminState">Administrator is locked</div>
<div class="actions"><button type="button" id="unlockAdmin">Unlock configuration</button><button type="button" class="alt" id="lockAdmin">Lock administrator</button></div>
</div>
<div class="panel" style="margin-bottom:14px">
<h2>Tank installation</h2>
<p class="muted">The ESP32 currently measures ONE level sensor. Multiple tanks can be configured only as connected tanks sharing the same level. Separate tank-level readings require additional sensor nodes.</p>
<form id="tankForm">
<div class="row">
<div><label for="tankCount">Number of connected tanks</label><select id="tankCount" required>
<option value="1">1 tank</option><option value="2">2 tanks</option>
<option value="3">3 tanks</option><option value="4">4 tanks</option>
<option value="5">5 tanks</option><option value="6">6 tanks</option>
<option value="7">7 tanks</option><option value="8">8 tanks</option>
</select></div>
<div><label for="capacity">Total capacity of all tanks (litres)</label><input id="capacity" type="number" min="1" max="100000" step="1" required></div>
<div><label for="height">Tank / sensor calibration height (cm)</label><input id="height" type="number" min="20" max="500" step=".5" required></div>



</div>
<div class="help" id="tankPreview">Configured system capacity will appear here.</div>
<button type="submit">Save tank configuration</button><div id="settingsmsg" class="message" role="status"></div>
</form></div>
<div class="panel">
<h2>Configuration details</h2>
<p class="muted">The total litres estimate assumes linked tanks rise and fall together. It is not an independent measurement of every tank. Auto filling and dry-run safety still use the one validated level signal.</p>
</div></section>
<footer>SmartTank · Local ESP32-S3 portal · Pump safety remains in firmware · Weather by <a href="https://open-meteo.com/" target="_blank" rel="noopener noreferrer" style="color:#73ddca">Open-Meteo</a></footer>
</div></main>
<script>
"use strict";
const $=id=>document.getElementById(id);
let adminActive=false, pendingPage="overview";
let loc={city:"",country:"",lat:0,lon:0,tz:"",offset:330}, weatherUpdated=0;
async function request(url,method,body){
  const opts={method:method||"GET",cache:"no-store",credentials:"same-origin",headers:{}};
  if(body){opts.headers["Content-Type"]="application/x-www-form-urlencoded";opts.body=new URLSearchParams(body);}
  if(url==="/api/operating-settings" && method==="POST")opts.headers["X-SmartTank-Settings"]="1";
  const res=await fetch(url,opts),txt=await res.text();
  if(!res.ok)throw Error(txt||("HTTP "+res.status));
  try{return JSON.parse(txt);}catch{return txt;}
}
let operatingDirty=false;
$("operatingForm").addEventListener("input",()=>operatingDirty=true);
$("operatingForm").addEventListener("change",()=>operatingDirty=true);
let tankDirty=false;
function updateTankPreview(){
 const count=Number($("tankCount").value)||1;
 const capacity=Number($("capacity").value)||0;
 const each=capacity/count;
 $("tankPreview").textContent="Configured: "+count+" connected tank"+(count===1?"":"s")+
   " · "+capacity.toLocaleString()+" L total · about "+
   (Number.isFinite(each)?each.toLocaleString(undefined,{maximumFractionDigits:1}):"--")+
   " L per tank on average. Assumes a common water level.";
}
$("tankForm").addEventListener("input",()=>{tankDirty=true;updateTankPreview();});
$("tankForm").addEventListener("change",()=>{tankDirty=true;updateTankPreview();});
function syncAdminState(){
 $("adminState").textContent=adminActive?
  "Administrator unlocked · configuration changes enabled":
  "Administrator locked · open Configuration to sign in";
 $("unlockAdmin").textContent=adminActive?"Already unlocked":"Unlock configuration";
}
function showAuth(mode){
 $("loginBox").classList.toggle("hidden",mode!=="login");
 $("changeBox").classList.toggle("hidden",mode!=="change");
 $("app").classList.remove("hidden");
}
function switchPage(id){
 document.querySelectorAll(".page").forEach(el=>el.classList.toggle("active",el.id===id));
 document.querySelectorAll("[data-page]").forEach(el=>el.classList.toggle("active",el.dataset.page===id));
 if(id==="settings"){operatingDirty=false;refresh();}
 if(id==="display")loadDisplay(true);
 if(id==="network")loadNetwork().catch(()=>{});
 if(id==="location")loadLocation().catch(()=>{});
 if(id==="configuration") {
  syncAdminState();
  // A stored UI flag must not bypass the ESP32's 30-minute session timeout.
  request("/api/admin/status").then(d=>{
    adminActive=Boolean(d.authenticated&&!d.mustChange);
    syncAdminState();
    if(!adminActive && $("configuration").classList.contains("active")){
      pendingPage="configuration";
      showAuth(d.authenticated&&d.mustChange?"change":"login");
    }
  }).catch(()=>{
    adminActive=false;syncAdminState();
    if($("configuration").classList.contains("active"))showAuth("login");
  });
 }
}
document.querySelectorAll("[data-page]").forEach(el=>el.onclick=()=>switchPage(el.dataset.page));
$("openTankConfig").onclick=()=>switchPage("configuration");
$("unlockAdmin").onclick=()=>{
 if(adminActive)return;
 pendingPage="configuration";showAuth("login");
};
$("lockAdmin").onclick=async()=>{
 await request("/api/admin/logout","POST").catch(()=>{});
 adminActive=false;syncAdminState();
};
function adminRequired(){
 if(adminActive)return false;
 // Never request credentials outside the Configuration tab.
 const hint="Unlock Configuration before changing settings.";
 for(const id of ["pumpmsg","settingsmsg","displayMsg","wifiMsg","networkmsg","locationMsg","securityMsg","scanMsg"]){
  if($(id))$(id).textContent=hint;
 }
 return true;
}
$("cancelLogin").onclick=()=>{showAuth("app");pendingPage="overview";};
async function authStatus(){
 showAuth("app");
 try {
  const d=await request("/api/admin/status");
  adminActive=Boolean(d.authenticated&&!d.mustChange);
 }catch{adminActive=false;}
 syncAdminState();
 await Promise.allSettled([loadDisplay(true),loadLocation()]);
 refresh();weather();
}
$("loginForm").onsubmit=async e=>{
  e.preventDefault();$("loginMsg").textContent="Checking credentials...";
  try{const d=await request("/api/admin/login","POST",{password:$("loginPass").value});
    $("loginPass").value="";adminActive=!d.mustChange;showAuth(d.mustChange?"change":"app");syncAdminState();
    $("loginMsg").textContent="";
    if(!d.mustChange){switchPage(pendingPage);pendingPage="overview";refresh();weather();}
  }catch(err){$("loginMsg").textContent=err.message;}
};
async function changePassword(oldp,newp,target){
  await request("/api/admin/change","POST",{old:oldp,next:newp});
  target.textContent="Password updated.";
}
$("changeForm").onsubmit=async e=>{
  e.preventDefault();
  if($("newPass").value!==$("confirmPass").value){$("changeMsg").textContent="Passwords do not match";return;}
  try{await changePassword($("oldPass").value,$("newPass").value,$("changeMsg"));
    $("changeForm").reset();adminActive=true;showAuth("app");syncAdminState();switchPage(pendingPage);pendingPage="overview";refresh();weather();
  }catch(err){$("changeMsg").textContent=err.message;}
};
$("securityForm").onsubmit=async e=>{
  e.preventDefault();if(adminRequired())return;try{await changePassword($("securityOld").value,$("securityNew").value,$("securityMsg"));$("securityForm").reset();}
  catch(err){$("securityMsg").textContent=err.message;}
};
$("logout").onclick=async()=>{await request("/api/admin/logout","POST").catch(()=>{});adminActive=false;displayDirty=false;showAuth("app");syncAdminState();switchPage("overview");};
function fmt(sec){
  sec=Math.max(0,Math.floor(Number(sec)||0));
  return [Math.floor(sec/3600),Math.floor((sec%3600)/60),sec%60].map(n=>String(n).padStart(2,"0")).join(":");
}
async function refresh(){
  if($("app").classList.contains("hidden"))return;
  try{
    const d=await request("/api/status");$("level").textContent=d.levelValid?d.level.toFixed(1)+"%":"--%";
    $("volume").textContent=d.levelValid?d.volume.toFixed(0)+" / "+d.capacity.toFixed(0)+" L":"Level unavailable";
    $("pump").textContent=d.pumpOn?"ON":"OFF";$("pumpmode").textContent=d.override?"Manual override":d.autoState;
    $("link").textContent=d.sensorConnected?"ONLINE":"OFFLINE";
    $("distance").textContent=d.sensorValid?"Distance: "+d.distance.toFixed(1)+" cm":"Sensor data unavailable";
    $("auto").textContent=d.autoEnabled?"ENABLED":"DISABLED";
    $("thresholds").textContent="ON "+d.autoOn+"% · OFF "+d.autoOff+"%";
    $("net").textContent=d.wifiConnected?"Home Wi-Fi connected":d.apActive?"Setup hotspot active":"Wi-Fi offline";
    $("wifiinfo").textContent="IP: "+d.ip+"  ·  Host: "+d.hostname;
    $("clock").textContent=d.clock;
    $("locationClock").textContent=d.clock;
    $("retryAlert").classList.toggle("hidden",!d.retryWarning);
    if(d.retryWarning){$("retryCountdown").textContent=fmt(d.retrySeconds);
      $("retryLabel").textContent=d.retryManual?"User-scheduled retry at "+String(d.retryHour).padStart(2,"0")+":"+String(d.retryMinute).padStart(2,"0"):"Next automatic retry";}
    if(!tankDirty) {
      for(const [id,val] of Object.entries({
        height:d.height,capacity:d.capacity,tankCount:d.tankCount
      })) if(document.activeElement!==$(id))$(id).value=val;
      updateTankPreview();
    }
    if(!operatingDirty) {
      for(const [id,val] of Object.entries({
        on:d.autoOn,off:d.autoOff,mins:d.overrideMinutes
      })) if(document.activeElement!==$(id))$(id).value=val;
    }
  }catch(err){$("net").textContent="Connection lost";}
}
async function act(path,body,msg){
 if(adminRequired())return;
 try{msg.textContent=await request(path,"POST",body||{});refresh();}
 catch(err){msg.textContent="Error: "+err.message;}
}
$("pumpStart").onclick=()=>act("/api/pump/start",{},$("pumpmsg"));
$("pumpStop").onclick=()=>act("/api/pump/stop",{},$("pumpmsg"));
$("autoOn").onclick=()=>act("/api/auto",{enabled:"1"},$("pumpmsg"));
$("autoOff").onclick=()=>act("/api/auto",{enabled:"0"},$("pumpmsg"));
$("tankForm").onsubmit=async e=>{
 e.preventDefault();
 if(adminRequired())return;
 const o={
   height:$("height").value,capacity:$("capacity").value,
   tankCount:$("tankCount").value
 };
 const count=Number(o.tankCount),capacity=Number(o.capacity);
 if(!Number.isInteger(count)||count<1||count>8||!Number.isFinite(capacity)||capacity<=0){
   $("settingsmsg").textContent="Enter valid tank count and total capacity.";return;
 }
 try{
   $("settingsmsg").textContent=await request("/api/settings","POST",o);
   tankDirty=false;await refresh();
 }catch(err){$("settingsmsg").textContent="Save failed: "+err.message;}
};
$("operatingForm").onsubmit=async e=>{
 e.preventDefault();
 const o={on:$("on").value,off:$("off").value,minutes:$("mins").value};
 const on=Number(o.on),off=Number(o.off),minutes=Number(o.minutes);
 if(!Number.isFinite(on)||on<0||on>99||!Number.isFinite(off)||off>100||
    off-on<1||!Number.isInteger(minutes)||minutes<1||minutes>120){
   $("operatingMsg").textContent="Check the ON/OFF thresholds and duration.";return;
 }
 try{
   $("operatingMsg").textContent=await request("/api/operating-settings","POST",o);
   operatingDirty=false;await refresh();
 }catch(err){$("operatingMsg").textContent="Save failed: "+err.message;}
};
// The ESP32 is authoritative. Editing a form temporarily pauses sync so
// background reads never replace values the user is currently entering.
let displayDirty=false;
let displayLoading=false;
$("displayForm").addEventListener("input",()=>{displayDirty=true;});
$("displayForm").addEventListener("change",()=>{displayDirty=true;});

async function loadDisplay(force=false){
 if(displayLoading||(!force&&displayDirty))return;
 if(force)displayDirty=false;
 displayLoading=true;
 try{
  const d=await request("/api/display");
  if(!force&&displayDirty)return;
  for(const id of ["rotation","theme","timeout","dim","buzzer"]){
   $(id).value=String(d[id]);
  }
  // Live brightness is intentionally reflected, including manual knob changes.
  $("brightness").value=d.brightness<40?40:d.brightness;
  $("sleepEnabled").checked=Boolean(d.sleepEnabled);
  $("sleepStart").value=d.sleepStart;
  $("sleepEnd").value=d.sleepEnd;
  $("displaySync").textContent="Live settings from SmartTank · changes made with the rotary are reflected here";
 }catch(err){
  $("displaySync").textContent="Unable to sync: "+err.message;
 }finally{displayLoading=false;}
}
$("setDark").onclick=()=>{if(adminRequired())return;$("theme").value="0";displayDirty=true;$("displayForm").requestSubmit();};
$("setLight").onclick=()=>{if(adminRequired())return;$("theme").value="4";displayDirty=true;$("displayForm").requestSubmit();};
$("displayForm").onsubmit=async e=>{
 e.preventDefault();if(adminRequired())return;
 const o={};
 for(const id of ["rotation","theme","timeout","dim","brightness","buzzer","sleepStart","sleepEnd"])
  o[id]=$(id).value;
 o.sleepEnabled=$("sleepEnabled").checked?"1":"0";
 try{
  $("displayMsg").textContent=await request("/api/display","POST",o);
  displayDirty=false;
  await loadDisplay(true);
 }catch(err){$("displayMsg").textContent="Error: "+err.message;}
};
function toggleStatic(){const dhcp=$("netdhcp").value==="1";
 document.querySelectorAll("#staticFields input").forEach(el=>{el.disabled=dhcp;el.required=!dhcp;});
}
$("netdhcp").onchange=toggleStatic;
async function loadNetwork(){const d=await request("/api/network-config");
 $("netdhcp").value=d.dhcp?"1":"0";
 for(const id of ["ip","gateway","subnet","dns","hostname","apip","apsubnet"]){const ui={ip:"netip",gateway:"netgateway",subnet:"netsubnet",dns:"netdns",hostname:"nethost",apip:"netapip",apsubnet:"netapsubnet"};$(ui[id]).value=d[id];}
 toggleStatic();
}
$("networkForm").onsubmit=e=>{e.preventDefault();if(adminRequired())return;const o={dhcp:$("netdhcp").value,ip:$("netip").value,gateway:$("netgateway").value,subnet:$("netsubnet").value,dns:$("netdns").value,hostname:$("nethost").value,apip:$("netapip").value,apsubnet:$("netapsubnet").value};act("/api/network-config",o,$("networkmsg"));};
$("wifiForm").onsubmit=e=>{e.preventDefault();if(adminRequired())return;act("/save",{ssid:$("wifiSSID").value,password:$("wifiPassword").value},$("wifiMsg"));};
function wifiLabel(rssi){return rssi>=-60?"Excellent":rssi>=-70?"Good":rssi>=-80?"Fair":"Weak";}
$("scanBtn").onclick=async()=>{
 if(adminRequired())return;
 const btn=$("scanBtn");btn.disabled=true;$("wifiNetworks").replaceChildren();$("scanMsg").textContent="Scanning...";
 try{await request("/api/wifi/scan","POST",{});let done=false;
  for(let i=0;i<30;i++){await new Promise(resolve=>setTimeout(resolve,650));const d=await request("/api/wifi/scan");
   if(d.state==="scanning")continue;
   if(d.state!=="done")throw Error("Wi-Fi scan failed");
   done=true;const seen=new Set();let count=0;
   for(const net of (d.networks||[]).sort((a,b)=>b.rssi-a.rssi)){
    if(!net.ssid||seen.has(net.ssid))continue;seen.add(net.ssid);count++;
    const b=document.createElement("button");b.type="button";b.className="network";
    const name=document.createElement("span");name.textContent=net.ssid;
    const info=document.createElement("small");info.textContent=wifiLabel(net.rssi)+" · "+net.rssi+" dBm · "+(net.secured?"Secured":"Open");
    b.append(name,info);b.onclick=()=>{$("wifiSSID").value=net.ssid;$("wifiPassword").value="";$("wifiPassword").focus();$("scanMsg").textContent="Selected "+net.ssid+". Enter its password if required."};
    $("wifiNetworks").appendChild(b);
   }
   $("scanMsg").textContent=count+" Wi-Fi networks found";break;
  }
  if(!done)throw Error("Scan timed out");
 }catch(err){$("scanMsg").textContent=err.message;}finally{btn.disabled=false;}
};
async function loadLocation(){
 const d=await request("/api/location");loc=d;
 try{
  const w=await request("/api/weather");
  $("weatherDevice").textContent="Display weather: "+w.status+
    (w.valid?" · "+w.temperatureC+" °C":"");
 }catch{
  $("weatherDevice").textContent="Display weather status unavailable";
 }
 $("city").value=d.city||"";$("country").value=d.country||"";$("ntp").value=d.ntp||"pool.ntp.org";
 $("weatherPlace").textContent=d.city?(d.city+", "+d.country):"Choose a city";
}
function weatherDescription(code){
 if(code===0)return"Clear sky";if(code<=3)return"Partly cloudy";
 if(code<=48)return"Fog";if(code<=67)return"Rain / drizzle";
 if(code<=77)return"Snow";if(code<=82)return"Rain showers";
 if(code<=86)return"Snow showers";if(code>=95)return"Thunderstorm";return"Cloudy";
}
async function weather(){
 if(!Number.isFinite(Number(loc.lat))||!Number.isFinite(Number(loc.lon))||!loc.city||$("app").classList.contains("hidden"))return;
 if(Date.now()-weatherUpdated<600000)return;
 try{
 const url="https://api.open-meteo.com/v1/forecast?latitude="+encodeURIComponent(loc.lat)+"&longitude="+encodeURIComponent(loc.lon)+"&current=temperature_2m,weather_code&timezone=auto";
 const d=await(await fetch(url)).json();if(!d.current)throw Error("Weather unavailable");
 const t=d.current.temperature_2m.toFixed(1)+"°C",desc=weatherDescription(d.current.weather_code);
 $("temperature").textContent=t;$("weatherTemperature").textContent=t;
 $("weatherText").textContent=desc;$("weatherDescription").textContent=desc;
 $("weatherPlace").textContent=loc.city+", "+loc.country;weatherUpdated=Date.now();
 // Browser online forecast can populate the ESP32 TFT even when the S3's
 // independent HTTPS request fails (e.g. phone has mobile data).
 try{
  await request("/api/weather/sync","POST",{
    lat:loc.lat,lon:loc.lon,
    temperature:d.current.temperature_2m,code:d.current.weather_code
  });
  $("weatherDevice").textContent="Display weather: synced to ESP32-S3";
 }catch(syncError){
  $("weatherDevice").textContent="Display sync unavailable: "+syncError.message;
 }
 }catch(err){weatherUpdated=Date.now();$("weatherText").textContent="Weather unavailable (internet required)";$("weatherDescription").textContent="Weather unavailable";}
}
$("locationForm").onsubmit=async e=>{
 e.preventDefault();if(adminRequired())return;$("locationMsg").textContent="Finding city and its time zone...";
 try{
 const query=$("city").value.trim()+", "+$("country").value.trim();
 const u="https://geocoding-api.open-meteo.com/v1/search?name="+encodeURIComponent($("city").value.trim())+"&count=10&language=en";
 const g=await(await fetch(u)).json();
 const matches=(g.results||[]).filter(x=>x.country.toLowerCase()===$("country").value.trim().toLowerCase()||x.country_code.toLowerCase()===$("country").value.trim().toLowerCase());
 const place=matches.find(x=>x.name.toLowerCase()===$("city").value.trim().toLowerCase())||matches[0];
 if(!place)throw Error("City/country not found. Check spelling and try again.");
 const w=await(await fetch("https://api.open-meteo.com/v1/forecast?latitude="+place.latitude+"&longitude="+place.longitude+"&current=temperature_2m&timezone=auto")).json();
 if(typeof w.utc_offset_seconds!=="number")throw Error("Time zone lookup failed");
 const cfg={city:place.name,country:place.country,lat:place.latitude,lon:place.longitude,tz:place.timezone,offset:Math.round(w.utc_offset_seconds/60),ntp:$("ntp").value.trim()};
 await request("/api/location","POST",cfg);
 loc=cfg;weatherUpdated=0;$("locationMsg").textContent="Saved "+place.name+", "+place.country+". NTP/time zone updated.";
 weather();
 }catch(err){$("locationMsg").textContent=err.message+" (Internet access is required to search locations.)";}
};
setInterval(()=>{if(!$("app").classList.contains("hidden")){refresh();weather();}},2000);
setInterval(()=>{
 if(!$("app").classList.contains("hidden")&&!displayDirty){
  loadDisplay();
 }
},3000);
document.addEventListener("visibilitychange",()=>{
 if(!document.hidden&&!$("app").classList.contains("hidden")){
  refresh();
  loadDisplay();
 }
});
authStatus();
</script></body></html>
)stportal";
