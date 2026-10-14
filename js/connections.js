/* ===================================================================
   SkillSwap - My Connections JavaScript
   Manages Active Swaps, Pending Requests & Interactive Chat Simulation
   Integrated with C++ SQLite Backend API
   =================================================================== */

let activeConnectionsData = [...MOCK_CONNECTIONS.active];
let pendingRequestsData = [...MOCK_CONNECTIONS.pending];
let sentRequestsData = [...MOCK_CONNECTIONS.sent];

document.addEventListener('DOMContentLoaded', async () => {
  initConnectionTabs();
  initChatModal();
  initScheduleModal();
  await loadSwapsFromBackend();
});

async function loadSwapsFromBackend() {
  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.getSwaps();
    if (res.ok && res.data.success && res.data.swaps) {
      if (res.data.swaps.active && res.data.swaps.active.length > 0) {
        activeConnectionsData = res.data.swaps.active;
      }
      if (res.data.swaps.pending && res.data.swaps.pending.length > 0) {
        pendingRequestsData = res.data.swaps.pending;
      }
      if (res.data.swaps.sent && res.data.swaps.sent.length > 0) {
        sentRequestsData = res.data.swaps.sent;
      }
    }
  }
  renderAllConnectionLists();
}

function initConnectionTabs() {
  const tabs = document.querySelectorAll('.conn-tab-btn');
  tabs.forEach(tab => {
    tab.addEventListener('click', () => {
      tabs.forEach(t => t.classList.remove('active'));
      tab.classList.add('active');

      const targetId = tab.getAttribute('data-target');
      document.querySelectorAll('.tab-content-panel').forEach(panel => {
        panel.style.display = 'none';
      });

      const targetPanel = document.getElementById(targetId);
      if (targetPanel) {
        targetPanel.style.display = 'block';
      }
    });
  });
}

function updateTabCounts() {
  const activeCountEl = document.getElementById('activeCount');
  const pendingCountEl = document.getElementById('pendingCount');
  const sentCountEl = document.getElementById('sentCount');

  if (activeCountEl) activeCountEl.textContent = activeConnectionsData.length;
  if (pendingCountEl) pendingCountEl.textContent = pendingRequestsData.length;
  if (sentCountEl) sentCountEl.textContent = sentRequestsData.length;
}

function renderAllConnectionLists() {
  renderActiveConnections();
  renderPendingRequests();
  renderSentRequests();
  updateTabCounts();
}

/**
 * 1. Render Active Connections
 */
function renderActiveConnections() {
  const container = document.getElementById('activeConnectionsGrid');
  if (!container) return;

  if (activeConnectionsData.length === 0) {
    container.innerHTML = `
      <div class="empty-results" style="grid-column: 1 / -1;">
        <i class="fa-solid fa-user-group"></i>
        <h3>No active connections yet</h3>
        <p style="color: var(--text-secondary); margin: 0.5rem 0 1rem 0;">
          Explore students and send a swap request to start exchanging skills!
        </p>
        <a href="explore.html" class="btn btn-primary btn-sm">Explore Students</a>
      </div>
    `;
    return;
  }

  container.innerHTML = activeConnectionsData.map(conn => `
    <div class="connection-card" id="active-conn-${conn.id}">
      <div class="conn-card-head">
        <div class="conn-student-profile">
          <div class="swap-mini-avatar" style="width: 48px; height: 48px; font-size: 1.1rem;">
            ${escapeHTML(conn.avatarInitial || 'ST')}
          </div>
          <div>
            <div style="font-weight: 700; font-size: 1.05rem;">${escapeHTML(conn.studentName)}</div>
            <div style="font-size: 0.82rem; color: var(--text-secondary);">${escapeHTML(conn.department)}</div>
          </div>
        </div>
        <span class="badge badge-status-active">
          <i class="fa-solid fa-circle-check"></i> Connected
        </span>
      </div>

      <div class="conn-exchange-box">
        <div class="conn-exchange-row">
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">You Teach</div>
            <div class="conn-exchange-skill teach"><i class="fa-solid fa-chalkboard-user"></i> ${escapeHTML(conn.userTeaches || 'Skills')}</div>
          </div>
          <div style="color: var(--primary-purple); font-size: 1.1rem; padding: 0 0.5rem;">
            <i class="fa-solid fa-repeat"></i>
          </div>
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">You Learn</div>
            <div class="conn-exchange-skill learn"><i class="fa-solid fa-lightbulb"></i> ${escapeHTML(conn.partnerTeaches || 'Skills')}</div>
          </div>
        </div>
      </div>

      <div style="font-size: 0.85rem; color: var(--text-secondary); margin-bottom: 1.25rem;">
        <div><i class="fa-solid fa-calendar-check" style="color: var(--primary-blue);"></i> Next Session: <strong>${escapeHTML(conn.nextSession || 'Flexible Schedule')}</strong></div>
        <div style="margin-top: 0.25rem;"><i class="fa-regular fa-clock"></i> Status: ${escapeHTML(conn.status || 'Active Swap')}</div>
      </div>

      <div class="conn-card-actions">
        <button class="btn btn-primary btn-sm" onclick="openChatModal('${escapeHTML(conn.studentName)}', '${escapeHTML(conn.partnerTeaches || 'Exchange')}')">
          <i class="fa-solid fa-comments"></i> Message
        </button>
        <button class="btn btn-outline btn-sm" onclick="openScheduleModal('${escapeHTML(conn.studentName)}')">
          <i class="fa-solid fa-calendar-plus"></i> Schedule
        </button>
      </div>
    </div>
  `).join('');
}

