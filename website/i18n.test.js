const {test}=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const {translate,known}=require('./public/i18n');
const {sensorStatusView}=require('./public/control-view');

test('English and Chinese statuses translate full readings without changing values',()=>{
  const sample={version:4,valid_mask:28,water_raw:49,light_raw:499,wick_raw:2400,wick2_raw:1800,wick2_valid:1,moisture1_state:1,moisture2_state:2,valve_state:3,valve_pulse_us:1200,valve_open_ms:60000,outlet_blocked:1};
  const en=sensorStatusView(sample,false,'en'),zh=sensorStatusView(sample,false,'zh');
  assert.equal(en.left1.value,'Dry');assert.equal(en.left2.value,'Wet');assert.equal(en.valve.value,'Open');
  assert.match(en.water.value,/Below 50 ml/);assert.match(en.left2.detail,/1800/);
  assert.equal(/[\u4e00-\u9fff]/.test(JSON.stringify(en)),false);
  assert.equal(zh.left1.value,'干燥');assert.equal(zh.alerts[1].title,'出水口堵塞');
  assert.equal(sensorStatusView(sample,true,'en').valve.value,'Unknown');
});

test('controlled messages and errors translate in both directions',()=>{
  assert.equal(translate('已保存录音 clip_123.wav','en'),'Saved recording clip_123.wav');
  assert.equal(translate('API response is too large','zh'),'API响应过大');
  assert.equal(translate('wick_raw范围错误','en'),'wick_raw out of range');
  assert.equal(translate('Clip_Unknown_123.wav','en'),'Clip_Unknown_123.wav');
  assert.equal(known('Bird sounds recorded while valve was Open','zh'),'Bird sounds recorded while valve was Open');
});

test('English HTML exposes the language switch and shared module',()=>{
  const html=fs.readFileSync(require('node:path').join(__dirname,'public/index.html'),'utf8');
  assert.match(html,/<html lang="en">/);assert.match(html,/id="language-switch"/);
  assert.match(html,/src="\/i18n.js"/);assert.match(html,/Shelter status/);
});
