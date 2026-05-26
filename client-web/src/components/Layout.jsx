import React from 'react';
import { CheckCircle2, AlertCircle } from 'lucide-react';

export const NavItem = ({ icon: Icon, label, id, current, set }) => (
  <button
    onClick={() => set(id)}
    className={`w-full flex items-center space-x-3 px-3 py-2 rounded-md text-sm font-medium transition-all duration-200 ${
      current === id
        ? 'bg-blue-600/10 text-blue-400'
        : 'text-slate-400 hover:bg-slate-800 hover:text-slate-200'
    }`}
  >
    <Icon size={16} />
    <span>{label}</span>
  </button>
);

export const Card = ({ title, value, subtitle, icon: Icon, colorClass }) => (
  <div className="bg-white p-5 rounded-xl border border-slate-200 shadow-sm flex items-start justify-between">
    <div>
      <p className="text-xs font-semibold text-slate-500 uppercase tracking-wider mb-1">{title}</p>
      <h3 className="text-2xl font-bold text-slate-800">{value}</h3>
      {subtitle && <p className="text-xs text-slate-400 mt-1">{subtitle}</p>}
    </div>
    <div className={`p-3 rounded-lg ${colorClass}`}>
      <Icon size={20} />
    </div>
  </div>
);

export const StatusBadge = ({ status }) => {
  const isOk = status === 'healthy' || status === 'active' || status === 'pass' || status === 'ok' || status === 'ready';
  return (
    <span className={`inline-flex items-center px-2 py-0.5 rounded text-xs font-semibold border ${
      isOk ? 'bg-emerald-50 text-emerald-700 border-emerald-200' : 'bg-rose-50 text-rose-700 border-rose-200'
    }`}>
      {isOk ? <CheckCircle2 size={12} className="mr-1"/> : <AlertCircle size={12} className="mr-1"/>}
      {status ? status.toUpperCase() : 'UNKNOWN'}
    </span>
  );
};
