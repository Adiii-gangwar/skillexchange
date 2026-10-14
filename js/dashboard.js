/* ===================================================================
   SkillSwap - Student Dashboard JavaScript
   Personalized Stats & Suggested Matches via C++ Backend API
   =================================================================== */

document.addEventListener('DOMContentLoaded', async () => {
  await initDashboard();
});

async function initDashboard() {
  let profile = typeof getUserProfile === 'function' ? getUserProfile() : MOCK_CURRENT_USER;

  // Try fetching current user profile from C++ Backend API
  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.getProfile();
    if (res.ok && res.data.success && res.data.profile) {
      profile = res.data.profile;
      SkillSwapAPI.setCurrentUser(profile);
    }
  }

  // Set user greetings
  const greetingEl = document.getElementById('dashUserName');
  const userRoleEl = document.getElementById('dashUserMeta');
  const avatarEl = document.getElementById('dashAvatar');
  const navAvatarEl = document.getElementById('dashNavAvatar');
  const navNameEl = document.getElementById('dashNavName');

  const fullName = profile.name || profile.full_name || 'Alex Morgan';
  const dept = profile.department || 'Computer Science & Eng.';
  const year = profile.year || profile.year_of_study || '3rd Year';
  const initial = profile.avatar_initial || profile.avatarInitial || 'AM';

  if (greetingEl) greetingEl.textContent = fullName;
  if (userRoleEl) userRoleEl.textContent = `${dept} • ${year}`;
  if (avatarEl) avatarEl.textContent = initial;
  if (navAvatarEl) navAvatarEl.textContent = initial;
  if (navNameEl) navNameEl.textContent = fullName;

  // Set stats
  const statActiveSwaps = document.getElementById('statActiveSwaps');
  const statSkillsOffered = document.getElementById('statSkillsOffered');
  const statSkillsLearning = document.getElementById('statSkillsLearning');
  const statPendingRequests = document.getElementById('statPendingRequests');

  const teachSkills = profile.skills_teach || profile.skillsTeach || [];
  const learnSkills = profile.skills_learn || profile.skillsLearn || [];

  if (statSkillsOffered) statSkillsOffered.textContent = teachSkills.length;
  if (statSkillsLearning) statSkillsLearning.textContent = learnSkills.length;

  // Fetch connection counts from backend
  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const swapRes = await SkillSwapAPI.getSwaps();
    if (swapRes.ok && swapRes.data.swaps) {
      if (statActiveSwaps) statActiveSwaps.textContent = (swapRes.data.swaps.active || []).length;
      if (statPendingRequests) statPendingRequests.textContent = (swapRes.data.swaps.pending || []).length;
    }
  } else {
    if (statActiveSwaps) statActiveSwaps.textContent = MOCK_CONNECTIONS.active.length;
    if (statPendingRequests) statPendingRequests.textContent = MOCK_CONNECTIONS.pending.length;
  }

  await renderSuggestedMatches();
  initQuickSearch();
}

/**
 * Render smart suggested matches
 */
async function renderSuggestedMatches() {
  const container = document.getElementById('suggestedMatchesContainer');
  if (!container) return;

  let suggestions = [];

  // Try fetching peers from backend
  if (typeof SkillSwapAPI !== 'undefined') {
    const res = await SkillSwapAPI.searchStudents();
    if (res.ok && res.data.students && res.data.students.length > 0) {
      // Filter out self
      const myId = (typeof SkillSwapAPI.getCurrentUser() === 'object' && SkillSwapAPI.getCurrentUser()) ? SkillSwapAPI.getCurrentUser().id : 1;
      suggestions = res.data.students.filter(s => s.id !== myId).slice(0, 3);
    }
  }

  if (suggestions.length === 0) {
    suggestions = MOCK_STUDENTS.slice(0, 3);
  }

  container.innerHTML = suggestions.map(student => {
    const teachSkills = student.teach || student.skills_teach || [];
    const learnSkills = student.learn || student.skills_learn || [];
    const avatar = student.avatarInitial || student.avatar_initial || 'ST';
    const rating = student.rating || 5.0;

    return `
      <div class="student-card" style="margin-bottom: 1.25rem;">
        <div class="student-card-header">
          <div class="student-avatar" style="width: 48px; height: 48px; font-size: 1.1rem;">
            ${escapeHTML(avatar)}
          </div>
          <div class="student-info">
            <div class="student-name" style="font-size: 1.05rem;">${escapeHTML(student.name)}</div>
            <div class="student-meta">
              <span>${escapeHTML(student.department)}</span>
              <span>• <i class="fa-solid fa-star" style="color: #f59e0b;"></i> ${rating}</span>
            </div>
          </div>
          <span class="badge" style="background: #e0f2fe; color: #0369a1; font-size: 0.75rem;">
            <i class="fa-solid fa-wand-magic-sparkles"></i> Recommended Match
          </span>
        </div>

        <div class="skills-group" style="margin-bottom: 0.75rem;">
          <div class="skills-group-title title-teach" style="font-size: 0.75rem;">
            Can Teach You:
          </div>
          <div class="skill-tags-list">
            ${teachSkills.map(t => `
              <span class="badge badge-teach">${escapeHTML(t.name)}</span>
            `).join('')}
          </div>
        </div>

        <div class="skills-group" style="margin-bottom: 1rem;">
          <div class="skills-group-title title-learn" style="font-size: 0.75rem;">
            Wants to Learn:
          </div>
          <div class="skill-tags-list">
            ${learnSkills.map(l => `
              <span class="badge badge-learn">${escapeHTML(l.name)}</span>
            `).join('')}
          </div>
        </div>

        <div class="student-card-footer" style="padding-top: 0.85rem;">
          <span style="font-size: 0.8rem; color: var(--text-muted);">
            <i class="fa-solid fa-arrows-rotate"></i> Mutual Match
          </span>
          <a href="explore.html" class="btn btn-primary btn-sm">
            <i class="fa-solid fa-handshake"></i> View & Swap
          </a>
        </div>
      </div>
    `;
  }).join('');
}

function initQuickSearch() {
  const form = document.getElementById('dashQuickSearchForm');
  if (!form) return;

  form.addEventListener('submit', (e) => {
    e.preventDefault();
    const query = document.getElementById('dashQuickSearchInput')?.value.trim();
    if (query) {
      window.location.href = `explore.html?q=${encodeURIComponent(query)}`;
    } else {
      window.location.href = 'explore.html';
    }
  });
}
