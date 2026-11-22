/*
    Domain Header File - CourtCraft
    ===============================

    This file contains the C++ OOP part of the project.

    OOP CONCEPTS USED (good points for the presentation)
    ----------------------------------------------------
    1. Encapsulation   : data members are private, accessed by getters/setters
    2. Abstraction     : Printable, User, Report and Notification are abstract
                         classes (they have pure virtual functions)
    3. Inheritance     : AdminUser, JudgeUser... inherit from User
                         CaseReport, HearingReport inherit from Report
                         InAppNotification, EmailNotification... inherit from Notification
    4. Polymorphism    : the same function call behaves differently for
                         every child class (virtual functions + override)
    5. Interface       : Printable gives describe() to many unrelated classes
    6. Factory pattern : User::create() and Notification::create()
    7. Template Method : Report::toText() and Notification::validate()
                         use virtual functions of the child class
    8. Operator overloading : Case::operator< (priority order), operator==
                         and operator<< (printing)
    9. Static members  : Case::isValidStatus(), Hearing::isValidStatus()
   10. RAII            : std::unique_ptr deletes objects automatically

    CLASS DIAGRAM (simple)
    ----------------------
                       Printable (interface)
         ______________/    |      |       \_____________
        |                   |      |                     |
       User               Case   Hearing               Report (abstract)
    ____|______                                      _____|______
   |  |   |    |                                    |            |
 Admin Judge Lawyer Litigant                   CaseReport  HearingReport

   Notification (abstract)
    ________|___________
   |        |           |
 InApp    Email        Sms
*/

#ifndef CC_DOMAIN_H
#define CC_DOMAIN_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <crow/json.h>

namespace cc
{

// ======================== PRINTABLE (INTERFACE) ========================

// An "interface": it has only a pure virtual function.
// Any class that inherits it MUST write its own describe().
class Printable
{
public:
    virtual ~Printable() {}

    // Returns a one-line text description of the object
    virtual std::string describe() const = 0;
};

// Lets us write:  std::cout << someCase;   (works for every Printable)
inline std::ostream& operator<<(std::ostream& out, const Printable& item)
{
    return out << item.describe();
}


// ======================== USER PERMISSIONS ========================

// Different permissions available in CourtCraft
enum class Permission
{
    ViewOwnCases,
    CreateCase,
    ManageAllCases,
    ManageUsers,
    ScheduleHearings,
    SendNotifications,
    ViewReports,
    ViewAuditLogs
};


// ======================== USER CLASSES ========================

// Base class for all users (abstract because of pure virtual functions)
class User : public Printable
{
public:
    User(int id, std::string fullName, std::string email);

    virtual ~User();

    // Pure virtual functions - every role must write its own version
    virtual std::string roleName() const = 0;
    virtual bool hasPermission(Permission p) const = 0;

    // Same for all roles, so it is written once in the base class
    std::string describe() const override;

    // Getter functions
    int id() const
    {
        return id_;
    }

    const std::string& fullName() const
    {
        return fullName_;
    }

    const std::string& email() const
    {
        return email_;
    }

    // Factory: creates the correct child object according to the role
    static std::unique_ptr<User> create(
        const std::string& role,
        int id,
        const std::string& fullName,
        const std::string& email
    );

private:
    // Private data = encapsulation
    int id_;
    std::string fullName_;
    std::string email_;
};


// Litigant user class (normal user)
class LitigantUser : public User
{
public:
    using User::User;   // use the constructor of the base class

    std::string roleName() const override
    {
        return "USER";
    }

    bool hasPermission(Permission p) const override;
};


// Lawyer user class
class LawyerUser : public User
{
public:
    using User::User;

    std::string roleName() const override
    {
        return "LAWYER";
    }

    bool hasPermission(Permission p) const override;
};


// Judge user class
class JudgeUser : public User
{
public:
    using User::User;

    std::string roleName() const override
    {
        return "JUDGE";
    }

    bool hasPermission(Permission p) const override;
};


// Admin user class
class AdminUser : public User
{
public:
    using User::User;

    std::string roleName() const override
    {
        return "ADMIN";
    }

    bool hasPermission(Permission p) const override;
};


// ======================== CASE CLASS ========================

class Case : public Printable
{
public:
    // Constructor
    Case(int id, std::string caseNumber, std::string title,
         std::string description, std::string caseType, std::string status,
         std::string filingDate, int ownerId, std::string ownerName,
         std::string ownerEmail, std::string createdAt);

    // Create Case object from database row
    static Case fromRow(const std::vector<std::string>& row);

    // Allowed status values: PENDING, ONGOING, CLOSED, DISMISSED
    static bool isValidStatus(const std::string& status);

    // Getter functions
    int id() const
    {
        return id_;
    }

    const std::string& caseNumber() const
    {
        return caseNumber_;
    }

    const std::string& title() const
    {
        return title_;
    }

    const std::string& description() const
    {
        return description_;
    }

    const std::string& caseType() const
    {
        return caseType_;
    }

    const std::string& status() const
    {
        return status_;
    }

    const std::string& filingDate() const
    {
        return filingDate_;
    }

    int ownerId() const
    {
        return ownerId_;
    }

    const std::string& ownerName() const
    {
        return ownerName_;
    }

    const std::string& ownerEmail() const
    {
        return ownerEmail_;
    }

    const std::string& createdAt() const
    {
        return createdAt_;
    }

    // Setter with checking: returns false and keeps the old value if invalid
    bool setStatus(const std::string& newStatus);

    // Check whether case is open (PENDING or ONGOING)
    bool isOpen() const;

    // Check whether case is finished (CLOSED or DISMISSED)
    bool isClosed() const;

