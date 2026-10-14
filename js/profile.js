/* ===================================================================
   SkillSwap - Profile & Skill Management JavaScript
   Directly communicates with C++ Backend API and SQLite Database
   =================================================================== */

let currentProfile = null;

document.addEventListener('DOMContentLoaded', () => {
  if (document.getElementById('teachSkillsContainer') || document.getElementById('profileNameDisplay')) {
    initProfilePage();
  }
});

async function initProfilePage() {
  await loadProfileData();

  initAddTeachForm();
  initAddLearnForm();
  initProfileEditForm();
  initResetDataBtn();
}

/**
 * Fetch profile from C++ Backend (SQLite), or fallback to client cache
 */
async function loadProfileData() {
  // If user is logged in, fetch from C++ Backend API
  if (typeof SkillSwapAPI !== 'undefined') {
    const res = await SkillSwapAPI.getProfile();
    if (res.ok && res.data.success && res.data.profile) {
      currentProfile = res.data.profile;
      renderProfileHeader(currentProfile);
      renderSkillsTeach(currentProfile.skills_teach || []);
      renderSkillsLearn(currentProfile.skills_learn || []);
      return;
    }
  }

  // Fallback to local user or mock
  const localUser = typeof SkillSwapAPI !== 'undefined' ? SkillSwapAPI.getCurrentUser() : null;
  currentProfile = localUser || JSON.parse(JSON.stringify(MOCK_CURRENT_USER));
  renderProfileHeader(currentProfile);
  renderSkillsTeach(currentProfile.skillsTeach || currentProfile.skills_teach || []);
  renderSkillsLearn(currentProfile.skillsLearn || currentProfile.skills_learn || []);
}

/**
 * Render Profile Sidebar & Header
 */
function renderProfileHeader(profile) {
  const nameEl = document.getElementById('profileNameDisplay');
  const metaEl = document.getElementById('profileMetaDisplay');
  const bioEl = document.getElementById('profileBioDisplay');
  const avatarEl = document.getElementById('profileAvatarDisplay');

  if (nameEl) nameEl.textContent = profile.name || profile.full_name || 'Alex Morgan';
  if (metaEl) metaEl.textContent = `${profile.department || 'Computer Science'} • ${profile.year || profile.year_of_study || '3rd Year'}`;
  if (bioEl) bioEl.textContent = profile.bio || 'Passionate about peer skill exchange!';
  if (avatarEl) avatarEl.textContent = profile.avatar_initial || profile.avatarInitial || 'AM';

  // Fill editable inputs
  const inputName = document.getElementById('editFullName');
  const inputBio = document.getElementById('editBio');
  const inputDept = document.getElementById('editDepartment');
  const inputYear = document.getElementById('editYear');

  if (inputName) inputName.value = profile.name || profile.full_name || '';
  if (inputBio) inputBio.value = profile.bio || '';
  if (inputDept) inputDept.value = profile.department || '';
  if (inputYear) inputYear.value = profile.year || profile.year_of_study || '';
}

/**
 * Render "Skills I Can Teach" Tags
 */
function renderSkillsTeach(skillsList) {
  const container = document.getElementById('teachSkillsContainer');
  const countEl = document.getElementById('teachCountBadge');

  if (countEl) countEl.textContent = `${skillsList.length} skills listed`;
  if (!container) return;

  if (skillsList.length === 0) {
    container.innerHTML = `
      <p style="color: var(--text-muted); font-size: 0.88rem; font-style: italic;">
        No skills added yet. Use the form below to list what you can teach fellow students!
      </p>
    `;
    return;
  }

  container.innerHTML = skillsList.map((skill, index) => {
    const skillId = skill.id || index;
    return `
      <div class="skill-tag-interactive teach-tag" data-id="${skillId}">
        <i class="fa-solid fa-graduation-cap"></i>
        <span><strong>${escapeHTML(skill.name)}</strong> (${escapeHTML(skill.level || 'Intermediate')})</span>
        <button type="button" class="tag-remove-btn" onclick="removeTeachSkill(${skillId}, ${index})" title="Remove skill from SQLite database">
          <i class="fa-solid fa-xmark"></i>
        </button>
      </div>
    `;
  }).join('');
}

/**
 * Render "Skills I Want to Learn" Tags
 */
function renderSkillsLearn(skillsList) {
  const container = document.getElementById('learnSkillsContainer');
  const countEl = document.getElementById('learnCountBadge');

  if (countEl) countEl.textContent = `${skillsList.length} skills requested`;
  if (!container) return;

  if (skillsList.length === 0) {
    container.innerHTML = `
      <p style="color: var(--text-muted); font-size: 0.88rem; font-style: italic;">
        No learning goals added yet. Add skills you want to learn from campus peers!
      </p>
    `;
    return;
  }

  container.innerHTML = skillsList.map((skill, index) => {
    const skillId = skill.id || index;
    return `
      <div class="skill-tag-interactive learn-tag" data-id="${skillId}">
        <i class="fa-solid fa-lightbulb"></i>
        <span><strong>${escapeHTML(skill.name)}</strong> ${skill.goal ? `• <small>${escapeHTML(skill.goal)}</small>` : ''}</span>
        <button type="button" class="tag-remove-btn" onclick="removeLearnSkill(${skillId}, ${index})" title="Remove goal from SQLite database">
          <i class="fa-solid fa-xmark"></i>
        </button>
      </div>
    `;
  }).join('');
}

/**
 * Handle Add Skill to Teach Form (Saves to SQLite)
 */
