/* DOM Elements */
const pipeTrack = document.getElementById('pipeline-track');
const grid = document.getElementById('threads-grid');

const elCores = document.getElementById('val-cores');
const elQueue = document.getElementById('val-queue');
const elDone = document.getElementById('val-done');
const elLoad = document.getElementById('val-load');
const elBadge = document.getElementById('queue-badge');
const lblPoolSize = document.getElementById('lbl-pool-size');

/* Thread Pool Engine (Pro Version) */
class EnterpriseThreadPool {
    constructor(coreCount = 8) {
        this.queue = [];
        this.threads = [];
        this.taskInc = 0;
        this.tasksCompleted = 0;
        
        // Capping visual DOM items in queue to 50 for pure performance during bursts.
        this.MAX_VISUAL_QUEUE = 50; 

        // Spin up cores
        for(let i=1; i<=coreCount; i++) {
            this.addCore(i);
        }
        
        // Load metric loop
        setInterval(() => this.calculateLoad(), 500);
    }

    addCore(idMap = null) {
        // High density architecture - limits for browser memory
        if(this.threads.length >= 128) {
            alert("Maximum system cores (128) reached. Hardware limit.");
            return; 
        }

        const id = idMap || (this.threads.length > 0 ? Math.max(...this.threads.map(t => t.id)) + 1 : 1);
        
        // Create highly compact node
        const node = document.createElement('div');
        node.className = 'core-node';
        node.innerHTML = `
            <div class="core-header">
                <span class="core-title">CPU_${id.toString().padStart(2, '0')}</span>
                <span class="core-status"></span>
            </div>
            <div class="core-payload idle-text">awaiting load...</div>
            <div class="core-progress-track">
                <div class="core-progress-bar"></div>
            </div>
        `;
        grid.appendChild(node);

        const threadObj = {
            id: id,
            elem: node,
            payloadBox: node.querySelector('.core-payload'),
            progressBar: node.querySelector('.core-progress-bar'),
            isBusy: false
        };
        
        this.threads.push(threadObj);
        this.updateTelemetry();
        this.scheduler();
    }

    removeCore() {
        if(this.threads.length <= 1) return;
        
        // Remove an idle thread if possible, else the last one
        const targetIdx = this.threads.slice().reverse().findIndex(t => !t.isBusy);
        const idx = targetIdx !== -1 ? this.threads.length - 1 - targetIdx : this.threads.length - 1;
        
        const core = this.threads[idx];
        core.elem.remove();
        this.threads.splice(idx, 1);
        this.updateTelemetry();
    }

    submitTask(burst = 1) {
        let tasksToAdd = [];
        for(let i=0; i<burst; i++) {
            this.taskInc++;
            const t = {
                id: `TXN-${this.taskInc.toString().padStart(5, '0')}`,
                workload: Math.floor(Math.random() * 2500) + 500 // 500ms to 3000ms
            };
            this.queue.push(t);
            tasksToAdd.push(t);
        }
        
        // Render to DOM (visually cap at 50 to maintain 60FPS)
        this.renderQueueDOM();
        this.updateTelemetry();
        this.scheduler();
    }

    renderQueueDOM() {
        pipeTrack.innerHTML = '';
        
        // Render only up to MAX
        const visibleTasks = this.queue.slice(0, this.MAX_VISUAL_QUEUE);
        visibleTasks.forEach(task => {
            const tNode = document.createElement('div');
            tNode.className = 'task-item';
            tNode.innerText = task.id;
            pipeTrack.appendChild(tNode);
        });

        // Add an indicator if there are hidden tasks
        if(this.queue.length > this.MAX_VISUAL_QUEUE) {
            const hiddenCount = document.createElement('div');
            hiddenCount.className = 'task-item';
            hiddenCount.style.background = 'transparent';
            hiddenCount.style.borderColor = 'transparent';
            hiddenCount.style.color = 'var(--text-secondary)';
            hiddenCount.innerText = `+${this.queue.length - this.MAX_VISUAL_QUEUE} more in memory`;
            pipeTrack.appendChild(hiddenCount);
        }
    }

