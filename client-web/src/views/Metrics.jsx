import React, { useState, useEffect } from 'react';
import API from '../api';

function MetricsView() {
  const [metrics, setMetrics] = useState(null);

  useEffect(() => {
    API.fetchMetrics().then(setMetrics);
  }, []);

  if (!metrics) return <div>Carregando métricas...</div>;

  const mockChartData = Array.from({length: 24}, () => Math.floor(Math.random() * 100) + 20);

  return (
    <div className="space-y-6">
      <div className="bg-white border border-slate-200 rounded-xl shadow-sm p-6">
        <h3 className="text-lg font-semibold text-slate-800 mb-6 border-b border-slate-100 pb-2">Distribuição de Tráfego (Simulado)</h3>

        <div className="h-40 flex items-end gap-1.5 w-full">
          {mockChartData.map((val, i) => (
             <div
              key={i}
              className="flex-1 bg-blue-500/20 hover:bg-blue-500 rounded-t-sm transition-all duration-300 relative group"
              style={{ height: `${val}%` }}
             >
               <div className="absolute -top-8 left-1/2 -translate-x-1/2 bg-slate-800 text-white text-[10px] py-1 px-2 rounded opacity-0 group-hover:opacity-100 transition-opacity">
                 {val * 10} req
               </div>
             </div>
          ))}
        </div>
        <div className="flex justify-between text-xs text-slate-400 mt-2">
          <span>Há 24 min</span>
          <span>Agora</span>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        <div className="bg-white border border-slate-200 rounded-xl shadow-sm p-6">
           <h3 className="text-sm font-semibold text-slate-600 uppercase tracking-wider mb-4">Métricas de Operacionalidade</h3>
           <div className="space-y-3">
              <div>
                <div className="flex justify-between text-sm mb-1"><span className="text-slate-600">Rate Limited</span> <span className="font-bold text-slate-800">{metrics.rate_limit_hits_total || 0}</span></div>
                <div className="w-full bg-slate-100 rounded-full h-1.5"><div className="bg-amber-400 h-1.5 rounded-full" style={{ width: metrics.rate_limit_hits_total > 0 ? '40%' : '0%' }}></div></div>
              </div>
              <div>
                <div className="flex justify-between text-sm mb-1"><span className="text-slate-600">Cache Misses</span> <span className="font-bold text-slate-800">{metrics.cache_misses_total || 0}</span></div>
                <div className="w-full bg-slate-100 rounded-full h-1.5"><div className="bg-indigo-400 h-1.5 rounded-full" style={{ width: metrics.cache_misses_total > 0 ? '60%' : '0%' }}></div></div>
              </div>
              <div>
                <div className="flex justify-between text-sm mb-1"><span className="text-slate-600">Total Requests</span> <span className="font-bold text-slate-800">{metrics.requests_total || 0}</span></div>
                <div className="w-full bg-slate-100 rounded-full h-1.5"><div className="bg-blue-500 h-1.5 rounded-full" style={{ width: '100%' }}></div></div>
              </div>
           </div>
        </div>

        <div className="bg-slate-900 border border-slate-800 rounded-xl shadow-sm p-6 text-slate-300">
           <h3 className="text-sm font-semibold text-slate-400 uppercase tracking-wider mb-4 flex justify-between">
             <span>Endpoint Payload JSON</span>
             <span className="text-[10px] bg-slate-800 px-2 py-0.5 rounded border border-slate-700">/gateway/metrics</span>
           </h3>
           <pre className="text-xs font-mono text-green-400 overflow-x-auto p-4 bg-slate-950 rounded-lg border border-slate-800 max-h-[300px]">
             {JSON.stringify(metrics, null, 2)}
           </pre>
        </div>
      </div>
    </div>
  );
}

export default MetricsView;
