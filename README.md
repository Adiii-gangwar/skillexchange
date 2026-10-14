# SkillSwap – College PBL Full-Stack Platform
### *Peer-to-Peer Campus Skill Exchange Platform*
**Frontend:** HTML5, CSS3, Modern JavaScript  
**Backend:** C++ (Winsock2 Multi-threaded HTTP Server)  
**Database:** SQLite 3 (Persistent relational storage in `skillswap.db`)

---

## 📌 Architecture Overview

SkillSwap is a full-stack web application designed for a college **Problem-Based Learning (PBL)** project. It allows university students to trade academic and technical skills without money.

The platform architecture connects a responsive frontend to a custom, high-performance, beginner-friendly **C++ HTTP REST API backend** backed by **SQLite 3**:

```text
┌────────────────────────────────────────────────────────┐
│             Frontend (Browser / Web View)              │
│   HTML5 • CSS3 (Blue/Purple Theme) • Vanilla JS ES6    │
└───────────────────────────┬────────────────────────────┘
                            │ Fetch HTTP / JSON (CORS)
                            ▼
┌────────────────────────────────────────────────────────┐
│          SkillSwap C++ HTTP Server (Port 8080)          │
│   • Multi-threaded Winsock2 Listener                   │
│   • JSON Body & Header Parser                          │
│   • REST Router (GET, POST, PUT, DELETE, OPTIONS)      │
│   • Static Asset Server (serves HTML, CSS, JS)         │
│   • SHA-256 Salted Password Hasher (100 rounds)        │
│   • Bearer Session Token Authentication                │
└───────────────────────────┬────────────────────────────┘
                            │ Parameterized SQL Queries
                            ▼
┌────────────────────────────────────────────────────────┐
│              SQLite 3 Database (skillswap.db)          │
│   Tables: users • profiles • skills • sessions • swaps │
└────────────────────────────────────────────────────────┘
```

---

## 📁 Directory Structure & Created Files

```text
pbl/
├── backend/
│   ├── include/
│   │   ├── sha256.hpp          # Secure SHA-256 hashing with cryptographic salt generation
│   │   ├── database.hpp        # SQLite 3 wrapper with parameterized queries & schema setup
│   │   └── http_server.hpp     # Multi-threaded Winsock2 HTTP server with routing & CORS
│   ├── sqlite/
│   │   ├── sqlite3.c           # Official SQLite 3 C amalgamation source
│   │   ├── sqlite3.h           # SQLite 3 C API header
│   │   ├── sqlite3.o           # Precompiled SQLite 3 object file for fast linking
│   │   └── mingw_compat.h      # Windows / MinGW 32 compatibility header
│   ├── json.hpp                # nlohmann::json single-header modern C++ JSON library
│   └── main.cpp                # Backend entry point registering all HTTP REST API endpoints
├── css/
│   ├── style.css               # Core styling (Blue/Purple theme, navbar, footer, modals, badges)
│   └── pages.css               # Page-specific layouts (Dashboard, Explore grid, Profile cards)
├── js/
│   ├── api.js                  # Frontend HTTP API client communicating with C++ backend
│   ├── main.js                 # Global navigation, modal controller, and toast notifications
│   ├── profile.js              # Profile management & dynamic add/delete skills to SQLite
│   ├── explore.js              # Real-time search engine querying students from SQLite
│   ├── dashboard.js            # Dashboard stats, session timeline, and peer recommendations
│   ├── connections.js          # Tab controller, accept/decline swaps in SQLite & chat simulation
│   └── mock-data.js            # Standalone fallback dataset
├── .vscode/
│   └── tasks.json              # VS Code build task configuration (Ctrl+Shift+B)
├── build.bat                   # One-click Windows compilation script
├── run_server.bat              # One-click Windows server launch script
├── test_backend.py             # Automated API test suite verifying all C++ endpoints
├── skillswap.db                # Persistent SQLite database file
├── index.html                  # 1. Home landing page with hero & stats
├── register.html               # 2. Registration page with real backend account creation
├── login.html                  # 3. Login page with secure authentication & password reveal
├── dashboard.html              # 4. Student dashboard with live SQLite metrics & sessions
├── profile.html                # 5. My profile page with interactive Add/Delete skill forms
├── explore.html                # 6. Explore student directory with live database search
├── connections.html            # 7. My connections page with active swaps and accept/decline
└── README.md                   # Documentation and presentation guide
```

