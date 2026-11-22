#include <crow.h>
#include <mysql.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <cctype>

#include "oop/domain.h"

extern "C" {
#include "c_modules/case_validation.h"
#include "c_modules/case_list.h"
#include "c_modules/hearing_queue.h"
#include "c_modules/priority_queue.h"
#include "c_modules/history_stack.h"
}

// Database settings. Each value can be overridden with an environment
// variable (COURTCRAFT_DB_HOST / _USER / _PASSWORD / _NAME / _PORT) so the
// password does not have to live in source control. The defaults keep the
// existing local setup working.
static std::string envOr(const char* name, const char* fallback)
{
    const char* value = std::getenv(name);
    return (value && *value) ? std::string(value) : std::string(fallback);
}

static const std::string DB_HOST_STR     = envOr("COURTCRAFT_DB_HOST", "127.0.0.1");
static const std::string DB_USER_STR     = envOr("COURTCRAFT_DB_USER", "root");
static const std::string DB_PASSWORD_STR = envOr("COURTCRAFT_DB_PASSWORD", "");
static const std::string DB_NAME_STR     = envOr("COURTCRAFT_DB_NAME", "court_craft");

const char* DB_HOST = DB_HOST_STR.c_str();
const char* DB_USER = DB_USER_STR.c_str();
const char* DB_PASSWORD = DB_PASSWORD_STR.c_str();
const char* DB_NAME = DB_NAME_STR.c_str();
const unsigned int DB_PORT =
    (unsigned int)std::atoi(envOr("COURTCRAFT_DB_PORT", "3306").c_str());


// ============================================================
// Generate random salt
// ============================================================

std::string generateSalt(size_t length = 16)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);

    std::stringstream ss;

    for (size_t i = 0; i < length; ++i)
    {
        ss << std::hex << dis(gen);
    }

    return ss.str();
}


// ============================================================
// Database Connection
// ============================================================

MYSQL* getDbConnection()
{
    MYSQL* connection = mysql_init(nullptr);

    if (!connection)
    {
        return nullptr;
    }

    MYSQL* result = mysql_real_connect(
        connection,
        DB_HOST,
        DB_USER,
        DB_PASSWORD,
        DB_NAME,
        DB_PORT,
        nullptr,
        0
    );

    if (!result)
    {
        mysql_close(connection);
        return nullptr;
    }

    return connection;
}


// ============================================================
// Check Email Exists
// ============================================================

bool emailExists(const std::string& email, MYSQL* conn)
{
    std::string query =
        "SELECT id FROM users WHERE email = ? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    if (!stmt)
        return false;

    if (mysql_stmt_prepare(
            stmt,
            query.c_str(),
            query.length()))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)email.c_str();
    bind[0].buffer_length = email.length();

    if (mysql_stmt_bind_param(stmt, bind))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_RES* result =
        mysql_stmt_result_metadata(stmt);

    if (!result)
    {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));

    unsigned long id;

    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &id;

    mysql_stmt_bind_result(stmt, result_bind);

    bool exists =
        (mysql_stmt_fetch(stmt) == 0);

    mysql_free_result(result);
    mysql_stmt_close(stmt);

    return exists;
}


// ============================================================
// Insert User
// ============================================================

bool insertUser(
    const std::string& fullName,
    const std::string& email,
    const std::string& phone,
    const std::string& passwordHashWithSalt,
    MYSQL* conn,
    unsigned long& newUserId)
{
    std::string query =
        "INSERT INTO users "
        "(full_name, email, phone, password_hash, role) "
        "VALUES (?, ?, ?, ?, 'USER')";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    if (!stmt)
        return false;

    if (mysql_stmt_prepare(
            stmt,
            query.c_str(),
            query.length()))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)fullName.c_str();
    bind[0].buffer_length = fullName.length();

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)email.c_str();
    bind[1].buffer_length = email.length();

    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)phone.c_str();
    bind[2].buffer_length = phone.length();

    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (void*)passwordHashWithSalt.c_str();
    bind[3].buffer_length =
        passwordHashWithSalt.length();

    if (mysql_stmt_bind_param(stmt, bind))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt))
    {
        mysql_stmt_close(stmt);
        return false;
    }

    newUserId = mysql_insert_id(conn);

    mysql_stmt_close(stmt);

    return true;
}


// ============================================================
// Hash Password Using MySQL SHA2
// ============================================================

bool createPasswordHash(
    const std::string& password,
    const std::string& salt,
    std::string& finalHash)
{
    MYSQL* conn = getDbConnection();

    if (!conn)
        return false;

    std::string query =
        "SELECT SHA2(CONCAT(?, ?), 256)";

    MYSQL_STMT* stmt =
        mysql_stmt_init(conn);

    if (!stmt)
    {
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_prepare(
            stmt,
            query.c_str(),
            query.length()))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)password.c_str();
    bind[0].buffer_length = password.length();

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)salt.c_str();
    bind[1].buffer_length = salt.length();

    if (mysql_stmt_bind_param(stmt, bind))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_execute(stmt))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    char hashBuffer[65] = {0};

    MYSQL_BIND resultBind[1];
    memset(resultBind, 0, sizeof(resultBind));

    resultBind[0].buffer_type = MYSQL_TYPE_STRING;
    resultBind[0].buffer = hashBuffer;
    resultBind[0].buffer_length = 64;

    if (mysql_stmt_bind_result(
            stmt,
            resultBind))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_fetch(stmt) != 0)
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    finalHash = std::string(hashBuffer);

    mysql_stmt_close(stmt);
    mysql_close(conn);

    return true;
}


// ============================================================
// Verify Login Password
// ============================================================

bool verifyPassword(
    const std::string& password,
    const std::string& storedPassword)
{
    // Stored format:
    // salt:hash

    size_t separator =
        storedPassword.find(':');

    if (separator == std::string::npos)
        return false;

    std::string salt =
        storedPassword.substr(0, separator);

    std::string storedHash =
        storedPassword.substr(separator + 1);

    std::string calculatedHash;

    if (!createPasswordHash(
            password,
            salt,
            calculatedHash))
    {
        return false;
    }

    return calculatedHash == storedHash;
}


