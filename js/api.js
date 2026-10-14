/* ===================================================================
   SkillSwap - Frontend HTTP API Client
   Communicates with the C++ Backend Server (http://127.0.0.1:8080/api)
   Persists all state into SQLite database (skillswap.db)
   =================================================================== */

const API_BASE = 'http://127.0.0.1:8080/api';

const SkillSwapAPI = {
  // Token management
  getToken() {
    return localStorage.getItem('skillswap_token') || '';
  },

  setToken(token) {
    if (token) {
      localStorage.setItem('skillswap_token', token);
    }
  },

  removeToken() {
    localStorage.removeItem('skillswap_token');
    localStorage.removeItem('skillswap_user');
  },

  getCurrentUser() {
    const raw = localStorage.getItem('skillswap_user');
    if (raw) {
      try { return JSON.parse(raw); } catch (e) {}
    }
    return null;
  },

  setCurrentUser(user) {
    if (user) {
      localStorage.setItem('skillswap_user', JSON.stringify(user));
    }
  },

  // Base HTTP Request Wrapper
  async request(endpoint, options = {}) {
    const url = `${API_BASE}${endpoint}`;
    const token = this.getToken();

    const headers = {
      'Content-Type': 'application/json',
      ...(options.headers || {})
    };

    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }

    try {
      const response = await fetch(url, { ...options, headers });
      const data = await response.json();
      return { ok: response.ok, status: response.status, data };
    } catch (err) {
      console.warn(`[SkillSwap API Error] Failed to connect to C++ backend at ${url}:`, err);
      return { ok: false, status: 0, data: { success: false, error: 'Could not connect to C++ backend server. Make sure server.exe is running.' } };
    }
  },

  // 1. Health Check
  async checkHealth() {
    const res = await this.request('/health', { method: 'GET' });
    return res.ok && res.data.status === 'online';
  },

  // 2. User Registration
  async register(studentData) {
    return await this.request('/auth/register', {
      method: 'POST',
      body: JSON.stringify(studentData)
    });
  },

  // 3. User Login
  async login(email, password) {
    return await this.request('/auth/login', {
      method: 'POST',
      body: JSON.stringify({ email, password })
    });
  },

  // 4. Current User Session
  async getMe() {
    return await this.request('/auth/me', { method: 'GET' });
  },

  // 5. Logout
  async logout() {
    const res = await this.request('/auth/logout', { method: 'POST' });
    this.removeToken();
    return res;
  },

  // 6. Get Profile
  async getProfile(userId = null) {
    const endpoint = userId ? `/profile?user_id=${userId}` : '/profile';
    return await this.request(endpoint, { method: 'GET' });
  },

  // 7. Update Profile
  async updateProfile(profileData) {
    return await this.request('/profile', {
      method: 'PUT',
      body: JSON.stringify(profileData)
    });
  },

  // 8. Add Teachable Skill
  async addTeachSkill(name, level = 'Intermediate') {
    return await this.request('/skills/teach', {
      method: 'POST',
      body: JSON.stringify({ name, level })
    });
  },

  // 9. Add Learnable Skill
  async addLearnSkill(name, goal = 'Personal Project') {
    return await this.request('/skills/learn', {
      method: 'POST',
      body: JSON.stringify({ name, goal })
    });
  },

  // 10. Delete Skill
  async deleteSkill(skillId) {
    return await this.request(`/skills/${skillId}`, {
      method: 'DELETE'
    });
  },

  // 11. Search Students
  async searchStudents({ search = '', category = 'all', department = 'all', year = 'all' } = {}) {
    const params = new URLSearchParams();
    if (search) params.append('search', search);
    if (category && category !== 'all') params.append('category', category);
    if (department && department !== 'all') params.append('department', department);
    if (year && year !== 'all') params.append('year', year);

    const queryStr = params.toString() ? `?${params.toString()}` : '';
    return await this.request(`/students${queryStr}`, { method: 'GET' });
  },

  // 12. Connections & Swaps
  async getSwaps() {
    return await this.request('/swaps', { method: 'GET' });
  },

  // 13. Create Swap Request
  async requestSwap(receiverId, offeredSkill, requestedSkill, message) {
    return await this.request('/swaps/request', {
      method: 'POST',
      body: JSON.stringify({
        receiver_id: receiverId,
        offered_skill: offeredSkill,
        requested_skill: requestedSkill,
        message: message
      })
    });
  },

  // 14. Respond to Swap Request
  async respondSwap(swapId, action) {
    return await this.request(`/swaps/${swapId}/respond`, {
      method: 'POST',
      body: JSON.stringify({ action })
    });
  }
};