---

## 🛠️ How to Compile & Run the C++ Backend

### Prerequisites
* Windows 7, 8, 10, or 11.
* A standard C++ compiler: **MinGW GCC (`g++`)** or **MSVC (`cl.exe`)**.
  *(Verify by opening Command Prompt / PowerShell and running `g++ --version`).*

---

### Option 1: One-Click Batch Script (Easiest)
1. Double-click **`build.bat`** in the project folder to compile the server.
2. Double-click **`run_server.bat`** (or run `.\server.exe`) to start the server.

---

### Option 2: Command Line (Windows PowerShell / CMD)
To compile the C++ backend using MinGW GCC, open PowerShell in the project directory and run:

```powershell
g++ -std=c++14 backend\main.cpp backend\sqlite\sqlite3.o -o server.exe -lws2_32
```

Then start the server:
```powershell
.\server.exe
```

The server will output:
```text
============================================================
  SkillSwap C++ HTTP Server is RUNNING!                    
  Listening at: http://127.0.0.1:8080/
  API Base URL: http://127.0.0.1:8080/api
============================================================
```

> **Note on Compilation Flags:**
> * `-std=c++14`: Enables standard C++14 support.
> * `backend\sqlite\sqlite3.o`: Links the compiled SQLite 3 engine.
> * `-lws2_32`: Links the Windows Sockets 2 library (`ws2_32.lib`) for native HTTP networking.

---

### Option 3: Visual Studio Code Integration
1. Open the project folder in **VS Code**.
2. Press **`Ctrl + Shift + B`** (or go to **Terminal &rarr; Run Build Task**).
3. Select **"Build SkillSwap C++ Backend"**.
4. To run, open the integrated terminal and type `.\server.exe`.

---

## 🗄️ Database Schema & Persistence

All records are persistently stored in the file **`skillswap.db`** in the project root. The database survives application and computer restarts.

```sql
-- 1. Users table (Secure authentication)
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    email TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    salt TEXT NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. Profiles table (Student details)
CREATE TABLE profiles (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER UNIQUE NOT NULL,
    full_name TEXT NOT NULL,
    department TEXT NOT NULL,
    year_of_study TEXT NOT NULL,
    bio TEXT,
    study_mode TEXT DEFAULT 'Hybrid (Library & Online)',
    rating REAL DEFAULT 5.0,
    completed_swaps INTEGER DEFAULT 0,
    avatar_initial TEXT,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 3. Skills table (Teachable and Learnable skills)
CREATE TABLE skills (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    type TEXT NOT NULL, -- 'teach' or 'learn'
    name TEXT NOT NULL,
    level_or_goal TEXT, -- e.g. 'Advanced', 'Intermediate', or goal description
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 4. Sessions table (Active login tokens)
CREATE TABLE sessions (
    token TEXT PRIMARY KEY,
    user_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 5. Swap Requests table (Peer exchange proposals)
CREATE TABLE swap_requests (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    sender_id INTEGER NOT NULL,
    receiver_id INTEGER NOT NULL,
    offered_skill TEXT,
    requested_skill TEXT,
    message TEXT,
    status TEXT DEFAULT 'pending', -- 'pending', 'accepted', 'declined'
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(sender_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY(receiver_id) REFERENCES users(id) ON DELETE CASCADE
);
```

---

## 🔒 Security & Implementation Details

1. **Secure Salted Password Hashing**:
   * For every new student registered, a cryptographically random 16-byte hexadecimal salt is generated using `std::random_device`.
   * Passwords are never stored in plaintext. They are hashed using **SHA-256** with the salt and 100 stretching rounds in `backend/include/sha256.hpp`.