    scheduler() {
        if(this.queue.length === 0) return;

        const availableCores = this.threads.filter(t => !t.isBusy);
        
        availableCores.forEach(core => {
            if(this.queue.length > 0) {
                // Dequeue
                const task = this.queue.shift();
                this.renderQueueDOM(); // Fast re-render
                this.updateTelemetry();
                this.execute(core, task);
            }
        });
    }

    execute(core, task) {
        core.isBusy = true;
        
        // UI Change
        core.elem.classList.add('active');
        core.payloadBox.classList.remove('idle-text');
        core.payloadBox.innerText = `${task.id}`;
        
        let start = null;
        const step = (time) => {
            if (!start) start = time;
            const flow = time - start;
            let percent = Math.min((flow / task.workload) * 100, 100);
            
            core.progressBar.style.width = percent + '%';

            if (flow < task.workload && core.isBusy) { // Check if busy (incase of reset)
                requestAnimationFrame(step);
            } else if (core.isBusy) {
                this.finish(core);
            }
        };

        requestAnimationFrame(step);
    }

    finish(core) {
        this.tasksCompleted++;
        
        // Free up core
        core.isBusy = false;
        core.elem.classList.remove('active');
        core.payloadBox.classList.add('idle-text');
        core.payloadBox.innerText = 'awaiting load...';
        core.progressBar.style.width = '0%';
        
        this.updateTelemetry();
        this.scheduler();
    }

    calculateLoad() {
        if(this.threads.length === 0) return;
        const busyCores = this.threads.filter(t => t.isBusy).length;
        const load = Math.round((busyCores / this.threads.length) * 100);
        elLoad.innerText = `${load}%`;
    }

    updateTelemetry() {
        elCores.innerText = this.threads.length;
        elQueue.innerText = this.queue.length;
        elDone.innerText = this.tasksCompleted;
        lblPoolSize.innerText = this.threads.length + " Core System";
        elBadge.innerText = this.queue.length + " Pending";
        
        if (this.queue.length > 50) {
            elBadge.style.color = "var(--danger)";
        } else if (this.queue.length > 0) {
            elBadge.style.color = "var(--accent)";
        } else {
            elBadge.style.color = "var(--text-secondary)";
        }
    }

    flushSystem() {
        this.queue = [];
        this.taskInc = 0;
        this.tasksCompleted = 0;
        
        this.threads.forEach(core => {
            core.isBusy = false; // aborts RAF
            core.elem.classList.remove('active');
            core.payloadBox.classList.add('idle-text');
            core.payloadBox.innerText = 'flush protocol...';
            core.progressBar.style.width = '0%';
        });

        this.renderQueueDOM();
        this.updateTelemetry();
        
        // Reset label after 500ms
        setTimeout(() => {
            this.threads.forEach(core => {
                if(!core.isBusy) core.payloadBox.innerText = 'awaiting load...';
            });
        }, 500);
    }
}

// Instance
const engine = new EnterpriseThreadPool(16); // Scale up to 16 cores by default to look impressive

// Bindings
document.getElementById('btn-add-task').addEventListener('click', () => {
    engine.submitTask(1);
});

document.getElementById('btn-add-batch').addEventListener('click', () => {
    // Generate an insane burst of 100 tasks to prove scalability
    engine.submitTask(100); 
});

document.getElementById('btn-reset').addEventListener('click', () => {
    engine.flushSystem();
});

document.getElementById('btn-scale-up').addEventListener('click', () => {
    engine.addCore();
});

document.getElementById('btn-scale-down').addEventListener('click', () => {
    engine.removeCore();
});

// Create initial beautiful data flow
setTimeout(() => {
    engine.submitTask(35);
}, 500);
