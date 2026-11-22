// extra_routes.hpp -- additional API routes.
//
// This is where the C data structures and C++ classes are used for real:
//
//   linked list  -> GET  /api/my/cases/search        (search a user's cases)
//   stack        -> GET  /api/case-history/<id>      (history, newest first)
//   priority q   -> GET  /api/admin/priority-cases   (which case to handle first)
//   FIFO queue   -> GET  /api/admin/hearings         (order of the day)
//   C validation -> POST /api/admin/hearings, POST /api/cases (case id / dates)
//   C++ classes  -> cc::User (permissions), cc::Case, cc::Hearing, cc::Report
//
// MySQL stays the source of truth; the C structures are filled from query
// results for the duration of one request and freed afterwards.
#ifndef CC_EXTRA_ROUTES_HPP
#define CC_EXTRA_ROUTES_HPP

#include <map>
#include <string>
#include <vector>

// ------------------------------------------------------------------
// Query helpers
// ------------------------------------------------------------------

static const char* CASE_COLUMNS =
    "SELECT c.id, c.case_number, c.title, COALESCE(c.description, ''), "
    "COALESCE(c.case_type, ''), c.status, "
    "COALESCE(DATE_FORMAT(c.filing_date, '%Y-%m-%d'), ''), "
    "COALESCE(c.user_id, 0), COALESCE(u.full_name, ''), COALESCE(u.email, ''), "
    "COALESCE(DATE_FORMAT(c.created_at, '%Y-%m-%d %H:%i:%s'), '') "
    "FROM cases c LEFT JOIN users u ON c.user_id = u.id ";

inline bool loadCases(const std::string& whereClause,
                      const std::vector<DbParam>& params,
                      std::vector<cc::Case>& out)
{
    DbResult r = dbQuery(std::string(CASE_COLUMNS) + whereClause +
                         " ORDER BY c.created_at DESC, c.id DESC", params);

    if (!r.ok)
        return false;

    for (const auto& row : r.rows)
        out.push_back(cc::Case::fromRow(row));

    return true;
}

inline bool loadHearings(const std::string& whereClause,
                         const std::vector<DbParam>& params,
                         std::vector<cc::Hearing>& out)
{
    DbResult r = dbQuery(
        "SELECT h.id, h.case_id, c.case_number, c.title, "
        "DATE_FORMAT(h.hearing_date, '%Y-%m-%d'), "
        "COALESCE(TIME_FORMAT(h.hearing_time, '%H:%i'), ''), "
        "COALESCE(h.court_room, ''), COALESCE(h.judge_name, ''), "
        "COALESCE(h.purpose, ''), h.status "
        "FROM hearings h INNER JOIN cases c ON h.case_id = c.id " + whereClause +
        " ORDER BY h.hearing_date ASC, h.hearing_time ASC, h.id ASC", params);

    if (!r.ok)
        return false;

    for (const auto& row : r.rows)
        out.push_back(cc::Hearing::fromRow(row));

    return true;
}

inline bool isValidTimeHHMM(const std::string& t)
{
    if (t.size() != 5 || t[2] != ':')
        return false;

    for (int i : {0, 1, 3, 4})
        if (!std::isdigit((unsigned char)t[i]))
            return false;

    int hh = (t[0] - '0') * 10 + (t[1] - '0');
    int mm = (t[3] - '0') * 10 + (t[4] - '0');
    return hh < 24 && mm < 60;
}

inline bool isAllowedCaseStatus(const std::string& s)
{
    return cc::Case::isValidStatus(s);   // rule lives in the Case class
}

inline bool isAllowedHearingStatus(const std::string& s)
{
    return cc::Hearing::isValidStatus(s);   // rule lives in the Hearing class
}

inline bool isAllowedRole(const std::string& s)
{
    return s == "USER" || s == "ADMIN" || s == "LAWYER" || s == "JUDGE";
}

inline crow::response dbFailure(const std::string& what, const DbResult& r)
{
    std::cerr << "[db error] " << what << ": " << r.error << std::endl;
    return jsonError(500, what);
}

inline std::string stripCaseTag(const std::string& description)
{
    if (!description.empty() && description[0] == '[')
    {
        size_t close = description.find("] ");
        if (close != std::string::npos)
            return description.substr(close + 2);
    }
    return description;
}