2. **Parameterized SQL Queries**:
   * All database queries use SQLite prepared statements (`sqlite3_prepare_v2`, `sqlite3_bind_text`, `sqlite3_bind_int`).
   * Prevents SQL Injection attacks completely.
3. **Session Authentication**:
   * Upon successful login, the server generates a 24-byte bearer token saved in the `sessions` table.
   * Protected endpoints (`/api/profile`, `/api/skills/*`, `/api/swaps/*`) validate the token via the `Authorization: Bearer <token>` header.
4. **CORS Support**:
   * The C++ HTTP server automatically handles browser preflight `OPTIONS` requests and emits `Access-Control-Allow-Origin: *` headers on all responses.

---

## 📡 REST API Reference

| Method | Endpoint | Description | Auth Required |
|---|---|---|---|
| `GET` | `/api/health` | Backend status and engine version | No |
| `POST` | `/api/auth/register` | Register new student, create profile and skills in SQLite | No |
| `POST` | `/api/auth/login` | Verify salted password hash and return session token | No |
| `GET` | `/api/auth/me` | Fetch authenticated student profile and skills | Yes |
| `POST` | `/api/auth/logout` | Invalidate current session token | Yes |
| `GET` | `/api/profile` | Get student profile details and listed skills | Yes |
| `PUT` | `/api/profile` | Update profile information (name, department, year, bio) | Yes |
| `POST` | `/api/skills/teach` | Add a skill the student can teach (name, level) | Yes |
| `POST` | `/api/skills/learn` | Add a skill the student wants to learn (name, goal) | Yes |
| `DELETE` | `/api/skills/:id` | Delete a teachable or learnable skill from SQLite | Yes |
| `GET` | `/api/students` | Search directory by name, skill keyword, category, branch | No |
| `GET` | `/api/swaps` | Fetch user's active connections and swap requests | Yes |
| `POST` | `/api/swaps/request` | Propose a 1-on-1 swap to another student | Yes |
| `POST` | `/api/swaps/:id/respond`| Accept or decline an incoming swap request | Yes |

---

## 🎓 College Viva & Presentation Walkthrough

Follow this step-by-step sequence to present the project during your college viva:

1. **Launch the C++ Server**:
   * In a terminal, run `.\server.exe` (or double click `run_server.bat`).
   * Show the evaluator the server startup log showing SQLite initialization:
     ```text
     [Database] SQLite initialized successfully: skillswap.db
     [SkillSwap] Server is starting on port 8080...
     ```
2. **Access the Application**:
   * Open Chrome or Edge and go to `http://127.0.0.1:8080/`.
   * Point out that the C++ server is serving both the HTML/CSS/JS frontend and the REST API.
3. **Register a New Student (`register.html`)**:
   * Click **"Join Free"** and click **"Auto-Fill"** to populate sample credentials.
   * Click **"Complete Registration"** &rarr; observe the server log receiving the `POST /api/auth/register` request and inserting the salted password hash into SQLite.
4. **Manage Skills in SQLite (`profile.html`)**:
   * Go to **"My Profile"**.
   * Add a new teaching skill (e.g. *"Rust Programming - Advanced"*).
   * Delete an existing skill by clicking `×`.
   * Explain to the evaluator: *"This skill was inserted/deleted directly from the SQLite database via a parameterized C++ API call."*
5. **Real-Time Directory Search (`explore.html`)**:
   * Go to **"Explore Students"**.
   * Type *"Python"* or *"Figma"* in the search bar.
   * Explain that `GET /api/students?search=python` is performing a parameterized SQL query in C++ and returning matched peers in real time.
6. **Demonstrate Persistence**:
   * Stop the C++ server in the terminal (`Ctrl + C`).
   * Restart the server (`.\server.exe`).
   * Refresh `profile.html` or `explore.html` to prove that all data remained intact in `skillswap.db`.
#   s k i l l e x c h a n g e  
 