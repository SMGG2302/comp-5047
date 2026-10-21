const {chromium}=require(process.env.HH_PLAYWRIGHT_PATH||'playwright');
const path=require('node:path'),fs=require('node:fs'),assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({channel:'msedge',headless:true});
 try{
  const page=await browser.newPage({viewport:{width:1440,height:1100},deviceScaleFactor:1});
  const errors=[];page.on('pageerror',e=>errors.push(e.message));
  await page.goto('http://127.0.0.1:8765');
  await page.waitForFunction(()=>document.getElementById('archive-total').textContent!=='0 个');
  const shots=path.join(__dirname,'data','screenshots');fs.mkdirSync(shots,{recursive:true});
  await page.screenshot({path:path.join(shots,'overview.png'),fullPage:true});
  await page.locator('[data-tab="recordings"]').click();
  await page.waitForFunction(()=>document.getElementById('audio-meta').textContent.includes('Hz'));
  assert.ok(await page.locator('#audio-player').getAttribute('src'));
  await page.screenshot({path:path.join(shots,'recordings.png'),fullPage:true});
  await page.locator('#search-recordings').fill('no-file-matches-this');
  assert.ok((await page.locator('#record-list').innerText()).includes('暂无匹配'));
  await page.locator('#search-recordings').fill('');
  await page.locator('#classify').click();
  await page.waitForFunction(()=>!document.getElementById('settings').hidden);
  assert.equal(await page.locator('#apiKey').inputValue(),'');
  await page.screenshot({path:path.join(shots,'settings.png'),fullPage:true});
  await page.setViewportSize({width:390,height:844});
  await page.locator('[data-tab="overview"]').click();
  await page.screenshot({path:path.join(shots,'mobile.png'),fullPage:true});
  assert.equal(await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth),false);
  assert.deepEqual(errors,[]);
  console.log('PASS: desktop/mobile layout, archive selection, WAV metadata, search, API configuration guard; no browser errors.');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exitCode=1;});
