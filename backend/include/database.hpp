#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <string>
#include <vector>
#include <memory>
#include <windows.h>

class WinMutex {
public:
    WinMutex() { InitializeCriticalSection(&m_cs); }
    ~WinMutex() { DeleteCriticalSection(&m_cs); }
    void lock() { EnterCriticalSection(&m_cs); }
    void unlock() { LeaveCriticalSection(&m_cs); }
private:
    CRITICAL_SECTION m_cs;
};

class WinLockGuard {
public:
    WinLockGuard(WinMutex& m) : m_mutex(m) { m_mutex.lock(); }
    ~WinLockGuard() { m_mutex.unlock(); }
private:
    WinMutex& m_mutex;
};
#include <iostream>
#include "../sqlite/sqlite3.h"
#include "../json.hpp"
#include "sha256.hpp"

using json = nlohmann::json;

class Database {
public:
    Database(const std::string& db_path = "skillswap.db") : m_db(nullptr), m_db_path(db_path) {
        init();
    }

    ~Database() {
        if (m_db) {
            sqlite3_close(m_db);
            m_db = nullptr;
        }
    }

    bool init() {
        WinLockGuard lock(m_mutex);
        int rc = sqlite3_open(m_db_path.c_str(), &m_db);
        if (rc != SQLITE_OK) {
            std::cerr << "[Database Error] Cannot open database: " << sqlite3_errmsg(m_db) << std::endl;
            return false;
        }

        // Enable foreign keys
        char* errmsg = nullptr;
        sqlite3_exec(m_db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

        const char* schema_sql = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                email TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            );

            CREATE TABLE IF NOT EXISTS profiles (
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

            CREATE TABLE IF NOT EXISTS skills (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER NOT NULL,
                type TEXT NOT NULL, -- 'teach' or 'learn'
                name TEXT NOT NULL,
                level_or_goal TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS sessions (
                token TEXT PRIMARY KEY,
                user_id INTEGER NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
            );

            CREATE TABLE IF NOT EXISTS swap_requests (
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
        )";

        rc = sqlite3_exec(m_db, schema_sql, nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            std::cerr << "[Database Error] Schema creation failed: " << (errmsg ? errmsg : "unknown") << std::endl;
            if (errmsg) sqlite3_free(errmsg);
            return false;
        }

        // Check if database is empty and seed initial college demo students
        seed_initial_data_if_needed();

        std::cout << "[Database] SQLite initialized successfully: " << m_db_path << std::endl;
        return true;
    }

    // -------------------------------------------------------------
    // Authentication Operations
    // -------------------------------------------------------------
    bool register_user(const std::string& email, const std::string& password,
                       const std::string& full_name, const std::string& department,
                       const std::string& year_of_study, const std::string& teach_skill,
                       const std::string& learn_skill, int& out_user_id, std::string& out_error) {
        WinLockGuard lock(m_mutex);

        // 1. Check if user already exists (Parameterized Query)
        const char* check_sql = "SELECT id FROM users WHERE email = ?;";
        sqlite3_stmt* check_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, check_sql, -1, &check_stmt, nullptr) != SQLITE_OK) {
            out_error = "Database preparation error";
            return false;
        }
        sqlite3_bind_text(check_stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            sqlite3_finalize(check_stmt);
            out_error = "A student with this email is already registered.";
            return false;
        }
        sqlite3_finalize(check_stmt);

        // 2. Generate cryptographically random salt and hash password
        std::string salt = security::generate_random_hex(16);
        std::string password_hash = security::hash_password(password, salt);

        // 3. Insert into users table
        const char* insert_user_sql = "INSERT INTO users (email, password_hash, salt) VALUES (?, ?, ?);";
        sqlite3_stmt* user_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, insert_user_sql, -1, &user_stmt, nullptr) != SQLITE_OK) {
            out_error = "Failed to insert user";
            return false;
        }
        sqlite3_bind_text(user_stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(user_stmt, 2, password_hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(user_stmt, 3, salt.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(user_stmt) != SQLITE_DONE) {
            sqlite3_finalize(user_stmt);
            out_error = "Failed to create user record";
            return false;
        }
        sqlite3_finalize(user_stmt);

        int user_id = static_cast<int>(sqlite3_last_insert_rowid(m_db));
        out_user_id = user_id;

        // 4. Create initial profile
        std::string initial = extract_initials(full_name);
        const char* insert_prof_sql = "INSERT INTO profiles (user_id, full_name, department, year_of_study, avatar_initial, bio) VALUES (?, ?, ?, ?, ?, ?);";
        sqlite3_stmt* prof_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, insert_prof_sql, -1, &prof_stmt, nullptr) == SQLITE_OK) {
            std::string default_bio = "Enthusiastic college student ready to share skills and learn from peers.";
            sqlite3_bind_int(prof_stmt, 1, user_id);
            sqlite3_bind_text(prof_stmt, 2, full_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(prof_stmt, 3, department.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(prof_stmt, 4, year_of_study.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(prof_stmt, 5, initial.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(prof_stmt, 6, default_bio.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(prof_stmt);
            sqlite3_finalize(prof_stmt);
        }

        // 5. Insert initial skills if provided
        if (!teach_skill.empty()) {
            add_skill_internal(user_id, "teach", teach_skill, "Intermediate");
        }
        if (!learn_skill.empty()) {
            add_skill_internal(user_id, "learn", learn_skill, "Semester Goal");
        }

        return true;
    }

    bool authenticate_user(const std::string& email, const std::string& password, int& out_user_id, std::string& out_error) {
        WinLockGuard lock(m_mutex);

        const char* sql = "SELECT id, password_hash, salt FROM users WHERE email = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            out_error = "Database query error";
            return false;
        }

        sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            out_error = "Invalid college email or password.";
            return false;
        }

        int user_id = sqlite3_column_int(stmt, 0);
        std::string stored_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        sqlite3_finalize(stmt);

        // Verify with secure hash + salt
        if (!security::verify_password(password, salt, stored_hash)) {
            out_error = "Invalid college email or password.";
            return false;
        }

        out_user_id = user_id;
        return true;
    }

    std::string create_session(int user_id) {
        WinLockGuard lock(m_mutex);
        std::string token = security::generate_random_hex(24);

        const char* sql = "INSERT INTO sessions (token, user_id) VALUES (?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        return token;
    }

    bool get_user_id_from_token(const std::string& token, int& out_user_id) {
        if (token.empty()) return false;
        WinLockGuard lock(m_mutex);

        const char* sql = "SELECT user_id FROM sessions WHERE token = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
        bool found = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out_user_id = sqlite3_column_int(stmt, 0);
            found = true;
        }
        sqlite3_finalize(stmt);
        return found;
    }

    void delete_session(const std::string& token) {
        WinLockGuard lock(m_mutex);
        const char* sql = "DELETE FROM sessions WHERE token = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // -------------------------------------------------------------
    // Profile Operations
    // -------------------------------------------------------------
    json get_user_profile(int user_id) {
        WinLockGuard lock(m_mutex);
        json result = json::object();

        const char* sql = R"(
            SELECT u.email, p.full_name, p.department, p.year_of_study, 
                   p.bio, p.study_mode, p.rating, p.completed_swaps, p.avatar_initial
            FROM users u
            JOIN profiles p ON u.id = p.user_id
            WHERE u.id = ?;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, user_id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                result["id"] = user_id;
                result["email"] = column_text(stmt, 0);
                result["name"] = column_text(stmt, 1);
                result["department"] = column_text(stmt, 2);
                result["year"] = column_text(stmt, 3);
                result["bio"] = column_text(stmt, 4);
                result["study_mode"] = column_text(stmt, 5);
                result["rating"] = sqlite3_column_double(stmt, 6);
                result["completed_swaps"] = sqlite3_column_int(stmt, 7);
                result["avatar_initial"] = column_text(stmt, 8);
            }
            sqlite3_finalize(stmt);
        }

        if (result.empty()) return result;

        // Fetch skills
        result["skills_teach"] = json::array();
        result["skills_learn"] = json::array();

        const char* skills_sql = "SELECT id, type, name, level_or_goal FROM skills WHERE user_id = ? ORDER BY id ASC;";
        sqlite3_stmt* sk_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, skills_sql, -1, &sk_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(sk_stmt, 1, user_id);
            while (sqlite3_step(sk_stmt) == SQLITE_ROW) {
                json sk;
                sk["id"] = sqlite3_column_int(sk_stmt, 0);
                std::string type = column_text(sk_stmt, 1);
                sk["name"] = column_text(sk_stmt, 2);
                if (type == "teach") {
                    sk["level"] = column_text(sk_stmt, 3);
                    result["skills_teach"].push_back(sk);
                } else {
                    sk["goal"] = column_text(sk_stmt, 3);
                    result["skills_learn"].push_back(sk);
                }
            }
            sqlite3_finalize(sk_stmt);
        }

        return result;
    }

    bool update_user_profile(int user_id, const std::string& full_name,
                            const std::string& department, const std::string& year_of_study,
                            const std::string& bio, const std::string& study_mode, std::string& out_error) {
        WinLockGuard lock(m_mutex);

        const char* sql = R"(
            UPDATE profiles 
            SET full_name = ?, department = ?, year_of_study = ?, bio = ?, study_mode = ?, 
                avatar_initial = ?, updated_at = CURRENT_TIMESTAMP
            WHERE user_id = ?;
        )";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            out_error = "Database prepare error";
            return false;
        }

        std::string initial = extract_initials(full_name);
        sqlite3_bind_text(stmt, 1, full_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, department.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, year_of_study.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, bio.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, study_mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, initial.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 7, user_id);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }

    // -------------------------------------------------------------
    // Skills Operations (Add / Delete)
    // -------------------------------------------------------------
    json add_skill(int user_id, const std::string& type, const std::string& name, const std::string& level_or_goal) {
        WinLockGuard lock(m_mutex);
        int skill_id = add_skill_internal(user_id, type, name, level_or_goal);
        json res;
        res["id"] = skill_id;
        res["type"] = type;
        res["name"] = name;
        if (type == "teach") res["level"] = level_or_goal;
        else res["goal"] = level_or_goal;
        return res;
    }

    bool delete_skill(int skill_id, int user_id) {
        WinLockGuard lock(m_mutex);
        const char* sql = "DELETE FROM skills WHERE id = ? AND user_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, skill_id);
        sqlite3_bind_int(stmt, 2, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE && sqlite3_changes(m_db) > 0;
    }

    // -------------------------------------------------------------
    // Search Students by Name & Skill
    // -------------------------------------------------------------
    json search_students(const std::string& query, const std::string& category,
                         const std::string& department, const std::string& year) {
        WinLockGuard lock(m_mutex);
        json student_list = json::array();

        // Build query with parameterization
        std::string sql = R"(
            SELECT p.user_id, u.email, p.full_name, p.department, p.year_of_study, 
                   p.bio, p.rating, p.completed_swaps, p.avatar_initial, p.study_mode
            FROM profiles p
            JOIN users u ON p.user_id = u.id
            WHERE 1=1
        )";

        if (!department.empty() && department != "all") {
            sql += " AND p.department LIKE ?";
        }
        if (!year.empty() && year != "all") {
            sql += " AND p.year_of_study LIKE ?";
        }

        sql += " ORDER BY p.rating DESC, p.completed_swaps DESC;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return student_list;
        }

        int bind_idx = 1;
        if (!department.empty() && department != "all") {
            std::string dept_param = "%" + department + "%";
            sqlite3_bind_text(stmt, bind_idx++, dept_param.c_str(), -1, SQLITE_TRANSIENT);
        }
        if (!year.empty() && year != "all") {
            std::string year_param = "%" + year + "%";
            sqlite3_bind_text(stmt, bind_idx++, year_param.c_str(), -1, SQLITE_TRANSIENT);
        }

        std::string lower_query = to_lower(query);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int uid = sqlite3_column_int(stmt, 0);
            std::string email = column_text(stmt, 1);
            std::string name = column_text(stmt, 2);
            std::string dept = column_text(stmt, 3);
            std::string yr = column_text(stmt, 4);
            std::string bio = column_text(stmt, 5);
            double rating = sqlite3_column_double(stmt, 6);
            int swaps = sqlite3_column_int(stmt, 7);
            std::string initial = column_text(stmt, 8);
            std::string availability = column_text(stmt, 9);

            // Fetch skills for this user
            json teach_skills = json::array();
            json learn_skills = json::array();

            const char* sk_sql = "SELECT id, type, name, level_or_goal FROM skills WHERE user_id = ?;";
            sqlite3_stmt* sk_stmt = nullptr;
            bool skill_matched_query = false;

            if (sqlite3_prepare_v2(m_db, sk_sql, -1, &sk_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(sk_stmt, 1, uid);
                while (sqlite3_step(sk_stmt) == SQLITE_ROW) {
                    std::string stype = column_text(sk_stmt, 1);
                    std::string sname = column_text(sk_stmt, 2);
                    std::string sdetail = column_text(sk_stmt, 3);

                    if (!lower_query.empty() && to_lower(sname).find(lower_query) != std::string::npos) {
                        skill_matched_query = true;
                    }

                    json item;
                    item["id"] = sqlite3_column_int(sk_stmt, 0);
                    item["name"] = sname;
                    if (stype == "teach") {
                        item["level"] = sdetail;
                        teach_skills.push_back(item);
                    } else {
                        item["goal"] = sdetail;
                        learn_skills.push_back(item);
                    }
                }
                sqlite3_finalize(sk_stmt);
            }

            // Keyword text filter (matches name, bio, department, or any skills)
            if (!lower_query.empty()) {
                bool name_match = to_lower(name).find(lower_query) != std::string::npos;
                bool bio_match = to_lower(bio).find(lower_query) != std::string::npos;
                bool dept_match = to_lower(dept).find(lower_query) != std::string::npos;
                if (!name_match && !bio_match && !dept_match && !skill_matched_query) {
                    continue; // Skip student if query does not match
                }
            }

            // Category filter
            if (!category.empty() && category != "all") {
                bool category_match = false;
                std::string cat_lower = to_lower(category);
                for (auto& s : teach_skills) {
                    if (category_contains(s["name"].get<std::string>(), cat_lower)) {
                        category_match = true;
                        break;
                    }
                }
                for (auto& s : learn_skills) {
                    if (category_contains(s["name"].get<std::string>(), cat_lower)) {
                        category_match = true;
                        break;
                    }
                }
                if (!category_match) {
                    continue;
                }
            }

            json student;
            student["id"] = uid;
            student["name"] = name;
            student["email"] = email;
            student["department"] = dept;
            student["year"] = yr;
            student["bio"] = bio;
            student["rating"] = rating;
            student["swapsCount"] = swaps;
            student["avatarInitial"] = initial;
            student["availability"] = availability;
            student["teach"] = teach_skills;
            student["learn"] = learn_skills;

            student_list.push_back(student);
        }
        sqlite3_finalize(stmt);

        return student_list;
    }

    // -------------------------------------------------------------
    // Swap Request Operations
    // -------------------------------------------------------------
    bool create_swap_request(int sender_id, int receiver_id, const std::string& offered_skill,
                            const std::string& requested_skill, const std::string& message) {
        WinLockGuard lock(m_mutex);
        const char* sql = R"(
            INSERT INTO swap_requests (sender_id, receiver_id, offered_skill, requested_skill, message)
            VALUES (?, ?, ?, ?, ?);
        )";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, sender_id);
        sqlite3_bind_int(stmt, 2, receiver_id);
        sqlite3_bind_text(stmt, 3, offered_skill.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, requested_skill.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, message.c_str(), -1, SQLITE_TRANSIENT);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE;
    }

    json get_user_swaps(int user_id) {
        WinLockGuard lock(m_mutex);
        json res;
        res["active"] = json::array();
        res["pending"] = json::array();
        res["sent"] = json::array();

        // 1. Pending Received Requests
        const char* pending_sql = R"(
            SELECT sr.id, sr.sender_id, p.full_name, p.department, p.year_of_study, p.avatar_initial,
                   sr.offered_skill, sr.requested_skill, sr.message, sr.created_at
            FROM swap_requests sr
            JOIN profiles p ON sr.sender_id = p.user_id
            WHERE sr.receiver_id = ? AND sr.status = 'pending'
            ORDER BY sr.created_at DESC;
        )";
        sqlite3_stmt* p_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, pending_sql, -1, &p_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(p_stmt, 1, user_id);
            while (sqlite3_step(p_stmt) == SQLITE_ROW) {
                json item;
                item["id"] = sqlite3_column_int(p_stmt, 0);
                item["sender_id"] = sqlite3_column_int(p_stmt, 1);
                item["studentName"] = column_text(p_stmt, 2);
                item["department"] = column_text(p_stmt, 3) + " • " + column_text(p_stmt, 4);
                item["avatarInitial"] = column_text(p_stmt, 5);
                item["partnerSkillOffered"] = column_text(p_stmt, 6);
                item["userSkillRequested"] = column_text(p_stmt, 7);
                item["message"] = column_text(p_stmt, 8);
                item["date"] = column_text(p_stmt, 9);
                res["pending"].push_back(item);
            }
            sqlite3_finalize(p_stmt);
        }

        // 2. Sent Requests
        const char* sent_sql = R"(
            SELECT sr.id, sr.receiver_id, p.full_name, p.department, p.year_of_study, p.avatar_initial,
                   sr.offered_skill, sr.requested_skill, sr.message, sr.created_at, sr.status
            FROM swap_requests sr
            JOIN profiles p ON sr.receiver_id = p.user_id
            WHERE sr.sender_id = ? AND sr.status = 'pending'
            ORDER BY sr.created_at DESC;
        )";
        sqlite3_stmt* s_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sent_sql, -1, &s_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(s_stmt, 1, user_id);
            while (sqlite3_step(s_stmt) == SQLITE_ROW) {
                json item;
                item["id"] = sqlite3_column_int(s_stmt, 0);
                item["receiver_id"] = sqlite3_column_int(s_stmt, 1);
                item["studentName"] = column_text(s_stmt, 2);
                item["department"] = column_text(s_stmt, 3) + " • " + column_text(s_stmt, 4);
                item["avatarInitial"] = column_text(s_stmt, 5);
                item["skillOffered"] = column_text(s_stmt, 6);
                item["skillRequested"] = column_text(s_stmt, 7);
                item["message"] = column_text(s_stmt, 8);
                item["date"] = column_text(s_stmt, 9);
                item["status"] = column_text(s_stmt, 10);
                res["sent"].push_back(item);
            }
            sqlite3_finalize(s_stmt);
        }

        // 3. Active Connections (Accepted Swaps)
        const char* active_sql = R"(
            SELECT sr.id, 
                   CASE WHEN sr.sender_id = ? THEN sr.receiver_id ELSE sr.sender_id END as partner_id,
                   p.full_name, p.department, p.year_of_study, p.avatar_initial, p.rating,
                   CASE WHEN sr.sender_id = ? THEN sr.offered_skill ELSE sr.requested_skill END as user_teaches,
                   CASE WHEN sr.sender_id = ? THEN sr.requested_skill ELSE sr.offered_skill END as partner_teaches
            FROM swap_requests sr
            JOIN profiles p ON (p.user_id = CASE WHEN sr.sender_id = ? THEN sr.receiver_id ELSE sr.sender_id END)
            WHERE (sr.sender_id = ? OR sr.receiver_id = ?) AND sr.status = 'accepted';
        )";
        sqlite3_stmt* a_stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, active_sql, -1, &a_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(a_stmt, 1, user_id);
            sqlite3_bind_int(a_stmt, 2, user_id);
            sqlite3_bind_int(a_stmt, 3, user_id);
            sqlite3_bind_int(a_stmt, 4, user_id);
            sqlite3_bind_int(a_stmt, 5, user_id);
            sqlite3_bind_int(a_stmt, 6, user_id);
            while (sqlite3_step(a_stmt) == SQLITE_ROW) {
                json item;
                item["id"] = sqlite3_column_int(a_stmt, 0);
                item["partner_id"] = sqlite3_column_int(a_stmt, 1);
                item["studentName"] = column_text(a_stmt, 2);
                item["department"] = column_text(a_stmt, 3) + " • " + column_text(a_stmt, 4);
                item["avatarInitial"] = column_text(a_stmt, 5);
                item["rating"] = sqlite3_column_double(a_stmt, 6);
                item["userTeaches"] = column_text(a_stmt, 7);
                item["partnerTeaches"] = column_text(a_stmt, 8);
                item["status"] = "Active Swap";
                item["nextSession"] = "Upcoming";
                res["active"].push_back(item);
            }
            sqlite3_finalize(a_stmt);
        }

        return res;
    }

    bool respond_to_swap(int swap_id, int user_id, const std::string& new_status) {
        WinLockGuard lock(m_mutex);
        const char* sql = "UPDATE swap_requests SET status = ? WHERE id = ? AND receiver_id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, new_status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, swap_id);
        sqlite3_bind_int(stmt, 3, user_id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return rc == SQLITE_DONE && sqlite3_changes(m_db) > 0;
    }

private:
    sqlite3* m_db;
    std::string m_db_path;
    WinMutex m_mutex;

    int add_skill_internal(int user_id, const std::string& type, const std::string& name, const std::string& level_or_goal) {
        const char* sql = "INSERT INTO skills (user_id, type, name, level_or_goal) VALUES (?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, level_or_goal.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            return static_cast<int>(sqlite3_last_insert_rowid(m_db));
        }
        return -1;
    }

    std::string column_text(sqlite3_stmt* stmt, int col) {
        const unsigned char* txt = sqlite3_column_text(stmt, col);
        return txt ? reinterpret_cast<const char*>(txt) : "";
    }

    std::string extract_initials(const std::string& name) {
        std::string initials = "";
        bool next = true;
        for (char c : name) {
            if (std::isspace(c)) {
                next = true;
            } else if (next && std::isalpha(c)) {
                initials += static_cast<char>(std::toupper(c));
                next = false;
                if (initials.size() == 2) break;
            }
        }
        return initials.empty() ? "ST" : initials;
    }

    std::string to_lower(std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(c));
        return s;
    }

    bool category_contains(const std::string& skill, const std::string& cat) {
        std::string s = to_lower(skill);
        if (cat == "web-dev") return s.find("web") != std::string::npos || s.find("html") != std::string::npos || s.find("css") != std::string::npos || s.find("js") != std::string::npos || s.find("react") != std::string::npos || s.find("node") != std::string::npos;
        if (cat == "ai-ml") return s.find("ai") != std::string::npos || s.find("ml") != std::string::npos || s.find("data") != std::string::npos || s.find("python") != std::string::npos || s.find("tensor") != std::string::npos;
        if (cat == "design") return s.find("figma") != std::string::npos || s.find("design") != std::string::npos || s.find("ui") != std::string::npos || s.find("ux") != std::string::npos || s.find("wirefram") != std::string::npos;
        if (cat == "dsa") return s.find("dsa") != std::string::npos || s.find("algo") != std::string::npos || s.find("c++") != std::string::npos || s.find("leetcode") != std::string::npos || s.find("tree") != std::string::npos;
        if (cat == "mobile-dev") return s.find("mobile") != std::string::npos || s.find("flutter") != std::string::npos || s.find("android") != std::string::npos || s.find("app") != std::string::npos;
        if (cat == "cloud") return s.find("cloud") != std::string::npos || s.find("aws") != std::string::npos || s.find("devops") != std::string::npos || s.find("docker") != std::string::npos || s.find("linux") != std::string::npos;
        if (cat == "soft-skills") return s.find("speaking") != std::string::npos || s.find("interview") != std::string::npos || s.find("gd") != std::string::npos || s.find("pitch") != std::string::npos;
        return s.find(cat) != std::string::npos;
    }

    void seed_initial_data_if_needed() {
        const char* count_sql = "SELECT COUNT(*) FROM users;";
        sqlite3_stmt* stmt = nullptr;
        int count = 0;
        if (sqlite3_prepare_v2(m_db, count_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                count = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        if (count > 0) return; // Already seeded

        std::cout << "[Database] Fresh database detected. Seeding verified student mentors into SQLite..." << std::endl;

        struct SeedStudent {
            std::string email;
            std::string password;
            std::string name;
            std::string dept;
            std::string year;
            std::string bio;
            double rating;
            int swaps;
            std::vector<std::pair<std::string, std::string>> teach;
            std::vector<std::pair<std::string, std::string>> learn;
        };

        std::vector<SeedStudent> seeds = {
            {
                "alex.morgan@college.edu", "demo1234", "Alex Morgan", "Computer Science & Eng.", "3rd Year",
                "Passionate about full-stack web development and Python scripting. Looking forward to learning UI/UX design and cloud deployment from peers!",
                4.9, 12,
                {{"Python Programming", "Advanced"}, {"JavaScript & DOM", "Advanced"}, {"SQL & Relational DBs", "Intermediate"}, {"Git & GitHub Workflow", "Intermediate"}},
                {{"Figma & UI/UX Design", "Build portfolio mockups"}, {"Docker & Containerization", "Cloud deployment project"}, {"Machine Learning Basics", "Semester elective preparation"}}
            },
            {
                "sneha.patel@college.edu", "demo1234", "Sneha Patel", "Information Technology", "2nd Year",
                "Product designer with expertise in Figma, design systems, and wireframing. Seeking frontend HTML/CSS integration guidance.",
                5.0, 9,
                {{"Figma & Wireframing", "Advanced"}, {"Color Theory & Typography", "Advanced"}, {"User Research", "Intermediate"}},
                {{"HTML5 & Modern CSS", "Build responsive layouts"}, {"Flexbox & Grid", "Interactive cards"}}
            },
            {
                "aarav.sharma@college.edu", "demo1234", "Aarav Sharma", "Computer Science", "3rd Year",
                "Full stack enthusiast. Built multiple projects using Node.js & React. Keen to learn Competitive Programming and graph algorithms.",
                4.9, 14,
                {{"Node.js & Express", "Advanced"}, {"REST APIs", "Advanced"}, {"MongoDB", "Intermediate"}},
                {{"DSA (Trees & Graphs)", "Interview prep"}, {"C++ STL", "Coding competitions"}}
            },
            {
                "ananya.iyer@college.edu", "demo1234", "Ananya Iyer", "Computer Science", "3rd Year",
                "500+ LeetCode problems solved. I love simplifying dynamic programming. Keen on learning Flutter mobile development.",
                4.9, 22,
                {{"Data Structures & Algos", "Advanced"}, {"Dynamic Programming", "Advanced"}, {"C++ Optimization", "Advanced"}},
                {{"Flutter App Dev", "Mobile portfolio"}, {"State Management", "Clean app architecture"}}
            },
            {
                "rohan.verma@college.edu", "demo1234", "Rohan Verma", "Electronics & Comm.", "4th Year",
                "Final year student working on computer vision and neural networks. Happy to teach Python for data science in exchange for IoT / Embedded C.",
                4.8, 18,
                {{"Python for Data Science", "Advanced"}, {"TensorFlow & PyTorch", "Intermediate"}, {"OpenCV", "Intermediate"}},
                {{"Embedded C", "Hardware projects"}, {"Arduino / ESP32", "Microcontrollers"}}
            },
            {
                "vikram.singh@college.edu", "demo1234", "Vikramaditya Singh", "Mechanical Eng.", "3rd Year",
                "Pivoted towards DevOps & Cloud infrastructure. AWS Certified Cloud Practitioner. Looking for guidance on Linux shell scripting & Docker.",
                4.7, 7,
                {{"AWS Cloud Fundamentals", "Intermediate"}, {"CI/CD Concepts", "Intermediate"}, {"CAD / SolidWorks", "Advanced"}},
                {{"Linux Bash Scripting", "Automation"}, {"Kubernetes Basics", "Clusters"}}
            },
            {
                "kabir.mehta@college.edu", "demo1234", "Kabir Mehta", "Management & Tech", "4th Year",
                "College Debating Society President. Helped 30+ students ace behavioral interviews, resume screening, and stage confidence. Want to learn Python for automation.",
                5.0, 25,
                {{"Public Speaking & Pitching", "Advanced"}, {"Resume & LinkedIn Review", "Advanced"}, {"Group Discussion Strategy", "Advanced"}},
                {{"Python Scripting", "Data automation"}, {"Web Scraping", "Market research"}}
            }
        };

        for (auto& s : seeds) {
            int uid = 0;
            std::string err;
            if (register_user(s.email, s.password, s.name, s.dept, s.year, "", "", uid, err)) {
                // Update profile with bio, rating, swaps
                const char* upd_sql = "UPDATE profiles SET bio = ?, rating = ?, completed_swaps = ? WHERE user_id = ?;";
                sqlite3_stmt* ustmt = nullptr;
                if (sqlite3_prepare_v2(m_db, upd_sql, -1, &ustmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(ustmt, 1, s.bio.c_str(), -1, SQLITE_TRANSIENT);
                    sqlite3_bind_double(ustmt, 2, s.rating);
                    sqlite3_bind_int(ustmt, 3, s.swaps);
                    sqlite3_bind_int(ustmt, 4, uid);
                    sqlite3_step(ustmt);
                    sqlite3_finalize(ustmt);
                }

                for (auto& tk : s.teach) {
                    add_skill_internal(uid, "teach", tk.first, tk.second);
                }
                for (auto& lk : s.learn) {
                    add_skill_internal(uid, "learn", lk.first, lk.second);
                }
            }
        }

        // Seed 1 active swap and 2 pending swap requests for Alex Morgan (uid=1)
        create_swap_request(1, 2, "Python & DOM Scripting", "Figma & UI/UX Design", "Let's exchange frontend design and Python!");
        respond_to_swap(1, 2, "accepted");

        create_swap_request(5, 1, "Python for Data Science", "JavaScript & DOM", "Hey Alex! Would love to swap for JavaScript DOM manipulation.");
        create_swap_request(7, 1, "Public Speaking & Pitching", "SQL & Relational DBs", "Hi Alex, I can help you polish your upcoming technical presentation & resume.");

        std::cout << "[Database] Seeded 7 students and initial swap connections." << std::endl;
    }
};

#endif // DATABASE_HPP
