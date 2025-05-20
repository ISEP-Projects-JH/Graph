#pragma once

#include <string>

namespace default_page {

    std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Graph REPL</title>
    <style>
        body { background: #1e1e1e; color: #dcdcdc; font-family: monospace; padding: 20px; }
        #repl { width: 100%; height: 200px; background: #252526; color: #dcdcdc; border: none; padding: 10px; }
        #output { white-space: pre-wrap; margin-top: 20px; background: #1e1e1e; padding: 10px; border: 1px solid #555; }
        button { padding: 8px 16px; background: #007acc; color: white; border: none; cursor: pointer; }
        button:hover { background: #005a9e; }
    </style>
</head>
<body>
<h2>Graph REPL (Command Style)</h2>
<textarea id="repl" placeholder="a = createGraph(5, true, false)
a.addEdge(0, 1, 1)
a.degree(1)
a.centrality()
b = a
b.shortestPath(0)
del a
"></textarea><br>
<button onclick="run() ">Run</button>
<div id="output"></div>

<script>
    const graphs = {};

    class Graph {
        constructor(id, bi, weighted) {
            this.id = id;
            this.bi = bi;
            this.weighted = weighted;
        }

        async addEdge(u, v, weight = 1) {
            return await api('/graph/add-edge', { id: this.id, u, v, weight, bi: this.bi });
        }

        async batchAdd(edges) {
            const lines = edges.map(([u, v, w]) => ({ u, v, weight: w ?? 1 }));
            return await api('/graph/batch-edges', { id: this.id, bi: this.bi, lines });
        }

        async degree(node) {
            return await api(`/graph/degree?id=${this.id}&node=${node}`, null, 'GET');
        }

        async degreeStats() {
            return await api(`/graph/degree_stats?id=${this.id}`, null, 'GET');
        }

        async exists() {
            return await api(`/graph/exists?id=${this.id}`, null, 'GET');
        }

        async listIds() {
            return await api('/graph/list_ids', null, 'GET');
        }

        async centrality() {
            return await api(`/graph/betweenness_centrality?id=${this.id}`, null, 'GET');
        }

        async isolated() {
            return await api(`/graph/isolated_nodes?id=${this.id}`, null, 'GET');
        }

        async shortestPath(start) {
            return await api(`/graph/shortest_path?id=${this.id}&start=${start}`, null, 'GET');
        }

        async countTriangles() {
            return await api(`/graph/count_triangles?id=${this.id}`, null, 'GET');
        }

        async destroy() {
            await api(`/graph/destroy?id=${this.id}`, null, 'DELETE');
        }
    }

    window.addEventListener("beforeunload", async () => {
        for (const key in graphs) {
            await graphs[key].destroy();
        }
    });

    async function createGraph(size, bi = true, weighted = false) {
        const res = await api('/graph/create', { size });
        return new Graph(res.id, bi, weighted);
    }

    async function api(url, body = null, method = 'POST') {
        const res = await fetch(url, {
            method,
            headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer YOUR_TOKEN' },
            body: body ? JSON.stringify(body) : undefined
        });
        return await res.json();
    }

    window.run = async function run() {
        const input = document.getElementById("repl").value;
        const output = document.getElementById("output");
        output.innerText = '';

        const lines = input.split('\n');
        for (const line of lines) {
            const trimmed = line.trim();
            if (!trimmed) continue;

            try {
                const assignMatch = trimmed.match(/^(\w+)\s*=\s*createGraph\(([^)]*)\)/);
                if (assignMatch) {
                    const [, varName, argStr] = assignMatch;
                    const [size, bi, weighted] = argStr.split(',').map(s => s.trim());
                    const graph = await createGraph(parseInt(size), bi === 'true', weighted === 'true');
                    graphs[varName] = graph;
                    output.innerText += `${varName} = { id: ${graph.id} }\n`;
                    continue;
                }
                // Alias assignment
                const refAssign = trimmed.match(/^(\w+)\s*=\s*(\w+)$/);
                if (refAssign) {
                    const [, lhs, rhs] = refAssign;
                    if (graphs[rhs]) {
                        graphs[lhs] = graphs[rhs];
                        output.innerText += `${lhs} now refers to ${rhs} (id=${graphs[rhs].id})\n`;
                    } else {
                        output.innerText += `Error: Graph "${rhs}" not found\n`;
                    }
                    continue;
                }

                const delMatch = trimmed.match(/^del\s+(\w+)$/);
                if (delMatch) {
                    const [, varName] = delMatch;
                    if (graphs[varName]) {
                        await graphs[varName].destroy();
                        delete graphs[varName];
                        output.innerText += `${varName} destroyed\n`;
                    } else {
                        output.innerText += `Error: Graph "${varName}" not found\n`;
                    }
                    continue;
                }

                const callMatch = trimmed.match(/^(\w+)\.(\w+)\((.*)\)$/);
                if (callMatch) {
                    const [, varName, method, args] = callMatch;
                    const graph = graphs[varName];
                    if (!graph) throw new Error(`Graph "${varName}" not found`);
                    const parsedArgs = eval(`[${args}]`);
                    const result = await graph[method](...parsedArgs);
                    output.innerText += `${JSON.stringify(result, null, 2)}\n`;
                    continue;
                }

                output.innerText += `Unknown command: ${line}\n`;
            } catch (err) {
                output.innerText += `Error: ${err.message}\n`;
            }
        }
    };
</script>
</body>
</html>
    )";
}
