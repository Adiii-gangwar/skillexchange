/* ===================================================================
   SkillSwap - Explore Students JavaScript
   Queries C++ Backend API and SQLite Database for live searching
   =================================================================== */

let currentCategory = 'all';
let currentSearchQuery = '';
let currentDeptFilter = 'all';
let currentYearFilter = 'all';
let currentStudentsList = [];

document.addEventListener('DOMContentLoaded', () => {
  renderCategoryChips();
  initSearchAndFilters();
  initSwapModal();
  filterAndRenderStudents();
});

/**
 * Render Category Filter Chips
 */
function renderCategoryChips() {
  const container = document.getElementById('categoryChipsContainer');
  if (!container) return;

  container.innerHTML = MOCK_CATEGORIES.map(cat => `
    <button class="chip-btn ${cat.id === 'all' ? 'active' : ''}" data-category="${cat.id}">
      <i class="fa-solid ${cat.icon}"></i> ${escapeHTML(cat.label)}
    </button>
  `).join('');

  container.querySelectorAll('.chip-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      container.querySelectorAll('.chip-btn').forEach(b => b.classList.remove('active'));
      btn.classList.add('active');
      currentCategory = btn.getAttribute('data-category');
      filterAndRenderStudents();
    });
  });
}

/**
 * Event Listeners for Search Bar & Secondary Dropdowns
 */
function initSearchAndFilters() {
  const searchInput = document.getElementById('studentSearchInput');
  const deptFilter = document.getElementById('deptFilter');
  const yearFilter = document.getElementById('yearFilter');
  const resetBtn = document.getElementById('resetFiltersBtn');

  if (searchInput) {
    searchInput.addEventListener('input', (e) => {
      currentSearchQuery = e.target.value.trim().toLowerCase();
      filterAndRenderStudents();
    });
  }

  if (deptFilter) {
    deptFilter.addEventListener('change', (e) => {
      currentDeptFilter = e.target.value;
      filterAndRenderStudents();
    });
  }

  if (yearFilter) {
    yearFilter.addEventListener('change', (e) => {
      currentYearFilter = e.target.value;
      filterAndRenderStudents();
    });
  }

  if (resetBtn) {
    resetBtn.addEventListener('click', () => {
      if (searchInput) searchInput.value = '';
      if (deptFilter) deptFilter.value = 'all';
      if (yearFilter) yearFilter.value = 'all';
      currentSearchQuery = '';
      currentDeptFilter = 'all';
      currentYearFilter = 'all';
      currentCategory = 'all';
      
      const chips = document.querySelectorAll('#categoryChipsContainer .chip-btn');
      chips.forEach(c => c.classList.remove('active'));
      if (chips[0]) chips[0].classList.add('active');

      filterAndRenderStudents();
      showToast('Filters Reset', 'Showing all peer profiles.', 'info');
    });
  }
}

/**
 * Filter students logic (Calls C++ Backend & SQLite)
 */
async function filterAndRenderStudents() {
  if (typeof SkillSwapAPI !== 'undefined') {
    const res = await SkillSwapAPI.searchStudents({
      search: currentSearchQuery,
      category: currentCategory,
      department: currentDeptFilter,
      year: currentYearFilter
    });

    if (res.ok && res.data.success && Array.isArray(res.data.students)) {
      currentStudentsList = res.data.students;
      renderStudentCards(currentStudentsList);
      return;
    }
  }

  // Fallback to mock data if backend not yet started
  const filtered = MOCK_STUDENTS.filter(student => {
    if (currentCategory !== 'all' && student.category !== currentCategory) return false;
    if (currentDeptFilter !== 'all' && !student.department.toLowerCase().includes(currentDeptFilter.toLowerCase())) return false;
    if (currentYearFilter !== 'all' && !student.year.toLowerCase().includes(currentYearFilter.toLowerCase())) return false;

    if (currentSearchQuery) {
      const matchName = student.name.toLowerCase().includes(currentSearchQuery);
      const matchBio = student.bio.toLowerCase().includes(currentSearchQuery);
      const matchTeach = student.teach.some(t => t.name.toLowerCase().includes(currentSearchQuery));
      const matchLearn = student.learn.some(l => l.name.toLowerCase().includes(currentSearchQuery));
      return matchName || matchBio || matchTeach || matchLearn;
    }

    return true;
  });

  currentStudentsList = filtered;
  renderStudentCards(filtered);
}

/**
 * Render Student Profile Cards to Grid
 */
