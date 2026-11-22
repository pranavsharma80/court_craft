// db_helpers.hpp -- shared helpers used by the route handlers.
// Included by main.cpp AFTER getDbConnection(), jsonError() and the session
// helpers have been defined.
#ifndef CC_DB_HELPERS_HPP
#define CC_DB_HELPERS_HPP

#include <memory>
#include <string>
#include <vector>

// ------------------------------------------------------------------
// Generic prepared-statement runner
//
// Every value reaches MySQL through a bound parameter, never by string
// concatenation, so user input cannot change the structure of a query.
// SELECT results come back as rows of strings (NULL becomes "").
// ------------------------------------------------------------------

struct DbParam
{
    enum Kind { Str, Int, Null } kind = Null;
    std::string s;
    long long i = 0;
};

inline DbParam P(const std::string& v)
{
    DbParam p; p.kind = DbParam::Str; p.s = v; return p;
}

inline DbParam PI(long long v)
{
    DbParam p; p.kind = DbParam::Int; p.i = v; return p;
}

inline DbParam PNull()
{
    return DbParam();
}

struct DbResult
{
    bool ok = false;
    unsigned int errNo = 0;
    std::string error;
    unsigned long long insertId = 0;
    unsigned long long affected = 0;
    std::vector<std::vector<std::string>> rows;
};

inline DbResult dbQuery(const std::string& sql,
                        const std::vector<DbParam>& params = {})
{
    DbResult r;

    MYSQL* conn = getDbConnection();

    if (!conn)
    {
        r.errNo = 2003;
        r.error = "Database connection failed";
        return r;
    }

    mysql_set_character_set(conn, "utf8mb4");

    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    if (!stmt || mysql_stmt_prepare(stmt, sql.c_str(), (unsigned long)sql.length()))
    {
        r.errNo = stmt ? mysql_stmt_errno(stmt) : mysql_errno(conn);
        r.error = stmt ? mysql_stmt_error(stmt) : mysql_error(conn);
        if (stmt) mysql_stmt_close(stmt);
        mysql_close(conn);
        return r;
    }

    const size_t n = params.size();
    std::vector<MYSQL_BIND> binds(n);
    std::vector<long long> ints(n, 0);

    if (n > 0)
        memset(binds.data(), 0, n * sizeof(MYSQL_BIND));

    for (size_t k = 0; k < n; ++k)
    {
        if (params[k].kind == DbParam::Str)
        {
            binds[k].buffer_type = MYSQL_TYPE_STRING;
            binds[k].buffer = (void*)params[k].s.c_str();
            binds[k].buffer_length = (unsigned long)params[k].s.length();
        }
        else if (params[k].kind == DbParam::Int)
        {
            ints[k] = params[k].i;
            binds[k].buffer_type = MYSQL_TYPE_LONGLONG;
            binds[k].buffer = &ints[k];
        }
        else
        {
            binds[k].buffer_type = MYSQL_TYPE_NULL;
        }
    }

    if ((n > 0 && mysql_stmt_bind_param(stmt, binds.data())) ||
        mysql_stmt_execute(stmt))
    {
        r.errNo = mysql_stmt_errno(stmt);
        r.error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return r;
    }

    r.insertId = mysql_stmt_insert_id(stmt);

    unsigned int fields = mysql_stmt_field_count(stmt);

    if (fields == 0)
    {
        r.affected = mysql_stmt_affected_rows(stmt);
        r.ok = true;
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return r;
    }

    const size_t BUF = 8192;

    std::vector<std::vector<char>> buffers(fields, std::vector<char>(BUF));
    std::vector<unsigned long> lengths(fields, 0);
    std::unique_ptr<bool[]> nulls(new bool[fields]());
    std::vector<MYSQL_BIND> out(fields);
    memset(out.data(), 0, fields * sizeof(MYSQL_BIND));

    for (unsigned int c = 0; c < fields; ++c)
    {
        out[c].buffer_type = MYSQL_TYPE_STRING;
        out[c].buffer = buffers[c].data();
        out[c].buffer_length = BUF;
        out[c].length = &lengths[c];
        out[c].is_null = &nulls[c];
    }

    if (mysql_stmt_bind_result(stmt, out.data()))
    {
        r.errNo = mysql_stmt_errno(stmt);
        r.error = mysql_stmt_error(stmt);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return r;
    }

    for (;;)
    {
        int rc = mysql_stmt_fetch(stmt);

        if (rc == MYSQL_NO_DATA)
            break;

        if (rc == 1)
        {
            r.errNo = mysql_stmt_errno(stmt);
            r.error = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            mysql_close(conn);
            return r;
        }

        // rc == 0 or MYSQL_DATA_TRUNCATED: keep what fits in the buffer
        std::vector<std::string> row;
        row.reserve(fields);

        for (unsigned int c = 0; c < fields; ++c)
        {
            if (nulls[c])
                row.emplace_back();
            else
                row.emplace_back(buffers[c].data(),
                                 lengths[c] < BUF ? lengths[c] : BUF);
        }

        r.rows.push_back(std::move(row));
    }

    r.ok = true;
    mysql_stmt_close(stmt);
    mysql_close(conn);
    return r;
}

