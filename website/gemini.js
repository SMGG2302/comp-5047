'use strict';
const DEFAULT_GEMINI_MODEL='gemini-3.8-flash';
const GEMINI_URL='https://generativelanguage.googleapis.com/v1beta/interactions';
function audioRequest(model,prompt,wav,key){
  return {url:GEMINI_URL,options:{method:'POST',headers:{'Content-Type':'application/json','x-goog-api-key':key},body:JSON.stringify({
    model,store:false,input:[{type:'text',text:prompt},{type:'audio',mime_type:'audio/wav',data:wav.toString('base64')}],
    response_format:{type:'text',mime_type:'application/json',schema:{type:'object',properties:{label:{type:'string'},evidence:{type:'string'},uncertainty:{type:'string'}},required:['label','evidence','uncertainty']}}
  })}};
}
function parseInteraction(result){
  if(result.status&&result.status!=='completed')throw Error('Gemini did not complete the analysis ('+result.status+').');
  // REST exposes final text in model_output steps, not SDK output_text helpers.
  const final=(result.steps||[]).filter(step=>step.type==='model_output').at(-1);
  const text=final?.content?.filter(part=>part.type==='text').map(part=>part.text||'').join('')
    ||(result.outputs||[]).filter(part=>part.type==='text').map(part=>part.text||'').join('')
    ||result.output_text;
  if(!text)throw Error('Gemini returned no analysis text. The request may have been blocked.');
  return JSON.parse(text);
}
function serviceError(status,result,key,language='en'){
  const zh=language==='zh';
  const hints={
    400:zh?'请检查请求参数、模型和项目计费前提。':'Check request parameters, model and billing prerequisites.',
    401:zh?'密钥无效或认证失败。':'Invalid API key or authentication failure.',
    403:zh?'请检查密钥绑定账号的权限、API启用状态和地区限制。':'Check the key account permissions, API enablement and region restrictions.',
    404:zh?'未找到模型或接口，请核对模型名称及账号可用模型。':'Model or endpoint not found. Check the model name and account availability.',
    429:zh?'触及限流或配额，请查看项目额度及计费状态。':'Rate limit or quota exceeded. Check project limits and billing status.'
  };
  let detail=typeof result?.error==='string'?result.error:result?.error?.message||result?.message||'';
  detail=String(detail);
  if(key)for(const secret of [key,encodeURIComponent(key)])detail=detail.split(secret).join('[redacted]');
  detail=detail.replace(/[\r\n\t]+/g,' ').slice(0,1500);
  return (zh?'识别服务返回 HTTP ':'Analysis service returned HTTP ')+status+'. '+(hints[status]||'')+(detail?' '+detail:'');
}
module.exports={DEFAULT_GEMINI_MODEL,audioRequest,parseInteraction,serviceError};