// ============================================================
// Get User By Email
// ============================================================

bool getUserByEmail(
    const std::string& email,
    unsigned long& userId,
    std::string& fullName,
    std::string& userEmail,
    std::string& phone,
    std::string& passwordHash,
    std::string& role)
{
    MYSQL* conn = getDbConnection();

    if (!conn)
        return false;

    std::string query =
        "SELECT id, full_name, email, phone, "
        "password_hash, role "
        "FROM users WHERE email = ? LIMIT 1";

    MYSQL_STMT* stmt =
        mysql_stmt_init(conn);

    if (!stmt)
    {
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_prepare(
            stmt,
            query.c_str(),
            query.length()))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    MYSQL_BIND param[1];
    memset(param, 0, sizeof(param));

    param[0].buffer_type =
        MYSQL_TYPE_STRING;

    param[0].buffer =
        (void*)email.c_str();

    param[0].buffer_length =
        email.length();

    if (mysql_stmt_bind_param(
            stmt,
            param))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_execute(stmt))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    MYSQL_RES* metadata =
        mysql_stmt_result_metadata(stmt);

    if (!metadata)
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    char nameBuffer[101] = {0};
    char emailBuffer[151] = {0};
    char phoneBuffer[21] = {0};
    char passwordBuffer[256] = {0};
    char roleBuffer[20] = {0};

    unsigned long id = 0;

    unsigned long nameLength = 0;
    unsigned long emailLength = 0;
    unsigned long phoneLength = 0;
    unsigned long passwordLength = 0;
    unsigned long roleLength = 0;

    MYSQL_BIND result[6];
    memset(result, 0, sizeof(result));

    result[0].buffer_type =
        MYSQL_TYPE_LONG;

    result[0].buffer =
        &id;

    result[1].buffer_type =
        MYSQL_TYPE_STRING;

    result[1].buffer =
        nameBuffer;

    result[1].buffer_length =
        sizeof(nameBuffer);

    result[1].length =
        &nameLength;

    result[2].buffer_type =
        MYSQL_TYPE_STRING;

    result[2].buffer =
        emailBuffer;

    result[2].buffer_length =
        sizeof(emailBuffer);

    result[2].length =
        &emailLength;

    result[3].buffer_type =
        MYSQL_TYPE_STRING;

    result[3].buffer =
        phoneBuffer;

    result[3].buffer_length =
        sizeof(phoneBuffer);

    result[3].length =
        &phoneLength;

    result[4].buffer_type =
        MYSQL_TYPE_STRING;

    result[4].buffer =
        passwordBuffer;

    result[4].buffer_length =
        sizeof(passwordBuffer);

    result[4].length =
        &passwordLength;

    result[5].buffer_type =
        MYSQL_TYPE_STRING;

    result[5].buffer =
        roleBuffer;

    result[5].buffer_length =
        sizeof(roleBuffer);

    result[5].length =
        &roleLength;

    if (mysql_stmt_bind_result(
            stmt,
            result))
    {
        mysql_free_result(metadata);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    if (mysql_stmt_fetch(stmt) != 0)
    {
        mysql_free_result(metadata);
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return false;
    }

    userId = id;
    fullName =
        std::string(nameBuffer, nameLength);

    userEmail =
        std::string(emailBuffer, emailLength);

    phone =
        std::string(phoneBuffer, phoneLength);

    passwordHash =
        std::string(
            passwordBuffer,
            passwordLength);

    role =
        std::string(roleBuffer, roleLength);

    mysql_free_result(metadata);
    mysql_stmt_close(stmt);
    mysql_close(conn);

    return true;
}


// ============================================================
// Database Test
// ============================================================

bool testDatabase()
{
    MYSQL* connection =
        mysql_init(nullptr);

    if (!connection)
        return false;

    MYSQL* result =
        mysql_real_connect(
            connection,
            DB_HOST,
            DB_USER,
            DB_PASSWORD,
            DB_NAME,
            DB_PORT,
            nullptr,
            0
        );

    if (!result)
    {
        mysql_close(connection);
        return false;
    }

    mysql_close(connection);

    return true;
}


// ============================================================
// JSON error helper
// ============================================================

crow::response jsonError(int code, const std::string& message)
{
    crow::json::wvalue error;
    error["status"] = "error";
    error["message"] = message;
    return crow::response(code, error);
}


// ============================================================
// Server-side sessions (in memory)
// Login issues a random token. Protected routes derive the
// user from "Authorization: Bearer <token>", never from the
// URL or request body. Sessions are lost on server restart.
// ============================================================

struct SessionInfo
{
    unsigned long userId;
    std::string role;
    std::chrono::steady_clock::time_point expiresAt;
};

static std::mutex g_sessionMutex;
static std::unordered_map<std::string, SessionInfo> g_sessions;
static const std::chrono::hours SESSION_LIFETIME(8);

std::string generateToken()
{
    std::random_device rd;
    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    for (int i = 0; i < 8; ++i)
        ss << std::setw(8) << rd();

    return ss.str();   // 64 hex chars = 256 bits
}

std::string createSession(unsigned long userId, const std::string& role)
{
    std::string token = generateToken();

    std::lock_guard<std::mutex> lock(g_sessionMutex);

    auto now = std::chrono::steady_clock::now();

    for (auto it = g_sessions.begin(); it != g_sessions.end();)
    {
        if (it->second.expiresAt < now)
            it = g_sessions.erase(it);
        else
            ++it;
    }

    g_sessions[token] = { userId, role, now + SESSION_LIFETIME };
    return token;
}

std::string extractBearerToken(const crow::request& req)
{
    const std::string prefix = "Bearer ";
    std::string header = req.get_header_value("Authorization");

    if (header.size() <= prefix.size() ||
        header.compare(0, prefix.size(), prefix) != 0)
        return "";

    return header.substr(prefix.size());
}

