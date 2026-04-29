import { useEffect, useState } from 'react';

interface SensorNode {
  id: number;
  scheme: string;
}

interface GatewayState {
  edge_node_id: number;
  status: string;
  uptime: number;
  connected_nodes: SensorNode[];
  firing_nodes: number[];
}

function App() {
  const [gateways, setGateways] = useState<GatewayState[]>([]);
  const [wsError, setWsError] = useState<boolean>(false);

  useEffect(() => {
    // Connect to the Go WebSocket server
    const ws = new WebSocket('ws://localhost:8080/ws');

    ws.onopen = () => {
      console.log('Connected to WS');
      setWsError(false);
    };

    ws.onmessage = (event) => {
      try {
        const data: GatewayState[] = JSON.parse(event.data);
        if (data) {
          setGateways(data);
        } else {
          setGateways([]);
        }
      } catch (err) {
        console.error('Failed to parse WS message', err);
      }
    };

    ws.onerror = (error) => {
      console.error('WebSocket Error:', error);
      setWsError(true);
    };

    ws.onclose = () => {
      console.log('WebSocket closed');
      setWsError(true);
    };

    return () => {
      ws.close();
    };
  }, []);

  const handleSetScheme = async (gatewayId: number, scheme: string) => {
    try {
      await fetch('http://localhost:8080/api/set_scheme', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ gateway_id: gatewayId, scheme })
      });
    } catch (err) {
      console.error('Failed to set scheme', err);
    }
  };

  return (
    <div className="min-h-screen bg-gray-900 text-gray-100 p-8 font-sans">
      <header className="mb-10 text-center">
        <h1 className="text-4xl font-bold tracking-tight text-white mb-2">
          Fire Detection Network
        </h1>
        <p className="text-gray-400">
          Live monitoring of Edge Gateways and Mesh Sensor Nodes
        </p>
        {wsError && (
          <div className="mt-4 inline-block bg-red-900/50 text-red-400 px-4 py-2 rounded-full text-sm font-medium border border-red-800">
            Disconnected from Server. Retrying...
          </div>
        )}
      </header>

      {gateways.length === 0 ? (
        <div className="text-center py-20">
          <div className="inline-block animate-spin rounded-full h-8 w-8 border-t-2 border-b-2 border-indigo-500 mb-4"></div>
          <p className="text-gray-500 text-lg">Waiting for Gateway heartbeats...</p>
        </div>
      ) : (
        <div className="max-w-6xl mx-auto space-y-8">
          {gateways.map((gw) => (
            <div key={gw.edge_node_id} className="bg-gray-800 rounded-xl shadow-xl overflow-hidden border border-gray-700">
              
              {/* Gateway Header */}
              <div className="bg-gray-800/80 px-6 py-4 border-b border-gray-700 flex justify-between items-center">
                <div className="flex items-center space-x-4">
                  <div className="h-3 w-3 bg-green-500 rounded-full shadow-[0_0_8px_#22c55e]"></div>
                  <h2 className="text-2xl font-semibold text-white">Gateway {gw.edge_node_id}</h2>
                  <select 
                    className="ml-4 bg-gray-700 text-white rounded px-2 py-1 border border-gray-600 focus:outline-none focus:border-indigo-500 text-sm"
                    onChange={(e) => handleSetScheme(gw.edge_node_id, e.target.value)}
                    defaultValue="diffusion"
                  >
                    <option value="diffusion">Diffusion Scheme</option>
                    <option value="iterative">Iterative Scheme</option>
                  </select>
                </div>
                <div className="text-sm text-gray-400">
                  Uptime: {gw.uptime}s
                </div>
              </div>

              {/* Connected Nodes Grid */}
              <div className="p-6 bg-gray-900/50">
                <h3 className="text-sm font-medium text-gray-400 uppercase tracking-wider mb-4">
                  Connected Sensor Nodes ({gw.connected_nodes ? gw.connected_nodes.length : 0})
                </h3>
                
                {!gw.connected_nodes || gw.connected_nodes.length === 0 ? (
                  <div className="text-gray-500 italic py-4">No sensor nodes connected yet.</div>
                ) : (
                  <div className="grid grid-cols-2 sm:grid-cols-3 md:grid-cols-4 lg:grid-cols-6 gap-4">
                    {gw.connected_nodes.map((node) => {
                      const isFiring = gw.firing_nodes && gw.firing_nodes.includes(node.id);
                      
                      return (
                        <div 
                          key={node.id}
                          className={`border rounded-lg p-4 flex flex-col items-center justify-center transition-all ${
                            isFiring 
                              ? 'bg-red-900/50 border-red-500 hover:bg-red-900/70' 
                              : 'bg-indigo-900/30 border-indigo-500/30 hover:bg-indigo-900/50'
                          }`}
                        >
                          <div className={`mb-1 ${isFiring ? 'text-red-400' : 'text-indigo-400'}`}>
                            {isFiring ? (
                              <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8 animate-pulse" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M17.657 18.657A8 8 0 016.343 7.343S7 9 9 10c0-2 .5-5 2.986-7C14 5 16.09 5.777 17.656 7.343A7.975 7.975 0 0120 13a7.975 7.975 0 01-2.343 5.657z" />
                                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9.879 16.121A3 3 0 1012.015 11L11 14H9c0 .768.293 1.536.879 2.121z" />
                              </svg>
                            ) : (
                              <svg xmlns="http://www.w3.org/2000/svg" className="h-8 w-8" fill="none" viewBox="0 0 24 24" stroke="currentColor">
                                <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M9 3v2m6-2v2M9 19v2m6-2v2M5 9H3m2 6H3m18-6h-2m2 6h-2M7 19h10a2 2 0 002-2V7a2 2 0 00-2-2H7a2 2 0 00-2 2v10a2 2 0 002 2zM9 9h6v6H9V9z" />
                              </svg>
                            )}
                          </div>
                          <span className={`text-lg font-bold ${isFiring ? 'text-red-100' : 'text-indigo-100'}`}>Node {node.id}</span>
                          <span className={`text-xs mt-1 ${isFiring ? 'text-red-300 font-bold animate-pulse' : 'text-indigo-300/70'}`}>
                            {isFiring ? '🔥 ALARM!' : 'Online'}
                          </span>
                          <span className="text-[10px] uppercase text-gray-400 mt-2 bg-gray-800 px-2 py-0.5 rounded-full border border-gray-700">
                            {node.scheme}
                          </span>
                        </div>
                      );
                    })}
                  </div>
                )}
              </div>

            </div>
          ))}
        </div>
      )}
    </div>
  );
}

export default App;
