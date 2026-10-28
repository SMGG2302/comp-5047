'use strict';
const LEGACY_COLUMNS='version,boot_id,sequence,uptime_ms,inside_temp_c,inside_rh_pct,outside_temp_c,wick_raw,light_raw,water_raw,valid_mask,sound_rms,sound_p2p,sound_samples,sound_clipped,adc_overflows,distance_mm,animal_present,presence_valid,audio_recording'.split(',');
const COLUMNS=[...LEGACY_COLUMNS,'wick2_raw','wick2_valid','moisture1_state','moisture2_state','valve_state','valve_pulse_us','valve_open_ms','outlet_blocked'];
const GREETING='#HOLLOWHAVEN/4 '+COLUMNS.join(',');
const LEGACY_GREETING='#HOLLOWHAVEN/3 '+LEGACY_COLUMNS.join(',');
function parseSample(line){
  const fields=line.trim().split(',');
  const v4=fields[0]==='4'&&fields.length===28;
  if(!v4 && !(fields.length===20&&fields[0]==='3'))throw Error('不兼容的传感器数据，需使用v4固件');
  const s={};
  fields.forEach((v,i)=>{
    if(v==='nan'&&[4,5,6,11].includes(i))s[COLUMNS[i]]=null;
    else {if(!/^-?\d+(\.\d+)?$/.test(v)||!Number.isFinite(+v))throw Error('数据格式错误');s[COLUMNS[i]]=+v;}
  });
  for(const i of [0,1,2,3,7,8,9,10,12,13,14,15,16,17,18,19])
    if(!Number.isSafeInteger(s[COLUMNS[i]]))throw Error('整数数据无效');
  for(const k of ['boot_id','sequence','sound_samples','sound_clipped','adc_overflows'])
    if(s[k]<0||s[k]>0xffffffff)throw Error('计数无效');
  if(s.uptime_ms<0||s.valid_mask<0||s.valid_mask>127||s.sound_clipped>s.sound_samples)throw Error('状态无效');
  for(const k of ['animal_present','presence_valid','audio_recording'])if(![0,1].includes(s[k]))throw Error('布尔值无效');
  function check(k,bit,lo,hi,missing=null){const v=s[k];if(s.valid_mask&bit){if(v===null||v<lo||v>hi)throw Error(k+'范围错误');}else if(v!==missing)throw Error(k+'有效位不一致');}
  check('inside_temp_c',1,-45,130);check('inside_rh_pct',1,0,100);check('outside_temp_c',2,-55,125);
  check('wick_raw',4,0,4095,-1);check('light_raw',8,0,4095,-1);check('water_raw',16,0,4095,-1);
  check('sound_rms',32,0,4095);check('sound_p2p',32,0,4095,-1);check('distance_mm',64,1,2000,-1);
  if(!!(s.valid_mask&64)!==!!s.presence_valid||!!(s.valid_mask&32)!==(s.sound_samples>0))throw Error('有效标志不一致');
  if(v4){
    for(const key of COLUMNS.slice(20))if(!Number.isSafeInteger(s[key]))throw Error('扩展状态无效');
    if(![0,1].includes(s.wick2_valid)||![0,1].includes(s.outlet_blocked))throw Error('扩展布尔值无效');
    if(s.wick2_valid ? s.wick2_raw<0||s.wick2_raw>4095 : s.wick2_raw!==-1)throw Error('第二路湿度无效');
    if(![0,1,2].includes(s.moisture1_state)||![0,1,2].includes(s.moisture2_state)||
       (!(s.valid_mask&4)&&s.moisture1_state)||(!s.wick2_valid&&s.moisture2_state))throw Error('湿润状态无效');
    if(s.valve_state<0||s.valve_state>4||s.valve_pulse_us<0||s.valve_pulse_us>=20000||s.valve_open_ms<0||s.valve_open_ms>0xffffffff)throw Error('阀门状态无效');
    if(s.valve_state!==3&&s.valve_open_ms!==0)throw Error('开阀计时无效');
    if(s.outlet_blocked&&(s.valve_state!==3||s.valve_open_ms<60000||![s.moisture1_state,s.moisture2_state].includes(1)))throw Error('堵塞状态无效');
  }else Object.assign(s,{wick2_raw:-1,wick2_valid:0,moisture1_state:0,moisture2_state:0,valve_state:0,valve_pulse_us:0,valve_open_ms:0,outlet_blocked:0});
  return {...s,received_at:Date.now()};
}
function crc32(data){
  let crc=0xffffffff;
  for(const byte of data){crc^=byte;for(let i=0;i<8;i++)crc=(crc>>>1)^((crc&1)?0xedb88320:0);}
  return (crc^0xffffffff)>>>0;
}
function audioHeader(h){
  if(h.length!==32||h.toString('ascii',0,4)!=='HHW1')throw Error('音频头不兼容');
  const [boot,seq,count,rate,crc,gaps,duration]=Array.from({length:7},(_,i)=>h.readUInt32LE(4+i*4));
  if(!count||count>320000||rate<1000||rate>10000||!((duration>=29000&&duration<=32000)||(duration>=59000&&duration<=62000)))throw Error('音频头数据无效');
  return {boot,seq,count,rate,crc,gaps,duration};
}
function makeWav(pcm,rate){
  const b=Buffer.alloc(44+pcm.length);b.write('RIFF');b.writeUInt32LE(pcm.length+36,4);b.write('WAVEfmt ',8);
  b.writeUInt32LE(16,16);b.writeUInt16LE(1,20);b.writeUInt16LE(1,22);b.writeUInt32LE(rate,24);
  b.writeUInt32LE(rate*2,28);b.writeUInt16LE(2,32);b.writeUInt16LE(16,34);b.write('data',36);b.writeUInt32LE(pcm.length,40);pcm.copy(b,44);return b;
}
function wavInfo(b){
  if(b.length<44||b.toString('ascii',0,4)!=='RIFF'||b.toString('ascii',8,12)!=='WAVE')throw Error('请选择WAV文件');
  let format,pcm;
  for(let i=12;i+8<=b.length;){
    const id=b.toString('ascii',i,i+4),n=b.readUInt32LE(i+4);if(i+8+n>b.length)throw Error('WAV文件不完整');
    if(id==='fmt '&&n>=16)format={encoding:b.readUInt16LE(i+8),channels:b.readUInt16LE(i+10),rate:b.readUInt32LE(i+12),align:b.readUInt16LE(i+20),bits:b.readUInt16LE(i+22)};
    if(id==='data')pcm=b.subarray(i+8,i+8+n);i+=8+n+(n%2);
  }
  if(!format||!pcm||format.encoding!==1||format.bits!==16||![1,2].includes(format.channels)||format.rate<1000||format.rate>192000||format.align!==format.channels*2||pcm.length%format.align)throw Error('仅支持完整的16位PCM WAV（单/双声道）');
  const frames=pcm.length/format.align;if(!frames)throw Error('空音频');
  let energy=0,peak=0;const buckets=Array(160).fill(0);
  for(let i=0;i<frames;i++){
    let value=0;for(let ch=0;ch<format.channels;ch++)value=Math.max(value,Math.abs(pcm.readInt16LE(i*format.align+ch*2)));
    energy+=value*value;peak=Math.max(peak,value);const k=Math.min(159,Math.floor(i*160/frames));buckets[k]=Math.max(buckets[k],value);
  }
  return {rate:format.rate,channels:format.channels,duration:frames/format.rate,bytes:b.length,peak,rms:Math.sqrt(energy/frames),waveform:buckets.map(v=>v/32768)};
}
function normalizeResult(value){
  if(Array.isArray(value)){
    if(!value.length||value.length>100)throw Error('分类结果为空或过长');
    const predictions=value.map(v=>{
      if(typeof v.label!=='string'||!v.label.trim()||typeof v.score!=='number'||!Number.isFinite(v.score)||v.score<0||v.score>1)throw Error('API分类结果需包含label和0~1的score');
      return {label:v.label.slice(0,150),score:v.score};
    }).sort((a,b)=>b.score-a.score);
    return {label:predictions[0].label,evidence:'分类模型输出，需人工核实。分数不等于已校准的识别概率。',uncertainty:'未经过本项目物种验证',predictions};
  }
  if(value&&Array.isArray(value.predictions))return normalizeResult(value.predictions);
  if(!value||typeof value.label!=='string'||!value.label.trim())throw Error('API未返回可识别的label字段');
  return {label:value.label.slice(0,150),evidence:String(value.evidence||'未提供声音依据').slice(0,3000),
    uncertainty:String(value.uncertainty||'需人工核实').slice(0,1500),predictions:[]};
}
module.exports={COLUMNS,GREETING,LEGACY_GREETING,parseSample,crc32,audioHeader,makeWav,wavInfo,normalizeResult};
