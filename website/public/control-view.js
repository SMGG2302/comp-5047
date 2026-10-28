'use strict';
function sensorStatusView(s,stale){
  const unknown=label=>({label,value:'未知',tone:'',detail:'暂无新鲜有效数据'});
  const view={water:unknown('水箱补水'),sun:unknown('太阳直射'),valve:unknown('水阀'),left1:unknown('导水布 · 左一'),left2:unknown('导水布 · 左二'),alerts:[]};
  if(stale||!s)return view;
  if(s.valid_mask&16){
    const low=s.water_raw<50;
    view.water={label:'水箱补水',value:low?'需要补水 · 小于50ml':'暂不需要补水',tone:low?'alert':'ok',detail:`当前水位 ADC：${s.water_raw}；低于50触发补水。小于50ml为设定提示，非连续体积测量。`};
    if(low)view.alerts.push({id:'water',title:'需要补水维护',detail:'水箱水位低于设定阈值，显示小于50ml，请补充水源。'});
  }
  if(s.valid_mask&8)view.sun={label:'太阳直射',value:s.light_raw<500?'太阳直射':'未达到直射阈值',tone:s.light_raw<500?'warn':'ok',detail:`当前光照 ADC：${s.light_raw}；低于500判定太阳直射。`};
  for(const [key,label,raw,valid,status] of [['left1','导水布 · 左一',s.wick_raw,s.valid_mask&4,s.moisture1_state],['left2','导水布 · 左二',s.wick2_raw,s.wick2_valid,s.moisture2_state]]){
    if(valid)view[key]={label,value:['待判定','干燥','湿润'][status]||'待判定',tone:status===2?'ok':status===1?'warn':'',detail:`当前电容 ADC：${raw}；低于2000为湿润，达到2300为干燥（干态约2400），中间保持此前状态。`};
  }
  if(s.version>=4&&s.valve_state>0)view.valve={label:'水阀',value:['未知','关闭','正在打开','打开','正在关闭'][s.valve_state],tone:s.valve_state===3?'ok':s.valve_state===1?'':'warn',detail:`当前输出：${s.valve_pulse_us}μs；完全开阀已持续 ${Math.floor(s.valve_open_ms/1000)} 秒。1200μs打开，2500μs关闭；状态来自舵机输出，无机械位置传感器。`};
  if(s.version>=4&&s.outlet_blocked){
    const dry=[s.moisture1_state===1?'左一':'',s.moisture2_state===1?'左二':''].filter(Boolean).join('、');
    view.alerts.push({id:'blocked',title:'出水口堵塞',detail:`开阀已满1分钟，${dry}导水布仍干燥。请检查出水口及供水；这是规则告警，需现场核查。`});
  }
  return view;
}
if(typeof module!=='undefined')module.exports={sensorStatusView};
