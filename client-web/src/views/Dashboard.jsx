import React, { useState, useEffect } from 'react';
import { Activity, Zap, Network, AlertCircle, Cpu, Server } from 'lucide-react';
import { Card, StatusBadge } from '../components/Layout';
import API from '../api';

function DashboardView() {
  const [metrics, setMetrics] = useState(null);
  const [health, setHealth] = useState(null);

  useEffect(() => {
    const fetchData = async () => {
        const m = await API.fetchMetrics();
        const h = await API.fetchHealth();
        setMetrics(m);
        setHealth(h);
    };
    fetchData();
    const interval = setInterval(fetchData, 5000);
    return () => clearInterval(interval);
  }, []);

  if (!metrics) return <div className="text-slate-500 animate-pulse">Sincronizando com gateway...</div>;

  const totalReq = metrics.requests_total || 0;
  const errors = (metrics.responses_by_status && metrics.responses_by_status["500"]) || 0;
  const errorRate = totalReq > 0 ? ((errors / totalReq) * 100).toFixed(4) : "0.0000";

  const simdModules = [
    { name: "HTTP Header Parser", type: "AVX2/SSE4.2", status: "active" },
    { name: "AES-GCM Crypto", type: "AES-NI", status: "active" },
    { name: "SHA256 Hashing", type: "Assembly NASM", status: "active" },
    { name: "Memory Operations", type: "SIMD Aligned", status: "active" }
  ];

  return (
    <div className="space-y-6">
      <div className="grid grid-cols-1 md:grid-cols-4 gap-4">
        <Card title="Total Requests" value={totalReq > 1000000 ? (totalReq / 1000000).toFixed(2) + "M" : totalReq} subtitle="Desde o último restart" icon={Activity} colorClass="bg-blue-50 text-blue-600" />
        <Card title="Rate Limit Hits" value={metrics.rate_limit_hits_total || 0} subtitle="Requisições bloqueadas" icon={Zap} colorClass="bg-amber-50 text-amber-600" />
        <Card title="Cache Hits" value={metrics.cache_hits_total || 0} subtitle="Respostas do cache" icon={Network} colorClass="bg-indigo-50 text-indigo-600" />
        <Card title="Taxa de Erro (500)" value={`${errorRate}%`} subtitle="Status 500 HTTP" icon={AlertCircle} colorClass="bg-rose-50 text-rose-600" />
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 bg-white border border-slate-200 rounded-xl shadow-sm">
          <div className="px-5 py-4 border-b border-slate-200 flex justify-between items-center">
            <h3 className="font-semibold text-slate-800 flex items-center gap-2">
              <Cpu size={18} className="text-slate-500"/> Aceleração de Hardware
            </h3>
            <span className="text-xs bg-slate-100 text-slate-600 px-2 py-1 rounded font-mono">C++17 Core</span>
          </div>
          <div className="p-0">
            <table className="w-full text-sm text-left">
              <thead className="bg-slate-50 border-b border-slate-200 text-slate-500 uppercase text-[10px] font-bold">
                <tr>
                  <th className="px-5 py-3">Módulo de Sistema</th>
                  <th className="px-5 py-3">Instruções / Engine</th>
                  <th className="px-5 py-3 text-right">Status</th>
                </tr>
              </thead>
              <tbody className="divide-y divide-slate-100">
                {simdModules.map((mod, i) => (
                  <tr key={i} className="hover:bg-slate-50 transition-colors">
                    <td className="px-5 py-3 font-medium text-slate-800">{mod.name}</td>
                    <td className="px-5 py-3 font-mono text-xs text-blue-600">{mod.type}</td>
                    <td className="px-5 py-3 text-right"><StatusBadge status={mod.status} /></td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>

        <div className="bg-white border border-slate-200 rounded-xl shadow-sm flex flex-col">
          <div className="px-5 py-4 border-b border-slate-200">
            <h3 className="font-semibold text-slate-800 flex items-center gap-2">
              <Server size={18} className="text-slate-500"/> Host System
            </h3>
          </div>
          <div className="p-5 flex-1 flex flex-col justify-center space-y-6">
            <div>
                <div className="flex justify-between text-xs mb-1">
                    <span className="text-slate-500 font-medium">Status do Gateway</span>
                    <StatusBadge status={health?.status || 'unknown'} />
                </div>
                <p className="text-[10px] text-slate-400 mt-2">O gateway está operando normalmente e aceitando conexões HTTPS na porta 8443.</p>
            </div>

            <div>
              <div className="flex justify-between text-xs mb-1">
                <span className="text-slate-500 font-medium">Uptime Estimado</span>
                <span className="text-slate-700 font-bold">Sessão Ativa</span>
              </div>
              <div className="w-full bg-slate-100 rounded-full h-2">
                <div className="bg-blue-600 h-2 rounded-full" style={{ width: '100%' }}></div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default DashboardView;