// ------------------------------------------------------------------
// Audit log + notifications (best effort: a failure here must never
// make the main action fail, so errors are ignored).
// ------------------------------------------------------------------

// Description convention: "[CASE-NUMBER] details" so a case's history can be
// rebuilt from audit_logs.
inline void logAudit(unsigned long userId,
                     const std::string& action,
                     const std::string& description)
{
    dbQuery("INSERT INTO audit_logs (user_id, action, description) VALUES (?, ?, ?)",
            { PI((long long)userId), P(action), P(description) });
}

inline void notifyUser(long long userId,
                       const std::string& title,
                       const std::string& message)
{
    if (userId <= 0)
        return;

    dbQuery("INSERT INTO notifications (user_id, title, message) VALUES (?, ?, ?)",
            { PI(userId), P(title), P(message) });
}

// ------------------------------------------------------------------
// Authorization built on the polymorphic cc::User classes
// ------------------------------------------------------------------

// True if the caller is logged in AND their role has permission `perm`.
// On failure `deny` holds the 401/403 response to return.
inline bool authorize(const crow::request& req,
                      cc::Permission perm,
                      unsigned long& userId,
                      crow::response& deny,
                      const std::string& forbiddenMessage = "")
{
    std::string role;

    if (!getAuthenticatedSession(req, userId, role))
    {
        deny = jsonError(401, "Not authenticated. Please log in again.");
        return false;
    }

    std::unique_ptr<cc::User> user =
        cc::User::create(role, (int)userId, "", "");

    if (!user->hasPermission(perm))
    {
        deny = jsonError(403, forbiddenMessage.empty()
            ? "You do not have permission to perform this action"
            : forbiddenMessage + " (your role: " + user->roleName() + ")");
        return false;
    }

    return true;
}

// Logged in AND (the target user is the caller, or the caller may manage
// all cases). Used by the "per-user" endpoints so one user cannot read
// another user's data by changing the id in the URL.
inline bool authorizeSelfOrAdmin(const crow::request& req,
                                 long long targetUserId,
                                 unsigned long& callerId,
                                 crow::response& deny)
{
    std::string role;

    if (!getAuthenticatedSession(req, callerId, role))
    {
        deny = jsonError(401, "Not authenticated. Please log in again.");
        return false;
    }

    std::unique_ptr<cc::User> user =
        cc::User::create(role, (int)callerId, "", "");

    if ((long long)callerId != targetUserId &&
        !user->hasPermission(cc::Permission::ManageAllCases))
    {
        deny = jsonError(403, "You can only access your own data");
        return false;
    }

    return true;
}

// ------------------------------------------------------------------
// Small JSON helpers (crow's .s() throws on a wrong type)
// ------------------------------------------------------------------

inline bool jsonGetString(const crow::json::rvalue& data,
                          const char* key,
                          std::string& out)
{
    if (!data.has(key) || data[key].t() != crow::json::type::String)
        return false;

    out = data[key].s();
    return true;
}

inline bool jsonGetInt(const crow::json::rvalue& data,
                       const char* key,
                       long long& out)
{
    if (!data.has(key) || data[key].t() != crow::json::type::Number)
        return false;

    out = (long long)data[key].i();
    return true;
}

inline void copyToBuffer(char* dst, size_t cap, const std::string& src)
{
    strncpy(dst, src.c_str(), cap - 1);
    dst[cap - 1] = '\0';
}

#endif // CC_DB_HELPERS_HPP
