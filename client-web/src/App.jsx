import React, { useState } from 'react';
import { Activity, Server, ShieldCheck, BarChart2, TerminalSquare, Network, Route, Cpu, Clock } from 'lucide-react';
import { NavItem } from './components/Layout';
import DashboardView from './views/Dashboard';
import RoutesView from './views/Routes';
import UpstreamsView from './views/Upstreams';
import SecurityView from './views/Security';
import MetricsView from './views/Metrics';
import TesterView from './views/Tester';

function App() {
  const [activeTab, setActiveTab] = useState('dashboard');

  return (
    <div className="flex h-screen bg-[#F8FAFC] font-sans text-slate-800 w-full">

      {/* Sidebar Lateral Escura */}
      <aside className="w-64 bg-slate-900 text-slate-300 flex flex-col shadow-xl z-10 shrink-0">
        <div className="p-5 border-b border-slate-800 flex items-center space-x-3">
          <div className="bg-blue-600 p-1.5 rounded-lg text-white">
            <Server size={20} />
          </div>
          <div>
            <h1 className="text-sm font-bold text-white tracking-wide uppercase">C++ API Gateway</h1>
            <p className="text-[10px] text-blue-400 font-mono mt-0.5">v1.0.0-cpp17 • Release</p>
          </div>
        </div>

        <nav className="flex-1 py-6 px-3 space-y-1 overflow-y-auto custom-scrollbar">
          <p className="px-3 text-[10px] font-bold text-slate-500 uppercase tracking-wider mb-2 mt-4">Observabilidade</p>
          <NavItem icon={Activity} label="Dashboard" id="dashboard" current={activeTab} set={setActiveTab} />
          <NavItem icon={BarChart2} label="Métricas" id="metrics" current={activeTab} set={setActiveTab} />

          <p className="px-3 text-[10px] font-bold text-slate-500 uppercase tracking-wider mb-2 mt-6">Roteamento</p>
          <NavItem icon={Route} label="Proxy Routes" id="routes" current={activeTab} set={setActiveTab} />
          <NavItem icon={Network} label="Upstreams (Pools)" id="upstreams" current={activeTab} set={setActiveTab} />

          <p className="px-3 text-[10px] font-bold text-slate-500 uppercase tracking-wider mb-2 mt-6">Segurança</p>
          <NavItem icon={ShieldCheck} label="Auth & Rate Limit" id="security" current={activeTab} set={setActiveTab} />

          <p className="px-3 text-[10px] font-bold text-slate-500 uppercase tracking-wider mb-2 mt-6">Desenvolvimento</p>
          <NavItem icon={TerminalSquare} label="Console de Testes" id="tester" current={activeTab} set={setActiveTab} />
        </nav>

        <div className="p-4 bg-slate-950 border-t border-slate-800 flex items-center space-x-3">
          <div className="w-2 h-2 rounded-full bg-emerald-500 animate-pulse"></div>
          <span className="text-xs font-medium text-slate-400">Gateway Online</span>
        </div>
      </aside>

      {/* Área Principal */}
      <main className="flex-1 flex flex-col h-screen overflow-hidden">
        {/* Topbar */}
        <header className="bg-white border-b border-slate-200 px-8 py-4 flex justify-between items-center z-0">
          <h2 className="text-xl font-semibold text-slate-800 capitalize flex items-center gap-2">
            {activeTab.replace('_', ' ')}
          </h2>
          <div className="flex items-center space-x-4 text-sm">
            <span className="flex items-center space-x-1.5 px-3 py-1 bg-slate-100 rounded-md border border-slate-200 text-slate-600 font-mono text-xs">
              <Cpu size={14} className="text-blue-600" />
              <span>SIMD/ASM Ativos</span>
            </span>
            <span className="flex items-center space-x-1.5 px-3 py-1 bg-slate-100 rounded-md border border-slate-200 text-slate-600 font-mono text-xs">
              <Clock size={14} className="text-slate-500" />
              <span>Session: Real-time</span>
            </span>
          </div>
        </header>

        {/* Conteúdo Rolável */}
        <div className="flex-1 overflow-auto p-8 bg-slate-50">
          <div className="max-w-6xl mx-auto">
            {activeTab === 'dashboard' && <DashboardView />}
            {activeTab === 'routes' && <RoutesView />}
            {activeTab === 'upstreams' && <UpstreamsView />}
            {activeTab === 'security' && <SecurityView />}
            {activeTab === 'metrics' && <MetricsView />}
            {activeTab === 'tester' && <TesterView />}
          </div>
        </div>
      </main>

    </div>
  );
}

export default App;
