'use strict';
const {chromium}=require(process.env.HH_PLAYWRIGHT_PATH||'playwright');
const path=require('node:path'),fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({channel:'msedge',headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1440,height:1100},deviceScaleFactor:1});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  const base=process.env.HH_UI_URL||'http://127.0.0.1:8766';
  await page.goto(base);await page.waitForFunction(()=>document.querySelector('#network-title').textContent.includes('Reception paused'));
  assert.equal(await page.locator('html').getAttribute('lang'),'en');
  assert.equal(await page.locator('#connection-badge').innerText(),'Device disconnected');
  const shots=path.join(__dirname,'data','screenshots');fs.mkdirSync(shots,{recursive:true});
  await page.screenshot({path:path.join(shots,'overview-en.png'),fullPage:true});
  await page.locator('[data-tab="settings"]').click();
  await page.locator('#apiKey').fill('unsaved-ui-test-key');
  await page.locator('#deviceHost').fill('192.168.1.123');
  await page.locator('#language-switch').selectOption('zh');
  assert.equal(await page.locator('html').getAttribute('lang'),'zh-CN');
  assert.equal(await page.locator('#page-name').innerText(),'连接与识别设置');
  assert.equal(await page.locator('#apiKey').inputValue(),'unsaved-ui-test-key');
  assert.equal(await page.locator('#deviceHost').inputValue(),'192.168.1.123');
  await page.locator('[data-tab="overview"]').click();
  await page.screenshot({path:path.join(shots,'overview-zh.png'),fullPage:true});
  await page.reload();await page.waitForFunction(()=>document.querySelector('#network-title').textContent.includes('接收已暂停'));
  assert.equal(await page.locator('html').getAttribute('lang'),'zh-CN');
  await page.locator('#language-switch').selectOption('en');
  for(const name of ['overview','recordings','settings']){
   await page.locator('[data-tab="'+name+'"]').click();
   const untranslated=await page.evaluate(()=>{
    const walker=document.createTreeWalker(document.body,NodeFilter.SHOW_TEXT);const found=[];
    for(let n;n=walker.nextNode();){const el=n.parentElement;if(el.closest('script,style,#language-switch,textarea')||!el.getClientRects().length)continue;if(/[\u4e00-\u9fff]/.test(n.nodeValue))found.push(n.nodeValue);}
    return found;
   });assert.deepEqual(untranslated,[],name+' has untranslated English text');
  }
  await page.locator('[data-tab="recordings"]').click();
  await page.locator('#search-recordings').fill('no-file-matches-this');
  assert.match(await page.locator('#record-list').innerText(),/No matching recordings/);
  await page.screenshot({path:path.join(shots,'recordings-en.png'),fullPage:true});
  await page.locator('[data-tab="settings"]').click();
  await page.screenshot({path:path.join(shots,'settings-en.png'),fullPage:true});
  const online=await (await page.request.get(base+'/api/state')).json();
  online.wanted=true;online.stale=false;online.statuses={telemetry:'connected',audio:'connected'};online.deviceAddress='192.168.1.50';
  online.latest={version:4,boot_id:123,sequence:1,received_at:Date.now(),valid_mask:125,inside_temp_c:30,inside_rh_pct:60,water_raw:49,light_raw:499,wick_raw:2400,wick2_raw:1800,wick2_valid:1,moisture1_state:1,moisture2_state:2,valve_state:3,valve_pulse_us:1200,valve_open_ms:60000,outlet_blocked:1,distance_mm:180,presence_valid:1,animal_present:1,audio_recording:1,sound_rms:10,sound_p2p:40,sound_clipped:0};
  online.history=[online.latest];online.events=[{at:Date.now(),type:'info',message:'ESP32正在录制30秒事件音频'}];
  await page.route('**/api/state',route=>route.fulfill({json:online}));
  await page.locator('[data-tab="overview"]').click();
  await page.waitForFunction(()=>document.getElementById('maintenance-dialog').open);
  assert.match(await page.locator('#maintenance-dialog-body').innerText(),/Possible outlet blockage/);
  await page.locator('#dismiss-maintenance').click();
  assert.equal(await page.locator('#status-left1 strong').innerText(),'Dry');
  assert.match(await page.locator('#event-list').innerText(),/ESP32 is recording/);
  await page.locator('#language-switch').selectOption('zh');
  assert.equal(await page.locator('#status-left1 strong').innerText(),'干燥');
  assert.match(await page.locator('#maintenance-alerts').innerText(),/出水口堵塞/);
  await page.locator('#language-switch').selectOption('en');
  await page.screenshot({path:path.join(shots,'live-alerts-en.png'),fullPage:true});
  // Exercise loaded audio/result content without uploading data or using a real device.
  online.records=[{name:'clip_test_gaps0.wav',bytes:2044,modified:Date.now(),result:{label:'Bird',model:'自定义分类服务',evidence:'Bird sounds recorded while valve was Open',uncertainty:'Human review required',at:Date.now(),predictions:[{label:'Bird',score:.7}]}}];
  await page.route('**/api/audio-info?*',route=>route.fulfill({json:{duration:1,rate:1000,channels:1,bytes:2044,peak:100,rms:10,waveform:[0,.5,1,.5,0]}}));
  const {makeWav}=require('./protocol');
  await page.route('**/api/audio?*',route=>route.fulfill({contentType:'audio/wav',body:makeWav(Buffer.alloc(2000),1000)}));
  await page.locator('[data-tab="recordings"]').click();
  await page.locator('#search-recordings').fill('');
  await page.waitForFunction(()=>document.getElementById('audio-meta').textContent.includes('1000 Hz'));
  const audioSource=await page.locator('#audio-player').getAttribute('src');
  assert.match(await page.locator('#audio-meta').innerText(),/Mono/);
  assert.match(await page.locator('#result').innerText(),/Acoustic evidence/);
  await page.locator('#language-switch').selectOption('zh');
  assert.match(await page.locator('#audio-meta').innerText(),/单声道/);
  assert.equal(await page.locator('#audio-player').getAttribute('src'),audioSource);
  assert.match(await page.locator('#result').innerText(),/Bird sounds recorded while valve was Open/);
  assert.match(await page.locator('#result').innerText(),/声音依据/);
  await page.locator('#language-switch').selectOption('en');
  await page.screenshot({path:path.join(shots,'recording-details-en.png'),fullPage:true});

  await page.setViewportSize({width:390,height:844});
  for(const name of ['overview','recordings','settings']){
   await page.locator('[data-tab="'+name+'"]').click();
   assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false,name+' overflows at mobile width');
  }
  await page.locator('[data-tab="overview"]').click();
  await page.screenshot({path:path.join(shots,'mobile-en.png'),fullPage:true});
  assert.deepEqual(errors,[]);
  console.log('PASS: English/Chinese switching and persistence, unsaved settings, all tabs, live statuses and alerts, translated logs, desktop/mobile layout; no browser errors.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
