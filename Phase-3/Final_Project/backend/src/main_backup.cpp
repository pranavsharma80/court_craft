#include <crow.h>
#include <mysql.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <random>
#include <cstring>

const char* DB_HOST = "127.0.0.1";
const char* DB_USER = "root";
const char* DB_PASSWORD = "";
const char* DB_NAME = "court_craft";
const unsigned int DB_PORT = 3306;

// Generate a random salt (hex string)
std::string generateSalt(size_t length = 32)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    for (size_t i = 0; i < length; ++i) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

// Connect to database and return connection
MYSQL* getDbConnection()
{
    MYSQL* connection = mysql_init(nullptr);
    if (!connection) {
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
    
    if (!result) {
        mysql_close(connection);
        return nullptr;
    }
    
    return connection;
}

// Check if email already exists in users table
bool emailExists(const std::string& email, MYSQL* conn)
{
    std::string query = "SELECT id FROM users WHERE email = ? LIMIT 1";
    
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND bind[1];
    memset(bind, 0, sizeof(bind));
    
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)email.c_str();
    bind[0].buffer_length = email.length();
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND result_bind[1];
    memset(result_bind, 0, sizeof(result_bind));
    
    unsigned long id;
    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &id;
    
    mysql_stmt_bind_result(stmt, result_bind);
    
    bool exists = (mysql_stmt_fetch(stmt) == 0);
    
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    return exists;
}

// Insert new user into database
// passwordHash should be in format "salt:hash" (salt and hash separated by colon)
bool insertUser(const std::string& fullName, const std::string& email, 
                const std::string& phone, const std::string& passwordHashWithSalt, MYSQL* conn, unsigned long& newUserId)
{
    std::string query = "INSERT INTO users (full_name, email, phone, password_hash, role) VALUES (?, ?, ?, ?, 'USER')";
    
    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt) {
        return false;
    }
    
    if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));
    
    // full_name
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void*)fullName.c_str();
    bind[0].buffer_length = fullName.length();
    
    // email
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void*)email.c_str();
    bind[1].buffer_length = email.length();
    
    // phone
    bind[2].buffer_type = MYSQL_TYPE_STRING;
    bind[2].buffer = (void*)phone.c_str();
    bind[2].buffer_length = phone.length();
    
    // password_hash (includes salt: "salt:hash")
    bind[3].buffer_type = MYSQL_TYPE_STRING;
    bind[3].buffer = (void*)passwordHashWithSalt.c_str();
    bind[3].buffer_length = passwordHashWithSalt.length();
    
    if (mysql_stmt_bind_param(stmt, bind)) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    if (mysql_stmt_execute(stmt)) {
        mysql_stmt_close(stmt);
        return false;
    }
    
    newUserId = mysql_insert_id(conn);
    mysql_stmt_close(stmt);
    
    return true;
}

bool testDatabase()
{
    std::cerr << "testDatabase called\n";
    fflush(stderr);

    MYSQL* connection = mysql_init(nullptr);

    if (!connection)
    {
        std::cerr << "MySQL initialization failed.\n";
        fflush(stderr);
        return false;
    }

    std::cerr << "MySQL initialized\n";
    fflush(stderr);

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
        std::cerr << "MySQL connection failed: "
                  << mysql_error(connection) << "\n";
        fflush(stderr);

        mysql_close(connection);
        return false;
    }

    std::cerr << "MySQL connected successfully!\n";
    fflush(stderr);

    mysql_close(connection);
    return true;
}

// CORS Middleware
struct CorsMiddleware
{
    struct context {};

    void before_handle(crow::request& req, crow::response& res, context& ctx) {}

    void after_handle(crow::request& req, crow::response& res, context& ctx)
    {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.add_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }
};