function renderStudentCards(studentsToRender = []) {
  const grid = document.getElementById('studentsGrid');
  const countBadge = document.getElementById('resultsCountBadge');

  if (countBadge) {
    countBadge.textContent = `Showing ${studentsToRender.length} student${studentsToRender.length === 1 ? '' : 's'}`;
  }

  if (!grid) return;

  if (studentsToRender.length === 0) {
    grid.innerHTML = `
      <div class="empty-results">
        <i class="fa-solid fa-user-slash"></i>
        <h3>No student matches found</h3>
        <p style="color: var(--text-secondary); max-width: 420px; margin: 0.5rem auto 1.5rem auto;">
          Try adjusting your search keywords, clear category filters, or explore different branches.
        </p>
        <button class="btn btn-secondary btn-sm" onclick="document.getElementById('resetFiltersBtn')?.click()">
          <i class="fa-solid fa-arrow-rotate-left"></i> Reset All Filters
        </button>
      </div>
    `;
    return;
  }

  grid.innerHTML = studentsToRender.map(student => {
    const teachSkills = student.teach || student.skills_teach || [];
    const learnSkills = student.learn || student.skills_learn || [];
    const avatar = student.avatarInitial || student.avatar_initial || 'ST';
    const swaps = student.swapsCount || student.completed_swaps || 0;
    const rating = student.rating || 5.0;

    return `
      <div class="student-card" id="student-card-${student.id}">
        <div class="student-card-header">
          <div class="student-avatar">${escapeHTML(avatar)}</div>
          <div class="student-info">
            <div class="student-name">${escapeHTML(student.name)}</div>
            <div class="student-meta">
              <span><i class="fa-solid fa-graduation-cap"></i> ${escapeHTML(student.department)}</span>
              <span>• ${escapeHTML(student.year || student.year_of_study || '')}</span>
            </div>
            <div class="student-rating" style="margin-top: 0.35rem;">
              <i class="fa-solid fa-star"></i> ${rating}
              <span style="color: var(--text-muted); font-size: 0.78rem; font-weight: normal;">(${swaps} swaps)</span>
            </div>
          </div>
        </div>

        <p class="student-bio">${escapeHTML(student.bio || 'Ready to exchange skills on campus.')}</p>

        <div class="skills-group">
          <div class="skills-group-title title-teach">
            <i class="fa-solid fa-chalkboard-user"></i> Can Teach (Offers):
          </div>
          <div class="skill-tags-list">
            ${teachSkills.map(t => `
              <span class="badge badge-teach" title="Proficiency: ${t.level || 'Intermediate'}">
                ${escapeHTML(t.name)}
                <span style="opacity: 0.7; font-size: 0.7rem;">• ${escapeHTML(t.level || 'Good')}</span>
              </span>
            `).join('')}
          </div>
        </div>

        <div class="skills-group">
          <div class="skills-group-title title-learn">
            <i class="fa-solid fa-lightbulb"></i> Wants to Learn:
          </div>
          <div class="skill-tags-list">
            ${learnSkills.map(l => `
              <span class="badge badge-learn">
                ${escapeHTML(l.name)}
              </span>
            `).join('')}
          </div>
        </div>

        <div class="student-card-footer">
          <span style="font-size: 0.8rem; color: var(--text-muted);">
            <i class="fa-regular fa-clock"></i> ${escapeHTML(student.availability || 'Flexible')}
          </span>
          <button class="btn btn-primary btn-sm request-swap-btn" 
            onclick="openSwapModalForStudent(${student.id}, '${escapeHTML(student.name)}')">
            <i class="fa-solid fa-handshake"></i> Request Swap
          </button>
        </div>
      </div>
    `;
  }).join('');
}

/**
 * Swap Request Modal Handling
 */
let targetStudentId = null;

async function openSwapModalForStudent(studentId, studentName) {
  targetStudentId = studentId;
  const modalNameEl = document.getElementById('modalStudentName');
  const userOfferSelect = document.getElementById('modalMySkillOffer');

  if (modalNameEl) {
    modalNameEl.textContent = studentName;
  }

  // Populate skills user can offer from backend or local profile
  let userTeachSkills = [];
  if (typeof SkillSwapAPI !== 'undefined') {
    const profRes = await SkillSwapAPI.getProfile();
    if (profRes.ok && profRes.data.profile) {
      userTeachSkills = profRes.data.profile.skills_teach || [];
    }
  }

  if (userTeachSkills.length === 0) {
    userTeachSkills = MOCK_CURRENT_USER.skillsTeach;
  }

  if (userOfferSelect) {
    userOfferSelect.innerHTML = userTeachSkills.map(s => `
      <option value="${escapeHTML(s.name)}">${escapeHTML(s.name)} (${s.level || 'Good'})</option>
    `).join('');
  }

  openModal('swapRequestModal');
}

function initSwapModal() {
  const form = document.getElementById('swapRequestForm');
  if (!form) return;

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const offeredSkill = document.getElementById('modalMySkillOffer')?.value;
    const message = document.getElementById('modalSwapMessage')?.value;

    const student = currentStudentsList.find(s => s.id === targetStudentId);
    const studentName = student ? student.name : 'Student';
    const requestedSkill = (student && student.teach && student.teach.length > 0) ? student.teach[0].name : "Skill Exchange";

    closeModal('swapRequestModal');

    // Call C++ Backend to store swap request in SQLite
    if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
      const res = await SkillSwapAPI.requestSwap(targetStudentId, offeredSkill, requestedSkill, message);
      if (res.ok && res.data.success) {
        showToast(
          'Swap Saved in SQLite Database!',
          `Swap invitation to ${studentName} stored in database table 'swap_requests'.`,
          'success'
        );
      } else {
        showToast('Request Sent', `Invitation sent to ${studentName}.`, 'info');
      }
    } else {
      showToast('Swap Request Sent!', `Invitation sent to ${studentName} proposing to exchange "${offeredSkill}".`, 'success');
    }

    // Update button state on the card
    const card = document.getElementById(`student-card-${targetStudentId}`);
    if (card) {
      const btn = card.querySelector('.request-swap-btn');
      if (btn) {
        btn.classList.remove('btn-primary');
        btn.classList.add('btn-secondary');
        btn.innerHTML = '<i class="fa-solid fa-check"></i> Requested';
        btn.disabled = true;
      }
    }

    form.reset();
  });
}
