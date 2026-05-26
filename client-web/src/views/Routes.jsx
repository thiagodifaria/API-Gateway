import React, { useState, useEffect } from 'react';
import API from '../api';

function RoutesView() {
  const [metrics, setMetrics] = useState(null);

  useEffect(() => {
    API.fetchMetrics().then(setMetrics);
  }, []);

  const activeRoutes = metrics?.requests_by_route ? Object.keys(metrics.requests_by_route).map(r => {
      const [method, ...pathParts] = r.split(' ');
      return { method, path: pathParts.join(' '), hits: metrics.requests_by_route[r] };
  }) : [];

  return (
    <div className="bg-white border border-slate-200 rounded-xl shadow-sm overflow-hidden">
      <div className="px-6 py-5 border-b border-slate-200 bg-slate-50 flex justify-between items-center">
        <div>
          <h3 className="text-lg font-semibold text-slate-800">Tabela de Roteamento Ativa</h3>
          <p className="text-xs text-slate-500 mt-1">Rotas que receberam tráfego desde a inicialização.</p>
        </div>
      </div>

      <div className="overflow-x-auto">
        <table className="w-full text-left text-sm border-collapse">
          <thead className="bg-white border-b border-slate-200">
            <tr className="text-slate-500 font-semibold text-xs uppercase tracking-wider">
              <th className="px-6 py-4">Method</th>
              <th className="px-6 py-4">Path Matcher</th>
              <th className="px-6 py-4">Total Hits</th>
              <th className="px-6 py-4 text-center">Auth required</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-100">
            {activeRoutes.length > 0 ? activeRoutes.map((route, i) => (
              <tr key={i} className="hover:bg-slate-50">
                <td className="px-6 py-4">
                  <span className={`px-2 py-1 text-[10px] font-bold rounded border ${
                    route.method === 'GET' ? 'bg-blue-50 text-blue-700 border-blue-200' :
                    route.method === 'POST' ? 'bg-green-50 text-green-700 border-green-200' :
                    'bg-slate-100 text-slate-600 border-slate-300'
                  }`}>
                    {route.method}
                  </span>
                </td>
                <td className="px-6 py-4 font-mono text-slate-700 font-medium">{route.path}</td>
                <td className="px-6 py-4 text-slate-500 font-mono text-xs">{route.hits}</td>
                <td className="px-6 py-4 text-center">
                    <span className="text-slate-300 text-xs font-bold">PUBLIC</span>
                </td>
              </tr>
            )) : (
              <tr>
                <td colSpan="4" className="px-6 py-10 text-center text-slate-400 italic">
                    Nenhuma rota ativa detectada. Use o console de testes para gerar tráfego.
                </td>
              </tr>
            )}
          </tbody>
        </table>
      </div>
    </div>
  );
}

export default RoutesView;
