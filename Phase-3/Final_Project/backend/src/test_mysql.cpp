#include <mysql.h>
#include <iostream>

int main()
{
    std::cout << "Starting MySQL connection test...\n";

    MYSQL* connection = mysql_init(nullptr);

    if (!connection)
    {
        std::cerr << "MySQL initialization failed.\n";
        return 1;
    }

    std::cout << "MySQL initialized successfully.\n";

    MYSQL* result = mysql_real_connect(
        connection,
        "127.0.0.1",
        "root",
        "",
        "court_craft",
        3306,
        nullptr,
        0
    );

    if (!result)
    {
        std::cerr << "MySQL connection failed: " << mysql_error(connection) << "\n";
        mysql_close(connection);
        return 1;
    }

    std::cout << "MySQL connected successfully!\n";
    mysql_close(connection);
    return 0;
}
