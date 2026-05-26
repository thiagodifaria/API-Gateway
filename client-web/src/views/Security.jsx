import React from 'react';
import { Zap, Key } from 'lucide-react';
import { StatusBadge } from '../components/Layout';

function SecurityView() {
  return (
    <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
      <div className="bg-white border border-slate-200 rounded-xl shadow-sm p-6">
        <div className="flex items-center space-x-3 mb-6">
          <div className="bg-amber-100 text-amber-600 p-2 rounded-md"><Zap size={20} /></div>
          <h3 className="text-lg font-semibold text-slate-800">Rate Limiting (Token Bucket)</h3>
        </div>
        <div className="space-y-4">
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">Status</span>
            <StatusBadge status="active" />
          </div>
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">Capacidade do Bucket</span>
            <span className="text-sm font-mono font-bold text-slate-800">120 tokens</span>
          </div>
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">Taxa de Refill</span>
            <span className="text-sm font-mono font-bold text-slate-800">20 / sec</span>
          </div>
          <div className="flex justify-between items-center py-2">
            <span className="text-sm text-slate-600">Chave de Agrupamento</span>
            <span className="text-sm font-mono bg-slate-100 px-2 py-0.5 rounded border border-slate-200">IP Address</span>
          </div>
        </div>
      </div>

      <div className="bg-white border border-slate-200 rounded-xl shadow-sm p-6">
        <div className="flex items-center space-x-3 mb-6">
          <div className="bg-blue-100 text-blue-600 p-2 rounded-md"><Key size={20} /></div>
          <h3 className="text-lg font-semibold text-slate-800">Autenticação (JWT & API Key)</h3>
        </div>
        <div className="space-y-4">
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">Status</span>
            <StatusBadge status="disabled" />
          </div>
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">API Key Header</span>
            <span className="text-sm font-mono bg-slate-100 px-2 py-0.5 rounded border border-slate-200">X-API-Key</span>
          </div>
          <div className="flex justify-between items-center py-2 border-b border-slate-100">
            <span className="text-sm text-slate-600">JWT Validação</span>
            <span className="text-sm font-mono font-bold text-slate-800">HMAC-SHA256</span>
          </div>
        </div>
      </div>
    </div>
  );
}

export default SecurityView;
