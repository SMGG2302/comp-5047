'use strict';
const $=id=>document.getElementById(id);
let state=null,selected=null,audioInfo=null,formReady=false,recordSignature='',resultSignature='',activeTab='overview',busy=false;
const time=v=>new Date(v).toLocaleTimeString(i18n.locale(),{hour12:false}),date=v=>new Date(v).toLocaleString(i18n.locale(),{hour12:false});
const escapeHTML=v=>String(v).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let toastTimer;
function toast(text){text=t(text);$('toast').textContent=text;$('toast').hidden=false;clearTimeout(toastTimer);toastTimer=setTimeout(()=>$('toast').hidden=true,8000);}
async function request(url,data){
  const r=await fetch(url,{method:'POST',headers:{'Content-Type':'application/json','X-HH-Token':state?.csrf||'','X-HH-Language':i18n.language()},body:JSON.stringify(data||{})});
  const value=await r.json();if(!r.ok)throw Error(value.error||t('请求失败'));return value;
}
function tab(name){
  activeTab=name;document.querySelectorAll('.tab').forEach(el=>el.hidden=el.id!==name);
  document.querySelectorAll('.nav').forEach(el=>el.classList.toggle('active',el.dataset.tab===name));
  $('page-name').textContent={overview:t('实时监测'),recordings:t('声音档案'),settings:t('连接与识别设置')}[name];
  if(name==='overview')drawTrend();if(name==='recordings')drawWave();
}
document.querySelectorAll('[data-tab]').forEach(el=>el.onclick=()=>tab(el.dataset.tab));
document.querySelectorAll('[data-go]').forEach(el=>el.onclick=()=>tab(el.dataset.go));
document.querySelectorAll('.connect-button').forEach(el=>el.onclick=async()=>{
  el.disabled=true;try{await request(state?.wanted?'/api/disconnect':'/api/connect');await poll();}catch(e){toast(e.message);}finally{el.disabled=false;}
});
function fillForm(){
  if(formReady)return;
  for(const key of ['deviceHost','provider','model','endpoint','candidates'])$(key).value=state.config[key];
  formReady=true;providerFields();
}
function providerFields(){const custom=$('provider').value==='custom';$('model-field').hidden=custom;$('endpoint-field').hidden=!custom;$('custom-help').hidden=!custom;}
$('provider').onchange=providerFields;
$('settings-form').onsubmit=async e=>{
  e.preventDefault();const data={};for(const k of ['deviceHost','provider','model','endpoint','candidates','apiKey'])data[k]=$(k).value;
  data.clearKey=$('clearKey').checked;const btn=e.target.querySelector('button');btn.disabled=true;
  try{await request('/api/settings',data);$('apiKey').value='';$('clearKey').checked=false;$('settings-feedback').textContent=t('已保存');await poll();}
  catch(err){toast(err.message);$('settings-feedback').textContent=t('保存失败');}finally{btn.disabled=false;}
};
function displayMetric(id,key,bit,digits){
  const s=state.latest,valid=!state.stale&&s&&(s.valid_mask&bit)&&s[key]!==null;
  $(id).textContent=valid?Number(s[key]).toFixed(digits):'—';
}
function render(){
  fillForm();const stale=state.stale,s=state.latest;
  $('clock').textContent=new Date().toLocaleDateString(i18n.locale(),{month:'long',day:'numeric'})+' · '+time(Date.now());
  $('connection-badge').textContent=!state.wanted?t('未连接设备'):stale?t('等待设备数据'):t('设备在线');
  $('connection-badge').className='badge'+(!stale?' green':'');
  document.querySelector('.connect-button').textContent=state.wanted?t('暂停接收'):t('连接 ESP32');
  $('network-title').textContent=!state.wanted?t('本地工作空间 · 接收已暂停'):stale?t('正在等待 ESP32，请检查路由器连接'):t('正在接收树屋数据');
  const audioStatus={connected:t('音频通道已连接'),connecting:t('音频通道连接中'),retrying:t('音频通道重连中'),disconnected:t('音频通道未连接')}[state.statuses.audio];
  $('network-detail').textContent=!state.wanted?t('电脑与 ESP32 连接同一路由器，再点击连接。'):stale?t('正在')+(state.deviceAddress?t('重连 ')+state.deviceAddress:t('自动发现 ESP32'))+t('。请确认路由器2.4GHz网络可用、设备已上电，且未开启客户端隔离。'):audioStatus+t(' · 设备 ')+state.deviceAddress+t(' · 最后更新 ')+time(s.received_at);
  displayMetric('temperature','inside_temp_c',1,2);displayMetric('humidity','inside_rh_pct',1,1);
  displayMetric('distance','distance_mm',64,0);
  renderSensorStatus(s,stale);
  displayMetric('sound-rms','sound_rms',32,1);displayMetric('sound-p2p','sound_p2p',32,0);displayMetric('sound-clipped','sound_clipped',32,0);
  const valid=!stale&&s?.presence_valid,occupied=valid&&s.animal_present;
  $('presence-badge').textContent=!valid?t('状态未知'):occupied?t('疑似占用'):t('监测区空闲');
  $('presence-badge').className='badge'+(valid?' green':'');
  $('presence-title').textContent=!valid?t('等待有效测距'):occupied?t('有物体靠近'):t('暂未检测到占用');
  $('presence-detail').textContent=!valid?t('设备离线或红外读数无效，不能推断动物是否离开'):occupied?t('近物体距离 ')+s.distance_mm+t(' mm · 动物身份待识别'):t('距离 ')+s.distance_mm+t(' mm · 等待下次进入');
  document.querySelector('.occupancy').classList.toggle('occupied',!!occupied);
  $('recording-state').textContent=stale?t('○ 录音状态未知'):s.audio_recording?t('● 正在录制约30秒'):t('○ 录音待机');
  $('last-update').textContent=s?t('最近接收 ')+date(s.received_at)+(stale?t(' · 已过期'):''):t('暂无设备数据');
  $('session-info').textContent=t('本次 ')+state.history.length+t(' 条缓存记录');
  const rms=!stale&&s?.sound_rms?s.sound_rms:0;
  document.querySelectorAll('.sound-bars i').forEach((el,i)=>{el.style.height=(4+Math.min(29,rms/3)*(0.35+0.65*Math.abs(Math.sin(i*1.9+s?.sequence||0))))+'px';});
  $('event-list').innerHTML=state.events.length?state.events.slice(0,25).map(e=>'<div class="event '+escapeHTML(e.type)+'"><time>'+time(e.at)+'</time><span>'+escapeHTML(t(e.message))+'</span></div>').join(''):t('<p class="empty-small">连接设备后，这里会记录占用变化与文件接收事件。</p>');
  $('key-state').textContent=state.config.hasKey?t('密钥已设置 · 本次运行有效'):t('尚未配置密钥');
  $('record-count').textContent=state.records.length;$('archive-total').textContent=state.records.length+t(' 个');
  const sig=JSON.stringify(state.records.map(r=>[r.name,r.modified,r.result?.at]));
  if(sig!==recordSignature){recordSignature=sig;renderRecords();if(!selected&&state.records.length)selectRecord(state.records[0].name);}
  $('classify').disabled=!selected||busy||!!state.activeJob;
  $('classify').textContent=busy||state.activeJob?t('识别中，请稍候…'):t('上传并识别声音 ↗');
  renderResult();if(activeTab==='overview')drawTrend();
}
const acknowledgedAlerts=new Set();
let alarmBoot=null;
function renderSensorStatus(sample,stale){
  const view=sensorStatusView(sample,stale,i18n.language());
  for(const key of ['water','sun','valve','left1','left2']){
    const value=view[key],el=$('status-'+key);
    el.className='status-item '+value.tone;el.title=value.detail;
    el.setAttribute('aria-label',value.label+'：'+value.value+'。'+value.detail);
    el.querySelector('strong').textContent=value.value;
    el.querySelector('.status-tooltip').textContent=value.detail;
  }
  $('maintenance-alerts').innerHTML=view.alerts.map(a=>'<div class="maintenance-alert"><strong>'+escapeHTML(a.title)+'</strong><span>'+escapeHTML(a.detail)+'</span></div>').join('');
  $('maintenance-alerts').hidden=!view.alerts.length;
  if(stale||!sample){if($('maintenance-dialog').open)$('maintenance-dialog').close();return;}
  if(alarmBoot!==sample.boot_id){acknowledgedAlerts.clear();alarmBoot=sample.boot_id;}
  for(const id of [...acknowledgedAlerts])if(!view.alerts.some(a=>a.id===id))acknowledgedAlerts.delete(id);
  const fresh=view.alerts.filter(a=>!acknowledgedAlerts.has(a.id));
  if(fresh.length){
    $('maintenance-dialog-body').innerHTML=fresh.map(a=>'<h3>'+escapeHTML(a.title)+'</h3><p>'+escapeHTML(a.detail)+'</p>').join('');
    for(const alert of fresh)acknowledgedAlerts.add(alert.id);
    if(!$('maintenance-dialog').open)$('maintenance-dialog').showModal();
  }else if(!view.alerts.length&&$('maintenance-dialog').open)$('maintenance-dialog').close();
}
$('dismiss-maintenance').onclick=()=>$('maintenance-dialog').close();
function renderRecords(){
  const q=$('search-recordings').value.toLowerCase();
  const rows=state.records.filter(r=>r.name.toLowerCase().includes(q));
  $('record-list').innerHTML=rows.length?rows.map(r=>'<button class="record-list-item '+(r.name===selected?'selected':'')+'" data-name="'+escapeHTML(r.name)+'"><strong>'+escapeHTML(r.name)+'</strong><small>'+date(r.modified)+' · '+(r.bytes/1024).toFixed(0)+' KB</small><small class="result-tag">'+(r.result?t('AI候选：')+escapeHTML(r.result.label):t('本地文件 · 尚未识别'))+'</small></button>').join(''):t('<p class="empty-small">暂无匹配录音。连接设备等待录制完成，或导入本地 WAV。</p>');
  $('record-list').querySelectorAll('button').forEach(el=>el.onclick=()=>selectRecord(el.dataset.name));
}
$('search-recordings').oninput=renderRecords;
async function selectRecord(name){
  selected=name;audioInfo=null;resultSignature='';renderRecords();$('audio-name').textContent=name;
  const url='/api/audio?name='+encodeURIComponent(name);$('audio-player').src=url;$('download-audio').href=url;$('download-audio').download=name;$('download-audio').hidden=false;
  $('audio-meta').textContent=t('读取录音信息…');drawWave();renderResult();
  try{
    const r=await fetch('/api/audio-info?name='+encodeURIComponent(name)),info=await r.json();
    if(!r.ok)throw Error(info.error);if(selected!==name)return;
    audioInfo=info;renderAudioMeta();
    drawWave();$('classify').disabled=busy||!!state.activeJob;
  }catch(e){toast(e.message);$('audio-meta').textContent=t('无法解析录音');}
}
function renderAudioMeta(){
  const info=audioInfo,name=selected;if(!info)return;
  $('audio-meta').textContent=info.duration.toFixed(2)+t(' 秒 · ')+info.rate+' Hz · '+(info.channels===1?t('单声道'):t('双声道'))+' · '+(info.bytes/1024).toFixed(0)+' KB'+(info.peak===0?t(' · 静音文件'):'')+(name.includes('_gaps0')?'':t(' · 导入或需检查采样间断'));
}
function renderResult(){
  const r=state?.records.find(r=>r.name===selected)?.result;
  const sig=JSON.stringify([selected,r]);if(sig===resultSignature)return;resultSignature=sig;
  if(!r){$('result').innerHTML=t('<div class="result-empty">⌁<h3>声音里，还有待发现的线索</h3><p>选择录音，配置 API 后上传分析。<br>没有足够证据时，保留为未知。</p></div>');return;}
  $('result').innerHTML='<div class="result-label">'+escapeHTML(r.label)+'</div><div class="muted">'+escapeHTML(i18n.known(r.model))+' · '+date(r.at)+t('</div><div class="result-copy"><strong>声音依据</strong>')+escapeHTML(i18n.known(r.evidence))+t('<strong>不确定性</strong>')+escapeHTML(i18n.known(r.uncertainty))+'</div>'+
    (r.predictions||[]).slice(0,5).map(p=>'<div class="prediction"><span>'+escapeHTML(p.label)+t('</span><span>模型分数 ')+p.score.toFixed(3)+'</span></div>').join('')+t('<p class="small-note">AI输出尚未经专业人员核实，不用于自动驱赶或改变动物待遇。</p>');
}
$('classify').onclick=async()=>{
  if(!selected)return;
  if((state.config.provider==='gemini'&&!state.config.hasKey)||(state.config.provider==='custom'&&!state.config.endpoint)){toast(t('请先配置识别服务和密钥'));tab('settings');return;}
  busy=true;$('classify').disabled=true;$('classify').textContent=t('识别中，请稍候…');
  try{await request('/api/classify',{name:selected});toast(t('识别完成，结果已保存'));await poll();}catch(e){toast(e.message);}finally{busy=false;if(state)render();}
};
$('import-file').onchange=async e=>{
  const file=e.target.files[0];if(!file)return;
  try{if(file.size>8*1024*1024)throw Error(t('请选择8MB以内的WAV'));
    const r=await fetch('/api/import',{method:'POST',headers:{'X-HH-Token':state.csrf,'X-HH-Language':i18n.language(),'Content-Type':'audio/wav'},body:file}),v=await r.json();if(!r.ok)throw Error(v.error);
    await poll();selectRecord(v.name);toast(t('录音已导入本地'));
  }catch(err){toast(err.message);}finally{e.target.value='';}
};
function canvasContext(id){
  const c=$(id),width=c.clientWidth,height=c.clientHeight;if(!width||!height)return null;
  const ratio=window.devicePixelRatio||1;c.width=width*ratio;c.height=height*ratio;const ctx=c.getContext('2d');ctx.scale(ratio,ratio);
  return {ctx,width,height};
}
function drawTrend(){
  const canvas=canvasContext('trend');if(!canvas)return;const {ctx,width:w,height:h}=canvas;const metric=$('metric').value;
  const rows=state?.history||[],bit={inside_temp_c:1,inside_rh_pct:1,wick_raw:4,light_raw:8,water_raw:16,distance_mm:64,sound_rms:32}[metric];
  const vals=rows.filter(r=>(r.valid_mask&bit)&&r[metric]!==null).map(r=>r[metric]);ctx.font='10px Segoe UI';ctx.fillStyle='#9ba692';
  if(!vals.length){ctx.textAlign='center';ctx.fillText(t('等待真实传感器数据'),w/2,h/2);return;}
  let low=Math.min(...vals),high=Math.max(...vals),margin=Math.max((high-low)*.15,metric==='inside_temp_c'?1:2);low-=margin;high+=margin;
  const left=40,right=w-12,top=15,bottom=h-26;
  for(let i=0;i<4;i++){const y=top+(bottom-top)*i/3;ctx.strokeStyle='#edf1e9';ctx.beginPath();ctx.moveTo(left,y);ctx.lineTo(right,y);ctx.stroke();ctx.fillStyle='#97a18e';ctx.textAlign='right';ctx.fillText((high-(high-low)*i/3).toFixed(metric==='inside_temp_c'?1:0),left-8,y+3);}
  const end=Math.max(Date.now(),rows[rows.length-1].received_at),start=Math.max(rows[0].received_at,end-30*60*1000),span=Math.max(60000,end-start);
  ctx.strokeStyle='#60966a';ctx.lineWidth=2;ctx.beginPath();let previous=null;
  for(const r of rows){if(r.received_at<start)continue;if(!(r.valid_mask&bit)||r[metric]===null){previous=null;continue;}
    const x=left+(r.received_at-start)/span*(right-left),y=bottom-(r[metric]-low)/(high-low)*(bottom-top);
    if(!previous||r.received_at-previous.received_at>6000||r.boot_id!==previous.boot_id)ctx.moveTo(x,y);else ctx.lineTo(x,y);previous=r;
  }ctx.stroke();ctx.fillStyle='#97a18e';ctx.textAlign='left';ctx.fillText(time(start),left,h-6);ctx.textAlign='right';ctx.fillText(time(end),right,h-6);
}
function drawWave(){
  const c=canvasContext('waveform');if(!c)return;const {ctx,width:w,height:h}=c;
  ctx.fillStyle='#f4f7ee';ctx.fillRect(0,0,w,h);
  if(!audioInfo){ctx.fillStyle='#a0ad94';ctx.font='11px Segoe UI';ctx.textAlign='center';ctx.fillText(t('选择录音后显示波形'),w/2,h/2);return;}
  const values=audioInfo.waveform,peak=Math.max(.001,...values),step=w/values.length;
  ctx.fillStyle='#8eaf78';values.forEach((v,i)=>{const height=Math.max(2,v/peak*(h-30));ctx.fillRect(i*step,h/2-height/2,Math.max(1,step-1),height);});
  ctx.fillStyle='#7c8b70';ctx.font='9px Segoe UI';ctx.fillText(t('波形已自动缩放，播放音量保持原始值'),10,h-5);
}
$('metric').onchange=drawTrend;
for(let i=0;i<32;i++)document.querySelector('.sound-bars').appendChild(document.createElement('i'));
window.addEventListener('resize',()=>{drawTrend();drawWave();});
let polling=false;
async function poll(){
  if(polling)return;polling=true;
  try{const r=await fetch('/api/state',{cache:'no-store'});if(!r.ok)throw Error();state=await r.json();render();}
  catch{if(state){state.stale=true;render();}$('connection-badge').textContent=t('本地服务未连接');$('network-title').textContent=t('请重新启动 HollowHaven 软件');}
  finally{polling=false;}
}
i18n.bind();
document.addEventListener('languagechange',()=>{
  recordSignature='';resultSignature='';tab(activeTab);
  if(state)render();
  if(audioInfo)renderAudioMeta();
  if($('maintenance-dialog').open){
    const view=sensorStatusView(state?.latest,state?.stale,i18n.language());
    $('maintenance-dialog-body').innerHTML=view.alerts.map(a=>'<h3>'+escapeHTML(a.title)+'</h3><p>'+escapeHTML(a.detail)+'</p>').join('');
  }
  drawWave();
});
poll();setInterval(poll,1500);
