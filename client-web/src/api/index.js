const API = {
    async fetchMetrics() {
        try {
            const response = await fetch('/gateway/metrics', {
                headers: { 'Accept': 'application/json' }
            });
            if (!response.ok) throw new Error('Falha ao buscar métricas');
            return await response.json();
        } catch (error) {
            console.error('Metrics API Error:', error);
            return null;
        }
    },

    async fetchHealth() {
        try {
            const response = await fetch('/gateway/health');
            if (!response.ok) throw new Error('Falha ao buscar status');
            return await response.json();
        } catch (error) {
            console.error('Health API Error:', error);
            return null;
        }
    },

    async fetchUpstreams() {
        try {
            const response = await fetch('/gateway/ready');
            if (!response.ok) throw new Error('Falha ao buscar upstreams');
            return await response.json();
        } catch (error) {
            console.error('Upstreams API Error:', error);
            return null;
        }
    },

    async echo(data, headers = {}) {
        try {
            const response = await fetch('/api/echo', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    ...headers
                },
                body: JSON.stringify(data)
            });
            const result = await response.json();
            return {
                status: response.status,
                statusText: response.statusText,
                headers: Object.fromEntries(response.headers.entries()),
                data: result
            };
        } catch (error) {
            return {
                status: 500,
                statusText: 'Fetch Error',
                data: { error: error.message }
            };
        }
    },

    async customRequest(method, url, body, headers = {}) {
        try {
            const options = {
                method,
                headers: {
                    'Content-Type': 'application/json',
                    ...headers
                }
            };
            if (['POST', 'PUT', 'PATCH'].includes(method)) {
                options.body = body;
            }

            const response = await fetch(url, options);
            const contentType = response.headers.get('content-type');
            let data;
            if (contentType && contentType.includes('application/json')) {
                data = await response.json();
            } else {
                data = { message: await response.text() };
            }

            return {
                status: response.status,
                statusText: response.statusText,
                headers: Object.fromEntries(response.headers.entries()),
                data: data
            };
        } catch (error) {
            return {
                status: 500,
                statusText: 'Fetch Error',
                data: { error: error.message }
            };
        }
    }
};

export default API;
