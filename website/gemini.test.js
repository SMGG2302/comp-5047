'use strict';
const {test}=require('node:test'),assert=require('node:assert/strict');
const {audioRequest,parseInteraction,serviceError}=require('./gemini');
test('Interactions request includes WAV, schema, current model and stateless storage',()=>{
 const {url,options}=audioRequest('gemini-3.8-flash','Analyze',Buffer.from('wav-data'),'test-key');
 assert.equal(url,'https://generativelanguage.googleapis.com/v1beta/interactions');
 const body=JSON.parse(options.body);
 assert.equal(body.model,'gemini-3.8-flash');assert.equal(body.store,false);
 assert.equal(body.input[1].mime_type,'audio/wav');assert.equal(Buffer.from(body.input[1].data,'base64').toString(),'wav-data');
 assert.equal(options.headers['x-goog-api-key'],'test-key');assert.equal(body.response_format.mime_type,'application/json');
 assert.deepEqual(body.response_format.schema.required,['label','evidence','uncertainty']);
});
test('REST output parsing ignores thought and tool steps and rejects incomplete output',()=>{
 const content=JSON.stringify({label:'Unknown',evidence:'Quiet',uncertainty:'Unclear'});
 assert.equal(parseInteraction({status:'completed',steps:[{type:'thought',content:[{type:'text',text:'not json'}]},{type:'model_output',content:[{type:'text',text:content}]}]}).label,'Unknown');
 assert.throws(()=>parseInteraction({status:'failed',steps:[]}));
 assert.throws(()=>parseInteraction({status:'completed',steps:[{type:'thought',content:[{type:'text',text:content}]}]}));
 assert.throws(()=>parseInteraction({status:'completed',steps:[{type:'model_output',content:[{type:'text',text:'bad json'}]}]}));
});
test('HTTP errors distinguish quota and missing model and redact provider-echoed credentials',()=>{
 const key='secret+key/123';
 const error=serviceError(404,{error:{message:'Model missing '+key+' '+encodeURIComponent(key)}},key,'zh');
 assert.match(error,/404/);assert.match(error,/未找到模型/);assert.match(error,/Model missing/);assert.ok(!error.includes(key)&&!error.includes(encodeURIComponent(key)));
 assert.match(serviceError(429,{error:{message:'Quota exhausted'}},key),/quota exceeded/);
 assert.match(serviceError(403,{},key),/permissions/);
});
test('server classification persists Interactions result and propagates upstream errors without secrets',async()=>{
 const fs=require('node:fs'),os=require('node:os'),path=require('node:path');
 const folder=fs.mkdtempSync(path.join(os.tmpdir(),'hh-gemini-test-'));
 const previous={HH_DATA_DIR:process.env.HH_DATA_DIR,HH_RECORDINGS_DIR:process.env.HH_RECORDINGS_DIR};
 const originalFetch=global.fetch;
 process.env.HH_DATA_DIR=path.join(folder,'data');process.env.HH_RECORDINGS_DIR=path.join(folder,'recordings');
 const api=require('./server'),{makeWav}=require('./protocol');let count=0;
 try{
  fs.writeFileSync(path.join(folder,'recordings','test.wav'),makeWav(Buffer.alloc(2000),1000));
  api.saveConfig({deviceHost:'auto',provider:'gemini',model:'gemini-3.8-flash',endpoint:'',candidates:'Bird, unknown',apiKey:'private-test-key'});
  global.fetch=async(url,options)=>{
   count++;assert.equal(url,'https://generativelanguage.googleapis.com/v1beta/interactions');
   const request=JSON.parse(options.body);assert.equal(request.store,false);assert.equal(request.input[1].mime_type,'audio/wav');assert.match(request.input[0].text,/Respond in Chinese/);
   return new Response(JSON.stringify({status:'completed',steps:[{type:'model_output',content:[{type:'text',text:JSON.stringify({label:'未知',evidence:'声音弱',uncertainty:'无法判断'})}]}]}),{status:200});
  };
  const result=await api.classify('test.wav','zh');assert.equal(result.label,'未知');assert.equal(count,1);
  assert.equal(JSON.parse(fs.readFileSync(path.join(folder,'data','results.json')) )['test.wav'].model,'gemini-3.8-flash');
  global.fetch=async()=>new Response(JSON.stringify({error:{message:'Model unavailable private-test-key'}}),{status:404});
  await assert.rejects(api.classify('test.wav'),err=>err.message.includes('404')&&err.message.includes('Model unavailable')&&!err.message.includes('private-test-key'));
  for(const file of fs.readdirSync(path.join(folder,'data')))assert.ok(!fs.readFileSync(path.join(folder,'data',file),'utf8').includes('private-test-key'));
 }finally{
  global.fetch=originalFetch;api.disconnect();
  for(const [key,value]of Object.entries(previous)){if(value===undefined)delete process.env[key];else process.env[key]=value;}
  assert.equal(path.dirname(path.resolve(folder)),path.resolve(os.tmpdir()));assert.ok(path.basename(folder).startsWith('hh-gemini-test-'));fs.rmSync(folder,{recursive:true,force:true});
 }
});