bool getAuthenticatedSession(
    const crow::request& req,
    unsigned long& userId,
    std::string& role)
{
    std::string token = extractBearerToken(req);

    if (token.empty())
        return false;

    std::lock_guard<std::mutex> lock(g_sessionMutex);

    auto it = g_sessions.find(token);

    if (it == g_sessions.end())
        return false;

    if (it->second.expiresAt < std::chrono::steady_clock::now())
    {
        g_sessions.erase(it);
        return false;
    }

    userId = it->second.userId;
    role = it->second.role;
    return true;
}

bool getAuthenticatedUserId(const crow::request& req, unsigned long& userId)
{
    std::string role;
    return getAuthenticatedSession(req, userId, role);
}

void destroySession(const std::string& token)
{
    std::lock_guard<std::mutex> lock(g_sessionMutex);
    g_sessions.erase(token);
}


// ============================================================
// Profile helpers
// ============================================================

struct UserProfile
{
    unsigned long id = 0;
    std::string fullName;
    std::string email;
    std::string phone;
    std::string role;
    std::string createdAt;
};

std::string trimString(const std::string& s)
{
    size_t start = 0;
    size_t end = s.size();

    while (start < end && std::isspace((unsigned char)s[start])) ++start;
    while (end > start && std::isspace((unsigned char)s[end - 1])) --end;

    return s.substr(start, end - start);
}

bool isValidEmail(const std::string& email)
{
    if (email.empty() || email.size() > 150)
        return false;

    size_t at = email.find('@');

    if (at == std::string::npos || at == 0 ||
        email.find('@', at + 1) != std::string::npos)
        return false;

    size_t dot = email.find('.', at + 1);

    if (dot == std::string::npos || dot == at + 1 ||
        dot == email.size() - 1)
        return false;

    for (char c : email)
        if (std::isspace((unsigned char)c))
            return false;

    return true;
}

// Empty is allowed (phone column is nullable); otherwise 10-15 of 0-9 + - space
bool isValidPhone(const std::string& phone)
{
    if (phone.empty())
        return true;

    if (phone.size() < 10 || phone.size() > 15)
        return false;

    for (char c : phone)
        if (!std::isdigit((unsigned char)c) &&
            c != '+' && c != '-' && c != ' ')
            return false;

    return true;
}

// Returns 0 = found, 1 = not found, 2 = database error
int getUserProfileById(unsigned long userId, UserProfile& profile)
{
    MYSQL* conn = getDbConnection();

    if (!conn)
        return 2;

    const char* query =
        "SELECT id, full_name, email, phone, role, "
        "DATE_FORMAT(created_at, '%Y-%m-%d') "
        "FROM users WHERE id = ? LIMIT 1";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    if (!stmt || mysql_stmt_prepare(stmt, query, strlen(query)))
    {
        if (stmt) mysql_stmt_close(stmt);
        mysql_close(conn);
        return 2;
    }

    int idParam = (int)userId;

    MYSQL_BIND param[1];
    memset(param, 0, sizeof(param));
    param[0].buffer_type = MYSQL_TYPE_LONG;
    param[0].buffer = &idParam;

    if (mysql_stmt_bind_param(stmt, param) || mysql_stmt_execute(stmt))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 2;
    }

    int id = 0;
    char nameBuf[101] = {0};
    char emailBuf[151] = {0};
    char phoneBuf[21] = {0};
    char roleBuf[21] = {0};
    char createdBuf[21] = {0};

    unsigned long len[6] = {0};
    bool isNull[6] = {false};

    MYSQL_BIND res[6];
    memset(res, 0, sizeof(res));

    res[0].buffer_type = MYSQL_TYPE_LONG;
    res[0].buffer = &id;
    res[0].is_null = &isNull[0];
    res[0].length = &len[0];

    char* bufs[5] = { nameBuf, emailBuf, phoneBuf, roleBuf, createdBuf };
    size_t sizes[5] = { sizeof(nameBuf), sizeof(emailBuf), sizeof(phoneBuf),
                        sizeof(roleBuf), sizeof(createdBuf) };

    for (int i = 0; i < 5; ++i)
    {
        res[i + 1].buffer_type = MYSQL_TYPE_STRING;
        res[i + 1].buffer = bufs[i];
        res[i + 1].buffer_length = sizes[i] - 1;
        res[i + 1].is_null = &isNull[i + 1];
        res[i + 1].length = &len[i + 1];
    }

    if (mysql_stmt_bind_result(stmt, res))
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 2;
    }

    int rc = mysql_stmt_fetch(stmt);

    if (rc == MYSQL_NO_DATA)
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 1;
    }

    if (rc == 1)
    {
        mysql_stmt_close(stmt);
        mysql_close(conn);
        return 2;
    }

    profile.id = (unsigned long)id;
    profile.fullName  = isNull[1] ? "" : std::string(nameBuf,    std::min(len[1], (unsigned long)100));
    profile.email     = isNull[2] ? "" : std::string(emailBuf,   std::min(len[2], (unsigned long)150));
    profile.phone     = isNull[3] ? "" : std::string(phoneBuf,   std::min(len[3], (unsigned long)20));
    profile.role      = isNull[4] ? "" : std::string(roleBuf,    std::min(len[4], (unsigned long)20));
    profile.createdAt = isNull[5] ? "" : std::string(createdBuf, std::min(len[5], (unsigned long)20));

    mysql_stmt_close(stmt);
    mysql_close(conn);
    return 0;
}

// Returns 0 = updated, 1 = duplicate email, 2 = database error
int updateUserProfile(
    unsigned long userId,
    const std::string& fullName,
    const std::string& email,
    const std::string& phone)
{
    MYSQL* conn = getDbConnection();

    if (!conn)
        return 2;

    const char* query =
        "UPDATE users SET full_name = ?, email = ?, phone = ? "
        "WHERE id = ?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);

    if (!stmt || mysql_stmt_prepare(stmt, query, strlen(query)))
    {
        if (stmt) mysql_stmt_close(stmt);
        mysql_close(conn);
        return 2;
    }

    int idParam = (int)userId;
    bool phoneIsNull = phone.empty();

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)fullName.c_str();
    bind[0].buffer_length = fullName.length();

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)email.c_str();
    bind[1].buffer_length = email.length();

    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)phone.c_str();
    bind[2].buffer_length = phone.length();
    bind[2].is_null = &phoneIsNull;

    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &idParam;

    int result = 0;

    if (mysql_stmt_bind_param(stmt, bind) || mysql_stmt_execute(stmt))
        result = (mysql_stmt_errno(stmt) == 1062) ? 1 : 2;

    mysql_stmt_close(stmt);
    mysql_close(conn);
    return result;
}

