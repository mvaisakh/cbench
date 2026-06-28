document.addEventListener('DOMContentLoaded', () => {
    const dropBefore = document.getElementById('dropBefore');
    const dropAfter = document.getElementById('dropAfter');
    const fileBefore = document.getElementById('fileBefore');
    const fileAfter = document.getElementById('fileAfter');
    const statusBefore = document.getElementById('statusBefore');
    const statusAfter = document.getElementById('statusAfter');
    const analyzeBtn = document.getElementById('analyzeBtn');
    const resetBtn = document.getElementById('resetBtn');

    const uploadSection = document.getElementById('uploadSection');
    const dashboardSection = document.getElementById('dashboardSection');

    let jsonBefore = null;
    let jsonAfter = null;

    // Helper: Determine if metric improvement is positive or negative change
    // Higher is better for: ops/sec, MB/s, ops/J, MB/J
    // Lower is better for: ns, J
    function isHigherBetter(unit) {
        if (unit.includes('/sec') || unit.includes('/s') || unit.includes('/J')) return true;
        if (unit === 'ns' || unit === 'J') return false;
        return true; // Default assume higher is better
    }

    function handleFile(file, isBefore) {
        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                const data = JSON.parse(e.target.result);
                if (isBefore) {
                    jsonBefore = data;
                    statusBefore.textContent = 'Loaded: ' + file.name;
                    statusBefore.className = 'status-chip loaded';
                } else {
                    jsonAfter = data;
                    statusAfter.textContent = 'Loaded: ' + file.name;
                    statusAfter.className = 'status-chip loaded';
                }
                checkReady();
            } catch (err) {
                alert('Invalid JSON file');
                if (isBefore) {
                    statusBefore.textContent = 'Error';
                    statusBefore.className = 'status-chip error';
                } else {
                    statusAfter.textContent = 'Error';
                    statusAfter.className = 'status-chip error';
                }
            }
        };
        reader.readAsText(file);
    }

    function checkReady() {
        if (jsonBefore && jsonAfter) {
            analyzeBtn.disabled = false;
        }
    }

    // Drag and Drop Event Listeners
    [dropBefore, dropAfter].forEach(zone => {
        zone.addEventListener('dragover', (e) => {
            e.preventDefault();
            zone.classList.add('dragover');
        });
        zone.addEventListener('dragleave', () => {
            zone.classList.remove('dragover');
        });
        zone.addEventListener('drop', (e) => {
            e.preventDefault();
            zone.classList.remove('dragover');
            const file = e.dataTransfer.files[0];
            if (file) {
                handleFile(file, zone.id === 'dropBefore');
            }
        });
        zone.addEventListener('click', () => {
            if (zone.id === 'dropBefore') fileBefore.click();
            else fileAfter.click();
        });
    });

    fileBefore.addEventListener('change', (e) => {
        if (e.target.files[0]) handleFile(e.target.files[0], true);
    });
    fileAfter.addEventListener('change', (e) => {
        if (e.target.files[0]) handleFile(e.target.files[0], false);
    });

    resetBtn.addEventListener('click', () => {
        jsonBefore = null;
        jsonAfter = null;
        fileBefore.value = '';
        fileAfter.value = '';
        statusBefore.textContent = 'Waiting...';
        statusBefore.className = 'status-chip empty';
        statusAfter.textContent = 'Waiting...';
        statusAfter.className = 'status-chip empty';
        analyzeBtn.disabled = true;
        dashboardSection.classList.add('hidden');
        uploadSection.classList.remove('hidden');
    });

    analyzeBtn.addEventListener('click', () => {
        uploadSection.classList.add('hidden');
        dashboardSection.classList.remove('hidden');
        renderAnalysis();
    });

    function renderAnalysis() {
        // 1. Sysinfo
        const sysinfoContent = document.getElementById('sysinfoContent');
        sysinfoContent.innerHTML = '';
        const keys = new Set([...Object.keys(jsonBefore.sysinfo || {}), ...Object.keys(jsonAfter.sysinfo || {})]);
        keys.forEach(key => {
            const v1 = jsonBefore.sysinfo?.[key] || 'N/A';
            const v2 = jsonAfter.sysinfo?.[key] || 'N/A';
            const val = v1 === v2 ? v2 : `${v1} ➔ ${v2}`;
            sysinfoContent.innerHTML += `
                <div class="sys-item">
                    <div class="sys-item-key">${key}</div>
                    <div class="sys-item-val">${val}</div>
                </div>
            `;
        });

        // 2. Metrics
        const tbody = document.getElementById('metricsBody');
        tbody.innerHTML = '';
        
        let improvements = 0;
        let regressions = 0;
        let unchanged = 0;

        const metricsB = {};
        (jsonBefore.metrics || []).forEach(m => {
            metricsB[m.subsystem + '|' + m.metric] = m;
        });

        const metricsA = {};
        (jsonAfter.metrics || []).forEach(m => {
            metricsA[m.subsystem + '|' + m.metric] = m;
        });

        const allMetricKeys = new Set([...Object.keys(metricsB), ...Object.keys(metricsA)]);

        allMetricKeys.forEach(key => {
            const mb = metricsB[key];
            const ma = metricsA[key];
            
            const subsystem = mb ? mb.subsystem : ma.subsystem;
            const metricName = mb ? mb.metric : ma.metric;
            const unit = mb ? mb.unit : ma.unit;
            const valB = mb ? mb.value : null;
            const valA = ma ? ma.value : null;

            let deltaStr = 'N/A';
            let deltaClass = 'delta-neutral';

            if (valB !== null && valA !== null) {
                const diff = valA - valB;
                if (Math.abs(diff) < 0.0001) {
                    deltaStr = '0.00%';
                    unchanged++;
                } else {
                    const pct = (diff / valB) * 100;
                    const higherBetter = isHigherBetter(unit);
                    
                    const isImproved = higherBetter ? (pct > 0) : (pct < 0);
                    
                    if (isImproved) {
                        improvements++;
                        deltaClass = 'delta-positive';
                        deltaStr = (pct > 0 ? '+' : '') + pct.toFixed(2) + '%';
                    } else {
                        regressions++;
                        deltaClass = 'delta-negative';
                        deltaStr = (pct > 0 ? '+' : '') + pct.toFixed(2) + '%';
                    }
                }
            }

            tbody.innerHTML += `
                <tr>
                    <td><strong>${subsystem}</strong></td>
                    <td>${metricName}</td>
                    <td>${valB !== null ? valB.toFixed(2) : '-'}</td>
                    <td>${valA !== null ? valA.toFixed(2) : '-'}</td>
                    <td>${unit}</td>
                    <td class="${deltaClass}">${deltaStr}</td>
                </tr>
            `;
        });

        document.getElementById('totalImprovements').textContent = improvements;
        document.getElementById('totalRegressions').textContent = regressions;
        document.getElementById('totalUnchanged').textContent = unchanged;

        // 3. Heuristics
        const hBList = document.getElementById('heuristicsBeforeList');
        const hAList = document.getElementById('heuristicsAfterList');
        hBList.innerHTML = '';
        hAList.innerHTML = '';

        (jsonBefore.heuristics || []).forEach(h => {
            hBList.innerHTML += `<li><strong>${h.subsystem}</strong> ${h.message}</li>`;
        });
        if (!jsonBefore.heuristics || jsonBefore.heuristics.length === 0) {
            hBList.innerHTML = `<li><span class="material-icons-round">check_circle</span> No bottlenecks detected.</li>`;
        }

        (jsonAfter.heuristics || []).forEach(h => {
            hAList.innerHTML += `<li><strong>${h.subsystem}</strong> ${h.message}</li>`;
        });
        if (!jsonAfter.heuristics || jsonAfter.heuristics.length === 0) {
            hAList.innerHTML = `<li><span class="material-icons-round">check_circle</span> No bottlenecks detected.</li>`;
        }
    }
});
