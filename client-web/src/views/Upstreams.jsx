import React, { useState, useEffect } from 'react';
import { Database } from 'lucide-react';
import { StatusBadge } from '../components/Layout';
import API from '../api';

function UpstreamsView() {
  const [upstreams, setUpstreams] = useState(null);

  useEffect(() => {
    API.fetchUpstreams().then(setUpstreams);
  }, []);

  return (
    <div className="bg-white border border-slate-200 rounded-xl shadow-sm overflow-hidden">
       <div className="px-6 py-5 border-b border-slate-200">
        <h3 className="text-lg font-semibold text-slate-800">Pools de Upstream (Load Balancing)</h3>
        <p className="text-xs text-slate-500 mt-1">Status em tempo real dos health checks ativos.</p>
      </div>

      <div className="p-6 space-y-6">
        {upstreams && Object.entries(upstreams).length > 0 ? Object.entries(upstreams).map(([poolName, config]) => (
          <div key={poolName} className="border border-slate-200 rounded-lg p-5">
            <div className="flex justify-between items-start mb-4 pb-4 border-b border-slate-100">
              <div className="flex items-center space-x-3">
                <div className="bg-indigo-100 text-indigo-600 p-2 rounded-md">
                  <Database size={20} />
                </div>
                <div>
                  <h4 className="font-bold text-slate-800 text-lg">{poolName}</h4>
                  <p className="text-xs text-slate-500">LB Strategy: <span className="font-mono text-slate-700">Round Robin (Default)</span></p>
                </div>
              </div>
              <StatusBadge status="healthy" />
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <div>
                <p className="text-xs font-bold text-slate-400 uppercase mb-2">Targets Ativos</p>
                <div className="space-y-2">
                  {config.map(target => (
                    <div key={target} className="flex justify-between items-center bg-slate-50 px-3 py-2 rounded border border-slate-100 text-sm">
                      <span className="font-mono text-slate-600">{target}</span>
                      <span className="w-2 h-2 rounded-full bg-emerald-500"></span>
                    </div>
                  ))}
                </div>
              </div>
            </div>
          </div>
        )) : (
          <div className="text-center py-10 text-slate-400 italic">
            Nenhum upstream configurado no gateway.
          </div>
        )}
      </div>
    </div>
  );
}

export default UpstreamsView;