crow::json::wvalue profileToJson(const UserProfile& p)
{
    crow::json::wvalue user;
    user["id"] = (int)p.id;
    user["full_name"] = p.fullName;
    user["email"] = p.email;
    user["phone"] = p.phone;
    user["role"] = p.role;
    user["created_at"] = p.createdAt;
    return user;
}


#include "db_helpers.hpp"


// ============================================================
// CORS
// ============================================================

struct CorsMiddleware
{
    struct context {};

    void before_handle(
        crow::request& req,
        crow::response& res,
        context& ctx)
    {
    }

    void after_handle(
        crow::request& req,
        crow::response& res,
        context& ctx)
    {
        res.add_header(
            "Access-Control-Allow-Origin",
            "*"
        );

        res.add_header(
            "Access-Control-Allow-Methods",
            "GET, POST, PUT, DELETE, OPTIONS"
        );

        res.add_header(
            "Access-Control-Allow-Headers",
            "Content-Type, Authorization"
        );
    }
};


#include "extra_routes.hpp"


// ============================================================
// MAIN
// ============================================================

int main()
{
    crow::App<CorsMiddleware> app;

    // search, history, priority queue, hearing queue, admin modules, reports
    registerExtraRoutes(app);


    // --------------------------------------------------------
    // OPTIONS
    // --------------------------------------------------------

    CROW_ROUTE(app, "/api/register")
        .methods("OPTIONS"_method)
    ([] {
        return crow::response(200);
    });


    CROW_ROUTE(app, "/api/login")
        .methods("OPTIONS"_method)
    ([] {
        return crow::response(200);
    });


    // --------------------------------------------------------
    // Home
    // --------------------------------------------------------

    CROW_ROUTE(app, "/")
    ([] {
        return "CourtCraft API is running!";
    });


    // --------------------------------------------------------
    // Health
    // --------------------------------------------------------

    CROW_ROUTE(app, "/api/health")
    ([] {

        crow::json::wvalue response;

        response["status"] = "ok";
        response["message"] =
            "CourtCraft backend is healthy";

        return response;
    });


    // --------------------------------------------------------
    // DB Test
    // --------------------------------------------------------

    CROW_ROUTE(app, "/api/db-test")
    ([] {

        crow::json::wvalue response;

        if (testDatabase())
        {
            response["status"] =
                "connected";

            response["database"] =
                "court_craft";
        }
        else
        {
            response["status"] =
                "failed";

            response["database"] =
                "court_craft";
        }

        return response;
    });


    // ========================================================
    // REGISTER
    // ========================================================

    CROW_ROUTE(app, "/api/register")
        .methods("POST"_method)
    ([](const crow::request& req)
    {

        auto data =
            crow::json::load(req.body);

        if (!data ||
            !data.has("full_name") ||
            !data.has("email") ||
            !data.has("phone") ||
            !data.has("password"))
        {
            crow::json::wvalue error;

            error["status"] = "error";

            error["message"] =
                "Missing required fields";

            return crow::response(
                400,
                error
            );
        }


        std::string fullName =
            data["full_name"].s();

        std::string email =
            data["email"].s();

        std::string phone =
            data["phone"].s();

        std::string password =
            data["password"].s();


        if (email.find('@') ==
                std::string::npos ||
            email.find('.') ==
                std::string::npos)
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Invalid email format";

            return crow::response(
                400,
                error
            );
        }


        if (password.length() < 8)
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Password must be at least 8 characters long";

            return crow::response(
                400,
                error
            );
        }


        MYSQL* conn =
            getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Database connection failed";

            return crow::response(
                500,
                error
            );
        }


        if (emailExists(
                email,
                conn))
        {
            mysql_close(conn);

            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Email already registered";

            return crow::response(
                409,
                error
            );
        }


        std::string salt =
            generateSalt();


        std::string hash;


        if (!createPasswordHash(
                password,
                salt,
                hash))
        {
            mysql_close(conn);

            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Password hashing failed";

            return crow::response(
                500,
                error
            );
        }


        std::string storedPassword =
            salt + ":" + hash;


        unsigned long newUserId = 0;


        if (!insertUser(
                fullName,
                email,
                phone,
                storedPassword,
                conn,
                newUserId))
        {
            mysql_close(conn);

            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Failed to create user";

            return crow::response(
                500,
                error
            );
        }


        mysql_close(conn);


        crow::json::wvalue response;

        response["status"] =
            "success";

        response["message"] =
            "User registered successfully";

        response["user"]["id"] =
            (int)newUserId;

        response["user"]["full_name"] =
            fullName;

        response["user"]["email"] =
            email;

        response["user"]["phone"] =
            phone;

        response["user"]["role"] =
            "USER";


        return crow::response(
            201,
            response
        );
    });


    // ========================================================
    // LOGIN
    // ========================================================

    CROW_ROUTE(app, "/api/login")
        .methods("POST"_method)
    ([](const crow::request& req)
    {

        auto data =
            crow::json::load(req.body);


        if (!data ||
            !data.has("email") ||
            !data.has("password"))
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Email and password are required";

            return crow::response(
                400,
                error
            );
        }


        std::string email =
            data["email"].s();

        std::string password =
            data["password"].s();


        if (email.empty() ||
            password.empty())
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Email and password cannot be empty";

            return crow::response(
                400,
                error
            );
        }


        unsigned long userId = 0;

        std::string fullName;
        std::string userEmail;
        std::string phone;
        std::string passwordHash;
        std::string role;


        bool found =
            getUserByEmail(
                email,
                userId,
                fullName,
                userEmail,
                phone,
                passwordHash,
                role
            );


        if (!found)
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Invalid email or password";

            return crow::response(
                401,
                error
            );
        }


        bool valid =
            verifyPassword(
                password,
                passwordHash
            );


        if (!valid)
        {
            crow::json::wvalue error;

            error["status"] =
                "error";

            error["message"] =
                "Invalid email or password";

            return crow::response(
                401,
                error
            );
        }


        // ----------------------------------------------------
        // Login successful
        // ----------------------------------------------------

        crow::json::wvalue response;

        response["status"] =
            "success";

        response["message"] =
            "Login successful";


        response["user"]["id"] =
            (int)userId;

        response["user"]["full_name"] =
            fullName;

        response["user"]["email"] =
            userEmail;

        response["user"]["phone"] =
            phone;

        response["user"]["role"] =
            role;

        response["token"] =
            createSession(userId, role);


        return crow::response(
            200,
            response
        );
    });


    // ========================================================
    // LOGOUT  (invalidates the session token)
    // ========================================================

    CROW_ROUTE(app, "/api/logout")
        .methods("POST"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        std::string token = extractBearerToken(req);

        if (!token.empty())
            destroySession(token);

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Logged out";
        return crow::response(200, response);
    });


    // ========================================================
    // PROFILE - GET / PUT  (identity comes from the token only)
    // ========================================================

    CROW_ROUTE(app, "/api/profile")
        .methods("GET"_method, "PUT"_method, "OPTIONS"_method)
    ([](const crow::request& req)
    {
        if (req.method == "OPTIONS"_method)
            return crow::response(200);

        unsigned long userId = 0;

        if (!getAuthenticatedUserId(req, userId))
            return jsonError(401, "Not authenticated. Please log in again.");

        // ---------------- GET ----------------
        if (req.method == "GET"_method)
        {
            UserProfile profile;
            int rc = getUserProfileById(userId, profile);

            if (rc == 1)
                return jsonError(404, "User not found");

            if (rc != 0)
                return jsonError(500, "Database error while loading profile");

            crow::json::wvalue response;
            response["status"] = "success";
            response["user"] = profileToJson(profile);
            return crow::response(200, response);
        }

        // ---------------- PUT ----------------
        auto data = crow::json::load(req.body);

        if (!data ||
            !data.has("full_name") || data["full_name"].t() != crow::json::type::String ||
            !data.has("email")     || data["email"].t()     != crow::json::type::String ||
            !data.has("phone")     || data["phone"].t()     != crow::json::type::String)
        {
            return jsonError(400, "full_name, email and phone are required");
        }

        std::string fullName = trimString(data["full_name"].s());
        std::string email    = trimString(data["email"].s());
        std::string phone    = trimString(data["phone"].s());

        std::transform(email.begin(), email.end(), email.begin(),
            [](unsigned char c) { return (char)std::tolower(c); });

        if (fullName.empty() || fullName.size() > 100)
            return jsonError(400, "Full name is required (max 100 characters)");

        if (!isValidEmail(email))
            return jsonError(400, "Invalid email format");

        if (!isValidPhone(phone))
            return jsonError(400, "Phone must be 10-15 characters: digits, +, - or spaces");

        int rc = updateUserProfile(userId, fullName, email, phone);

        if (rc == 1)
            return jsonError(409, "Email already registered to another account");

        if (rc != 0)
            return jsonError(500, "Database error while saving profile");

        // Re-read so the response reflects what is really stored in MySQL
        UserProfile profile;
        int readRc = getUserProfileById(userId, profile);

        if (readRc == 1)
            return jsonError(404, "User not found");

        if (readRc != 0)
            return jsonError(500, "Profile saved but could not be re-read");

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Profile updated successfully";
        response["user"] = profileToJson(profile);
        return crow::response(200, response);
    });



    // ========================================================
    // CASES - CREATE
    // ========================================================

    CROW_ROUTE(app, "/api/cases")
        .methods("OPTIONS"_method)
    ([] {
        return crow::response(200);
    });


    CROW_ROUTE(app, "/api/cases")
        .methods("POST"_method)
    ([](const crow::request& req)
    {
        // The case owner is the logged-in user (from the token), never
        // a user_id sent by the client.
        unsigned long authUserId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::CreateCase, authUserId, deny,
                       "Only litigants, lawyers and administrators can register cases"))
            return deny;

        auto data = crow::json::load(req.body);

        std::string caseNumber, title, description, caseType, filingDate;

        if (!data ||
            !jsonGetString(data, "case_number", caseNumber) ||
            !jsonGetString(data, "title", title) ||
            !jsonGetString(data, "case_type", caseType) ||
            !jsonGetString(data, "filing_date", filingDate))
        {
            return jsonError(400, "case_number, title, case_type and filing_date are required");
        }

        jsonGetString(data, "description", description);

        caseNumber = trimString(caseNumber);
        title = trimString(title);
        description = trimString(description);
        caseType = trimString(caseType);
        filingDate = trimString(filingDate);

        std::transform(caseNumber.begin(), caseNumber.end(), caseNumber.begin(),
            [](unsigned char c) { return (char)std::toupper(c); });

        int userId = (int)authUserId;

        // Validation implemented in the C module
        char validationError[CC_ERR_BUF] = {0};

        if (!cc_validate_case_id(caseNumber.c_str(), validationError, sizeof validationError))
            return jsonError(400, validationError);

        if (title.empty() || title.size() > 255)
            return jsonError(400, "Title is required (max 255 characters)");

        if (description.size() > 5000)
            return jsonError(400, "Description must be at most 5000 characters");

        if (!cc_validate_case_type(caseType.c_str()))
            return jsonError(400, "Case type must be 1-50 letters, digits, spaces or hyphens");

        if (!cc_validate_date(filingDate.c_str()))
            return jsonError(400, "Filing date must be a valid date (YYYY-MM-DD)");

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "INSERT INTO cases "
            "(case_number, title, description, case_type, status, filing_date, user_id) "
            "VALUES (?, ?, ?, ?, 'PENDING', ?, ?)";

        MYSQL_STMT* stmt = mysql_stmt_init(conn);

        if (!stmt ||
            mysql_stmt_prepare(stmt, query.c_str(), query.length()))
        {
            if (stmt) mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to prepare case creation";
            return crow::response(500, error);
        }

        MYSQL_BIND bind[6];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = (void*)caseNumber.c_str();
        bind[0].buffer_length = caseNumber.length();

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)title.c_str();
        bind[1].buffer_length = title.length();

        bind[2].buffer_type = MYSQL_TYPE_STRING;
        bind[2].buffer = (void*)description.c_str();
        bind[2].buffer_length = description.length();

        bind[3].buffer_type = MYSQL_TYPE_STRING;
        bind[3].buffer = (void*)caseType.c_str();
        bind[3].buffer_length = caseType.length();

        bind[4].buffer_type = MYSQL_TYPE_STRING;
        bind[4].buffer = (void*)filingDate.c_str();
        bind[4].buffer_length = filingDate.length();

        bind[5].buffer_type = MYSQL_TYPE_LONG;
        bind[5].buffer = &userId;

        if (mysql_stmt_bind_param(stmt, bind) ||
            mysql_stmt_execute(stmt))
        {
            unsigned int mysqlError = mysql_stmt_errno(stmt);

            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";

            if (mysqlError == 1062)
                error["message"] = "Case number already exists";
            else if (mysqlError == 1452)
                error["message"] = "User does not exist";
            else
                error["message"] = "Failed to create case";

            return crow::response(
                mysqlError == 1062 ? 409 : 500,
                error
            );
        }

        unsigned long newCaseId = mysql_insert_id(conn);

        mysql_stmt_close(stmt);
        mysql_close(conn);

        logAudit(authUserId, "CASE_CREATED",
                 "[" + caseNumber + "] Case registered: " + title);

        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "Case created successfully";
        response["case"]["id"] = (int)newCaseId;
        response["case"]["case_number"] = caseNumber;
        response["case"]["title"] = title;
        response["case"]["description"] = description;
        response["case"]["case_type"] = caseType;
        response["case"]["status"] = "PENDING";
        response["case"]["filing_date"] = filingDate;
        response["case"]["user_id"] = userId;

        return crow::response(201, response);
    });


    // ========================================================
    // CASES - GET ALL CASES FOR A USER
    // ========================================================

    CROW_ROUTE(app, "/api/cases/<int>")
        .methods("GET"_method)
    ([](const crow::request& req, int userId)
    {
        unsigned long callerId = 0;
        crow::response deny;

        if (!authorizeSelfOrAdmin(req, userId, callerId, deny))
            return deny;

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "SELECT id, case_number, title, description, case_type, "
            "status, DATE_FORMAT(filing_date, '%Y-%m-%d'), user_id "
            "FROM cases WHERE user_id = ? ORDER BY created_at DESC";

        MYSQL_STMT* stmt = mysql_stmt_init(conn);

        if (!stmt ||
            mysql_stmt_prepare(stmt, query.c_str(), query.length()))
        {
            if (stmt) mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to prepare case query";
            return crow::response(500, error);
        }

        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));

        param[0].buffer_type = MYSQL_TYPE_LONG;
        param[0].buffer = &userId;

        if (mysql_stmt_bind_param(stmt, param) ||
            mysql_stmt_execute(stmt))
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to fetch cases";
            return crow::response(500, error);
        }

        MYSQL_RES* metadata = mysql_stmt_result_metadata(stmt);

        if (!metadata)
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to read case data";
            return crow::response(500, error);
        }

        MYSQL_BIND result[8];
        memset(result, 0, sizeof(result));

        unsigned long id = 0;
        int resultUserId = 0;

        char caseNumber[51] = {0};
        char title[256] = {0};
        char description[5001] = {0};
        char caseType[101] = {0};
        char status[20] = {0};
        char filingDate[11] = {0};

        unsigned long lengths[8] = {0};

        result[0].buffer_type = MYSQL_TYPE_LONG;
        result[0].buffer = &id;
        result[0].length = &lengths[0];

        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = caseNumber;
        result[1].buffer_length = sizeof(caseNumber);
        result[1].length = &lengths[1];

        result[2].buffer_type = MYSQL_TYPE_STRING;
        result[2].buffer = title;
        result[2].buffer_length = sizeof(title);
        result[2].length = &lengths[2];

        result[3].buffer_type = MYSQL_TYPE_STRING;
        result[3].buffer = description;
        result[3].buffer_length = sizeof(description);
        result[3].length = &lengths[3];

        result[4].buffer_type = MYSQL_TYPE_STRING;
        result[4].buffer = caseType;
        result[4].buffer_length = sizeof(caseType);
        result[4].length = &lengths[4];

        result[5].buffer_type = MYSQL_TYPE_STRING;
        result[5].buffer = status;
        result[5].buffer_length = sizeof(status);
        result[5].length = &lengths[5];

        result[6].buffer_type = MYSQL_TYPE_STRING;
        result[6].buffer = filingDate;
        result[6].buffer_length = sizeof(filingDate);
        result[6].length = &lengths[6];

        result[7].buffer_type = MYSQL_TYPE_LONG;
        result[7].buffer = &resultUserId;
        result[7].length = &lengths[7];

        if (mysql_stmt_bind_result(stmt, result))
        {
            mysql_free_result(metadata);
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to bind case results";
            return crow::response(500, error);
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["cases"] = crow::json::wvalue::list();

        int index = 0;

        while (mysql_stmt_fetch(stmt) == 0)
        {
            crow::json::wvalue item;

            item["id"] = (int)id;
            item["case_number"] =
                std::string(caseNumber, lengths[1]);
            item["title"] =
                std::string(title, lengths[2]);
            item["description"] =
                std::string(description, lengths[3]);
            item["case_type"] =
                std::string(caseType, lengths[4]);
            item["status"] =
                std::string(status, lengths[5]);
            item["filing_date"] =
                std::string(filingDate, lengths[6]);
            item["user_id"] = resultUserId;

            response["cases"][index++] = std::move(item);
        }

        response["count"] = index;

        mysql_free_result(metadata);
        mysql_stmt_close(stmt);
        mysql_close(conn);

        return crow::response(200, response);
    });


    // ========================================================
    // ADMIN - GET DASHBOARD STATS
    // ========================================================

    CROW_ROUTE(app, "/api/admin/stats")
        .methods("GET"_method)
    ([](const crow::request& req)
    {
        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageAllCases, adminId, deny))
            return deny;

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "SELECT "
            "(SELECT COUNT(*) FROM users) AS total_users, "
            "(SELECT COUNT(*) FROM cases) AS total_cases, "
            "(SELECT COUNT(*) FROM hearings) AS total_hearings, "
            "(SELECT COUNT(*) FROM cases WHERE status = 'ONGOING') AS active_cases";

        if (mysql_query(conn, query.c_str()) != 0)
        {
            std::string dbError = mysql_error(conn);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to fetch admin statistics: " + dbError;
            return crow::response(500, error);
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (!result)
        {
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to read admin statistics";
            return crow::response(500, error);
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (!row)
        {
            mysql_free_result(result);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "No statistics available";
            return crow::response(500, error);
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["stats"]["total_users"] =
            std::stoi(row[0] ? row[0] : "0");
        response["stats"]["total_cases"] =
            std::stoi(row[1] ? row[1] : "0");
        response["stats"]["total_hearings"] =
            std::stoi(row[2] ? row[2] : "0");
        response["stats"]["active_cases"] =
            std::stoi(row[3] ? row[3] : "0");

        mysql_free_result(result);
        mysql_close(conn);

        return crow::response(200, response);
    });


    // ========================================================
    // ADMIN - GET ALL CASES
    // ========================================================

    CROW_ROUTE(app, "/api/admin/cases")
        .methods("GET"_method)
    ([](const crow::request& req)
    {
        unsigned long adminId = 0;
        crow::response deny;

        if (!authorize(req, cc::Permission::ManageAllCases, adminId, deny))
            return deny;

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "SELECT "
            "c.id, "
            "c.case_number, "
            "c.title, "
            "c.description, "
            "c.case_type, "
            "c.status, "
            "DATE_FORMAT(c.filing_date, '%Y-%m-%d'), "
            "c.user_id, "
            "COALESCE(u.full_name, 'Unassigned'), "
            "COALESCE(u.email, ''), "
            "DATE_FORMAT(c.created_at, '%Y-%m-%d %H:%i:%s') "
            "FROM cases c "
            "LEFT JOIN users u ON c.user_id = u.id "
            "ORDER BY c.created_at DESC";

        if (mysql_query(conn, query.c_str()) != 0)
        {
            std::string dbError = mysql_error(conn);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to fetch cases: " + dbError;
            return crow::response(500, error);
        }

        MYSQL_RES* result = mysql_store_result(conn);

        if (!result)
        {
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to read cases";
            return crow::response(500, error);
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["count"] = static_cast<int>(mysql_num_rows(result));

        crow::json::wvalue::list cases;

        MYSQL_ROW row;

        while ((row = mysql_fetch_row(result)))
        {
            crow::json::wvalue item;

            item["id"] = std::stoi(row[0] ? row[0] : "0");
            item["case_number"] = row[1] ? row[1] : "";
            item["title"] = row[2] ? row[2] : "";
            item["description"] = row[3] ? row[3] : "";
            item["case_type"] = row[4] ? row[4] : "";
            item["status"] = row[5] ? row[5] : "";
            item["filing_date"] = row[6] ? row[6] : "";
            item["user_id"] = std::stoi(row[7] ? row[7] : "0");
            item["user_name"] = row[8] ? row[8] : "Unassigned";
            item["user_email"] = row[9] ? row[9] : "";
            item["created_at"] = row[10] ? row[10] : "";

            cases.push_back(std::move(item));
        }

        response["cases"] = std::move(cases);

        mysql_free_result(result);
        mysql_close(conn);

        return crow::response(200, response);
    });

    // ========================================================
    // HEARINGS - GET ALL HEARINGS FOR A USER
    // ========================================================

    CROW_ROUTE(app, "/api/hearings/<int>")
        .methods("GET"_method)
    ([](const crow::request& req, int userId)
    {
        unsigned long callerId = 0;
        crow::response deny;

        if (!authorizeSelfOrAdmin(req, userId, callerId, deny))
            return deny;

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "SELECT h.id, c.case_number, c.title, "
            "DATE_FORMAT(h.hearing_date, '%Y-%m-%d'), "
            "TIME_FORMAT(h.hearing_time, '%H:%i'), "
            "h.court_room, h.judge_name, h.purpose, h.status "
            "FROM hearings h "
            "INNER JOIN cases c ON h.case_id = c.id "
            "WHERE c.user_id = ? "
            "ORDER BY h.hearing_date ASC, h.hearing_time ASC";

        MYSQL_STMT* stmt = mysql_stmt_init(conn);

        if (!stmt ||
            mysql_stmt_prepare(stmt, query.c_str(), query.length()))
        {
            if (stmt) mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to prepare hearing query";
            return crow::response(500, error);
        }

        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));

        param[0].buffer_type = MYSQL_TYPE_LONG;
        param[0].buffer = &userId;

        if (mysql_stmt_bind_param(stmt, param) ||
            mysql_stmt_execute(stmt))
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to fetch hearings";
            return crow::response(500, error);
        }

        MYSQL_RES* metadata = mysql_stmt_result_metadata(stmt);

        if (!metadata)
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to read hearing data";
            return crow::response(500, error);
        }

        MYSQL_BIND result[9];
        memset(result, 0, sizeof(result));

        unsigned long id = 0;

        char caseNumber[51] = {0};
        char title[256] = {0};
        char hearingDate[11] = {0};
        char hearingTime[9] = {0};
        char courtRoom[101] = {0};
        char judgeName[151] = {0};
        char purpose[256] = {0};
        char status[20] = {0};

        unsigned long lengths[9] = {0};

        result[0].buffer_type = MYSQL_TYPE_LONG;
        result[0].buffer = &id;
        result[0].length = &lengths[0];

        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = caseNumber;
        result[1].buffer_length = sizeof(caseNumber);
        result[1].length = &lengths[1];

        result[2].buffer_type = MYSQL_TYPE_STRING;
        result[2].buffer = title;
        result[2].buffer_length = sizeof(title);
        result[2].length = &lengths[2];

        result[3].buffer_type = MYSQL_TYPE_STRING;
        result[3].buffer = hearingDate;
        result[3].buffer_length = sizeof(hearingDate);
        result[3].length = &lengths[3];

        result[4].buffer_type = MYSQL_TYPE_STRING;
        result[4].buffer = hearingTime;
        result[4].buffer_length = sizeof(hearingTime);
        result[4].length = &lengths[4];

        result[5].buffer_type = MYSQL_TYPE_STRING;
        result[5].buffer = courtRoom;
        result[5].buffer_length = sizeof(courtRoom);
        result[5].length = &lengths[5];

        result[6].buffer_type = MYSQL_TYPE_STRING;
        result[6].buffer = judgeName;
        result[6].buffer_length = sizeof(judgeName);
        result[6].length = &lengths[6];

        result[7].buffer_type = MYSQL_TYPE_STRING;
        result[7].buffer = purpose;
        result[7].buffer_length = sizeof(purpose);
        result[7].length = &lengths[7];

        result[8].buffer_type = MYSQL_TYPE_STRING;
        result[8].buffer = status;
        result[8].buffer_length = sizeof(status);
        result[8].length = &lengths[8];

        if (mysql_stmt_bind_result(stmt, result))
        {
            mysql_free_result(metadata);
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to bind hearing results";
            return crow::response(500, error);
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["hearings"] = crow::json::wvalue::list();

        int index = 0;

        while (mysql_stmt_fetch(stmt) == 0)
        {
            crow::json::wvalue item;

            item["id"] = (int)id;
            item["case_number"] = std::string(caseNumber, lengths[1]);
            item["title"] = std::string(title, lengths[2]);
            item["hearing_date"] = std::string(hearingDate, lengths[3]);
            item["hearing_time"] = std::string(hearingTime, lengths[4]);
            item["court_room"] = std::string(courtRoom, lengths[5]);
            item["judge_name"] = std::string(judgeName, lengths[6]);
            item["purpose"] = std::string(purpose, lengths[7]);
            item["status"] = std::string(status, lengths[8]);

            response["hearings"][index++] = std::move(item);
        }

        response["count"] = index;

        mysql_free_result(metadata);
        mysql_stmt_close(stmt);
        mysql_close(conn);

        return crow::response(200, response);
    });




    // ========================================================
    // NOTIFICATIONS - GET ALL NOTIFICATIONS FOR A USER
    // ========================================================

    CROW_ROUTE(app, "/api/notifications/<int>")
        .methods("GET"_method)
    ([](const crow::request& req, int userId)
    {
        unsigned long callerId = 0;
        crow::response deny;

        if (!authorizeSelfOrAdmin(req, userId, callerId, deny))
            return deny;

        MYSQL* conn = getDbConnection();

        if (!conn)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }

        std::string query =
            "SELECT id, title, message, is_read, "
            "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') "
            "FROM notifications "
            "WHERE user_id = ? "
            "ORDER BY created_at DESC";

        MYSQL_STMT* stmt = mysql_stmt_init(conn);

        if (!stmt ||
            mysql_stmt_prepare(stmt, query.c_str(), query.length()))
        {
            if (stmt) mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to prepare notification query";
            return crow::response(500, error);
        }

        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));

        param[0].buffer_type = MYSQL_TYPE_LONG;
        param[0].buffer = &userId;

        if (mysql_stmt_bind_param(stmt, param) ||
            mysql_stmt_execute(stmt))
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to fetch notifications";
            return crow::response(500, error);
        }

        MYSQL_RES* metadata = mysql_stmt_result_metadata(stmt);

        if (!metadata)
        {
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to read notification data";
            return crow::response(500, error);
        }

        MYSQL_BIND result[5];
        memset(result, 0, sizeof(result));

        unsigned long id = 0;
        int isRead = 0;

        char title[256] = {0};
        char message[5001] = {0};
        char createdAt[20] = {0};

        unsigned long lengths[5] = {0};

        result[0].buffer_type = MYSQL_TYPE_LONG;
        result[0].buffer = &id;
        result[0].length = &lengths[0];

        result[1].buffer_type = MYSQL_TYPE_STRING;
        result[1].buffer = title;
        result[1].buffer_length = sizeof(title);
        result[1].length = &lengths[1];

        result[2].buffer_type = MYSQL_TYPE_STRING;
        result[2].buffer = message;
        result[2].buffer_length = sizeof(message);
        result[2].length = &lengths[2];

        result[3].buffer_type = MYSQL_TYPE_TINY;
        result[3].buffer = &isRead;
        result[3].length = &lengths[3];

        result[4].buffer_type = MYSQL_TYPE_STRING;
        result[4].buffer = createdAt;
        result[4].buffer_length = sizeof(createdAt);
        result[4].length = &lengths[4];

        if (mysql_stmt_bind_result(stmt, result))
        {
            mysql_free_result(metadata);
            mysql_stmt_close(stmt);
            mysql_close(conn);

            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to bind notification results";
            return crow::response(500, error);
        }

        crow::json::wvalue response;
        response["status"] = "success";
        response["notifications"] = crow::json::wvalue::list();

        int index = 0;
        int unreadCount = 0;

        while (mysql_stmt_fetch(stmt) == 0)
        {
            crow::json::wvalue item;

            item["id"] = (int)id;
            item["title"] = std::string(title, lengths[1]);
            item["message"] = std::string(message, lengths[2]);
            item["is_read"] = (isRead != 0);
            item["created_at"] = std::string(createdAt, lengths[4]);

            if (isRead == 0)
                unreadCount++;

            response["notifications"][index++] = std::move(item);
        }

        response["count"] = index;
        response["unread_count"] = unreadCount;

        mysql_free_result(metadata);
        mysql_stmt_close(stmt);
        mysql_close(conn);

        return crow::response(200, response);
    });


    // ========================================================
    // START SERVER
    // ========================================================

    std::cout
        << "CourtCraft backend running on port 8080..."
        << std::endl;

    app.port(8080)
       .multithreaded()
       .run();


    return 0;
}
