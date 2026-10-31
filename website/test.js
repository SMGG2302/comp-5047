'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict'),fs=require('node:fs'),os=require('node:os'),path=require('node:path'),net=require('node:net'),http=require('node:http'),{spawn}=require('node:child_process');
const {GREETING,parseSample,crc32,audioHeader,makeWav,wavInfo,normalizeResult}=require('./protocol');
const {sensorStatusView}=require('./public/control-view');
test('status bar thresholds, both cloth sensors, valve state and stale alarms',()=>{
  const s={version:4,valid_mask:28,water_raw:49,light_raw:499,wick_raw:2400,wick2_raw:1800,wick2_valid:1,moisture1_state:1,moisture2_state:2,valve_state:3,valve_pulse_us:1200,valve_open_ms:60000,outlet_blocked:1};
  const v=sensorStatusView(s,false);
  assert.match(v.water.value,/小于50ml/);assert.equal(v.sun.value,'太阳直射');
  assert.equal(v.left1.value,'干燥');assert.equal(v.left2.value,'湿润');assert.equal(v.valve.value,'打开');
  assert.deepEqual(v.alerts.map(a=>a.title),['需要补水维护','出水口堵塞']);
  assert.match(v.left1.detail,/2400/);assert.match(v.left2.detail,/1800/);
  assert.equal(sensorStatusView({...s,water_raw:50},false).water.value,'暂不需要补水');
  assert.equal(sensorStatusView({...s,light_raw:500},false).sun.value,'未达到直射阈值');
  assert.equal(sensorStatusView({...s,valid_mask:0,wick2_valid:0,moisture1_state:0,moisture2_state:0,outlet_blocked:0},false).alerts.length,0);
  assert.equal(sensorStatusView(s,true).alerts.length,0);assert.equal(sensorStatusView(s,true).valve.value,'未知');
  assert.equal(sensorStatusView({...s,version:3},false).valve.value,'未知');
});
test('v4 telemetry validates extended fields and blockage consistency',()=>{
  const row4='4,123,1,80000,32,60,nan,2400,199,49,29,nan,-1,0,0,0,-1,0,0,0,1800,1,1,2,3,1200,60000,1';
  const s=parseSample(row4);assert.equal(s.wick2_raw,1800);assert.equal(s.valve_open_ms,60000);
  for(const bad of [row4.replace(',60000,1',',59999,1'),row4.replace(',3,1200,',',1,1200,'),row4.replace(',1800,1,1,2',',1800,0,1,2'),row4+',0'])assert.throws(()=>parseSample(bad));
});
test('enabled light and water retain raw ADC values including zero',()=>{
  const sample=parseSample('3,123,1,2000,31.50,61.25,nan,2600,0,4095,125,15.50,120,16384,0,0,180,1,1,1');
  assert.equal(sample.light_raw,0);assert.equal(sample.water_raw,4095);
  assert.throws(()=>parseSample('3,123,1,2000,31.50,61.25,nan,2600,0,4096,125,15.50,120,16384,0,0,180,1,1,1'));
});
const row='3,123,1,2000,24.50,61.25,nan,2600,-1,-1,101,15.50,120,16384,0,0,180,1,1,1';
test('telemetry preserves invalid/unknown semantics and validates values',()=>{
  assert.equal(parseSample(row).distance_mm,180);
  assert.equal(parseSample('3,123,2,4000,nan,nan,nan,-1,-1,-1,0,nan,-1,0,0,0,-1,1,0,0').animal_present,1);
  for(const invalid of [row.replace('24.50','Infinity'),row.replace(',101,',',37,'),row.replace(',1,1,1',',1,2,1'),row.replace('61.25','101'),row.replace('2600','4096'),row+',junk'])assert.throws(()=>parseSample(invalid));
});
test('WAV, CRC and API validation',()=>{
  assert.equal(crc32(Buffer.from('123456789')),0xcbf43926);
  const pcm=Buffer.alloc(60000);pcm.writeInt16LE(1500,0);
  const wav=makeWav(pcm,1000),info=wavInfo(wav);
  assert.equal(info.duration,30);assert.equal(info.peak,1500);assert.equal(info.channels,1);
  assert.throws(()=>wavInfo(wav.subarray(0,100)));
  assert.equal(normalizeResult([{label:'bird',score:.7},{label:'unknown',score:.9}]).label,'unknown');
  assert.throws(()=>normalizeResult([{label:'bird',score:9}]));
  assert.throws(()=>audioHeader(Buffer.alloc(32)));
});
const listen=s=>new Promise(resolve=>s.listen(0,'127.0.0.1',()=>resolve(s.address().port)));
const delay=ms=>new Promise(r=>setTimeout(r,ms));
test('end-to-end TCP, CRC retry, persistence-before-ACK, local API and classification',{timeout:25000},async()=>{
  const folder=fs.mkdtempSync(path.join(os.tmpdir(),'hh-desktop-test-')),data=path.join(folder,'data'),records=path.join(folder,'recordings');
  let child;const clients=new Set();let audioAttempts=0,acknowledged=false,apiCalls=0;
  const pcm=Buffer.alloc(60000);for(let i=0;i<30000;i++)pcm.writeInt16LE(i%2?1000:-1000,i*2);
  const file='clip_0000007b_000001_gaps0.wav';
  const header=Buffer.alloc(32);header.write('HHW1');[123,1,30000,1000,crc32(pcm),0,30000].forEach((n,i)=>header.writeUInt32LE(n,4+i*4));
  const telemetry=net.createServer(sock=>{clients.add(sock);sock.on('error',()=>{});sock.on('close',()=>clients.delete(sock));const text=GREETING+'\n'+row+'\n';sock.write(text.slice(0,17));setTimeout(()=>sock.write(text.slice(17)),10);});
  const audio=net.createServer(sock=>{
    clients.add(sock);sock.on('error',()=>{});sock.on('close',()=>clients.delete(sock));audioAttempts++;
    if(audioAttempts===1){sock.end(Buffer.concat([header,pcm.subarray(0,50)]));return;}
    if(audioAttempts===2){const bad=Buffer.from(header);bad.writeUInt32LE(0,20);sock.write(Buffer.concat([bad,pcm]));return;}
    let ack=Buffer.alloc(0);
    sock.on('data',chunk=>{ack=Buffer.concat([ack,chunk]);if(ack.length>=12){assert.equal(ack.toString('ascii',0,4),'ACK1');assert.deepEqual(fs.readFileSync(path.join(records,file)),makeWav(pcm,1000));acknowledged=true;}});
    sock.write(header.subarray(0,7));setTimeout(()=>sock.write(Buffer.concat([header.subarray(7),pcm])),10);
  });
  const api=http.createServer(async(req,res)=>{apiCalls++;const chunks=[];for await(const c of req)chunks.push(c);assert.deepEqual(Buffer.concat(chunks),makeWav(pcm,1000));assert.equal(req.headers.authorization,'Bearer test-only-secret');res.setHeader('Content-Type','application/json');res.end(JSON.stringify([{label:'未知',score:.8},{label:'鸟类',score:.2}]));});
  let webPort;const tempServer=net.createServer();webPort=await listen(tempServer);await new Promise(r=>tempServer.close(r));
  try{
    const telePort=await listen(telemetry),audioPort=await listen(audio),apiPort=await listen(api);
    child=spawn(process.execPath,[path.join(__dirname,'server.js')],{env:{...process.env,HH_PORT:String(webPort),HH_DATA_DIR:data,HH_RECORDINGS_DIR:records,HH_TELEMETRY_PORT:String(telePort),HH_AUDIO_PORT:String(audioPort)},stdio:'pipe'});
    let output='';child.stderr.on('data',b=>output+=b);
    const base='http://127.0.0.1:'+webPort;let state;
    for(let i=0;i<50;i++){try{state=await(await fetch(base+'/api/state')).json();break;}catch{await delay(100);}}
    assert.ok(state,output);
    const html=await(await fetch(base+'/')).text();
    assert.ok(html.includes('id="status-left2"')&&html.includes('id="status-water"')&&html.includes('id="status-valve"'));
    const viewScript=await fetch(base+'/control-view.js');assert.equal(viewScript.status,200);
    assert.match(await viewScript.text(),/function sensorStatusView/);
    const post=(url,value,token=state.csrf)=>fetch(base+url,{method:'POST',headers:{'Content-Type':'application/json','X-HH-Token':token},body:JSON.stringify(value||{})});
    const rejected=await post('/api/connect',{},'wrong');
    assert.equal(rejected.status,403);assert.match((await rejected.json()).error,/Invalid request token/);
    const chineseRejected=await fetch(base+'/api/connect',{method:'POST',headers:{'X-HH-Token':'wrong','X-HH-Language':'zh'}});
    assert.match((await chineseRejected.json()).error,/请求来源无效/);
    assert.equal((await fetch(base+'/i18n.js')).status,200);
    const cfg={...state.config,deviceHost:'127.0.0.1',provider:'custom',endpoint:'http://127.0.0.1:'+apiPort,apiKey:'test-only-secret'};
    assert.equal((await post('/api/settings',cfg)).status,200);
    await post('/api/connect');
    for(let i=0;i<100&&!acknowledged;i++)await delay(100);
    assert.ok(acknowledged,'file must be persisted before ACK, after two failed transfers');
    state=await(await fetch(base+'/api/state')).json();assert.equal(state.latest.distance_mm,180);assert.equal(state.records.length,1);
    assert.equal(JSON.stringify(state).includes('test-only-secret'),false);
    assert.equal(fs.readFileSync(path.join(data,'settings.json'),'utf8').includes('test-only-secret'),false);
    assert.equal(apiCalls,0,'record reception must never automatically upload');
    const result=await(await post('/api/classify',{name:file})).json();assert.equal(result.label,'未知');assert.equal(apiCalls,1);
    assert.equal(JSON.parse(fs.readFileSync(path.join(data,'results.json')))[file].label,'未知');
    const partial=await fetch(base+'/api/audio?name='+file,{headers:{Range:'bytes=0-43'}});assert.equal(partial.status,206);assert.equal((await partial.arrayBuffer()).byteLength,44);
    assert.equal((await fetch(base+'/api/audio?name=..%2Fserver.js')).status,400);
    const csv=await(await fetch(base+'/api/export')).text();assert.ok(csv.includes('distance_mm,animal_present'));
    const exported=parseSample(csv.trim().split('\n')[1].split(',').slice(1).join(','));
    assert.equal(exported.inside_temp_c,24.5);assert.equal(exported.sequence,1);assert.equal(exported.valve_state,0);
    await post('/api/disconnect');state=await(await fetch(base+'/api/state')).json();assert.equal(state.stale,true);
  }finally{
    if(child){child.kill();await new Promise(r=>{child.once('exit',r);setTimeout(r,1000).unref();});}
    for(const s of clients)s.destroy();for(const s of [telemetry,audio,api])s.close();
    assert.equal(path.dirname(path.resolve(folder)),path.resolve(os.tmpdir()));
    assert.ok(path.basename(folder).startsWith('hh-desktop-test-'));
    fs.rmSync(folder,{recursive:true,force:true});
  }
});