function initAddTeachForm() {
  const form = document.getElementById('addTeachSkillForm');
  if (!form) return;

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const nameInput = document.getElementById('teachSkillName');
    const levelInput = document.getElementById('teachSkillLevel');

    const skillName = nameInput?.value.trim();
    const skillLevel = levelInput?.value || 'Intermediate';

    if (!skillName) {
      showToast('Validation Error', 'Please enter a valid skill name.', 'warning');
      return;
    }

    if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
      const res = await SkillSwapAPI.addTeachSkill(skillName, skillLevel);
      if (res.ok && res.data.success) {
        showToast('Saved to SQLite Database', `Skill "${skillName}" saved to backend!`, 'success');
        nameInput.value = '';
        await loadProfileData();
        return;
      }
    }

    // Fallback client addition
    if (!currentProfile.skills_teach) currentProfile.skills_teach = [];
    currentProfile.skills_teach.push({ name: skillName, level: skillLevel, id: Date.now() });
    renderSkillsTeach(currentProfile.skills_teach);
    nameInput.value = '';
    showToast('Teaching Skill Added!', `Added "${skillName}" (${skillLevel}).`, 'success');
  });
}

/**
 * Handle Add Skill to Learn Form (Saves to SQLite)
 */
function initAddLearnForm() {
  const form = document.getElementById('addLearnSkillForm');
  if (!form) return;

  form.addEventListener('submit', async (e) => {
    e.preventDefault();
    const nameInput = document.getElementById('learnSkillName');
    const goalInput = document.getElementById('learnSkillGoal');

    const skillName = nameInput?.value.trim();
    const skillGoal = goalInput?.value.trim() || 'General Learning';

    if (!skillName) {
      showToast('Validation Error', 'Please enter a valid skill you want to learn.', 'warning');
      return;
    }

    if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
      const res = await SkillSwapAPI.addLearnSkill(skillName, skillGoal);
      if (res.ok && res.data.success) {
        showToast('Saved to SQLite Database', `Learning goal "${skillName}" stored!`, 'success');
        nameInput.value = '';
        if (goalInput) goalInput.value = '';
        await loadProfileData();
        return;
      }
    }

    // Fallback
    if (!currentProfile.skills_learn) currentProfile.skills_learn = [];
    currentProfile.skills_learn.push({ name: skillName, goal: skillGoal, id: Date.now() });
    renderSkillsLearn(currentProfile.skills_learn);
    nameInput.value = '';
    if (goalInput) goalInput.value = '';
    showToast('Learning Goal Added!', `Added "${skillName}" to your wishlist.`, 'success');
  });
}

/**
 * Remove Teaching Skill (Deletes from SQLite)
 */
async function removeTeachSkill(skillId, index) {
  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.deleteSkill(skillId);
    if (res.ok && res.data.success) {
      showToast('Deleted from SQLite', 'Skill removed from persistent database.', 'info');
      await loadProfileData();
      return;
    }
  }

  // Fallback
  if (currentProfile.skills_teach) {
    currentProfile.skills_teach.splice(index, 1);
    renderSkillsTeach(currentProfile.skills_teach);
  }
  showToast('Skill Removed', 'Removed skill from list.', 'info');
}

/**
 * Remove Learning Skill (Deletes from SQLite)
 */
async function removeLearnSkill(skillId, index) {
  if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
    const res = await SkillSwapAPI.deleteSkill(skillId);
    if (res.ok && res.data.success) {
      showToast('Deleted from SQLite', 'Goal removed from database.', 'info');
      await loadProfileData();
      return;
    }
  }

  // Fallback
  if (currentProfile.skills_learn) {
    currentProfile.skills_learn.splice(index, 1);
    renderSkillsLearn(currentProfile.skills_learn);
  }
  showToast('Goal Removed', 'Removed goal from list.', 'info');
}

/**
 * Edit Profile Form (Updates in SQLite)
 */
function initProfileEditForm() {
  const form = document.getElementById('editProfileDetailsForm');
  if (!form) return;

  form.addEventListener('submit', async (e) => {
    e.preventDefault();

    const name = document.getElementById('editFullName')?.value.trim();
    const bio = document.getElementById('editBio')?.value.trim();
    const dept = document.getElementById('editDepartment')?.value.trim();
    const year = document.getElementById('editYear')?.value.trim();
    const mode = document.getElementById('editMode')?.value || 'Hybrid';

    if (typeof SkillSwapAPI !== 'undefined' && SkillSwapAPI.getToken()) {
      const res = await SkillSwapAPI.updateProfile({
        name: name,
        department: dept,
        year: year,
        bio: bio,
        study_mode: mode
      });

      if (res.ok && res.data.success) {
        showToast('Profile Saved to SQLite', 'Your academic details were updated in the database!', 'success');
        await loadProfileData();
        return;
      }
    }

    // Fallback
    currentProfile.name = name;
    currentProfile.bio = bio;
    currentProfile.department = dept;
    currentProfile.year = year;
    renderProfileHeader(currentProfile);
    showToast('Profile Updated', 'Profile details updated locally.', 'success');
  });
}

/**
 * Reset to defaults button for demo
 */
function initResetDataBtn() {
  const resetBtn = document.getElementById('resetProfileDemoBtn');
  if (!resetBtn) return;

  resetBtn.addEventListener('click', async () => {
    if (confirm('Reset your demo profile to Alex Morgan defaults?')) {
      if (typeof SkillSwapAPI !== 'undefined') {
        // Log in as Alex Morgan
        const res = await SkillSwapAPI.login('alex.morgan@college.edu', 'demo1234');
        if (res.ok && res.data.token) {
          SkillSwapAPI.setToken(res.data.token);
          SkillSwapAPI.setCurrentUser(res.data.user);
          await loadProfileData();
          showToast('Reset to Demo Student', 'Loaded Alex Morgan profile from SQLite database.', 'info');
          return;
        }
      }
      localStorage.clear();
      location.reload();
    }
  });
}