    // Get case priority (1 = most urgent) using the C module
    int priority() const;

    // Priority as a word: Critical, High, Medium, Low, Normal
    std::string priorityLabel() const;

    // Convert case into JSON
    crow::json::wvalue toJson() const;

    // Printable interface
    std::string describe() const override;

    // Operator overloading:
    //   a < b  means "a is more urgent than b"
    //   a == b means "same database id"
    bool operator<(const Case& other) const;
    bool operator==(const Case& other) const;

private:
    int id_;
    std::string caseNumber_;
    std::string title_;
    std::string description_;
    std::string caseType_;
    std::string status_;
    std::string filingDate_;
    int ownerId_;
    std::string ownerName_;
    std::string ownerEmail_;
    std::string createdAt_;
};


// ======================== HEARING CLASS ========================

class Hearing : public Printable
{
public:
    // Constructor
    Hearing(int id, int caseId, std::string caseNumber,
            std::string caseTitle, std::string date, std::string time,
            std::string courtRoom, std::string judgeName,
            std::string purpose, std::string status);

    // Create Hearing object from database row
    static Hearing fromRow(const std::vector<std::string>& row);

    // Allowed status values: UPCOMING, COMPLETED, POSTPONED
    static bool isValidStatus(const std::string& status);

    // Getter functions
    int id() const
    {
        return id_;
    }

    int caseId() const
    {
        return caseId_;
    }

    const std::string& caseNumber() const
    {
        return caseNumber_;
    }

    const std::string& caseTitle() const
    {
        return caseTitle_;
    }

    const std::string& date() const
    {
        return date_;
    }

    const std::string& time() const
    {
        return time_;
    }

    const std::string& courtRoom() const
    {
        return courtRoom_;
    }

    const std::string& judgeName() const
    {
        return judgeName_;
    }

    const std::string& purpose() const
    {
        return purpose_;
    }

    const std::string& status() const
    {
        return status_;
    }

    // Setter with checking: returns false and keeps the old value if invalid
    bool setStatus(const std::string& newStatus);

    // Check whether hearing is upcoming
    bool isUpcoming() const
    {
        return status_ == "UPCOMING";
    }

    // Convert hearing into JSON
    crow::json::wvalue toJson() const;

    // Printable interface
    std::string describe() const override;

    // a < b means "a happens earlier than b" (date, then time)
    bool operator<(const Hearing& other) const;

private:
    int id_;
    int caseId_;
    std::string caseNumber_;
    std::string caseTitle_;
    std::string date_;
    std::string time_;
    std::string courtRoom_;
    std::string judgeName_;
    std::string purpose_;
    std::string status_;
};


// ======================== REPORT CLASS ========================

// Stores one line of a report
struct ReportLine
{
    std::string label;
    std::string value;
};


// Base class for all reports (abstract)
class Report : public Printable
{
public:
    virtual ~Report();

    // Pure virtual functions - child classes decide the content
    virtual std::string title() const = 0;
    virtual std::vector<ReportLine> lines() const = 0;

    // Template Method: these functions are written once here
    // but call the virtual functions of the child class.
    crow::json::wvalue toJson() const;
    std::string toText() const;

    // Printable interface (short summary line)
    std::string describe() const override;
};


// Case report class
class CaseReport : public Report
{
public:
    explicit CaseReport(const std::vector<Case>& cases);

    std::string title() const override
    {
        return "Case Status Report";
    }

    std::vector<ReportLine> lines() const override;

private:
    std::vector<Case> cases_;
};


// Hearing report class
class HearingReport : public Report
{
public:
    explicit HearingReport(const std::vector<Hearing>& hearings);

    std::string title() const override
    {
        return "Hearing Schedule Report";
    }

    std::vector<ReportLine> lines() const override;

private:
    std::vector<Hearing> hearings_;
};


// ======================== NOTIFICATION CLASSES ========================

// Base class for all notifications (abstract).
// Each child class is a different way of sending the same message.
class Notification : public Printable
{
public:
    Notification(std::string title, std::string message);

    virtual ~Notification();

    // Pure virtual functions - every channel is different
    virtual std::string channel() const = 0;    // e.g. "IN_APP"
    virtual std::string format() const = 0;     // final text to be delivered

    // Virtual with a default value - SMS overrides it (shorter limit)
    virtual size_t maxMessageLength() const;

    // Template Method: checks the common rules and uses
    // maxMessageLength() of the child class.
    // Returns "" if valid, otherwise the error message.
    std::string validate() const;

    // Printable interface
    std::string describe() const override;

    const std::string& title() const
    {
        return title_;
    }

    const std::string& message() const
    {
        return message_;
    }

    // Factory: channel = "IN_APP", "EMAIL" or "SMS" (nullptr if unknown)
    static std::unique_ptr<Notification> create(
        const std::string& channel,
        const std::string& title,
        const std::string& message
    );

private:
    std::string title_;
    std::string message_;
};


// Notification shown inside the CourtCraft website
class InAppNotification : public Notification
{
public:
    using Notification::Notification;

    std::string channel() const override
    {
        return "IN_APP";
    }

    std::string format() const override;
};


// Notification sent as an email (has subject and body)
class EmailNotification : public Notification
{
public:
    using Notification::Notification;

    std::string channel() const override
    {
        return "EMAIL";
    }

    std::string format() const override;
};


// Notification sent as an SMS (very short)
class SmsNotification : public Notification
{
public:
    using Notification::Notification;

    std::string channel() const override
    {
        return "SMS";
    }

    // SMS can have only 160 characters
    size_t maxMessageLength() const override
    {
        return 160;
    }

    std::string format() const override;
};

} // namespace cc

#endif