/**
 * 2. Render Pending Received Requests
 */
function renderPendingRequests() {
  const container = document.getElementById('pendingRequestsGrid');
  if (!container) return;

  if (pendingRequestsData.length === 0) {
    container.innerHTML = `
      <div class="empty-results" style="grid-column: 1 / -1;">
        <i class="fa-solid fa-inbox"></i>
        <h3>No pending requests</h3>
        <p style="color: var(--text-secondary);">You're all caught up! New requests from peers will show up here.</p>
      </div>
    `;
    return;
  }

  container.innerHTML = pendingRequestsData.map(req => `
    <div class="connection-card" id="pending-req-${req.id}">
      <div class="conn-card-head">
        <div class="conn-student-profile">
          <div class="swap-mini-avatar" style="width: 48px; height: 48px; font-size: 1.1rem;">
            ${escapeHTML(req.avatarInitial || 'ST')}
          </div>
          <div>
            <div style="font-weight: 700; font-size: 1.05rem;">${escapeHTML(req.studentName)}</div>
            <div style="font-size: 0.82rem; color: var(--text-secondary);">${escapeHTML(req.department)} • ${escapeHTML(req.date)}</div>
          </div>
        </div>
        <span class="badge badge-status-pending">
          <i class="fa-regular fa-clock"></i> Pending
        </span>
      </div>

      <div style="background: white; border: 1px solid var(--border-light); border-radius: var(--radius-md); padding: 0.85rem; margin-bottom: 1rem; font-size: 0.88rem; color: var(--text-secondary); font-style: italic;">
        "${escapeHTML(req.message || 'Would love to exchange skills!')}"
      </div>

      <div class="conn-exchange-box">
        <div class="conn-exchange-row">
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">They Offer to Teach</div>
            <div class="conn-exchange-skill teach">${escapeHTML(req.partnerSkillOffered || 'Skill')}</div>
          </div>
          <div style="color: var(--primary-purple); font-size: 1.1rem; padding: 0 0.5rem;">
            <i class="fa-solid fa-arrow-right-arrow-left"></i>
          </div>
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">They Want to Learn</div>
            <div class="conn-exchange-skill learn">${escapeHTML(req.userSkillRequested || 'Skill')}</div>
          </div>
        </div>
      </div>

      <div class="conn-card-actions">
        <button class="btn btn-success btn-sm" onclick="acceptSwapRequest(${req.id})">
          <i class="fa-solid fa-check"></i> Accept Swap
        </button>
        <button class="btn btn-danger-outline btn-sm" onclick="declineSwapRequest(${req.id})">
          <i class="fa-solid fa-xmark"></i> Decline
        </button>
      </div>
    </div>
  `).join('');
}

/**
 * 3. Render Sent Requests
 */
