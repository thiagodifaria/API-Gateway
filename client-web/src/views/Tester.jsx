import React, { useState } from 'react';
import { Play } from 'lucide-react';
import API from '../api';

function TesterView() {
  const [method, setMethod] = useState('GET');
  const [url, setUrl] = useState('/api/echo');
  const [useAuth, setUseAuth] = useState(false);
  const [reqBody, setReqBody] = useState('{\n  "message": "Olá do Console!",\n  "encode_data": "https-server"\n}');

  const [loading, setLoading] = useState(false);
  const [response, setResponse] = useState(null);
  const [time, setTime] = useState(0);

  const handleSend = async () => {
    setLoading(true);
    const start = performance.now();

    const headers = {};
    if (useAuth) headers['X-API-Key'] = 'dev-key';

    try {
      const res = await API.customRequest(method, url, reqBody, headers);
      setTime((performance.now() - start).toFixed(2));
      setResponse(res);
    } catch (err) {
      setResponse({ status: 500, statusText: 'Fetch Error', data: { error: err.message } });
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="grid grid-cols-1 lg:grid-cols-2 gap-6 items-start">
      <div className="bg-white border border-slate-200 rounded-xl shadow-sm overflow-hidden flex flex-col">
        <div className="px-4 py-3 bg-slate-50 border-b border-slate-200 flex justify-between items-center">
          <span className="text-sm font-bold text-slate-700 uppercase">Requisição</span>
          <button
            onClick={handleSend}
            disabled={loading}
            className="flex items-center space-x-1 px-4 py-1.5 bg-blue-600 text-white text-xs font-semibold rounded hover:bg-blue-700 disabled:opacity-50 transition-colors"
          >
            <Play size={14} /> <span>{loading ? '...' : 'Enviar'}</span>
          </button>
        </div>

        <div className="p-4 space-y-4">
          <div className="flex rounded-md shadow-sm">
            <select
              value={method}
              onChange={e => setMethod(e.target.value)}
              className="px-3 py-2 bg-slate-100 border border-slate-300 rounded-l-md text-sm font-bold text-slate-700 focus:outline-none"
            >
              {['GET', 'POST', 'PUT', 'DELETE'].map(m => <option key={m}>{m}</option>)}
            </select>
            <input
              type="text"
              value={url}
              onChange={e => setUrl(e.target.value)}
              className="flex-1 px-3 py-2 border-y border-r border-slate-300 rounded-r-md text-sm font-mono focus:outline-none focus:ring-1 focus:ring-blue-500"
            />
          </div>

          <div className="flex items-center justify-between p-3 border border-slate-200 rounded-md bg-slate-50">
            <span className="text-xs font-semibold text-slate-600">Header: X-API-Key</span>
            <label className="relative inline-flex items-center cursor-pointer">
              <input type="checkbox" className="sr-only peer" checked={useAuth} onChange={e => setUseAuth(e.target.checked)} />
              <div className="w-9 h-5 bg-slate-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all peer-checked:bg-blue-600"></div>
            </label>
          </div>

          {['POST', 'PUT', 'PATCH'].includes(method) && (
            <div>
              <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider mb-2 block">Body (JSON)</span>
              <textarea
                value={reqBody}
                onChange={e => setReqBody(e.target.value)}
                className="w-full h-40 p-3 font-mono text-xs bg-slate-900 text-slate-300 rounded-md border border-slate-800 focus:outline-none focus:ring-1 focus:ring-blue-500 resize-y"
              ></textarea>
            </div>
          )}
        </div>
      </div>

      <div className="bg-slate-900 border border-slate-800 rounded-xl shadow-sm overflow-hidden flex flex-col h-full min-h-[400px]">
        <div className="px-4 py-3 bg-slate-950 border-b border-slate-800 flex justify-between items-center">
          <span className="text-sm font-bold text-slate-300 uppercase">Resposta</span>
          {response && (
            <div className="flex items-center space-x-3">
              <span className="text-xs font-mono text-slate-400">{time}ms</span>
              <span className={`px-2 py-0.5 rounded text-xs font-bold border ${
                response.status < 300 ? 'bg-emerald-900/50 text-emerald-400 border-emerald-800' : 'bg-rose-900/50 text-rose-400 border-rose-800'
              }`}>
                {response.status} {response.statusText}
              </span>
            </div>
          )}
        </div>

        <div className="p-0 flex-1 overflow-auto">
          {!response ? (
             <div className="h-full flex items-center justify-center text-slate-600 text-sm italic">
               Nenhuma requisição enviada.
             </div>
          ) : (
            <div>
              {response.headers && Object.keys(response.headers).length > 0 && (
                <div className="p-4 border-b border-slate-800 bg-slate-900/50">
                  {Object.entries(response.headers).map(([k, v]) => (
                    <div key={k} className="text-[11px] font-mono leading-tight mb-1">
                      <span className="text-blue-400">{k}:</span> <span className="text-slate-300">{v}</span>
                    </div>
                  ))}
                </div>
              )}
              <pre className="p-4 font-mono text-sm text-green-400">
                {JSON.stringify(response.data, null, 2)}
              </pre>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default TesterView;
