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
    let radarChartInstance = null;
    let parsedMetrics = [];

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

    const radarFilter = document.getElementById('radarFilter');
    radarFilter.addEventListener('change', () => {
        updateRadarChart(radarFilter.value);
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
        
        radarFilter.value = 'aggregate';
        
        if (radarChartInstance) {
            radarChartInstance.destroy();
            radarChartInstance = null;
        }
    });

    analyzeBtn.addEventListener('click', () => {
        uploadSection.classList.add('hidden');
        dashboardSection.classList.remove('hidden');
        renderAnalysis();
    });

    function updateRadarChart(filterValue) {
        let labels = [];
        let dataBefore = [];
        let dataAfter = [];

        if (filterValue === 'aggregate') {
            const groups = {};
            parsedMetrics.forEach(m => {
                if (!groups[m.subsystem]) {
                    groups[m.subsystem] = [];
                }
                groups[m.subsystem].push(m.relativePerf);
            });

            Object.keys(groups).sort().forEach(sub => {
                const values = groups[sub];
                const avg = values.reduce((sum, v) => sum + v, 0) / values.length;
                labels.push(sub);
                dataBefore.push(100);
                dataAfter.push(avg);
            });
        } else if (filterValue === 'all') {
            parsedMetrics.forEach(m => {
                labels.push([m.subsystem, m.metric]);
                dataBefore.push(100);
                dataAfter.push(m.relativePerf);
            });
        } else if (filterValue.startsWith('subsystem:')) {
            const subName = filterValue.split(':')[1];
            const filtered = parsedMetrics.filter(m => m.subsystem === subName);
            filtered.forEach(m => {
                labels.push(m.metric);
                dataBefore.push(100);
                dataAfter.push(m.relativePerf);
            });
        }

        if (radarChartInstance) {
            radarChartInstance.data.labels = labels;
            radarChartInstance.data.datasets[0].data = dataBefore;
            radarChartInstance.data.datasets[1].data = dataAfter;
            radarChartInstance.update();
        } else {
            const ctx = document.getElementById('radarChart').getContext('2d');
            radarChartInstance = new Chart(ctx, {
                type: 'radar',
                data: {
                    labels: labels,
                    datasets: [
                        {
                            label: 'Before (Baseline = 100%)',
                            data: dataBefore,
                            backgroundColor: 'rgba(154, 160, 166, 0.08)',
                            borderColor: 'rgba(154, 160, 166, 0.6)',
                            pointBackgroundColor: 'rgba(154, 160, 166, 0.8)',
                            pointBorderColor: 'rgba(154, 160, 166, 1)',
                            borderWidth: 1.5,
                        },
                        {
                            label: 'After (Relative Performance)',
                            data: dataAfter,
                            backgroundColor: 'rgba(168, 199, 250, 0.15)',
                            borderColor: 'rgba(168, 199, 250, 0.85)',
                            pointBackgroundColor: 'rgba(168, 199, 250, 1)',
                            pointBorderColor: '#fff',
                            borderWidth: 2,
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: {
                        duration: 800,
                        easing: 'easeOutQuart'
                    },
                    scales: {
                        r: {
                            angleLines: { color: 'rgba(255, 255, 255, 0.08)' },
                            grid: { color: 'rgba(255, 255, 255, 0.08)' },
                            pointLabels: {
                                color: 'rgba(255, 255, 255, 0.7)',
                                font: { family: 'Inter', size: 10, weight: '500' }
                            },
                            ticks: { display: false }
                        }
                    },
                    plugins: {
                        legend: {
                            labels: {
                                color: 'rgba(255, 255, 255, 0.9)',
                                font: { family: 'Inter', size: 12, weight: '500' }
                            }
                        },
                        tooltip: {
                            backgroundColor: 'rgba(30, 30, 30, 0.95)',
                            titleColor: '#fff',
                            bodyColor: '#e3e3e3',
                            borderColor: 'rgba(255, 255, 255, 0.1)',
                            borderWidth: 1,
                            padding: 12,
                            cornerRadius: 8,
                            callbacks: {
                                label: function(context) {
                                    let label = context.dataset.label || '';
                                    if (label) {
                                        label += ': ';
                                    }
                                    if (context.parsed.r !== undefined) {
                                        label += context.parsed.r.toFixed(1) + '%';
                                    }
                                    return label;
                                },
                                footer: function(tooltipItems) {
                                    const filterVal = document.getElementById('radarFilter').value;
                                    if (filterVal !== 'aggregate') return '';

                                    const subsystem = tooltipItems[0].label;
                                    const subMetrics = parsedMetrics.filter(m => m.subsystem === subsystem);

                                    let lines = ['\nMetrics breakdown:'];
                                    subMetrics.forEach(m => {
                                        const pct = m.relativePerf - 100;
                                        const sign = pct >= 0 ? '+' : '';
                                        lines.push(`• ${m.metric}: ${sign}${pct.toFixed(1)}%`);
                                    });
                                    return lines.join('\n');
                                }
                            }
                        }
                    }
                }
            });
        }
    }

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

        // Calculate Scores
        function calculateScores(metricsArray) {
            let compute = 0;
            let memio = 0;
            let sys = 0;

            metricsArray.forEach(m => {
                const val = m.value;
                if (!val || val <= 0) return;

                switch (m.subsystem) {
                    case 'syscall':
                        compute += (val / 1000); // Usually 5M ops/s -> 5000
                        break;
                    case 'sched':
                        compute += (val / 100); // Usually 1M ops/s -> 10000
                        break;
                    case 'futex':
                        compute += (val / 1000); // Usually 3M ops/s -> 3000
                        break;
                    case 'eas':
                        if (m.metric.includes('throughput')) compute += (val / 10);
                        break;
                    case 'neon':
                        if (m.metric.includes('throughput')) compute += (val / 1000);
                        break;
                    
                    case 'mem':
                        memio += val; // Usually 20,000 MB/s -> 20000
                        break;
                    case 'io':
                        if (m.metric.includes('write')) memio += (val * 10); // Usually 500 MB/s -> 5000
                        else memio += val;
                        break;
                    case 'zram':
                        memio += (val * 5);
                        break;
                    case 'sqlite':
                        memio += (val * 10);
                        break;
                    case 'zero':
                        memio += (val);
                        break;
                    
                    case 'rng':
                        sys += (val * 10);
                        break;
                    case 'net':
                        sys += (val / 100); // UDP ops/s
                        break;
                    case 'crypto':
                        sys += (val / 100);
                        break;
                    case 'rcu':
                        sys += (val / 100);
                        break;
                }
            });

            return {
                compute: Math.round(compute),
                memio: Math.round(memio),
                sys: Math.round(sys),
                total: Math.round(compute + memio + sys)
            };
        }

        const scoresBefore = calculateScores(jsonBefore.metrics || []);
        const scoresAfter = calculateScores(jsonAfter.metrics || []);

        document.getElementById('scoreBefore').textContent = scoresBefore.total.toLocaleString();
        document.getElementById('compBefore').textContent = scoresBefore.compute.toLocaleString();
        document.getElementById('memBefore').textContent = scoresBefore.memio.toLocaleString();
        document.getElementById('sysBefore').textContent = scoresBefore.sys.toLocaleString();

        document.getElementById('scoreAfter').textContent = scoresAfter.total.toLocaleString();
        document.getElementById('compAfter').textContent = scoresAfter.compute.toLocaleString();
        document.getElementById('memAfter').textContent = scoresAfter.memio.toLocaleString();
        document.getElementById('sysAfter').textContent = scoresAfter.sys.toLocaleString();

        const scoreDeltaEl = document.getElementById('scoreDelta');
        if (scoresBefore.total > 0) {
            const diff = scoresAfter.total - scoresBefore.total;
            const pct = (diff / scoresBefore.total) * 100;
            scoreDeltaEl.textContent = (pct > 0 ? '+' : '') + pct.toFixed(1) + '%';
            if (pct > 0.5) scoreDeltaEl.className = 'score-delta delta-positive';
            else if (pct < -0.5) scoreDeltaEl.className = 'score-delta delta-negative';
            else scoreDeltaEl.className = 'score-delta delta-neutral';
        } else {
            scoreDeltaEl.textContent = 'N/A';
            scoreDeltaEl.className = 'score-delta delta-neutral';
        }

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

        parsedMetrics = [];

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
                
                // Radar Chart Data Prep
                if (valB > 0 && valA > 0) {
                    const higherBetter = isHigherBetter(unit);
                    let normalizedAfter = 100;
                    if (higherBetter) {
                        normalizedAfter = (valA / valB) * 100;
                    } else {
                        normalizedAfter = (valB / valA) * 100;
                    }
                    parsedMetrics.push({
                        subsystem: subsystem,
                        metric: metricName,
                        unit: unit,
                        valB: valB,
                        valA: valA,
                        relativePerf: normalizedAfter
                    });
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

        // Populate radar filter select options dynamically
        const subsystemsSet = new Set(parsedMetrics.map(m => m.subsystem));
        const uniqueSubsystems = Array.from(subsystemsSet).sort();
        
        radarFilter.innerHTML = `
            <option value="aggregate">All Subsystems (Aggregate)</option>
            <option value="all">All Metrics (Detailed)</option>
        `;
        uniqueSubsystems.forEach(sub => {
            const opt = document.createElement('option');
            opt.value = `subsystem:${sub}`;
            opt.textContent = `Subsystem: ${sub}`;
            radarFilter.appendChild(opt);
        });

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

        // Render Radar Chart with active selection
        updateRadarChart(radarFilter.value);
    }
});