function renderSentRequests() {
  const container = document.getElementById('sentRequestsGrid');
  if (!container) return;

  if (sentRequestsData.length === 0) {
    container.innerHTML = `
      <div class="empty-results" style="grid-column: 1 / -1;">
        <i class="fa-regular fa-paper-plane"></i>
        <h3>No active sent invitations</h3>
        <p style="color: var(--text-secondary);">Send swap requests from the Explore page to collaborate with peers.</p>
      </div>
    `;
    return;
  }

  container.innerHTML = sentRequestsData.map(req => `
    <div class="connection-card" id="sent-req-${req.id}">
      <div class="conn-card-head">
        <div class="conn-student-profile">
          <div class="swap-mini-avatar" style="width: 48px; height: 48px; font-size: 1.1rem;">
            ${escapeHTML(req.avatarInitial || 'ST')}
          </div>
          <div>
            <div style="font-weight: 700; font-size: 1.05rem;">${escapeHTML(req.studentName)}</div>
            <div style="font-size: 0.82rem; color: var(--text-secondary);">${escapeHTML(req.department)} • ${escapeHTML(req.date)}</div>
          </div>
        </div>
        <span class="badge" style="background: #e2e8f0; color: #475569;">
          <i class="fa-solid fa-hourglass-half"></i> Waiting Response
        </span>
      </div>

      <div style="background: white; border: 1px solid var(--border-light); border-radius: var(--radius-md); padding: 0.85rem; margin-bottom: 1rem; font-size: 0.88rem; color: var(--text-secondary);">
        <strong>Your note:</strong> "${escapeHTML(req.message || 'Invitation sent')}"
      </div>

      <div class="conn-exchange-box">
        <div class="conn-exchange-row">
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">You Offered</div>
            <div class="conn-exchange-skill teach">${escapeHTML(req.skillOffered || 'Skill')}</div>
          </div>
          <div style="color: var(--primary-purple); font-size: 1.1rem; padding: 0 0.5rem;">
            <i class="fa-solid fa-arrow-right"></i>
          </div>
          <div class="conn-exchange-item">
            <div class="conn-exchange-label">Requested Skill</div>
            <div class="conn-exchange-skill learn">${escapeHTML(req.skillRequested || 'Skill')}</div>
          </div>
        </div>
      </div>

      <div class="conn-card-actions">
        <button class="btn btn-outline btn-sm" onclick="cancelSentRequest(${req.id})">
          <i class="fa-solid fa-ban"></i> Cancel Request
        </button>
      </div>
    </div>
  `).join('');
}

/**
 * Handle Accept & Decline Actions (Updates in SQLite)
 */
async function acceptSwapRequest(id) {
  const reqIndex = pendingRequestsData.findIndex(r => r.id === id);
  if (reqIndex === -1) return;

  const req = pendingRequestsData[reqIndex];

  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.respondSwap(id, 'accepted');
    if (res.ok && res.data.success) {
      showToast(
        'Swap Accepted in SQLite Database!',
        `You and ${req.studentName} are now connected. Meeting details stored in database!`,
        'success'
      );
      await loadSwapsFromBackend();
      return;
    }
  }

  // Client Fallback
  const newActiveConn = {
    id: Date.now(),
    studentName: req.studentName,
    avatarInitial: req.avatarInitial,
    department: req.department,
    userTeaches: req.userSkillRequested,
    partnerTeaches: req.partnerSkillOffered,
    status: "Newly Connected (Ready to schedule)",
    nextSession: "Not scheduled yet",
    rating: 5.0
  };

  activeConnectionsData.unshift(newActiveConn);
  pendingRequestsData.splice(reqIndex, 1);
  renderAllConnectionLists();
  showToast('Swap Request Accepted!', `You are now connected with ${req.studentName}.`, 'success');
}

async function declineSwapRequest(id) {
  const reqIndex = pendingRequestsData.findIndex(r => r.id === id);
  if (reqIndex === -1) return;

  const req = pendingRequestsData[reqIndex];

  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.respondSwap(id, 'declined');
    if (res.ok && res.data.success) {
      showToast('Declined in SQLite', `Swap request declined.`, 'info');
      await loadSwapsFromBackend();
      return;
    }
  }

  pendingRequestsData.splice(reqIndex, 1);
  renderAllConnectionLists();
  showToast('Request Declined', `Declined swap request from ${req.studentName}.`, 'info');
}

