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
const char* DB_PASSWORD = "184141";
const char* DB_NAME = "court_craft";
const unsigned int DB_PORT = 3306;


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


// ============================================================
// MAIN
// ============================================================

int main()
{
    crow::App<CorsMiddleware> app;


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


        return crow::response(
            200,
            response
        );
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
        auto data = crow::json::load(req.body);

        if (!data ||
            !data.has("case_number") ||
            !data.has("title") ||
            !data.has("description") ||
            !data.has("case_type") ||
            !data.has("filing_date") ||
            !data.has("user_id"))
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "All case fields are required";
            return crow::response(400, error);
        }

        std::string caseNumber = data["case_number"].s();
        std::string title = data["title"].s();
        std::string description = data["description"].s();
        std::string caseType = data["case_type"].s();
        std::string filingDate = data["filing_date"].s();
        int userId = data["user_id"].i();

        if (caseNumber.empty() || title.empty() ||
            description.empty() || caseType.empty() ||
            filingDate.empty() || userId <= 0)
        {
            crow::json::wvalue error;
            error["status"] = "error";
            error["message"] = "Invalid case information";
            return crow::response(400, error);
        }

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
    ([](int userId)
    {
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
    ([]()
    {
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
    ([]()
    {
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
    ([](int userId)
    {
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
    ([](int userId)
    {
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