int main()
{
    crow::App<CorsMiddleware> app;

    // Handle OPTIONS preflight requests
    CROW_ROUTE(app, "/api/register").methods("OPTIONS"_method)
    ([]
    {
        return crow::response(200);
    });

    CROW_ROUTE(app, "/")
    ([] {
        return "CourtCraft API is running!";
    });

    CROW_ROUTE(app, "/api/health")
    ([] {
        crow::json::wvalue response;

        response["status"] = "ok";
        response["message"] = "CourtCraft backend is healthy";

        return response;
    });

    CROW_ROUTE(app, "/api/db-test")
    ([] {
        crow::json::wvalue response;

        if (testDatabase())
        {
            response["status"] = "connected";
            response["database"] = "court_craft";
        }
        else
        {
            response["status"] = "failed";
            response["database"] = "court_craft";
        }

        return response;
    });

    CROW_ROUTE(app, "/api/register")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto data = crow::json::load(req.body);
        
        // Validate required fields
        if (!data || !data.has("full_name") || !data.has("email") || 
            !data.has("phone") || !data.has("password")) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Missing required fields: full_name, email, phone, password";
            return crow::response(400, error);
        }
        
        std::string fullName = data["full_name"].s();
        std::string email = data["email"].s();
        std::string phone = data["phone"].s();
        std::string password = data["password"].s();
        
        // Validate email format (basic check)
        if (email.find('@') == std::string::npos || email.find('.') == std::string::npos) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Invalid email format";
            return crow::response(400, error);
        }
        
        // Validate password strength (minimum 8 characters)
        if (password.length() < 8) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Password must be at least 8 characters long";
            return crow::response(400, error);
        }
        
        // Connect to database
        MYSQL* conn = getDbConnection();
        if (!conn) {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Database connection failed";
            return crow::response(500, error);
        }
        
        // Check if email already exists
        if (emailExists(email, conn)) {
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Email already registered";
            return crow::response(409, error);
        }
        
        // Generate salt and compute password hash using MySQL's SHA2
        std::string salt = generateSalt(16);
        
        // Query: SELECT SHA2(CONCAT(?, ?), 256) to hash password + salt
        std::string hashQuery = "SELECT SHA2(CONCAT(?, ?), 256) as password_hash";
        MYSQL_STMT* hashStmt = mysql_stmt_init(conn);
        if (!hashStmt) {
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        if (mysql_stmt_prepare(hashStmt, hashQuery.c_str(), hashQuery.length())) {
            mysql_stmt_close(hashStmt);
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        MYSQL_BIND hashBind[2];
        memset(hashBind, 0, sizeof(hashBind));
        
        hashBind[0].buffer_type = MYSQL_TYPE_STRING;
        hashBind[0].buffer = (void*)password.c_str();
        hashBind[0].buffer_length = password.length();
        
        hashBind[1].buffer_type = MYSQL_TYPE_STRING;
        hashBind[1].buffer = (void*)salt.c_str();
        hashBind[1].buffer_length = salt.length();
        
        if (mysql_stmt_bind_param(hashStmt, hashBind)) {
            mysql_stmt_close(hashStmt);
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        if (mysql_stmt_execute(hashStmt)) {
            mysql_stmt_close(hashStmt);
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        char passwordHash[65] = {0};
        MYSQL_BIND resultBind[1];
        memset(resultBind, 0, sizeof(resultBind));
        
        resultBind[0].buffer_type = MYSQL_TYPE_STRING;
        resultBind[0].buffer = passwordHash;
        resultBind[0].buffer_length = sizeof(passwordHash) - 1;
        
        if (mysql_stmt_bind_result(hashStmt, resultBind)) {
            mysql_stmt_close(hashStmt);
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        if (mysql_stmt_fetch(hashStmt) != 0) {
            mysql_stmt_close(hashStmt);
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Hashing failed";
            return crow::response(500, error);
        }
        
        mysql_stmt_close(hashStmt);
        
        std::string hash(passwordHash);
        
        // Combine salt and hash: "salt:hash" format for storage
        std::string passwordHashWithSalt = salt + ":" + hash;
        
        // Insert user into database
        unsigned long newUserId;
        if (!insertUser(fullName, email, phone, passwordHashWithSalt, conn, newUserId)) {
            mysql_close(conn);
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Failed to create user";
            return crow::response(500, error);
        }
        
        mysql_close(conn);
        
        // Return success response with safe user info (NO password or hash)
        crow::json::wvalue response;
        response["status"] = "success";
        response["message"] = "User registered successfully";
        response["user"]["id"] = (int)newUserId;
        response["user"]["full_name"] = fullName;
        response["user"]["email"] = email;
        response["user"]["phone"] = phone;
        response["user"]["role"] = "USER";
        
        return crow::response(201, response);
    });

    app.port(8080).multithreaded().run();

    return 0;
}