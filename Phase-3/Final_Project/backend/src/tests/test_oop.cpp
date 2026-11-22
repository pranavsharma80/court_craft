/*
 * test_oop.cpp -- self-checking tests for the C++ OOP classes.
 * Build target: test_oop      Run: build\test_oop.exe
 * Exits with code 0 only if every check passes.
 */
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

#include "../oop/domain.h"

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                    \
    do {                                                               \
        g_checks++;                                                    \
        if (!(cond)) {                                                 \
            g_failed++;                                                \
            std::cout << "  FAIL  line " << __LINE__ << ": " #cond "\n"; \
        }                                                              \
    } while (0)

static cc::Case makeCase(int id, const char* type, const char* status, const char* date)
{
    return cc::Case(id, "CC-" + std::to_string(id), "Title", "Desc", type,
                    status, date, 1, "Owner", "o@x.com", "2026-01-01 10:00:00");
}

int main()
{
    std::cout << "[users: factory + polymorphism]\n";
    std::vector<std::unique_ptr<cc::User>> users;
    users.push_back(cc::User::create("ADMIN", 1, "A", "a@x"));
    users.push_back(cc::User::create("JUDGE", 2, "J", "j@x"));
    users.push_back(cc::User::create("LAWYER", 3, "L", "l@x"));
    users.push_back(cc::User::create("whatever", 4, "U", "u@x"));
    CHECK(users[0]->roleName() == "ADMIN");
    CHECK(users[3]->roleName() == "USER");          // unknown role -> litigant
    CHECK(users[0]->hasPermission(cc::Permission::ManageUsers));
    CHECK(!users[1]->hasPermission(cc::Permission::ManageUsers));
    CHECK(users[1]->hasPermission(cc::Permission::ViewReports));
    CHECK(!users[2]->hasPermission(cc::Permission::ViewReports));
    CHECK(users[3]->hasPermission(cc::Permission::CreateCase));
    CHECK(users[0]->describe() == "ADMIN #1: A (a@x)");

    std::cout << "[case]\n";
    cc::Case c = makeCase(1, "Criminal", "PENDING", "2026-01-05");
    CHECK(c.isOpen() && !c.isClosed());
    CHECK(c.priority() == 1 && c.priorityLabel() == "Critical");
    CHECK(!c.setStatus("BOGUS") && c.status() == "PENDING");   // rejected, unchanged
    CHECK(c.setStatus("CLOSED") && c.isClosed() && !c.isOpen());
    CHECK(cc::Case::isValidStatus("DISMISSED"));
    CHECK(!cc::Case::isValidStatus("pending"));

    std::cout << "[case: operator< and operator==]\n";
    cc::Case civil = makeCase(2, "Civil", "PENDING", "2026-01-01");
    cc::Case crim  = makeCase(3, "Criminal", "PENDING", "2026-03-01");
    cc::Case crimOld = makeCase(4, "Criminal", "PENDING", "2026-02-01");
    CHECK(crim < civil);                 // lower priority number first
    CHECK(crimOld < crim);               // same priority -> older date first
    CHECK(civil == makeCase(2, "X", "PENDING", ""));   // same id
    std::vector<cc::Case> list = {civil, crim, crimOld};
    std::sort(list.begin(), list.end());
    CHECK(list[0].id() == 4 && list[1].id() == 3 && list[2].id() == 2);

    std::cout << "[hearing]\n";
    cc::Hearing h1(1, 1, "CC-1", "T", "2026-05-02", "10:00", "R1", "J", "P", "UPCOMING");
    cc::Hearing h2(2, 1, "CC-1", "T", "2026-05-01", "11:00", "R1", "J", "P", "UPCOMING");
    CHECK(h2 < h1);
    CHECK(h1.isUpcoming());
    CHECK(!h1.setStatus("X") && h1.setStatus("POSTPONED") && !h1.isUpcoming());
    CHECK(h1.describe() == "CC-1 on 2026-05-02 at 10:00 in R1 (POSTPONED)");

    std::cout << "[reports: polymorphism + template method]\n";
    std::vector<std::unique_ptr<cc::Report>> reports;
    reports.push_back(std::make_unique<cc::CaseReport>(list));
    reports.push_back(std::make_unique<cc::HearingReport>(std::vector<cc::Hearing>{h1, h2}));
    CHECK(reports[0]->title() == "Case Status Report");
    CHECK(reports[1]->title() == "Hearing Schedule Report");
    CHECK(reports[0]->toText().find("Total cases: 3") != std::string::npos);
    CHECK(reports[0]->toText().find("Urgent open cases (priority 1-2): 2") != std::string::npos);
    CHECK(reports[1]->toText().find("Next upcoming hearing date: 2026-05-01") != std::string::npos);

    std::cout << "[printable interface via operator<<]\n";
    std::cout << "   " << users[0]->describe() << "\n   " << c << "\n   " << *reports[0] << "\n";
    CHECK(c.describe().find("CC-1 | Title | CLOSED") == 0);

    std::cout << "[notifications: factory + polymorphism]\n";
    auto inApp = cc::Notification::create("IN_APP", "  Hi  ", "Hello");
    auto email = cc::Notification::create("EMAIL", "Hearing", "Tomorrow 10:00");
    auto sms   = cc::Notification::create("SMS", "Alert", std::string(161, 'x'));
    CHECK(inApp && email && sms);
    CHECK(cc::Notification::create("FAX", "a", "b") == nullptr);
    CHECK(inApp->title() == "Hi");                       // trimmed
    CHECK(inApp->validate().empty());
    CHECK(email->format().find("Subject: Hearing") == 0);
    CHECK(!sms->validate().empty());                     // 161 > 160 limit
    CHECK(sms->maxMessageLength() == 160 && inApp->maxMessageLength() == 2000);
    CHECK(cc::SmsNotification("T", std::string(200, 'y')).format().size() == 160);
    CHECK(cc::InAppNotification("", "m").validate() == "Title is required (max 255 characters)");
    CHECK(cc::InAppNotification("t", std::string(2001, 'm')).validate() ==
          "Message is required (max 2000 characters)");
    CHECK(email->describe() == "[EMAIL] Hearing");

    std::cout << "\n" << g_checks << " checks, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
