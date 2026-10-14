import urllib.request
import json
import sys

BASE_URL = "http://127.0.0.1:8080"

def post(endpoint, data, token=None):
    headers = {'Content-Type': 'application/json'}
    if token:
        headers['Authorization'] = f'Bearer {token}'
    req = urllib.request.Request(f"{BASE_URL}{endpoint}", data=json.dumps(data).encode('utf-8'), headers=headers, method='POST')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def get(endpoint, token=None):
    headers = {}
    if token:
        headers['Authorization'] = f'Bearer {token}'
    req = urllib.request.Request(f"{BASE_URL}{endpoint}", headers=headers, method='GET')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def put(endpoint, data, token=None):
    headers = {'Content-Type': 'application/json'}
    if token:
        headers['Authorization'] = f'Bearer {token}'
    req = urllib.request.Request(f"{BASE_URL}{endpoint}", data=json.dumps(data).encode('utf-8'), headers=headers, method='PUT')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def delete(endpoint, token=None):
    headers = {}
    if token:
        headers['Authorization'] = f'Bearer {token}'
    req = urllib.request.Request(f"{BASE_URL}{endpoint}", headers=headers, method='DELETE')
    with urllib.request.urlopen(req) as resp:
        return json.loads(resp.read().decode('utf-8'))

def main():
    print("=== Testing SkillSwap C++ Backend API ===")
    
    # 1. Health
    h = get("/api/health")
    print(f"[OK] Health Check: {h.get('status')} - {h.get('backend')}")

    # 2. Register
    reg_data = {
        "email": "test.student@college.edu",
        "password": "Password123!",
        "name": "Test Student",
        "department": "Computer Science",
        "year": "2nd Year",
        "teach_skill": "Flutter Dev",
        "learn_skill": "Machine Learning"
    }
    try:
        reg = post("/api/auth/register", reg_data)
        print(f"[OK] User Registration: Success, ID = {reg['user']['id']}")
        token = reg['token']
    except Exception as e:
        print(f"[!] User already registered, logging in...")
        login_res = post("/api/auth/login", {"email": "test.student@college.edu", "password": "Password123!"})
        token = login_res['token']
        print(f"[OK] User Login: Success, Token = {token[:8]}...")

    # 3. Get Profile
    prof = get("/api/profile", token=token)
    print(f"[OK] Get Profile: Name = {prof['profile']['name']}, Teaches = {len(prof['profile']['skills_teach'])}")

    # 4. Update Profile
    upd = put("/api/profile", {
        "name": "Test Student Updated",
        "department": "Information Technology",
        "year": "3rd Year",
        "bio": "Updated bio via C++ REST API.",
        "study_mode": "Online Only"
    }, token=token)
    print(f"[OK] Update Profile: New Name = {upd['profile']['name']}, Year = {upd['profile']['year']}")

    # 5. Add Teachable Skill
    s1 = post("/api/skills/teach", {"name": "Docker & DevOps", "level": "Advanced"}, token=token)
    skill_id = s1['skill']['id']
    print(f"[OK] Add Teachable Skill: '{s1['skill']['name']}' (ID: {skill_id})")

    # 6. Add Learnable Skill
    s2 = post("/api/skills/learn", {"name": "Kubernetes", "goal": "Semester Project"}, token=token)
    print(f"[OK] Add Learnable Skill: '{s2['skill']['name']}' (ID: {s2['skill']['id']})")

    # 7. Delete Skill
    del_res = delete(f"/api/skills/{skill_id}", token=token)
    print(f"[OK] Delete Skill: Success = {del_res.get('success')}")

    # 8. Search Students
    students_res = get("/api/students?search=python")
    print(f"[OK] Search Students ('python'): Found {students_res['count']} students")
    for s in students_res['students'][:3]:
        print(f"    - {s['name']} ({s['department']}) Rating: {s['rating']}")

    print("\n>>> ALL BACKEND API ENDPOINTS VERIFIED SUCCESSFULLY! <<<")

if __name__ == '__main__':
    main()
