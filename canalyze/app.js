document.addEventListener('DOMContentLoaded', () => {
    // DOM Elements
    const themeToggleBtn = document.getElementById('themeToggleBtn');
    let currentTheme = localStorage.getItem('canalyze_theme') || 'dark';
    document.documentElement.setAttribute('data-theme', currentTheme);
    themeToggleBtn.querySelector('.material-icons-round').textContent = 
        currentTheme === 'dark' ? 'light_mode' : 'dark_mode';

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
    
    // Export Dropdown & Modal Elements
    const exportMenuBtn = document.getElementById('exportMenuBtn');
    const exportMenu = document.getElementById('exportMenu');
    const exportMarkdownBtn = document.getElementById('exportMarkdownBtn');
    const exportCsvBtn = document.getElementById('exportCsvBtn');
    const exportJsonBtn = document.getElementById('exportJsonBtn');
    const exportModal = document.getElementById('exportModal');
    const modalBackdrop = document.getElementById('modalBackdrop');
    const closeModalBtn = document.getElementById('closeModalBtn');
    const modalTitle = document.getElementById('modalTitle');
    const modalDescription = document.getElementById('modalDescription');
    const modalTextarea = document.getElementById('modalTextarea');
    const copyModalBtn = document.getElementById('copyModalBtn');
    const downloadModalBtn = document.getElementById('downloadModalBtn');

    // Telemetry Highlights Card
    const telemetryHighlightsCard = document.getElementById('telemetryHighlightsCard');
    const telemetryGrid = document.getElementById('telemetryGrid');

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
    const chartSubtitle = document.getElementById('chartSubtitle');
    const radarFilter = document.getElementById('radarFilter');
    const chartTypeRadarBtn = document.getElementById('chartTypeRadarBtn');
    const chartTypeBarBtn = document.getElementById('chartTypeBarBtn');
    let radarChartInstance = null;
    let currentChartType = 'radar'; // 'radar' or 'bar'
    let renderedChartType = null;
    let renderedComparisonMode = null;
    
    // Metrics Table & Search/Filter
    const metricsTableTitle = document.getElementById('metricsTableTitle');
    const metricsCountBadge = document.getElementById('metricsCountBadge');
    const metricSearchInput = document.getElementById('metricSearchInput');
    const clearSearchBtn = document.getElementById('clearSearchBtn');
    const subsystemFilterSelect = document.getElementById('subsystemFilterSelect');
    const subsystemChipsContainer = document.getElementById('subsystemChipsContainer');
    const colValActive = document.getElementById('colValActive');
    const colValBaseline = document.getElementById('colValBaseline');
    const colDelta = document.getElementById('colDelta');
    const metricsBody = document.getElementById('metricsBody');
    let selectedSubsystemFilter = 'all';
    let searchQuery = '';
    let sortColumn = 'subsystem';
    let sortAscending = true;
    let activeExportFileContent = '';
    let activeExportFileName = '';
    let activeExportMime = 'text/plain';
    
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
    let currentTab = 'tabOverview';
    let previousFocusElement = null;

    // Helper: Determine if metric improvement is positive or negative change
    function isHigherBetter(unit) {
        if (!unit) return true;
        if (unit.includes('/sec') || unit.includes('/s') || unit.includes('/J') || unit.includes('IPC')) return true;
        if (unit === 'ns' || unit === 'J' || unit === 'faults' || unit === '%' && unit.includes('miss')) return false;
        if (unit === 'C') return false; // Lower peak temperature is better
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

    // Tab Management
    function switchTab(tabId) {
        currentTab = tabId;
        
        // Update nav buttons
        document.querySelectorAll('.tab-btn').forEach(btn => {
            if (btn.getAttribute('data-tab') === tabId) {
                btn.classList.add('active');
            } else {
                btn.classList.remove('active');
            }
        });
        
        // Show active panel
        document.querySelectorAll('.tab-panel').forEach(panel => {
            if (panel.id === tabId) {
                panel.classList.remove('hidden');
            } else {
                panel.classList.add('hidden');
            }
        });
        
        // Redraw chart if switching to charts tab (since canvas dimensions might be wonky when hidden)
        if (tabId === 'tabCharts' && radarChartInstance) {
            radarChartInstance.resize();
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
            
            // Reset to Overview tab
            switchTab('tabOverview');
            
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
        const cpuModel = activeRun.sysinfo?.["cpu_model"] ? ` | ${activeRun.sysinfo["cpu_model"]}` : '';
        activeRunMeta.textContent = `${arch} | ${cpu ? `${cpu} CPUs` : ''}${cpuModel} | ${kernel}`;

        // Calculate active run score
        const scoreAct = calculateScores(activeRun.metrics);

        // Render Telemetry Highlights (Peak temp, IPC, L1 miss, TLB miss, energy, etc.)
        renderTelemetryHighlights();

        // Populate Subsystem Filter Select & Chips
        populateSubsystemFilterControls();

        // Conditional rendering: Single Run vs. Comparison
        if (comparisonRun) {
            // Render COMPARISON MODE
            colValActive.textContent = "Current Value";
            colValBaseline.classList.remove('hidden');
            colDelta.classList.remove('hidden');
            
            comparisonVS.classList.remove('hidden');
            comparisonScoreCard.classList.remove('hidden');
            comparisonSummary.classList.remove('hidden');
            chartCard.classList.remove('hidden');
            adviceBaselineCard.classList.remove('hidden');
            
            // Set header names
            scoreHeaderLeft.textContent = "Current Score";
            metricsTableTitle.textContent = "Metrics Comparison";
            chartTitle.textContent = currentChartType === 'radar' ? "Performance Comparison Radar" : "Relative Performance Breakdown";
            chartSubtitle.textContent = `Comparing against baseline: ${comparisonRun.name}`;
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

            // Parse comparison metrics
            prepareComparisonMetrics();

            // Render table
            renderMetricsTable();
            
            // Populate Advice lists
            renderAdviceList(activeRun.heuristics, heuristicsActiveList);
            renderAdviceList(comparisonRun.heuristics, heuristicsBaselineList);

            // Update chart
            updateChart();

        } else {
            // Render SINGLE RUN MODE
            colValActive.textContent = "Value";
            colValBaseline.classList.add('hidden');
            colDelta.classList.add('hidden');
            
            comparisonVS.classList.add('hidden');
            comparisonScoreCard.classList.add('hidden');
            comparisonSummary.classList.add('hidden');
            chartCard.classList.remove('hidden'); // Show chart in single run mode as well
            adviceBaselineCard.classList.add('hidden');
            
            // Set header names
            scoreHeaderLeft.textContent = "Benchmark Score";
            metricsTableTitle.textContent = "Benchmark Metrics";
            chartTitle.textContent = "Subsystem Performance & Energy Breakdown";
            chartSubtitle.textContent = `Single run analysis for ${activeRun.name}`;
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

            // Parse single run metrics
            prepareSingleRunMetrics();

            // Render table
            renderMetricsTable();
            
            // Populate active Advice list
            renderAdviceList(activeRun.heuristics, heuristicsActiveList);
            
            // Update chart for single run mode
            updateChart();
        }
        
        updateActiveSidebarClasses();
    }

    // Render Telemetry Highlights Grid
    function renderTelemetryHighlights() {
        if (!activeRun || !activeRun.metrics) return;
        telemetryGrid.innerHTML = '';

        const metricsMap = {};
        activeRun.metrics.forEach(m => {
            metricsMap[m.metric] = m;
        });

        // 1. Peak Temperature
        const tempMetric = metricsMap['peak_temp_c'] || metricsMap['peak_temp'];
        if (tempMetric) {
            const tempVal = tempMetric.value;
            let status = 'status-good';
            if (tempVal >= 85) status = 'status-alert';
            else if (tempVal >= 75) status = 'status-warn';
            telemetryGrid.innerHTML += `
                <div class="telemetry-item ${status}">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">Peak Temperature</span>
                        <span class="material-icons-round" style="font-size: 18px;">thermostat</span>
                    </div>
                    <div class="telemetry-item-value">${tempVal.toFixed(1)}<span class="telemetry-item-unit">°C</span></div>
                    <div class="telemetry-item-sub">${tempVal >= 85 ? 'Severe Throttling' : tempVal >= 75 ? 'Elevated' : 'Nominal'}</div>
                </div>
            `;
        }

        // 2. Hardware IPC
        const ipcMetric = metricsMap['ipc'] || metricsMap['hw_ipc'];
        if (ipcMetric) {
            const ipcVal = ipcMetric.value;
            const status = ipcVal >= 1.0 ? 'status-good' : (ipcVal < 0.6 ? 'status-alert' : 'status-warn');
            telemetryGrid.innerHTML += `
                <div class="telemetry-item ${status}">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">Hardware IPC</span>
                        <span class="material-icons-round" style="font-size: 18px;">memory</span>
                    </div>
                    <div class="telemetry-item-value">${ipcVal.toFixed(2)}<span class="telemetry-item-unit">IPC</span></div>
                    <div class="telemetry-item-sub">${ipcVal >= 1.0 ? 'Efficient execution' : 'Memory/pipeline stalls'}</div>
                </div>
            `;
        }

        // 3. Hardware L1 Miss Rate
        const l1Metric = metricsMap['hw_l1_miss_rate'];
        if (l1Metric) {
            const l1Val = l1Metric.value;
            const status = l1Val <= 4.0 ? 'status-good' : (l1Val > 6.0 ? 'status-alert' : 'status-warn');
            telemetryGrid.innerHTML += `
                <div class="telemetry-item ${status}">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">L1D Cache Miss</span>
                        <span class="material-icons-round" style="font-size: 18px;">disc_full</span>
                    </div>
                    <div class="telemetry-item-value">${l1Val.toFixed(2)}<span class="telemetry-item-unit">%</span></div>
                    <div class="telemetry-item-sub">${l1Val > 5.0 ? 'Cache thrashing' : 'Good locality'}</div>
                </div>
            `;
        }

        // 4. Branch Misprediction
        const branchMetric = metricsMap['hw_branch_miss_rate'];
        if (branchMetric) {
            const bVal = branchMetric.value;
            const status = bVal <= 1.5 ? 'status-good' : (bVal > 2.0 ? 'status-alert' : 'status-warn');
            telemetryGrid.innerHTML += `
                <div class="telemetry-item ${status}">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">Branch Mispredict</span>
                        <span class="material-icons-round" style="font-size: 18px;">alt_route</span>
                    </div>
                    <div class="telemetry-item-value">${bVal.toFixed(2)}<span class="telemetry-item-unit">%</span></div>
                    <div class="telemetry-item-sub">${bVal > 2.0 ? 'High branch stalls' : 'Well predicted'}</div>
                </div>
            `;
        }

        // 5. Overall Average Frequency
        const freqMetric = metricsMap['avg_freq_mhz'];
        if (freqMetric) {
            const fVal = freqMetric.value;
            telemetryGrid.innerHTML += `
                <div class="telemetry-item status-good">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">Average Frequency</span>
                        <span class="material-icons-round" style="font-size: 18px;">bolt</span>
                    </div>
                    <div class="telemetry-item-value">${Math.round(fVal)}<span class="telemetry-item-unit">MHz</span></div>
                    <div class="telemetry-item-sub">Across all active cores</div>
                </div>
            `;
        }

        // 6. Energy Consumption (if present)
        const energyMetric = metricsMap['energy_joules'];
        if (energyMetric) {
            telemetryGrid.innerHTML += `
                <div class="telemetry-item status-good">
                    <div class="telemetry-item-header">
                        <span class="telemetry-item-label">Subsystem Energy</span>
                        <span class="material-icons-round" style="font-size: 18px;">battery_charging_full</span>
                    </div>
                    <div class="telemetry-item-value">${energyMetric.value.toFixed(1)}<span class="telemetry-item-unit">J</span></div>
                    <div class="telemetry-item-sub">Hardware power sensor</div>
                </div>
            `;
        }

        // If no telemetry metrics present, hide card or show fallback
        if (telemetryGrid.children.length === 0) {
            telemetryHighlightsCard.classList.add('hidden');
        } else {
            telemetryHighlightsCard.classList.remove('hidden');
        }
    }

    // Populate Advice Helper with severity badge
    function renderAdviceList(adviceArray, containerElement) {
        containerElement.innerHTML = '';
        if (!adviceArray || adviceArray.length === 0) {
            containerElement.innerHTML = `<li><span class="material-icons-round" style="color:var(--success-color); vertical-align:middle; margin-right:4px;">check_circle</span> No bottlenecks detected.</li>`;
            return;
        }
        adviceArray.forEach(h => {
            const severity = h.severity || 'warning';
            const badgeClass = severity === 'critical' ? 'severity-critical' : (severity === 'info' ? 'severity-info' : 'severity-warning');
            containerElement.innerHTML += `
                <li>
                    <div class="heuristic-meta">
                        <strong>${h.subsystem}</strong>
                        <span class="severity-badge ${badgeClass}">${severity}</span>
                    </div>
                    <span>${h.message}</span>
                </li>
            `;
        });
    }

    // Populate subsystem filter controls (select dropdown & chips)
    function populateSubsystemFilterControls() {
        const subsystems = new Set();
        if (activeRun && activeRun.metrics) {
            activeRun.metrics.forEach(m => subsystems.add(m.subsystem));
        }
        if (comparisonRun && comparisonRun.metrics) {
            comparisonRun.metrics.forEach(m => subsystems.add(m.subsystem));
        }

        const sortedSubsystems = Array.from(subsystems).sort();

        // Update select
        subsystemFilterSelect.innerHTML = '<option value="all">All Subsystems</option>';
        sortedSubsystems.forEach(sub => {
            const opt = document.createElement('option');
            opt.value = sub;
            opt.textContent = sub;
            subsystemFilterSelect.appendChild(opt);
        });
        subsystemFilterSelect.value = selectedSubsystemFilter;

        // Update chips
        subsystemChipsContainer.innerHTML = `
            <button class="filter-chip ${selectedSubsystemFilter === 'all' ? 'active' : ''}" data-sub="all" aria-pressed="${selectedSubsystemFilter === 'all' ? 'true' : 'false'}">All</button>
        `;
        sortedSubsystems.forEach(sub => {
            const chip = document.createElement('button');
            chip.className = `filter-chip ${selectedSubsystemFilter === sub ? 'active' : ''}`;
            chip.setAttribute('data-sub', sub);
            chip.setAttribute('aria-pressed', selectedSubsystemFilter === sub ? 'true' : 'false');
            chip.textContent = sub;
            subsystemChipsContainer.appendChild(chip);
        });
    }

    // Prepare Single Run Metrics structure
    function prepareSingleRunMetrics() {
        parsedMetrics = (activeRun.metrics || []).map(m => ({
            subsystem: m.subsystem,
            metric: m.metric,
            unit: m.unit,
            valA: m.value,
            valB: null,
            deltaStr: '-',
            deltaClass: 'delta-neutral',
            relativePerf: 100
        }));
    }

    // Prepare Comparison Metrics structure
    function prepareComparisonMetrics() {
        let improvements = 0;
        let regressions = 0;
        let unchanged = 0;

        const metricsBaseMap = {};
        (comparisonRun.metrics || []).forEach(m => {
            metricsBaseMap[m.subsystem + '|' + m.metric] = m;
        });

        const metricsActMap = {};
        (activeRun.metrics || []).forEach(m => {
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
            let relativePerf = 100;

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
                
                if (valB > 0 && valA > 0) {
                    const higherBetter = isHigherBetter(unit);
                    relativePerf = higherBetter ? (valA / valB) * 100 : (valB / valA) * 100;
                }
            }

            parsedMetrics.push({
                subsystem: subsystem,
                metric: metricName,
                unit: unit,
                valB: valB,
                valA: valA,
                deltaStr: deltaStr,
                deltaClass: deltaClass,
                relativePerf: relativePerf
            });
        });

        totalImprovements.textContent = improvements;
        totalRegressions.textContent = regressions;
        totalUnchanged.textContent = unchanged;
    }

    // Render Metrics Table with filtering, searching, and sorting
    function renderMetricsTable() {
        metricsBody.innerHTML = '';
        
        let filtered = parsedMetrics.slice();

        // Subsystem filter
        if (selectedSubsystemFilter !== 'all') {
            filtered = filtered.filter(m => m.subsystem === selectedSubsystemFilter);
        }

        // Search query
        if (searchQuery.trim()) {
            const q = searchQuery.toLowerCase().trim();
            filtered = filtered.filter(m => 
                m.subsystem.toLowerCase().includes(q) || 
                m.metric.toLowerCase().includes(q) ||
                m.unit.toLowerCase().includes(q)
            );
        }

        // Sorting
        filtered.sort((a, b) => {
            let valA, valB;
            switch (sortColumn) {
                case 'subsystem':
                    valA = a.subsystem.toLowerCase();
                    valB = b.subsystem.toLowerCase();
                    break;
                case 'metric':
                    valA = a.metric.toLowerCase();
                    valB = b.metric.toLowerCase();
                    break;
                case 'active':
                    valA = a.valA !== null ? a.valA : -Infinity;
                    valB = b.valA !== null ? b.valA : -Infinity;
                    break;
                case 'baseline':
                    valA = a.valB !== null ? a.valB : -Infinity;
                    valB = b.valB !== null ? b.valB : -Infinity;
                    break;
                case 'delta':
                    valA = a.relativePerf;
                    valB = b.relativePerf;
                    break;
                default:
                    valA = a.subsystem;
                    valB = b.subsystem;
            }

            if (valA < valB) return sortAscending ? -1 : 1;
            if (valA > valB) return sortAscending ? 1 : -1;
            return 0;
        });

        // Metrics count badge
        metricsCountBadge.textContent = `${filtered.length} of ${parsedMetrics.length} metrics`;

        if (filtered.length === 0) {
            metricsBody.innerHTML = `
                <tr>
                    <td colspan="${comparisonRun ? 6 : 4}" style="text-align: center; color: var(--neutral-color); padding: 2rem;">
                        No metrics matched your filter query.
                    </td>
                </tr>
            `;
            return;
        }

        const fragment = document.createDocumentFragment();

        filtered.forEach(m => {
            const tr = document.createElement('tr');
            if (comparisonRun) {
                tr.innerHTML = `
                    <td><strong>${m.subsystem}</strong></td>
                    <td>${m.metric}</td>
                    <td>${m.valA !== null ? m.valA.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td class="delta-neutral">${m.valB !== null ? m.valB.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td>${m.unit}</td>
                    <td class="${m.deltaClass}">${m.deltaStr}</td>
                `;
            } else {
                tr.innerHTML = `
                    <td><strong>${m.subsystem}</strong></td>
                    <td>${m.metric}</td>
                    <td>${m.valA !== null ? m.valA.toLocaleString(undefined, {minimumFractionDigits: 2, maximumFractionDigits: 2}) : '-'}</td>
                    <td>${m.unit}</td>
                `;
            }
            fragment.appendChild(tr);
        });

        metricsBody.appendChild(fragment);

        // Update sort icons on header
        const headers = document.querySelectorAll('#metricsTableHeader th.sortable');
        headers.forEach(th => {
            th.classList.remove('sorted-asc', 'sorted-desc');
            const col = th.getAttribute('data-sort');
            if (col === sortColumn) {
                th.classList.add(sortAscending ? 'sorted-asc' : 'sorted-desc');
                th.setAttribute('aria-sort', sortAscending ? 'ascending' : 'descending');
            } else {
                th.setAttribute('aria-sort', 'none');
            }
        });

        const announce = document.getElementById('metricsAnnounce');
        if (announce) {
            announce.textContent = `Showing ${filtered.length} of ${parsedMetrics.length} metrics, sorted by ${sortColumn} ${sortAscending ? 'ascending' : 'descending'}.`;
        }
    }

    function getChartTextColor() {
        return currentTheme === 'dark' ? 'rgba(255, 255, 255, 0.7)' : 'rgba(0, 0, 0, 0.7)';
    }
    function getChartGridColor() {
        return currentTheme === 'dark' ? 'rgba(255, 255, 255, 0.08)' : 'rgba(0, 0, 0, 0.08)';
    }

    // Unified Chart Rendering (Radar or Bar; Comparison or Single Run)
    function updateChart() {
        const filterValue = radarFilter.value;
        const validMetrics = parsedMetrics.filter(m => (comparisonRun ? (m.valA > 0 && m.valB > 0) : (m.valA > 0)));

        let filtered = validMetrics;
        if (filterValue.startsWith('subsystem:')) {
            const subName = filterValue.split(':')[1];
            filtered = validMetrics.filter(m => m.subsystem === subName);
        }

        // Rebuild subsystem dropdown select list
        const subsystemsSet = new Set(validMetrics.map(m => m.subsystem));
        const uniqueSubsystems = Array.from(subsystemsSet).sort();
        
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
        radarFilter.value = uniqueSubsystems.includes(currentFilterValue.split(':')[1]) ? currentFilterValue : (currentFilterValue === 'all' ? 'all' : 'aggregate');

        // Extract labels and data
        let labels = [];
        let dataSeriesA = [];
        let dataSeriesB = [];

        if (comparisonRun) {
            // COMPARISON MODE CHART
            if (radarFilter.value === 'aggregate') {
                const groups = {};
                filtered.forEach(m => {
                    if (!groups[m.subsystem]) groups[m.subsystem] = [];
                    groups[m.subsystem].push(m.relativePerf);
                });

                Object.keys(groups).sort().forEach(sub => {
                    const values = groups[sub];
                    const avg = values.reduce((sum, v) => sum + v, 0) / values.length;
                    labels.push(sub);
                    dataSeriesB.push(100);
                    dataSeriesA.push(avg);
                });
            } else if (radarFilter.value === 'all') {
                filtered.forEach(m => {
                    labels.push([m.subsystem, m.metric]);
                    dataSeriesB.push(100);
                    dataSeriesA.push(m.relativePerf);
                });
            } else {
                filtered.forEach(m => {
                    labels.push(m.metric);
                    dataSeriesB.push(100);
                    dataSeriesA.push(m.relativePerf);
                });
            }
        } else {
            // SINGLE RUN MODE CHART: Display normalized subsystem metrics
            if (radarFilter.value === 'aggregate') {
                const subScores = {};
                filtered.forEach(m => {
                    if (!subScores[m.subsystem]) subScores[m.subsystem] = 0;
                    subScores[m.subsystem] += 1;
                });
                Object.keys(subScores).sort().forEach(sub => {
                    labels.push(sub);
                    const subM = filtered.filter(m => m.subsystem === sub);
                    const avgVal = subM.reduce((sum, m) => sum + m.valA, 0) / subM.length;
                    dataSeriesA.push(Math.round(avgVal * 100) / 100);
                });
            } else {
                filtered.forEach(m => {
                    labels.push(m.metric);
                    dataSeriesA.push(m.valA);
                });
            }
        }

        const isComparing = !!comparisonRun;
        const chartTypeChanged = (renderedChartType !== currentChartType) || (renderedComparisonMode !== isComparing);

        if (radarChartInstance && !chartTypeChanged) {
            radarChartInstance.data.labels = labels;
            if (isComparing) {
                radarChartInstance.data.datasets[0].label = `${comparisonRun.name} (Baseline = 100%)`;
                radarChartInstance.data.datasets[0].data = dataSeriesB;
                radarChartInstance.data.datasets[1].label = `${activeRun.name} (Relative ${currentChartType === 'radar' ? 'Performance' : '%'})`;
                radarChartInstance.data.datasets[1].data = dataSeriesA;
            } else {
                radarChartInstance.data.datasets[0].label = `${activeRun.name} (Raw Metrics)`;
                radarChartInstance.data.datasets[0].data = dataSeriesA;
            }
            radarChartInstance.update();
            return;
        }

        // Destroy previous instance
        if (radarChartInstance) {
            radarChartInstance.destroy();
            radarChartInstance = null;
        }

        renderedChartType = currentChartType;
        renderedComparisonMode = isComparing;

        const ctx = document.getElementById('radarChart').getContext('2d');

        if (comparisonRun) {
            if (currentChartType === 'radar') {
                radarChartInstance = new Chart(ctx, {
                    type: 'radar',
                    data: {
                        labels: labels,
                        datasets: [
                            {
                                label: `${comparisonRun.name} (Baseline = 100%)`,
                                data: dataSeriesB,
                                backgroundColor: 'rgba(154, 160, 166, 0.08)',
                                borderColor: 'rgba(154, 160, 166, 0.6)',
                                pointBackgroundColor: 'rgba(154, 160, 166, 0.8)',
                                pointBorderColor: 'rgba(154, 160, 166, 1)',
                                borderWidth: 1.5,
                            },
                            {
                                label: `${activeRun.name} (Relative Performance)`,
                                data: dataSeriesA,
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
                        animation: { duration: 600, easing: 'easeOutQuart' },
                        scales: {
                            r: {
                                angleLines: { color: getChartGridColor() },
                                grid: { color: getChartGridColor() },
                                pointLabels: {
                                    color: getChartTextColor(),
                                    font: { family: 'Inter', size: 10, weight: '500' }
                                },
                                ticks: { display: false }
                            }
                        },
                        plugins: {
                            legend: {
                                labels: { color: getChartTextColor(), font: { family: 'Inter', size: 12, weight: '500' } }
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
                                        let label = context.dataset.label ? context.dataset.label.split(' (')[0] + ': ' : '';
                                        if (context.parsed.r !== undefined) {
                                            label += context.parsed.r.toFixed(1) + '%';
                                        }
                                        return label;
                                    }
                                }
                            }
                        }
                    }
                });
            } else {
                // Grouped Bar Chart Comparison
                radarChartInstance = new Chart(ctx, {
                    type: 'bar',
                    data: {
                        labels: labels,
                        datasets: [
                            {
                                label: `${comparisonRun.name} (Baseline = 100%)`,
                                data: dataSeriesB,
                                backgroundColor: 'rgba(154, 160, 166, 0.4)',
                                borderColor: 'rgba(154, 160, 166, 0.8)',
                                borderWidth: 1,
                                borderRadius: 4
                            },
                            {
                                label: `${activeRun.name} (Relative %)`,
                                data: dataSeriesA,
                                backgroundColor: 'rgba(168, 199, 250, 0.6)',
                                borderColor: 'rgba(168, 199, 250, 1)',
                                borderWidth: 1,
                                borderRadius: 4
                            }
                        ]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        animation: { duration: 600, easing: 'easeOutQuart' },
                        scales: {
                            x: {
                                grid: { color: getChartGridColor() },
                                ticks: { color: getChartTextColor(), font: { family: 'Inter', size: 10 } }
                            },
                            y: {
                                grid: { color: getChartGridColor() },
                                ticks: {
                                    color: getChartTextColor(),
                                    callback: function(v) { return v + '%'; }
                                }
                            }
                        },
                        plugins: {
                            legend: {
                                labels: { color: getChartTextColor(), font: { family: 'Inter', size: 12, weight: '500' } }
                            }
                        }
                    }
                });
            }
        } else {
            // SINGLE RUN BAR CHART
            radarChartInstance = new Chart(ctx, {
                type: 'bar',
                data: {
                    labels: labels,
                    datasets: [
                        {
                            label: `${activeRun.name} (Raw Metrics)`,
                            data: dataSeriesA,
                            backgroundColor: 'rgba(168, 199, 250, 0.5)',
                            borderColor: 'rgba(168, 199, 250, 0.9)',
                            borderWidth: 1,
                            borderRadius: 4
                        }
                    ]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: {
                            grid: { color: getChartGridColor() },
                            ticks: { color: getChartTextColor(), font: { family: 'Inter', size: 10 } }
                        },
                        y: {
                            grid: { color: getChartGridColor() },
                            ticks: { color: getChartTextColor() }
                        }
                    },
                    plugins: {
                        legend: {
                            labels: { color: getChartTextColor() }
                        }
                    }
                }
            });
        }
    }

    // Generate Markdown Benchmark Report
    function generateMarkdownReport() {
        if (!activeRun) return '';
        const now = new Date().toISOString();
        let md = `# Cerium Benchmark Report\n\n`;
        md += `*Generated by CAnalyze on ${now}*\n\n`;

        // Run Metadata
        md += `## System Environment\n\n`;
        md += `| Attribute | Current (${activeRun.name}) | ${comparisonRun ? `Baseline (${comparisonRun.name})` : ''} |\n`;
        md += `| :--- | :--- | ${comparisonRun ? ':--- |' : ''}\n`;
        const allKeys = new Set([...Object.keys(activeRun.sysinfo || {}), ...(comparisonRun?.sysinfo ? Object.keys(comparisonRun.sysinfo) : [])]);
        allKeys.forEach(k => {
            const vA = activeRun.sysinfo[k] || '-';
            const vB = comparisonRun ? (comparisonRun.sysinfo[k] || '-') : '';
            md += `| **${k}** | ${vA} | ${comparisonRun ? `${vB} |` : ''}\n`;
        });
        md += `\n`;

        // Executive Summary
        const scoreAct = calculateScores(activeRun.metrics);
        if (comparisonRun) {
            const scoreBase = calculateScores(comparisonRun.metrics);
            const diff = scoreAct.total - scoreBase.total;
            const pct = scoreBase.total > 0 ? ((diff / scoreBase.total) * 100).toFixed(1) : 'N/A';
            md += `## Executive Benchmark Summary\n\n`;
            md += `- **Active Run Score**: ${scoreAct.total.toLocaleString()} (Compute: ${scoreAct.compute}, Mem/IO: ${scoreAct.memio}, Sys: ${scoreAct.sys})\n`;
            md += `- **Baseline Score**: ${scoreBase.total.toLocaleString()} (Compute: ${scoreBase.compute}, Mem/IO: ${scoreBase.memio}, Sys: ${scoreBase.sys})\n`;
            md += `- **Overall Score Delta**: **${pct > 0 ? '+' : ''}${pct}%**\n\n`;
        } else {
            md += `## Executive Benchmark Summary\n\n`;
            md += `- **Total Benchmark Score**: ${scoreAct.total.toLocaleString()}\n`;
            md += `  - Compute: ${scoreAct.compute.toLocaleString()}\n`;
            md += `  - Memory & I/O: ${scoreAct.memio.toLocaleString()}\n`;
            md += `  - System & Crypto: ${scoreAct.sys.toLocaleString()}\n\n`;
        }

        // Metrics Table
        md += `## Detailed Subsystem Metrics\n\n`;
        if (comparisonRun) {
            md += `| Subsystem | Metric | Current | Baseline | Unit | Delta |\n`;
            md += `| :--- | :--- | :--- | :--- | :--- | :--- |\n`;
            parsedMetrics.forEach(m => {
                const valA = m.valA !== null ? m.valA.toFixed(2) : '-';
                const valB = m.valB !== null ? m.valB.toFixed(2) : '-';
                md += `| ${m.subsystem} | ${m.metric} | ${valA} | ${valB} | ${m.unit} | ${m.deltaStr} |\n`;
            });
        } else {
            md += `| Subsystem | Metric | Value | Unit |\n`;
            md += `| :--- | :--- | :--- | :--- |\n`;
            parsedMetrics.forEach(m => {
                const valA = m.valA !== null ? m.valA.toFixed(2) : '-';
                md += `| ${m.subsystem} | ${m.metric} | ${valA} | ${m.unit} |\n`;
            });
        }
        md += `\n`;

        // Heuristics & Advice
        if (activeRun.heuristics && activeRun.heuristics.length > 0) {
            md += `## Kernel Patching Advice & Bottlenecks\n\n`;
            activeRun.heuristics.forEach(h => {
                const sev = (h.severity || 'warning').toUpperCase();
                md += `- **[${sev}] ${h.subsystem}**: ${h.message}\n`;
            });
            md += `\n`;
        }

        return md;
    }

    // Generate CSV export
    function generateCsvContent() {
        if (!activeRun) return '';
        let csv = '';
        if (comparisonRun) {
            csv = `"Subsystem","Metric","Current Value","Baseline Value","Unit","Delta Percent"\n`;
            parsedMetrics.forEach(m => {
                const valA = m.valA !== null ? m.valA : '';
                const valB = m.valB !== null ? m.valB : '';
                csv += `"${m.subsystem}","${m.metric}","${valA}","${valB}","${m.unit}","${m.deltaStr}"\n`;
            });
        } else {
            csv = `"Subsystem","Metric","Value","Unit"\n`;
            parsedMetrics.forEach(m => {
                const valA = m.valA !== null ? m.valA : '';
                csv += `"${m.subsystem}","${m.metric}","${valA}","${m.unit}"\n`;
            });
        }
        return csv;
    }

    // Show Export Modal
    function showExportModal(title, description, content, filename, mimeType) {
        previousFocusElement = document.activeElement;
        modalTitle.textContent = title;
        modalDescription.textContent = description;
        modalTextarea.value = content;
        activeExportFileContent = content;
        activeExportFileName = filename;
        activeExportMime = mimeType;
        exportModal.classList.remove('hidden');
        exportMenu.classList.add('hidden');
        exportMenuBtn.setAttribute('aria-expanded', 'false');
        modalTextarea.focus();
    }

    function closeExportModal() {
        exportModal.classList.add('hidden');
        if (previousFocusElement) previousFocusElement.focus();
    }

    function downloadActiveExportFile() {
        if (!activeExportFileContent) return;
        const blob = new Blob([activeExportFileContent], { type: activeExportMime });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = activeExportFileName;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);
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
        themeToggleBtn.addEventListener('click', () => {
            currentTheme = currentTheme === 'dark' ? 'light' : 'dark';
            document.documentElement.setAttribute('data-theme', currentTheme);
            localStorage.setItem('canalyze_theme', currentTheme);
            themeToggleBtn.querySelector('.material-icons-round').textContent = 
                currentTheme === 'dark' ? 'light_mode' : 'dark_mode';
            // Re-render chart to update colors
            if (activeRun) updateChart();
        });

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

        // Tab Navigation
        document.querySelectorAll('.tab-btn').forEach(btn => {
            btn.addEventListener('click', (e) => {
                const tabId = e.currentTarget.getAttribute('data-tab');
                switchTab(tabId);
            });
        });

        // Export Dropdown menu toggle
        exportMenuBtn.addEventListener('click', (e) => {
            e.stopPropagation();
            const isHidden = exportMenu.classList.toggle('hidden');
            exportMenuBtn.setAttribute('aria-expanded', !isHidden);
        });
        document.addEventListener('click', (e) => {
            if (!e.target.closest('.export-dropdown')) {
                exportMenu.classList.add('hidden');
                exportMenuBtn.setAttribute('aria-expanded', 'false');
            }
        });
        
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                if (!exportMenu.classList.contains('hidden')) {
                    exportMenu.classList.add('hidden');
                    exportMenuBtn.setAttribute('aria-expanded', 'false');
                    exportMenuBtn.focus();
                } else if (!exportModal.classList.contains('hidden')) {
                    closeExportModal();
                }
            }
        });
        
        exportModal.addEventListener('keydown', (e) => {
            if (e.key === 'Tab') {
                const focusableElements = exportModal.querySelectorAll('button, textarea');
                if (focusableElements.length === 0) return;
                const first = focusableElements[0];
                const last = focusableElements[focusableElements.length - 1];
                
                if (e.shiftKey) {
                    if (document.activeElement === first) {
                        e.preventDefault();
                        last.focus();
                    }
                } else {
                    if (document.activeElement === last) {
                        e.preventDefault();
                        first.focus();
                    }
                }
            }
        });

        // Export Actions
        exportMarkdownBtn.addEventListener('click', () => {
            if (!activeRun) { alert("Please select an active run first."); return; }
            const md = generateMarkdownReport();
            showExportModal("Export Markdown Report", "Preview the GitHub-flavored markdown report below:", md, "cbench_report.md", "text/markdown");
        });

        exportCsvBtn.addEventListener('click', () => {
            if (!activeRun) { alert("Please select an active run first."); return; }
            const csv = generateCsvContent();
            showExportModal("Export Metrics CSV", "Preview CSV output below:", csv, "cbench_metrics.csv", "text/csv");
        });

        exportJsonBtn.addEventListener('click', () => {
            if (!activeRun) { alert("Please select an active run first."); return; }
            const jsonStr = JSON.stringify(activeRun, null, 2);
            showExportModal("Export Run JSON", "Download run raw telemetry data:", jsonStr, `${activeRun.name.replace(/\s+/g, '_')}.json`, "application/json");
        });

        // Modal event handlers
        closeModalBtn.addEventListener('click', closeExportModal);
        modalBackdrop.addEventListener('click', closeExportModal);
        copyModalBtn.addEventListener('click', () => {
            navigator.clipboard.writeText(modalTextarea.value).then(() => {
                const origText = copyModalBtn.innerHTML;
                copyModalBtn.innerHTML = `<span class="material-icons-round">check</span> Copied!`;
                setTimeout(() => { copyModalBtn.innerHTML = origText; }, 2000);
            });
        });
        downloadModalBtn.addEventListener('click', downloadActiveExportFile);

        // Chart Type Toggles (Radar vs Bar)
        chartTypeRadarBtn.addEventListener('click', () => {
            currentChartType = 'radar';
            chartTypeRadarBtn.classList.add('active');
            chartTypeBarBtn.classList.remove('active');
            updateChart();
        });
        chartTypeBarBtn.addEventListener('click', () => {
            currentChartType = 'bar';
            chartTypeBarBtn.classList.add('active');
            chartTypeRadarBtn.classList.remove('active');
            updateChart();
        });

        // Debounce utility
        function debounce(fn, delay) {
            let timer;
            return function(...args) {
                clearTimeout(timer);
                timer = setTimeout(() => fn.apply(this, args), delay);
            };
        }

        // Search and filter in metrics table
        metricSearchInput.addEventListener('input', debounce((e) => {
            searchQuery = e.target.value;
            if (searchQuery) clearSearchBtn.classList.remove('hidden');
            else clearSearchBtn.classList.add('hidden');
            renderMetricsTable();
        }, 150));

        clearSearchBtn.addEventListener('click', () => {
            metricSearchInput.value = '';
            searchQuery = '';
            clearSearchBtn.classList.add('hidden');
            renderMetricsTable();
        });

        subsystemFilterSelect.addEventListener('change', (e) => {
            selectedSubsystemFilter = e.target.value;
            populateSubsystemFilterControls();
            renderMetricsTable();
        });

        subsystemChipsContainer.addEventListener('click', (e) => {
            const chip = e.target.closest('.filter-chip');
            if (!chip) return;
            selectedSubsystemFilter = chip.getAttribute('data-sub');
            populateSubsystemFilterControls();
            renderMetricsTable();
        });

        // Table Sorting Header clicks
        document.querySelectorAll('#metricsTableHeader th.sortable').forEach(th => {
            th.addEventListener('click', () => {
                const col = th.getAttribute('data-sort');
                if (sortColumn === col) {
                    sortAscending = !sortAscending;
                } else {
                    sortColumn = col;
                    sortAscending = true;
                }
                renderMetricsTable();
            });
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
            updateChart();
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