// ------------------------------------------------------------------
// Route registration
// ------------------------------------------------------------------

template <typename App>
void registerExtraRoutes(App& app)
{
    // ==============================================================
    // USER: search own cases  (C linked list)
    // GET /api/my/cases/search?q=text
    // ==============================================================
    CROW_ROUTE(app, "/api/my/cases/search")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long userId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ViewOwnCases, userId, deny))
            return deny;

        const char* qParam = req.url_params.get("q");
        std::string query = qParam ? qParam : "";

        std::vector<cc::Case> cases;

        if (!loadCases("WHERE c.user_id = ?", { PI((long long)userId) }, cases))
            return jsonError(500, "Failed to load cases");

        // MySQL rows -> linked list -> search the list
        CaseList all, matches;
        case_list_init(&all);
        case_list_init(&matches);

        for (const cc::Case& c : cases)
            case_list_append(&all, c.id(), c.caseNumber().c_str(), c.title().c_str(),
                             c.description().c_str(), c.caseType().c_str(),
                             c.status().c_str(), c.filingDate().c_str());

        case_list_search(&all, query.c_str(), &matches);

        crow::json::wvalue response;
        response["status"] = "success";
        response["query"] = query;

        std::vector<crow::json::wvalue> items;

        for (CaseNode* n = matches.head; n; n = n->next)
        {
            crow::json::wvalue item;
            item["id"] = n->id;
            item["case_number"] = std::string(n->case_number);
            item["title"] = std::string(n->title);
            item["description"] = std::string(n->description);
            item["case_type"] = std::string(n->case_type);
            item["status"] = std::string(n->status);
            item["filing_date"] = std::string(n->filing_date);
            item["user_id"] = (int)userId;
            items.push_back(std::move(item));
        }

        response["count"] = (int)case_list_size(&matches);
        response["total"] = (int)case_list_size(&all);
        response["cases"] = std::move(items);

        case_list_free(&matches);
        case_list_free(&all);

        return crow::response(200, response);
    });


    // ==============================================================
    // USER/ADMIN: history of one case, newest first  (C stack)
    // GET /api/case-history/<case db id>
    // ==============================================================
    CROW_ROUTE(app, "/api/case-history/<int>")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req, int caseId)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        std::vector<cc::Case> found;

        if (!loadCases("WHERE c.id = ?", { PI(caseId) }, found))
            return jsonError(500, "Failed to load case");

        if (found.empty())
            return jsonError(404, "Case not found");

        unsigned long callerId = 0;
        crow::response deny;

        if (!authorizeSelfOrAdmin(req, found[0].ownerId(), callerId, deny))
            return deny;

        const std::string tag = "[" + found[0].caseNumber() + "] ";

        DbResult r = dbQuery(
            "SELECT a.action, COALESCE(a.description, ''), "
            "DATE_FORMAT(a.created_at, '%Y-%m-%d %H:%i:%s'), "
            "COALESCE(u.full_name, '') "
            "FROM audit_logs a LEFT JOIN users u ON a.user_id = u.id "
            "WHERE LEFT(a.description, CHAR_LENGTH(?)) = ? "
            "ORDER BY a.created_at ASC, a.id ASC",
            { P(tag), P(tag) });

        if (!r.ok)
            return dbFailure("Failed to load case history", r);

        // Push oldest -> newest, then pop: LIFO gives newest first
        HistoryStack stack;
        hs_init(&stack);

        for (const auto& row : r.rows)
        {
            HistoryEntry e;
            memset(&e, 0, sizeof e);
            copyToBuffer(e.action, sizeof e.action, row[0]);
            copyToBuffer(e.description, sizeof e.description, stripCaseTag(row[1]));
            copyToBuffer(e.created_at, sizeof e.created_at, row[2]);
            copyToBuffer(e.actor, sizeof e.actor, row[3]);
            hs_push(&stack, &e);
        }

        std::vector<crow::json::wvalue> items;
        HistoryEntry e;

        while (hs_pop(&stack, &e))
        {
            crow::json::wvalue item;
            item["action"] = std::string(e.action);
            item["description"] = std::string(e.description);
            item["created_at"] = std::string(e.created_at);
            item["actor"] = std::string(e.actor);
            items.push_back(std::move(item));
        }

        hs_free(&stack);

        crow::json::wvalue response;
        response["status"] = "success";
        response["case_number"] = found[0].caseNumber();
        response["count"] = (int)items.size();
        response["history"] = std::move(items);

        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: priority order of open cases  (C priority queue / heap)
    // GET /api/admin/priority-cases
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/priority-cases")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long userId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageAllCases, userId, deny))
            return deny;

        std::vector<cc::Case> cases;

        if (!loadCases("WHERE c.status IN ('PENDING', 'ONGOING')", {}, cases))
            return jsonError(500, "Failed to load cases");

        PriorityQueue pq;
        pq_init(&pq);

        for (const cc::Case& c : cases)
        {
            PriorityItem item;
            memset(&item, 0, sizeof item);
            item.id = c.id();
            item.priority = c.priority();
            copyToBuffer(item.case_number, sizeof item.case_number, c.caseNumber());
            copyToBuffer(item.title, sizeof item.title, c.title());
            copyToBuffer(item.case_type, sizeof item.case_type, c.caseType());
            copyToBuffer(item.status, sizeof item.status, c.status());
            copyToBuffer(item.filing_date, sizeof item.filing_date, c.filingDate());
            pq_push(&pq, &item);
        }

        std::vector<crow::json::wvalue> items;
        PriorityItem top;
        int rank = 0;

        while (pq_pop(&pq, &top))
        {
            crow::json::wvalue item;
            item["rank"] = ++rank;
            item["id"] = top.id;
            item["case_number"] = std::string(top.case_number);
            item["title"] = std::string(top.title);
            item["case_type"] = std::string(top.case_type);
            item["status"] = std::string(top.status);
            item["filing_date"] = std::string(top.filing_date);
            item["priority"] = top.priority;
            items.push_back(std::move(item));
        }

        pq_free(&pq);

        crow::json::wvalue response;
        response["status"] = "success";
        response["rule"] = "Lower priority number first (Criminal 1, Family 2, Property/Civil 3, "
                           "Consumer/Labour 4, other 5); ties go to the earlier filing date.";
        response["count"] = rank;
        response["cases"] = std::move(items);

        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: all hearings + order of the day  (C FIFO queue)
    // GET /api/admin/hearings
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/hearings")
        .methods("GET"_method, "POST"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ScheduleHearings, adminId, deny))
            return deny;

        // ---------------- GET: list ----------------
        if (req.method == "GET"_method)
        {
            DbResult r = dbQuery(
                "SELECT h.id, h.case_id, c.case_number, c.title, "
                "DATE_FORMAT(h.hearing_date, '%Y-%m-%d'), "
                "COALESCE(TIME_FORMAT(h.hearing_time, '%H:%i'), ''), "
                "COALESCE(h.court_room, ''), COALESCE(h.judge_name, ''), "
                "COALESCE(h.purpose, ''), h.status "
                "FROM hearings h INNER JOIN cases c ON h.case_id = c.id "
                "ORDER BY h.hearing_date ASC, h.hearing_time ASC, h.id ASC");

            if (!r.ok)
                return dbFailure("Failed to load hearings", r);

            // Upcoming hearings go into a FIFO queue in date/time order;
            // dequeuing hands out the serial position (order of the day).
            HearingQueue queue;
            hq_init(&queue);

            std::vector<cc::Hearing> hearings;

            for (const auto& row : r.rows)
            {
                cc::Hearing h = cc::Hearing::fromRow(row);

                if (h.isUpcoming())
                {
                    HearingItem item;
                    memset(&item, 0, sizeof item);
                    item.id = h.id();
                    copyToBuffer(item.hearing_date, sizeof item.hearing_date, row[4]);
                    copyToBuffer(item.hearing_time, sizeof item.hearing_time, row[5]);
                    hq_enqueue(&queue, &item);
                }

                hearings.push_back(std::move(h));
            }

            std::map<int, int> position;
            HearingItem next;
            int pos = 0;

            while (hq_dequeue(&queue, &next))
                position[next.id] = ++pos;

            hq_free(&queue);

            std::vector<crow::json::wvalue> items;

            for (const cc::Hearing& h : hearings)
            {
                crow::json::wvalue item = h.toJson();
                auto it = position.find(h.id());
                item["queue_position"] = it == position.end() ? 0 : it->second;
                items.push_back(std::move(item));
            }

            crow::json::wvalue response;
            response["status"] = "success";
            response["count"] = (int)hearings.size();
            response["upcoming_count"] = pos;
            response["hearings"] = std::move(items);

            return crow::response(200, response);
        }

        // ---------------- POST: schedule ----------------
        auto data = crow::json::load(req.body);

        if (!data)
            return jsonError(400, "Invalid JSON body");

        long long caseId = 0;
        std::string date, time, room, judge, purpose;

        if (!jsonGetInt(data, "case_id", caseId) || caseId <= 0)
            return jsonError(400, "case_id is required");

        if (!jsonGetString(data, "hearing_date", date) || !cc_validate_date(date.c_str()))
            return jsonError(400, "hearing_date must be a valid date (YYYY-MM-DD)");

        jsonGetString(data, "hearing_time", time);
        jsonGetString(data, "court_room", room);
        jsonGetString(data, "judge_name", judge);
        jsonGetString(data, "purpose", purpose);

        time = trimString(time);
        room = trimString(room);
        judge = trimString(judge);
        purpose = trimString(purpose);

        if (!time.empty() && !isValidTimeHHMM(time))
            return jsonError(400, "hearing_time must be HH:MM (24-hour)");

        if (room.size() > 100 || judge.size() > 150 || purpose.size() > 255)
            return jsonError(400, "Court room, judge name or purpose is too long");

        std::vector<cc::Case> found;

        if (!loadCases("WHERE c.id = ?", { PI(caseId) }, found))
            return jsonError(500, "Failed to load case");

        if (found.empty())
            return jsonError(404, "Case not found");

        DbResult ins = dbQuery(
            "INSERT INTO hearings (case_id, hearing_date, hearing_time, court_room, "
            "judge_name, purpose, status) VALUES (?, ?, ?, ?, ?, ?, 'UPCOMING')",
            { PI(caseId), P(date),
              time.empty() ? PNull() : P(time),
              room.empty() ? PNull() : P(room),
              judge.empty() ? PNull() : P(judge),
              purpose.empty() ? PNull() : P(purpose) });

        if (!ins.ok)
            return dbFailure("Failed to schedule hearing", ins);

        const cc::Case& c = found[0];
        std::string when = date + (time.empty() ? "" : " at " + time);

        logAudit(adminId, "HEARING_SCHEDULED",
                 "[" + c.caseNumber() + "] Hearing scheduled for " + when +
                 (room.empty() ? "" : " in " + room));

        notifyUser(c.ownerId(), "Hearing scheduled",
                   "A hearing for case " + c.caseNumber() + " has been scheduled on " + when + ".");

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Hearing scheduled";
        response["hearing_id"] = (int)ins.insertId;
        return crow::response(201, response);
    });


    // ==============================================================
    // ADMIN: update a hearing's status
    // PUT /api/admin/hearings/<id>/status   {"status":"COMPLETED"}
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/hearings/<int>/status")
        .methods("PUT"_method, "OPTIONS"_method)
    ([](const crow::request& req, int hearingId)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ScheduleHearings, adminId, deny))
            return deny;

        auto data = crow::json::load(req.body);
        std::string status;

        if (!data || !jsonGetString(data, "status", status) || !isAllowedHearingStatus(status))
            return jsonError(400, "status must be UPCOMING, COMPLETED or POSTPONED");

        std::vector<cc::Hearing> found;

        if (!loadHearings("WHERE h.id = ?", { PI(hearingId) }, found))
            return jsonError(500, "Failed to load hearing");

        if (found.empty())
            return jsonError(404, "Hearing not found");

        DbResult upd = dbQuery("UPDATE hearings SET status = ? WHERE id = ?",
                               { P(status), PI(hearingId) });

        if (!upd.ok)
            return dbFailure("Failed to update hearing", upd);

        // Look up the case for the audit entry / notification
        DbResult cr = dbQuery(
            "SELECT c.case_number, COALESCE(c.user_id, 0) FROM hearings h "
            "INNER JOIN cases c ON h.case_id = c.id WHERE h.id = ?", { PI(hearingId) });

        if (cr.ok && !cr.rows.empty())
        {
            const std::string number = cr.rows[0][0];
            long long owner = std::atoll(cr.rows[0][1].c_str());

            logAudit(adminId, "HEARING_STATUS_CHANGED",
                     "[" + number + "] Hearing on " + found[0].date() + " changed from " +
                     found[0].status() + " to " + status);

            notifyUser(owner, "Hearing " + status,
                       "Your hearing for case " + number + " on " + found[0].date() +
                       " is now marked " + status + ".");
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Hearing status updated";
        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: update a case's status
    // PUT /api/admin/cases/<id>/status   {"status":"ONGOING"}
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/cases/<int>/status")
        .methods("PUT"_method, "OPTIONS"_method)
    ([](const crow::request& req, int caseId)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageAllCases, adminId, deny))
            return deny;

        auto data = crow::json::load(req.body);
        std::string status;

        if (!data || !jsonGetString(data, "status", status) || !isAllowedCaseStatus(status))
            return jsonError(400, "status must be PENDING, ONGOING, CLOSED or DISMISSED");

        std::vector<cc::Case> found;

        if (!loadCases("WHERE c.id = ?", { PI(caseId) }, found))
            return jsonError(500, "Failed to load case");

        if (found.empty())
            return jsonError(404, "Case not found");

        const cc::Case& c = found[0];

        if (c.status() == status)
            return jsonError(409, "Case already has status " + status);

        DbResult upd = dbQuery("UPDATE cases SET status = ? WHERE id = ?",
                               { P(status), PI(caseId) });

        if (!upd.ok)
            return dbFailure("Failed to update case status", upd);

        logAudit(adminId, "CASE_STATUS_CHANGED",
                 "[" + c.caseNumber() + "] Status changed from " + c.status() + " to " + status);

        notifyUser(c.ownerId(), "Case status updated",
                   "Case " + c.caseNumber() + " (" + c.title() + ") is now " + status + ".");

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Case status updated";
        response["case_status"] = status;
        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: list users
    // GET /api/admin/users
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/users")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageUsers, adminId, deny))
            return deny;

        DbResult r = dbQuery(
            "SELECT u.id, u.full_name, u.email, COALESCE(u.phone, ''), u.role, "
            "COALESCE(DATE_FORMAT(u.created_at, '%Y-%m-%d'), ''), "
            "(SELECT COUNT(*) FROM cases c WHERE c.user_id = u.id) "
            "FROM users u ORDER BY u.id ASC");

        if (!r.ok)
            return dbFailure("Failed to load users", r);

        std::vector<crow::json::wvalue> items;

        for (const auto& row : r.rows)
        {
            crow::json::wvalue item;
            item["id"] = std::atoi(row[0].c_str());
            item["full_name"] = row[1];
            item["email"] = row[2];
            item["phone"] = row[3];
            item["role"] = row[4];
            item["created_at"] = row[5];
            item["case_count"] = std::atoi(row[6].c_str());
            items.push_back(std::move(item));
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["count"] = (int)items.size();
        response["current_admin_id"] = (int)adminId;
        response["users"] = std::move(items);
        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: change a user's role
    // PUT /api/admin/users/<id>/role   {"role":"LAWYER"}
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/users/<int>/role")
        .methods("PUT"_method, "OPTIONS"_method)
    ([](const crow::request& req, int targetId)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageUsers, adminId, deny))
            return deny;

        auto data = crow::json::load(req.body);
        std::string role;

        if (!data || !jsonGetString(data, "role", role) || !isAllowedRole(role))
            return jsonError(400, "role must be USER, LAWYER, JUDGE or ADMIN");

        if ((unsigned long)targetId == adminId)
            return jsonError(400, "You cannot change your own role");

        DbResult cur = dbQuery("SELECT full_name, role FROM users WHERE id = ?", { PI(targetId) });

        if (!cur.ok)
            return dbFailure("Failed to load user", cur);

        if (cur.rows.empty())
            return jsonError(404, "User not found");

        DbResult upd = dbQuery("UPDATE users SET role = ? WHERE id = ?", { P(role), PI(targetId) });

        if (!upd.ok)
            return dbFailure("Failed to update role", upd);

        logAudit(adminId, "USER_ROLE_CHANGED",
                 "User " + cur.rows[0][0] + " (#" + std::to_string(targetId) +
                 ") role changed from " + cur.rows[0][1] + " to " + role);

        notifyUser(targetId, "Role updated",
                   "Your account role is now " + role + ". Log in again to apply it.");

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Role updated";
        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: audit logs
    // GET /api/admin/audit-logs
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/audit-logs")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ViewAuditLogs, adminId, deny))
            return deny;

        DbResult r = dbQuery(
            "SELECT a.id, COALESCE(u.full_name, 'System'), a.action, "
            "COALESCE(a.description, ''), "
            "COALESCE(DATE_FORMAT(a.created_at, '%Y-%m-%d %H:%i:%s'), '') "
            "FROM audit_logs a LEFT JOIN users u ON a.user_id = u.id "
            "ORDER BY a.id DESC LIMIT 200");

        if (!r.ok)
            return dbFailure("Failed to load audit logs", r);

        std::vector<crow::json::wvalue> items;

        for (const auto& row : r.rows)
        {
            crow::json::wvalue item;
            item["id"] = std::atoi(row[0].c_str());
            item["actor"] = row[1];
            item["action"] = row[2];
            item["description"] = row[3];
            item["created_at"] = row[4];
            items.push_back(std::move(item));
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["count"] = (int)items.size();
        response["logs"] = std::move(items);
        return crow::response(200, response);
    });


    // ==============================================================
    // ADMIN: notifications (list recent + send)
    // GET  /api/admin/notifications
    // POST /api/admin/notifications  {"user_id":5|0,"title":"..","message":".."}
    //      user_id 0 = broadcast to every user
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/notifications")
        .methods("GET"_method, "POST"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::SendNotifications, adminId, deny))
            return deny;

        if (req.method == "GET"_method)
        {
            DbResult r = dbQuery(
                "SELECT n.id, n.user_id, COALESCE(u.full_name, ''), n.title, n.message, "
                "n.is_read, COALESCE(DATE_FORMAT(n.created_at, '%Y-%m-%d %H:%i:%s'), '') "
                "FROM notifications n LEFT JOIN users u ON n.user_id = u.id "
                "ORDER BY n.id DESC LIMIT 100");

            if (!r.ok)
                return dbFailure("Failed to load notifications", r);

            std::vector<crow::json::wvalue> items;

            for (const auto& row : r.rows)
            {
                crow::json::wvalue item;
                item["id"] = std::atoi(row[0].c_str());
                item["user_id"] = std::atoi(row[1].c_str());
                item["user_name"] = row[2];
                item["title"] = row[3];
                item["message"] = row[4];
                item["is_read"] = row[5] == "1";
                item["created_at"] = row[6];
                items.push_back(std::move(item));
            }

            crow::json::wvalue response;
            response["status"] = "success";
            response["count"] = (int)items.size();
            response["notifications"] = std::move(items);
            return crow::response(200, response);
        }

        auto data = crow::json::load(req.body);
        long long target = -1;
        std::string title, message;

        if (!data || !jsonGetInt(data, "user_id", target) || target < 0)
            return jsonError(400, "user_id is required (use 0 to send to everyone)");

        if (!jsonGetString(data, "title", title) || !jsonGetString(data, "message", message))
            return jsonError(400, "title and message are required");

        // OOP: an InAppNotification object trims and validates the text
        // (title <= 255, message <= 2000 characters).
        cc::InAppNotification notice(title, message);
        std::string noticeError = notice.validate();

        if (!noticeError.empty())
            return jsonError(400, noticeError);

        title = notice.title();
        message = notice.message();

        DbResult ins;

        if (target == 0)
        {
            ins = dbQuery("INSERT INTO notifications (user_id, title, message) "
                          "SELECT id, ?, ? FROM users", { P(title), P(message) });
        }
        else
        {
            DbResult exists = dbQuery("SELECT id FROM users WHERE id = ?", { PI(target) });

            if (!exists.ok)
                return dbFailure("Failed to check user", exists);

            if (exists.rows.empty())
                return jsonError(404, "User not found");

            ins = dbQuery("INSERT INTO notifications (user_id, title, message) VALUES (?, ?, ?)",
                          { PI(target), P(title), P(message) });
        }

        if (!ins.ok)
            return dbFailure("Failed to send notification", ins);

        logAudit(adminId, "NOTIFICATION_SENT",
                 "Notification \"" + title + "\" sent to " +
                 (target == 0 ? std::string("all users") : "user #" + std::to_string(target)));

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Notification sent";
        response["recipients"] = (int)(target == 0 ? ins.affected : 1);
        return crow::response(201, response);
    });


    // ==============================================================
    // ADMIN: reports (C++ polymorphism: Report -> CaseReport/HearingReport)
    // GET /api/admin/report
    // ==============================================================
    CROW_ROUTE(app, "/api/admin/report")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ViewReports, adminId, deny))
            return deny;

        std::vector<cc::Case> cases;
        std::vector<cc::Hearing> hearings;

        if (!loadCases("", {}, cases) || !loadHearings("", {}, hearings))
            return jsonError(500, "Failed to load data for the report");

        // One container of base-class pointers, each call dispatched
        // to the right subclass at run time.
        std::vector<std::unique_ptr<cc::Report>> reports;
        reports.push_back(std::make_unique<cc::CaseReport>(cases));
        reports.push_back(std::make_unique<cc::HearingReport>(hearings));

        std::vector<crow::json::wvalue> items;
        std::string text;

        for (const auto& report : reports)
        {
            items.push_back(report->toJson());
            text += report->toText() + "\n";
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["reports"] = std::move(items);
        response["text"] = text;
        return crow::response(200, response);
    });


    // ==============================================================
    // PUBLIC: live summary numbers for the home page (no login).
    // Privacy: returns only counts and generic activity labels -- no case
    // numbers, titles, names or emails.
    // GET /api/public/stats
    // ==============================================================
    CROW_ROUTE(app, "/api/public/stats")
        .methods("GET"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        DbResult r = dbQuery(
            "SELECT "
            "(SELECT COUNT(*) FROM cases), "
            "(SELECT COUNT(*) FROM cases WHERE status = 'ONGOING'), "
            "(SELECT COUNT(*) FROM cases WHERE status = 'PENDING'), "
            "(SELECT COUNT(*) FROM cases WHERE status IN ('CLOSED', 'DISMISSED')), "
            "(SELECT COUNT(*) FROM hearings WHERE status = 'UPCOMING' AND hearing_date >= CURDATE()), "
            "(SELECT COUNT(*) FROM hearings WHERE status = 'UPCOMING' "
            "   AND hearing_date BETWEEN CURDATE() AND DATE_ADD(CURDATE(), INTERVAL 7 DAY))");

        if (!r.ok || r.rows.empty())
            return jsonError(500, "Statistics unavailable");

        const auto& row = r.rows[0];
        int total = std::atoi(row[0].c_str());
        int resolved = std::atoi(row[3].c_str());

        crow::json::wvalue response;
        response["status"] = "success";
        response["total_cases"] = total;
        response["active_cases"] = std::atoi(row[1].c_str());
        response["pending_cases"] = std::atoi(row[2].c_str());
        response["resolved_cases"] = resolved;
        response["upcoming_hearings"] = std::atoi(row[4].c_str());
        response["hearings_this_week"] = std::atoi(row[5].c_str());
        response["resolved_percent"] = total > 0 ? (int)((resolved * 100.0) / total + 0.5) : 0;

        DbResult a = dbQuery(
            "SELECT action, DATE_FORMAT(created_at, '%Y-%m-%dT%H:%i:%s') "
            "FROM audit_logs "
            "WHERE action IN ('CASE_CREATED', 'HEARING_SCHEDULED', "
            "                 'CASE_STATUS_CHANGED', 'HEARING_STATUS_CHANGED') "
            "ORDER BY id DESC LIMIT 3");

        std::vector<crow::json::wvalue> recent;

        if (a.ok)
        {
            for (const auto& ar : a.rows)
            {
                crow::json::wvalue item;
                item["action"] = ar[0];
                item["at"] = ar[1];
                recent.push_back(std::move(item));
            }
        }

        response["recent_activity"] = std::move(recent);
        return crow::response(200, response);
    });
}

#endif // CC_EXTRA_ROUTES_HPP
