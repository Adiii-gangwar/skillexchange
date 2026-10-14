/* ===================================================================
   SkillSwap - Main Global JavaScript
   Utilities, Mobile Navigation, Toast Notifications & Modals
   =================================================================== */

document.addEventListener('DOMContentLoaded', () => {
  initMobileNavigation();
  highlightActiveNavLink();
  initToastContainer();
  initModalCloseEvents();
});

/**
 * Mobile Navigation Toggle
 */
function initMobileNavigation() {
  const menuBtn = document.querySelector('.mobile-menu-btn');
  const navLinks = document.querySelector('.nav-links');

  if (menuBtn && navLinks) {
    menuBtn.addEventListener('click', () => {
      navLinks.classList.toggle('open');
      const icon = menuBtn.querySelector('i');
      if (icon) {
        if (navLinks.classList.contains('open')) {
          icon.classList.remove('fa-bars');
          icon.classList.add('fa-xmark');
        } else {
          icon.classList.remove('fa-xmark');
          icon.classList.add('fa-bars');
        }
      }
    });

    // Close when clicking outside
    document.addEventListener('click', (e) => {
      if (!menuBtn.contains(e.target) && !navLinks.contains(e.target) && navLinks.classList.contains('open')) {
        navLinks.classList.remove('open');
        const icon = menuBtn.querySelector('i');
        if (icon) {
          icon.classList.remove('fa-xmark');
          icon.classList.add('fa-bars');
        }
      }
    });
  }
}

/**
 * Highlights current active nav item according to URL pathname
 */
function highlightActiveNavLink() {
  const currentPath = window.location.pathname.split('/').pop() || 'index.html';
  const navLinks = document.querySelectorAll('.nav-link');

  navLinks.forEach(link => {
    const href = link.getAttribute('href');
    if (href === currentPath || (currentPath === '' && href === 'index.html')) {
      link.classList.add('active');
    } else {
      link.classList.remove('active');
    }
  });
}

/**
 * Toast Notification System
 */
let toastContainer = null;

function initToastContainer() {
  toastContainer = document.querySelector('.toast-container');
  if (!toastContainer) {
    toastContainer = document.createElement('div');
    toastContainer.className = 'toast-container';
    document.body.appendChild(toastContainer);
  }
}

function showToast(title, message, type = 'success') {
  if (!toastContainer) initToastContainer();

  const toast = document.createElement('div');
  toast.className = `toast toast-${type}`;

  let iconClass = 'fa-circle-check';
  if (type === 'warning') iconClass = 'fa-triangle-exclamation';
  if (type === 'error') iconClass = 'fa-circle-xmark';
  if (type === 'info') iconClass = 'fa-circle-info';

  toast.innerHTML = `
    <div class="toast-icon"><i class="fa-solid ${iconClass}"></i></div>
    <div class="toast-content">
      <div class="toast-title">${escapeHTML(title)}</div>
      <div class="toast-message">${escapeHTML(message)}</div>
    </div>
    <button class="modal-close-btn" style="font-size: 0.9rem; margin-left: 0.5rem;" onclick="this.parentElement.remove()">
      <i class="fa-solid fa-xmark"></i>
    </button>
  `;

  toastContainer.appendChild(toast);

  // Trigger animation
  requestAnimationFrame(() => {
    toast.classList.add('show');
  });

  // Auto remove after 4.5 seconds
  setTimeout(() => {
    toast.classList.remove('show');
    setTimeout(() => {
      if (toast.parentElement) toast.remove();
    }, 300);
  }, 4500);
}

/**
 * Modal helpers
 */
function openModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) {
    modal.classList.add('active');
    document.body.style.overflow = 'hidden';
  }
}

function closeModal(modalId) {
  const modal = document.getElementById(modalId);
  if (modal) {
    modal.classList.remove('active');
    document.body.style.overflow = '';
  }
}

function initModalCloseEvents() {
  document.querySelectorAll('.modal-overlay').forEach(modal => {
    modal.addEventListener('click', (e) => {
      if (e.target === modal) {
        closeModal(modal.id);
      }
    });

    const closeBtns = modal.querySelectorAll('.modal-close-btn, [data-modal-close]');
    closeBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        closeModal(modal.id);
      });
    });
  });

  // ESC key listener
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
      const activeModal = document.querySelector('.modal-overlay.active');
      if (activeModal) {
        closeModal(activeModal.id);
      }
    }
  });
}

/**
 * Safe HTML escape
 */
function escapeHTML(str) {
  if (!str) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;');
}

/**
 * Demo auto-filler for Login & Register forms
 */
function demoFillLogin() {
  const emailInput = document.getElementById('loginEmail');
  const passInput = document.getElementById('loginPassword');
  if (emailInput && passInput) {
    emailInput.value = 'alex.morgan@college.edu';
    passInput.value = 'demo1234';
    showToast('Demo Credentials Filled', 'Sample student login ready for your PBL presentation!', 'info');
  }
}

function demoFillRegister() {
  const nameInput = document.getElementById('regName');
  const emailInput = document.getElementById('regEmail');
  const deptInput = document.getElementById('regDept');
  const yearInput = document.getElementById('regYear');
  const teachInput = document.getElementById('regTeach');
  const learnInput = document.getElementById('regLearn');
  const passInput = document.getElementById('regPass');
  const confirmPassInput = document.getElementById('regConfirmPass');

  if (nameInput) nameInput.value = 'Divya Sharma';
  if (emailInput) emailInput.value = 'divya.s@college.edu';
  if (deptInput) deptInput.value = 'Computer Science';
  if (yearInput) yearInput.value = '2nd Year';
  if (teachInput) teachInput.value = 'Python & Machine Learning';
  if (learnInput) learnInput.value = 'UI/UX Design with Figma';
  if (passInput) passInput.value = 'SecurePBL#2026';
  if (confirmPassInput) confirmPassInput.value = 'SecurePBL#2026';

  showToast('Demo Student Data Filled', 'Form pre-filled for easy project demonstration!', 'info');
}
