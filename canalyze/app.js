document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const landingSection = document.getElementById('landingSection');
    const dashboardSection = document.getElementById('dashboardSection');
    const mainDropZone = document.getElementById('mainDropZone');
    const mainFileInput = document.getElementById('mainFileInput');
    const compareRunSelect = document.getElementById('compareRunSelect');
    const uploadCompareBtn = document.getElementById('uploadCompareBtn');
    const compareFileInput = document.getElementById('compareFileInput');
    const importBtn = document.getElementById('importBtn');
    const resetBtn = document.getElementById('resetBtn');
    const historyList = document.getElementById('historyList');
    const historyCount = document.getElementById('historyCount');
    
    // Dashboard fields
    const activeRunName = document.getElementById('activeRunName');
    const activeRunMeta = document.getElementById('activeRunMeta');
    const renameActiveBtn = document.getElementById('renameActiveBtn');
    
    // Scores
    const scoreDashboard = document.getElementById('scoreDashboard');
    const scoreHeaderLeft = document.getElementById('scoreHeaderLeft');
    const scoreLeft = document.getElementById('scoreLeft');
    const compLeft = document.getElementById('compLeft');
    const memLeft = document.getElementById('memLeft');
    const sysLeft = document.getElementById('sysLeft');
    
    const comparisonVS = document.getElementById('comparisonVS');
    const scoreDelta = document.getElementById('scoreDelta');
    
    const comparisonScoreCard = document.getElementById('comparisonScoreCard');
    const scoreBaseline = document.getElementById('scoreBaseline');
    const compBaseline = document.getElementById('compBaseline');
    const memBaseline = document.getElementById('memBaseline');
    const sysBaseline = document.getElementById('sysBaseline');
    
    // Comparison Summary stats
    const comparisonSummary = document.getElementById('comparisonSummary');
    const totalImprovements = document.getElementById('totalImprovements');
    const totalRegressions = document.getElementById('totalRegressions');
    const totalUnchanged = document.getElementById('totalUnchanged');
    
    // System Info
    const sysinfoTitle = document.getElementById('sysinfoTitle');
    const sysinfoContent = document.getElementById('sysinfoContent');
    
    // Charts
    const chartCard = document.querySelector('.chart-card');
    const chartTitle = document.getElementById('chartTitle');
    const radarFilter = document.getElementById('radarFilter');
    let radarChartInstance = null;
    
    // Metrics Table
    const metricsTableTitle = document.getElementById('metricsTableTitle');
    const colValActive = document.getElementById('colValActive');
    const colValBaseline = document.getElementById('colValBaseline');
    const colDelta = document.getElementById('colDelta');
    const metricsBody = document.getElementById('metricsBody');
    
    // Heuristics
    const adviceActiveCard = document.getElementById('adviceActiveCard');
    const adviceActiveTitle = document.getElementById('adviceActiveTitle');
    const heuristicsActiveList = document.getElementById('heuristicsActiveList');
    const adviceBaselineCard = document.getElementById('adviceBaselineCard');
    const heuristicsBaselineList = document.getElementById('heuristicsBaselineList');
    
    // Sample buttons
    const loadSampleBefore = document.getElementById('loadSampleBefore');
    const loadSampleAfter = document.getElementById('loadSampleAfter');

    // State Variables
    let runs = [];
    let activeRun = null;
    let comparisonRun = null;
    let parsedMetrics = [];

    // Helper: Determine if metric improvement is positive or negative change
    function isHigherBetter(unit) {
        if (unit.includes('/sec') || unit.includes('/s') || unit.includes('/J')) return true;
        if (unit === 'ns' || unit === 'J') return false;
        return true; 
    }

    // Initialize the App
    function init() {
        loadHistory();
        setupEventListeners();
        renderHistoryList();
        
        // Show landing or load the last active run if present
        if (runs.length > 0) {
            setActiveRun(runs[0].id);
        } else {
            showLanding();
        }
    }

    // LocalStorage Operations
    function loadHistory() {
        try {
            const data = localStorage.getItem('canalyze_runs');
            runs = data ? JSON.parse(data) : [];
        } catch (e) {
            console.error("Failed to load runs from localStorage", e);
            runs = [];
        }
    }

    function saveHistory() {
        try {
            localStorage.setItem('canalyze_runs', JSON.stringify(runs));
        } catch (e) {
            console.error("Failed to save runs to localStorage", e);
        }
    }

    function addRunToHistory(runData, fileName) {
        const id = 'run_' + Date.now() + '_' + Math.random().toString(36).substr(2, 9);
        
        // Generate a friendly name based on sysinfo
        const kernel = runData.sysinfo?.["Kernel Version"] || "";
        const arch = runData.sysinfo?.["Architecture"] || "";
        const cpu = runData.sysinfo?.["CPUs"] || "";
        
        let generatedName = fileName ? fileName.replace('.json', '') : 'Run';
        if (kernel) {
            const kernelShort = kernel.split(' ')[0] + ' ' + (kernel.split(' ')[1] || '');
            generatedName = `Run - ${kernelShort} (${arch})`;
        }
        
        const newRun = {
            id: id,
            name: generatedName,
            timestamp: Date.now(),
            sysinfo: runData.sysinfo || {},
            metrics: runData.metrics || [],
            heuristics: runData.heuristics || []
        };
        
        // Insert at the beginning of list
        runs.unshift(newRun);
        
        // Cap runs list to 20 to avoid exceeding localStorage quota
        if (runs.length > 20) {
            runs.pop();
        }
        
        saveHistory();
        renderHistoryList();
        return id;
    }

    function deleteRun(id) {
        runs = runs.filter(r => r.id !== id);
        saveHistory();
        renderHistoryList();
        
        if (activeRun && activeRun.id === id) {
            activeRun = null;
            comparisonRun = null;
            if (runs.length > 0) {
                setActiveRun(runs[0].id);
            } else {
                showLanding();
            }
        } else if (comparisonRun && comparisonRun.id === id) {
            comparisonRun = null;
            compareRunSelect.value = "";
            renderDashboard();
        } else if (activeRun) {
            // Update the comparison selector since option was deleted
            populateComparisonSelector();
        }
    }

    function renameRun(id, newName) {
        const run = runs.find(r => r.id === id);
        if (run && newName && newName.trim()) {
            run.name = newName.trim();
            saveHistory();
            renderHistoryList();
            if (activeRun && activeRun.id === id) {
                activeRunName.textContent = run.name;
            }
            populateComparisonSelector();
        }
    }

    // UI State Toggles
    function showLanding() {
        landingSection.classList.remove('hidden');
        dashboardSection.classList.add('hidden');
        activeRun = null;
        comparisonRun = null;
        updateActiveSidebarClasses();
    }

    function setActiveRun(id) {
        const run = runs.find(r => r.id === id);
        if (run) {
            activeRun = run;
            // Clear comparison when changing active run, unless comparison select finds a match
            comparisonRun = null;
            landingSection.classList.add('hidden');
            dashboardSection.classList.remove('hidden');
            
            updateActiveSidebarClasses();
            populateComparisonSelector();
            renderDashboard();
        }
    }

    function updateActiveSidebarClasses() {
        const items = historyList.querySelectorAll('.history-item');
        items.forEach(item => {
            const itemId = item.getAttribute('data-id');
            item.classList.remove('active', 'comparing');
            if (activeRun && itemId === activeRun.id) {
                item.classList.add('active');
            } else if (comparisonRun && itemId === comparisonRun.id) {
                item.classList.add('comparing');
            }
        });
    }

    function populateComparisonSelector() {
        // Clear options keeping the default first option
        compareRunSelect.innerHTML = '<option value="">-- No Comparison (Single Run) --</option>';
        
        runs.forEach(run => {
            if (activeRun && run.id !== activeRun.id) {
                const opt = document.createElement('option');
                opt.value = run.id;
                
                // Add device and kernel info to the select label
                const arch = run.sysinfo?.["Architecture"] || "";
                const kernel = run.sysinfo?.["Kernel Version"] ? run.sysinfo["Kernel Version"].split(' ')[0] : "";
                const details = [arch, kernel].filter(Boolean).join(', ');
                
                opt.textContent = `${run.name} ${details ? `(${details})` : ''}`;
                compareRunSelect.appendChild(opt);
            }
        });
        
        compareRunSelect.value = comparisonRun ? comparisonRun.id : "";
    }

    // Rendering History list
    function renderHistoryList() {
        historyCount.textContent = `${runs.length} run${runs.length === 1 ? '' : 's'}`;
        
        if (runs.length === 0) {
            historyList.innerHTML = '<li class="empty-history">No runs imported yet. Import a benchmark JSON file to get started.</li>';
            return;
        }
        
        historyList.innerHTML = '';
        runs.forEach(run => {
            const li = document.createElement('li');
            li.className = 'history-item';
            li.setAttribute('data-id', run.id);
            
            // Calculate a score representation if we can
            const score = calculateScores(run.metrics || []);
            
            const arch = run.sysinfo?.["Architecture"] || "";
            const cpu = run.sysinfo?.["CPUs"] || "";
            const deviceMeta = [arch, cpu ? `${cpu} CPUs` : ""].filter(Boolean).join(' | ');
            
            const dateStr = new Date(run.timestamp).toLocaleDateString(undefined, {
                month: 'short',
                day: 'numeric',
                hour: '2-digit',
                minute: '2-digit'
            });

            li.innerHTML = `
                <div class="history-item-header">
                    <span class="history-item-name" title="${run.name}">${run.name}</span>
                    <div class="history-item-actions">
                        <button class="btn-icon btn-delete" title="Delete run" data-id="${run.id}">
                            <span class="material-icons-round" style="font-size: 16px;">delete</span>
                        </button>
                    </div>
                </div>
                <div class="history-item-meta">${dateStr}</div>
                ${deviceMeta ? `<div class="history-item-device"><span class="material-icons-round" style="font-size: 12px;">devices</span>${deviceMeta}</div>` : ''}
                <div class="history-item-score">Score: ${score.total.toLocaleString()}</div>
            `;
            
            li.addEventListener('click', (e) => {
                // If clicked delete button, do not select
                if (e.target.closest('.btn-delete')) return;
                setActiveRun(run.id);
            });
            
            // Delete button handler
            const delBtn = li.querySelector('.btn-delete');
            delBtn.addEventListener('click', (e) => {
                e.stopPropagation();
                if (confirm(`Are you sure you want to delete "${run.name}"?`)) {
                    deleteRun(run.id);
                }
            });

            historyList.appendChild(li);
        });
        
        updateActiveSidebarClasses();
    }

    // Score Calculations
    function calculateScores(metricsArray) {
        let compute = 0;
        let memio = 0;
        let sys = 0;

        metricsArray.forEach(m => {
            const val = m.value;
            if (!val || val <= 0) return;

            switch (m.subsystem) {
                case 'syscall':
                    compute += (val / 1000); 
                    break;
                case 'sched':
                    compute += (val / 100); 
                    break;
                case 'futex':
                    compute += (val / 1000); 
                    break;
                case 'eas':
                    if (m.metric.includes('throughput')) compute += (val / 10);
                    break;
                case 'neon':
                    if (m.metric.includes('throughput')) compute += (val / 1000);
                    break;
                
                case 'mem':
                    memio += val; 
                    break;
                case 'io':
                    if (m.metric.includes('write')) memio += (val * 10); 
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
                    sys += (val / 100); 
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

    // Dashboard Rendering
    function renderDashboard() {
        if (!activeRun) return;

        // Run Metadata
        activeRunName.textContent = activeRun.name;
        const kernel = activeRun.sysinfo?.["Kernel Version"] || "Unknown Kernel";
        const arch = activeRun.sysinfo?.["Architecture"] || "Unknown Arch";
        const cpu = activeRun.sysinfo?.["CPUs"] || "";
        activeRunMeta.textContent = `${arch} | ${cpu ? `${cpu} CPUs | ` : ''}${kernel}`;

        // Calculate active run score
        const scoreAct = calculateScores(activeRun.metrics);

        // Conditional rendering: Single Run vs. Comparison
        if (comparisonRun) {
            // Render COMPARISON MODE
            colValActive.textContent = "Current Value";
            colValBaseline.classList.remove('hidden');
            colDelta.classList.remove('hidden');
            
            comparisonVS.classList.remove('hidden');
            comparisonScoreCard.classList.remove('hidden');
            comparisonSummary.classList.remove('hidden');
            chartCard.classList.remove('hidden'); // Show radar chart
            adviceBaselineCard.classList.remove('hidden');
            
            // Set header names
            scoreHeaderLeft.textContent = "Current Score";
            metricsTableTitle.textContent = "Metrics Comparison";
            adviceActiveTitle.innerHTML = `<span class="material-icons-round">lightbulb</span> Current Advice`;
            
            // Set active score values
            scoreLeft.textContent = scoreAct.total.toLocaleString();
            compLeft.textContent = scoreAct.compute.toLocaleString();
            memLeft.textContent = scoreAct.memio.toLocaleString();
            sysLeft.textContent = scoreAct.sys.toLocaleString();

            // Set baseline score values
            const scoreBase = calculateScores(comparisonRun.metrics);
            scoreBaseline.textContent = scoreBase.total.toLocaleString();
            compBaseline.textContent = scoreBase.compute.toLocaleString();
            memBaseline.textContent = scoreBase.memio.toLocaleString();
            sysBaseline.textContent = scoreBase.sys.toLocaleString();

            // Score Delta calculation
            if (scoreBase.total > 0) {
                const diff = scoreAct.total - scoreBase.total;
                const pct = (diff / scoreBase.total) * 100;
                scoreDelta.textContent = (pct > 0 ? '+' : '') + pct.toFixed(1) + '%';
                if (pct > 0.5) scoreDelta.className = 'score-delta delta-positive';
                else if (pct < -0.5) scoreDelta.className = 'score-delta delta-negative';
                else scoreDelta.className = 'score-delta delta-neutral';
            } else {
                scoreDelta.textContent = 'N/A';
                scoreDelta.className = 'score-delta delta-neutral';
            }

            // Populate system info grid (comparison)
            sysinfoTitle.textContent = "System Information Comparison";
            sysinfoContent.innerHTML = '';
            const allSysKeys = new Set([...Object.keys(activeRun.sysinfo), ...Object.keys(comparisonRun.sysinfo)]);
            allSysKeys.forEach(key => {
                const vBase = comparisonRun.sysinfo[key] || 'N/A';
                const vAct = activeRun.sysinfo[key] || 'N/A';
                const val = vBase === vAct ? vAct : `${vBase} ➔ ${vAct}`;
                sysinfoContent.innerHTML += `
                    <div class="sys-item">
                        <div class="sys-item-key">${key}</div>
                        <div class="sys-item-val">${val}</div>
                    </div>
                `;
            });

            // Parse and render metrics in table
            renderComparisonMetricsTable(scoreAct, scoreBase);
            
            // Populate Advice lists
            renderAdviceList(activeRun.heuristics, heuristicsActiveList);
            renderAdviceList(comparisonRun.heuristics, heuristicsBaselineList);

            // Update radar chart
            updateRadarChart(radarFilter.value);

        } else {
            // Render SINGLE RUN MODE
            colValActive.textContent = "Value";
            colValBaseline.classList.add('hidden');
            colDelta.classList.add('hidden');
            
            comparisonVS.classList.add('hidden');
            comparisonScoreCard.classList.add('hidden');
            comparisonSummary.classList.add('hidden');
            chartCard.classList.add('hidden'); // Hide radar chart since no comparison baseline is loaded
            adviceBaselineCard.classList.add('hidden');
            
            // Set header names
            scoreHeaderLeft.textContent = "Benchmark Score";
            metricsTableTitle.textContent = "Benchmark Metrics";
            adviceActiveTitle.innerHTML = `<span class="material-icons-round">lightbulb</span> System Advice`;
            
            // Set active score values
            scoreLeft.textContent = scoreAct.total.toLocaleString();
            compLeft.textContent = scoreAct.compute.toLocaleString();
            memLeft.textContent = scoreAct.memio.toLocaleString();
            sysLeft.textContent = scoreAct.sys.toLocaleString();

            // Populate system info grid (single run)
            sysinfoTitle.textContent = "System Information";
            sysinfoContent.innerHTML = '';
            Object.keys(activeRun.sysinfo).forEach(key => {
                sysinfoContent.innerHTML += `
                    <div class="sys-item">
                        <div class="sys-item-key">${key}</div>
                        <div class="sys-item-val">${activeRun.sysinfo[key]}</div>
                    </div>
                `;
            });

            // Parse and render metrics in table
            renderSingleMetricsTable();
            
            // Populate active Advice list
            renderAdviceList(activeRun.heuristics, heuristicsActiveList);
            
            // Destroy chart if it exists
            if (radarChartInstance) {
                radarChartInstance.destroy();
                radarChartInstance = null;
            }
        }
        
        updateActiveSidebarClasses();
    }

    // Populate Advice Helper
    function renderAdviceList(adviceArray, containerElement) {
        containerElement.innerHTML = '';
        if (!adviceArray || adviceArray.length === 0) {
            containerElement.innerHTML = `<li><span class="material-icons-round" style="color:var(--success-color); vertical-align:middle; margin-right:4px;">check_circle</span> No bottlenecks detected.</li>`;
            return;
        }
        adviceArray.forEach(h => {
            containerElement.innerHTML += `<li><strong>${h.subsystem}</strong> ${h.message}</li>`;
        });
    }

    // Metrics Table for Single Run
    function renderSingleMetricsTable() {
        metricsBody.innerHTML = '';
        activeRun.metrics.forEach(m => {
            metricsBody.innerHTML += `
                <tr>
                    <td><strong>${m.subsystem}</strong></td>
                    <td>${m.metric}</td>
                    <td>${m.value !== null ? m.value.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td>${m.unit}</td>
                </tr>
            `;
        });
    }

    // Metrics Table for Comparison
    function renderComparisonMetricsTable() {
        metricsBody.innerHTML = '';
        
        let improvements = 0;
        let regressions = 0;
        let unchanged = 0;

        const metricsBaseMap = {};
        comparisonRun.metrics.forEach(m => {
            metricsBaseMap[m.subsystem + '|' + m.metric] = m;
        });

        const metricsActMap = {};
        activeRun.metrics.forEach(m => {
            metricsActMap[m.subsystem + '|' + m.metric] = m;
        });

        const allMetricKeys = new Set([...Object.keys(metricsBaseMap), ...Object.keys(metricsActMap)]);
        parsedMetrics = [];

        allMetricKeys.forEach(key => {
            const mb = metricsBaseMap[key];
            const ma = metricsActMap[key];
            
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
                    } else {
                        regressions++;
                        deltaClass = 'delta-negative';
                    }
                    deltaStr = (pct > 0 ? '+' : '') + pct.toFixed(2) + '%';
                }
                
                // Radar chart data preparation
                if (valB > 0 && valA > 0) {
                    const higherBetter = isHigherBetter(unit);
                    const normalizedAfter = higherBetter ? (valA / valB) * 100 : (valB / valA) * 100;
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

            metricsBody.innerHTML += `
                <tr>
                    <td><strong>${subsystem}</strong></td>
                    <td>${metricName}</td>
                    <td>${valA !== null ? valA.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td class="delta-neutral">${valB !== null ? valB.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td>${unit}</td>
                    <td class="${deltaClass}">${deltaStr}</td>
                </tr>
            `;
        });

        totalImprovements.textContent = improvements;
        totalRegressions.textContent = regressions;
        totalUnchanged.textContent = unchanged;
    }

    // Chart.js Radar Chart comparison rendering
    function updateRadarChart(filterValue) {
        if (!comparisonRun) return;

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

        // Rebuild subsystem dropdown select list
        const subsystemsSet = new Set(parsedMetrics.map(m => m.subsystem));
        const uniqueSubsystems = Array.from(subsystemsSet).sort();
        
        // Save current filter value before rebuilding options
        const currentFilterValue = radarFilter.value;
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
        radarFilter.value = uniqueSubsystems.includes(currentFilterValue.split(':')[1]) ? currentFilterValue : 'aggregate';

        if (radarChartInstance) {
            radarChartInstance.data.labels = labels;
            radarChartInstance.data.datasets[0].data = dataBefore;
            radarChartInstance.data.datasets[1].data = dataAfter;
            radarChartInstance.data.datasets[0].label = `${comparisonRun.name} (Baseline = 100%)`;
            radarChartInstance.data.datasets[1].label = `${activeRun.name} (Relative Performance)`;
            radarChartInstance.update();
        } else {
            const ctx = document.getElementById('radarChart').getContext('2d');
            radarChartInstance = new Chart(ctx, {
                type: 'radar',
                data: {
                    labels: labels,
                    datasets: [
                        {
                            label: `${comparisonRun.name} (Baseline = 100%)`,
                            data: dataBefore,
                            backgroundColor: 'rgba(154, 160, 166, 0.08)',
                            borderColor: 'rgba(154, 160, 166, 0.6)',
                            pointBackgroundColor: 'rgba(154, 160, 166, 0.8)',
                            pointBorderColor: 'rgba(154, 160, 166, 1)',
                            borderWidth: 1.5,
                        },
                        {
                            label: `${activeRun.name} (Relative Performance)`,
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
                                        label = label.split(' (')[0] + ': ';
                                    }
                                    if (context.parsed.r !== undefined) {
                                        label += context.parsed.r.toFixed(1) + '%';
                                    }
                                    return label;
                                },
                                footer: function(tooltipItems) {
                                    const filterVal = radarFilter.value;
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

    // Handle uploaded/imported file
    function handleFileImport(file, isBaseline = false) {
        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                const data = JSON.parse(e.target.result);
                if (!data.metrics) {
                    alert('Invalid JSON structure. Needs a "metrics" array.');
                    return;
                }
                const newId = addRunToHistory(data, file.name);
                
                if (isBaseline) {
                    comparisonRun = runs.find(r => r.id === newId);
                    populateComparisonSelector();
                    renderDashboard();
                } else {
                    setActiveRun(newId);
                }
            } catch (err) {
                alert('Invalid JSON file: ' + err.message);
            }
        };
        reader.readAsText(file);
    }

    // Set up Event Listeners
    function setupEventListeners() {
        // Drag & Drop
        mainDropZone.addEventListener('dragover', (e) => {
            e.preventDefault();
            mainDropZone.classList.add('dragover');
        });
        mainDropZone.addEventListener('dragleave', () => {
            mainDropZone.classList.remove('dragover');
        });
        mainDropZone.addEventListener('drop', (e) => {
            e.preventDefault();
            mainDropZone.classList.remove('dragover');
            const file = e.dataTransfer.files[0];
            if (file) handleFileImport(file);
        });
        mainDropZone.addEventListener('click', () => mainFileInput.click());
        mainFileInput.addEventListener('change', (e) => {
            if (e.target.files[0]) handleFileImport(e.target.files[0]);
        });

        // Top bar buttons
        importBtn.addEventListener('click', () => mainFileInput.click());
        resetBtn.addEventListener('click', () => {
            if (confirm("Reset analyzer? This will deselect the active run and return to the home screen. History will NOT be deleted.")) {
                showLanding();
            }
        });

        // Rename active run
        renameActiveBtn.addEventListener('click', () => {
            if (!activeRun) return;
            const newName = prompt("Enter a new name for this run:", activeRun.name);
            if (newName) renameRun(activeRun.id, newName);
        });

        // Compare run select changes
        compareRunSelect.addEventListener('change', (e) => {
            const selectVal = e.target.value;
            if (selectVal) {
                comparisonRun = runs.find(r => r.id === selectVal);
            } else {
                comparisonRun = null;
            }
            renderDashboard();
        });

        // Upload baseline file to compare
        uploadCompareBtn.addEventListener('click', () => compareFileInput.click());
        compareFileInput.addEventListener('change', (e) => {
            if (e.target.files[0]) handleFileImport(e.target.files[0], true);
        });

        // Filter select change
        radarFilter.addEventListener('change', () => {
            updateRadarChart(radarFilter.value);
        });

        // Sample Files loader
        loadSampleBefore.addEventListener('click', () => loadSampleFile('before.json', 'Before (Baseline)'));
        loadSampleAfter.addEventListener('click', () => loadSampleFile('after.json', 'After (Optimized)'));
    }

    // Fetch sample files from same dir
    function loadSampleFile(url, defaultName) {
        fetch(url)
            .then(res => {
                if (!res.ok) throw new Error("File not found");
                return res.json();
            })
            .then(data => {
                const newId = addRunToHistory(data, defaultName);
                setActiveRun(newId);
            })
            .catch(err => {
                alert(`Failed to load sample file (${url}). Ensure you are running locally via start_canalyze script.`);
                console.error(err);
            });
    }

    // Run the app!
    init();
});
