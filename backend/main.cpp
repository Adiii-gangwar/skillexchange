#include <iostream>
#include <string>
#include <memory>

#include "sqlite/sqlite3.h"
#include "json.hpp"
#include "include/sha256.hpp"
#include "include/database.hpp"
#include "include/http_server.hpp"

using json = nlohmann::json;

int main() {
    std::cout << "Starting SkillSwap C++ Backend Server..." << std::endl;

    // 1. Initialize SQLite Database
    Database db("skillswap.db");

    // 2. Initialize HTTP Server
    HttpServer server;
    server.set_static_dir("."); // Serve frontend files directly

    // =========================================================
    // API ROUTE 1: User Registration
    // POST /api/auth/register
    // =========================================================
    server.post("/api/auth/register", [&db](const HttpRequest& req, HttpResponse& res) {
        json body = req.json_body();

        std::string email = body.value("email", "");
        std::string password = body.value("password", "");
        std::string name = body.value("name", "");
        std::string dept = body.value("department", "Computer Science");
        std::string year = body.value("year", "1st Year");
        std::string teach_skill = body.value("teach_skill", "");
        std::string learn_skill = body.value("learn_skill", "");

        if (email.empty() || password.empty() || name.empty()) {
            res.error(400, "Missing required fields: email, password, and name are mandatory.");
            return;
        }

        if (password.length() < 4) {
            res.error(400, "Password must be at least 4 characters.");
            return;
        }

        int user_id = 0;
        std::string err;
        if (!db.register_user(email, password, name, dept, year, teach_skill, learn_skill, user_id, err)) {
            res.error(400, err);
            return;
        }

        // Generate session token
        std::string token = db.create_session(user_id);
        json user_profile = db.get_user_profile(user_id);

        json response;
        response["success"] = true;
        response["message"] = "Registration successful!";
        response["token"] = token;
        response["user"] = user_profile;
        res.json(response, 201);
    });

    // =========================================================
    // API ROUTE 2: User Login
    // POST /api/auth/login
    // =========================================================
    server.post("/api/auth/login", [&db](const HttpRequest& req, HttpResponse& res) {
        json body = req.json_body();

        std::string email = body.value("email", "");
        std::string password = body.value("password", "");

        if (email.empty() || password.empty()) {
            res.error(400, "Email and password are required.");
            return;
        }

        int user_id = 0;
        std::string err;
        if (!db.authenticate_user(email, password, user_id, err)) {
            res.error(401, err);
            return;
        }

        std::string token = db.create_session(user_id);
        json user_profile = db.get_user_profile(user_id);

        json response;
        response["success"] = true;
        response["message"] = "Login successful!";
        response["token"] = token;
        response["user"] = user_profile;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 3: Current Authenticated User & Profile
    // GET /api/auth/me
    // =========================================================
    server.get("/api/auth/me", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Invalid or expired session token.");
            return;
        }

        json user_profile = db.get_user_profile(user_id);
        json response;
        response["success"] = true;
        response["user"] = user_profile;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 4: Logout
    // POST /api/auth/logout
    // =========================================================
    server.post("/api/auth/logout", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        if (!token.empty()) {
            db.delete_session(token);
        }
        json response;
        response["success"] = true;
        response["message"] = "Logged out successfully.";
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 5: Get Profile
    // GET /api/profile
    // =========================================================
    server.get("/api/profile", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;

        // Check if query parameter user_id was requested
        std::string requested_uid = req.query("user_id");
        if (!requested_uid.empty()) {
            try {
                user_id = std::stoi(requested_uid);
            } catch (...) {
                res.error(400, "Invalid user_id parameter.");
                return;
            }
        } else {
            if (!db.get_user_id_from_token(token, user_id)) {
                res.error(401, "Unauthorized: Please log in.");
                return;
            }
        }

        json profile = db.get_user_profile(user_id);
        if (profile.empty()) {
            res.error(404, "Student profile not found.");
            return;
        }

        json response;
        response["success"] = true;
        response["profile"] = profile;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 6: Update Profile
    // PUT /api/profile
    // =========================================================
    server.put("/api/profile", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        json body = req.json_body();
        std::string full_name = body.value("name", "");
        if (full_name.empty()) full_name = body.value("full_name", "");

        std::string dept = body.value("department", "");
        std::string year = body.value("year", "");
        std::string bio = body.value("bio", "");
        std::string study_mode = body.value("study_mode", "Hybrid (Library & Online)");

        if (full_name.empty() || dept.empty() || year.empty()) {
            res.error(400, "Name, Department, and Year are required.");
            return;
        }

        std::string err;
        if (!db.update_user_profile(user_id, full_name, dept, year, bio, study_mode, err)) {
            res.error(500, "Failed to update profile: " + err);
            return;
        }

        json updated_profile = db.get_user_profile(user_id);
        json response;
        response["success"] = true;
        response["message"] = "Profile updated successfully!";
        response["profile"] = updated_profile;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 7: Add Teachable Skill
    // POST /api/skills/teach
    // =========================================================
    server.post("/api/skills/teach", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        json body = req.json_body();
        std::string name = body.value("name", "");
        std::string level = body.value("level", "Intermediate");

        if (name.empty()) {
            res.error(400, "Skill name cannot be empty.");
            return;
        }

        json skill = db.add_skill(user_id, "teach", name, level);
        json response;
        response["success"] = true;
        response["message"] = "Teaching skill added successfully.";
        response["skill"] = skill;
        res.json(response, 201);
    });

    // =========================================================
    // API ROUTE 8: Add Learnable Skill
    // POST /api/skills/learn
    // =========================================================
    server.post("/api/skills/learn", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        json body = req.json_body();
        std::string name = body.value("name", "");
        std::string goal = body.value("goal", "Personal Project");

        if (name.empty()) {
            res.error(400, "Skill name cannot be empty.");
            return;
        }

        json skill = db.add_skill(user_id, "learn", name, goal);
        json response;
        response["success"] = true;
        response["message"] = "Learning goal added successfully.";
        response["skill"] = skill;
        res.json(response, 201);
    });

    // =========================================================
    // API ROUTE 9: Delete Skill (Teachable or Learnable)
    // DELETE /api/skills/:id
    // =========================================================
    server.del("/api/skills/:id", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        std::string id_str = req.query("id");
        int skill_id = 0;
        try {
            skill_id = std::stoi(id_str);
        } catch (...) {
            res.error(400, "Invalid skill ID.");
            return;
        }

        if (!db.delete_skill(skill_id, user_id)) {
            res.error(404, "Skill not found or does not belong to you.");
            return;
        }

        json response;
        response["success"] = true;
        response["message"] = "Skill deleted successfully.";
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 10: Search Students by Name & Skill
    // GET /api/students
    // =========================================================
    server.get("/api/students", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string query = req.query("search");
        std::string category = req.query("category", "all");
        std::string dept = req.query("department", "all");
        std::string year = req.query("year", "all");

        json students = db.search_students(query, category, dept, year);

        json response;
        response["success"] = true;
        response["count"] = students.size();
        response["students"] = students;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 11: Get User Connections and Swaps
    // GET /api/swaps
    // =========================================================
    server.get("/api/swaps", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        json swaps = db.get_user_swaps(user_id);
        json response;
        response["success"] = true;
        response["swaps"] = swaps;
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 12: Propose Swap Request
    // POST /api/swaps/request
    // =========================================================
    server.post("/api/swaps/request", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        json body = req.json_body();
        int receiver_id = body.value("receiver_id", 0);
        std::string offered_skill = body.value("offered_skill", "");
        std::string requested_skill = body.value("requested_skill", "");
        std::string message = body.value("message", "");

        if (receiver_id <= 0 || receiver_id == user_id) {
            res.error(400, "Invalid receiver for swap request.");
            return;
        }

        if (!db.create_swap_request(user_id, receiver_id, offered_skill, requested_skill, message)) {
            res.error(500, "Failed to submit swap request.");
            return;
        }

        json response;
        response["success"] = true;
        response["message"] = "Swap request sent successfully!";
        res.json(response, 201);
    });

    // =========================================================
    // API ROUTE 13: Respond to Swap Request (Accept / Decline)
    // POST /api/swaps/:id/respond
    // =========================================================
    server.post("/api/swaps/:id/respond", [&db](const HttpRequest& req, HttpResponse& res) {
        std::string token = req.bearer_token();
        int user_id = 0;
        if (!db.get_user_id_from_token(token, user_id)) {
            res.error(401, "Unauthorized: Please log in.");
            return;
        }

        std::string id_str = req.query("id");
        int swap_id = 0;
        try {
            swap_id = std::stoi(id_str);
        } catch (...) {
            res.error(400, "Invalid swap ID.");
            return;
        }

        json body = req.json_body();
        std::string action = body.value("action", ""); // "accepted" or "declined"
        if (action != "accepted" && action != "declined") {
            res.error(400, "Action must be 'accepted' or 'declined'.");
            return;
        }

        if (!db.respond_to_swap(swap_id, user_id, action)) {
            res.error(404, "Swap request not found or not addressed to you.");
            return;
        }

        json response;
        response["success"] = true;
        response["message"] = (action == "accepted" ? "Swap accepted! Peer added to your connections." : "Swap declined.");
        res.json(response, 200);
    });

    // =========================================================
    // API ROUTE 14: System Health Check
    // GET /api/health
    // =========================================================
    server.get("/api/health", [](const HttpRequest& req, HttpResponse& res) {
        json response;
        response["status"] = "online";
        response["backend"] = "C++ 14 (Winsock2)";
        response["database"] = "SQLite 3.54";
        response["app"] = "SkillSwap - College PBL";
        res.json(response, 200);
    });

    // 4. Start Server on Port 8080
    std::cout << "[SkillSwap] Server is starting on port 8080..." << std::endl;
    server.listen(8080, "127.0.0.1");

    return 0;
}