function cancelSentRequest(id) {
  const reqIndex = sentRequestsData.findIndex(r => r.id === id);
  if (reqIndex === -1) return;

  const req = sentRequestsData[reqIndex];
  sentRequestsData.splice(reqIndex, 1);
  renderAllConnectionLists();
  showToast('Request Withdrawn', `Canceled invitation to ${req.studentName}.`, 'info');
}

/**
 * Interactive Demo Chat Modal
 */
function openChatModal(partnerName, topic) {
  const nameEl = document.getElementById('chatPartnerName');
  const topicEl = document.getElementById('chatTopicBadge');
  const msgBody = document.getElementById('chatMessagesBody');

  if (nameEl) nameEl.textContent = partnerName;
  if (topicEl) topicEl.textContent = `Learning: ${topic}`;

  if (msgBody) {
    msgBody.innerHTML = `
      <div style="background: var(--bg-surface-alt); padding: 0.75rem 1rem; border-radius: var(--radius-md); margin-bottom: 0.75rem; max-width: 80%;">
        <div style="font-size: 0.75rem; font-weight: 700; color: var(--primary-purple);">${partnerName}</div>
        <div style="font-size: 0.9rem; color: var(--text-primary); margin-top: 0.2rem;">
          Hey Alex! Looking forward to our next swap session. Shall we meet at the Central Library or jump on Google Meet?
        </div>
        <div style="font-size: 0.7rem; color: var(--text-muted); text-align: right; margin-top: 0.2rem;">Yesterday 4:15 PM</div>
      </div>
      <div style="background: var(--primary-purple-subtle); border: 1px solid var(--primary-purple-border); padding: 0.75rem 1rem; border-radius: var(--radius-md); margin-bottom: 0.75rem; max-width: 80%; margin-left: auto;">
        <div style="font-size: 0.75rem; font-weight: 700; color: var(--primary-blue);">You (Alex)</div>
        <div style="font-size: 0.9rem; color: var(--text-primary); margin-top: 0.2rem;">
          Central Library 2nd floor works great! I've prepared some starter Python exercises for you.
        </div>
        <div style="font-size: 0.7rem; color: var(--text-muted); text-align: right; margin-top: 0.2rem;">Yesterday 4:22 PM</div>
      </div>
    `;
  }

  openModal('chatPreviewModal');
}

function initChatModal() {
  const form = document.getElementById('chatSendForm');
  if (!form) return;

  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const input = document.getElementById('chatInputField');
    const msg = input?.value.trim();
    if (!msg) return;

    const msgBody = document.getElementById('chatMessagesBody');
    if (msgBody) {
      const msgDiv = document.createElement('div');
      msgDiv.style.cssText = 'background: var(--primary-purple-subtle); border: 1px solid var(--primary-purple-border); padding: 0.75rem 1rem; border-radius: var(--radius-md); margin-bottom: 0.75rem; max-width: 80%; margin-left: auto;';
      msgDiv.innerHTML = `
        <div style="font-size: 0.75rem; font-weight: 700; color: var(--primary-blue);">You (Alex)</div>
        <div style="font-size: 0.9rem; color: var(--text-primary); margin-top: 0.2rem;">${escapeHTML(msg)}</div>
        <div style="font-size: 0.7rem; color: var(--text-muted); text-align: right; margin-top: 0.2rem;">Just now</div>
      `;
      msgBody.appendChild(msgDiv);
      msgBody.scrollTop = msgBody.scrollHeight;
    }

    input.value = '';
    showToast('Message Sent', 'Demo chat message delivered in preview mode.', 'info');
  });
}

/**
 * Interactive Schedule Session Modal
 */
let scheduleTargetPartner = '';

function openScheduleModal(partnerName) {
  scheduleTargetPartner = partnerName;
  const nameEl = document.getElementById('schedulePartnerName');
  if (nameEl) nameEl.textContent = partnerName;
  openModal('scheduleSessionModal');
}

function initScheduleModal() {
  const form = document.getElementById('scheduleSessionForm');
  if (!form) return;

  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const date = document.getElementById('sessionDate')?.value;
    const time = document.getElementById('sessionTime')?.value;
    const venue = document.getElementById('sessionVenue')?.value;

    closeModal('scheduleSessionModal');
    showToast(
      'Session Scheduled!',
      `Study swap session with ${scheduleTargetPartner} set for ${date} at ${time} (${venue}).`,
      'success'
    );
    form.reset();
  });
}
