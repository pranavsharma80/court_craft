#include "domain.h"

#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Include C validation module (C code used inside C++)
extern "C"
{
    #include "../c_modules/case_validation.h"
}

namespace cc
{

// ======================== SMALL HELPER FUNCTIONS ========================

// Get a value from a database row
// If the position does not exist, return an empty string
static std::string getColumn(const std::vector<std::string>& row, size_t index)
{
    if (index >= row.size())
        return "";

    return row[index];
}


// Convert string into integer
// If conversion fails, return 0
static int textToInt(const std::string& text)
{
    if (text.empty())
        return 0;

    try
    {
        return std::stoi(text);
    }
    catch (...)
    {
        return 0;
    }
}


// Remove spaces from the beginning and end of a text
static std::string trimText(const std::string& text)
{
    size_t start = 0;
    size_t end = text.size();

    while (start < end && std::isspace((unsigned char)text[start]))
        start++;

    while (end > start && std::isspace((unsigned char)text[end - 1]))
        end--;

    return text.substr(start, end - start);
}


// ======================== USER CLASSES ========================

// User constructor
User::User(int id, std::string fullName, std::string email)
{
    id_ = id;
    fullName_ = fullName;
    email_ = email;
}


// Virtual destructor (needed so child objects are deleted properly)
User::~User()
{
}


// One-line description, e.g.  ADMIN #1: Rasmeet (rasmeet@geu.ac.in)
// roleName() is virtual, so every child prints its own role
std::string User::describe() const
{
    return roleName() + " #" + std::to_string(id_) + ": " +
           fullName_ + " (" + email_ + ")";
}


// Factory: create the correct user object according to role
std::unique_ptr<User> User::create(const std::string& role, int id,
                                   const std::string& fullName,
                                   const std::string& email)
{
    if (role == "ADMIN")
        return std::unique_ptr<User>(new AdminUser(id, fullName, email));

    if (role == "JUDGE")
        return std::unique_ptr<User>(new JudgeUser(id, fullName, email));

    if (role == "LAWYER")
        return std::unique_ptr<User>(new LawyerUser(id, fullName, email));

    // Unknown roles are treated as litigants (the safest, least powerful role)
    return std::unique_ptr<User>(new LitigantUser(id, fullName, email));
}


// Check litigant permissions
bool LitigantUser::hasPermission(Permission permission) const
{
    if (permission == Permission::ViewOwnCases ||
        permission == Permission::CreateCase)
    {
        return true;
    }

    return false;
}


// Check lawyer permissions
bool LawyerUser::hasPermission(Permission permission) const
{
    if (permission == Permission::ViewOwnCases ||
        permission == Permission::CreateCase)
    {
        return true;
    }

    return false;
}


// Check judge permissions
bool JudgeUser::hasPermission(Permission permission) const
{
    if (permission == Permission::ViewOwnCases ||
        permission == Permission::ViewReports)
    {
        return true;
    }

    return false;
}


// Admin has all permissions
bool AdminUser::hasPermission(Permission) const
{
    return true;
}


// ======================== CASE CLASS ========================

// Case constructor
Case::Case(int id, std::string caseNumber, std::string title,
           std::string description, std::string caseType, std::string status,
           std::string filingDate, int ownerId, std::string ownerName,
           std::string ownerEmail, std::string createdAt)
{
    id_ = id;
    caseNumber_ = caseNumber;
    title_ = title;
    description_ = description;
    caseType_ = caseType;
    status_ = status;
    filingDate_ = filingDate;
    ownerId_ = ownerId;
    ownerName_ = ownerName;
    ownerEmail_ = ownerEmail;
    createdAt_ = createdAt;
}


// Create a Case object using database row values
Case Case::fromRow(const std::vector<std::string>& row)
{
    int id = textToInt(getColumn(row, 0));
    int ownerId = textToInt(getColumn(row, 7));

    return Case(
        id,
        getColumn(row, 1),
        getColumn(row, 2),
        getColumn(row, 3),
        getColumn(row, 4),
        getColumn(row, 5),
        getColumn(row, 6),
        ownerId,
        getColumn(row, 8),
        getColumn(row, 9),
        getColumn(row, 10)
    );
}


// Check whether a status text is allowed for a case
bool Case::isValidStatus(const std::string& status)
{
    return status == "PENDING" || status == "ONGOING" ||
           status == "CLOSED"  || status == "DISMISSED";
}


// Change the status only if the new value is valid
bool Case::setStatus(const std::string& newStatus)
{
    if (!isValidStatus(newStatus))
        return false;

    status_ = newStatus;
    return true;
}


// Check whether case is open
bool Case::isOpen() const
{
    if (status_ == "PENDING" || status_ == "ONGOING")
        return true;

    return false;
}


// Check whether case is finished
bool Case::isClosed() const
{
    if (status_ == "CLOSED" || status_ == "DISMISSED")
        return true;

    return false;
}


// Get case priority from C validation module
int Case::priority() const
{
    return cc_case_type_priority(caseType_.c_str());
}


// Convert priority number into a readable word
std::string Case::priorityLabel() const
{
    switch (priority())
    {
        case 1:  return "Critical";
        case 2:  return "High";
        case 3:  return "Medium";
        case 4:  return "Low";
        default: return "Normal";
    }
}


// Convert case details into JSON
crow::json::wvalue Case::toJson() const
{
    crow::json::wvalue json;

    json["id"] = id_;
    json["case_number"] = caseNumber_;
    json["title"] = title_;
    json["description"] = description_;
    json["case_type"] = caseType_;
    json["status"] = status_;
    json["filing_date"] = filingDate_;
    json["user_id"] = ownerId_;

    // If owner name is empty, display Unassigned
    if (ownerName_.empty())
        json["user_name"] = "Unassigned";
    else
        json["user_name"] = ownerName_;

    json["user_email"] = ownerEmail_;
    json["created_at"] = createdAt_;
    json["priority"] = priority();

    return json;
}


// One-line description, e.g.  CC-3001 | Land dispute | ONGOING | Medium priority
std::string Case::describe() const
{
    return caseNumber_ + " | " + title_ + " | " + status_ +
           " | " + priorityLabel() + " priority";
}


// "a < b" means a is more urgent than b
// Same rule as the C priority queue: priority, then older date, then smaller id
bool Case::operator<(const Case& other) const
{
    if (priority() != other.priority())
        return priority() < other.priority();

    // Empty filing date is treated as the latest date
    std::string date1 = filingDate_.empty() ? "9999-12-31" : filingDate_;
    std::string date2 = other.filingDate_.empty() ? "9999-12-31" : other.filingDate_;

    if (date1 != date2)
        return date1 < date2;

    return id_ < other.id_;
}


// Two cases are equal if they have the same database id
bool Case::operator==(const Case& other) const
{
    return id_ == other.id_;
}


// ======================== HEARING CLASS ========================

// Hearing constructor
Hearing::Hearing(int id, int caseId, std::string caseNumber,
                 std::string caseTitle, std::string date, std::string time,
                 std::string courtRoom, std::string judgeName,
                 std::string purpose, std::string status)
{
    id_ = id;
    caseId_ = caseId;
    caseNumber_ = caseNumber;
    caseTitle_ = caseTitle;
    date_ = date;
    time_ = time;
    courtRoom_ = courtRoom;
    judgeName_ = judgeName;
    purpose_ = purpose;
    status_ = status;
}


// Create Hearing object from database row
Hearing Hearing::fromRow(const std::vector<std::string>& row)
{
    int id = textToInt(getColumn(row, 0));
    int caseId = textToInt(getColumn(row, 1));

    return Hearing(
        id,
        caseId,
        getColumn(row, 2),
        getColumn(row, 3),
        getColumn(row, 4),
        getColumn(row, 5),
        getColumn(row, 6),
        getColumn(row, 7),
        getColumn(row, 8),
        getColumn(row, 9)
    );
}


// Check whether a status text is allowed for a hearing
bool Hearing::isValidStatus(const std::string& status)
{
    return status == "UPCOMING" || status == "COMPLETED" ||
           status == "POSTPONED";
}


// Change the status only if the new value is valid
bool Hearing::setStatus(const std::string& newStatus)
{
    if (!isValidStatus(newStatus))
        return false;

    status_ = newStatus;
    return true;
}


// Convert hearing details into JSON
crow::json::wvalue Hearing::toJson() const
{
    crow::json::wvalue json;

    json["id"] = id_;
    json["case_id"] = caseId_;
    json["case_number"] = caseNumber_;
    json["title"] = caseTitle_;
    json["hearing_date"] = date_;
    json["hearing_time"] = time_;
    json["court_room"] = courtRoom_;
    json["judge_name"] = judgeName_;
    json["purpose"] = purpose_;
    json["status"] = status_;

    return json;
}


// One-line description, e.g.  CC-3001 on 2026-11-10 at 10:30 in Room 4 (UPCOMING)
std::string Hearing::describe() const
{
    return caseNumber_ + " on " + date_ + " at " + time_ +
           " in " + courtRoom_ + " (" + status_ + ")";
}


// "a < b" means hearing a is earlier than hearing b
bool Hearing::operator<(const Hearing& other) const
{
    if (date_ != other.date_)
        return date_ < other.date_;

    if (time_ != other.time_)
        return time_ < other.time_;

    return id_ < other.id_;
}


// ======================== REPORT CLASS ========================

// Report destructor
Report::~Report()
{
}


// Convert report into JSON
crow::json::wvalue Report::toJson() const
{
    crow::json::wvalue json;

    std::vector<crow::json::wvalue> jsonLines;

    // lines() and title() are virtual - the child class version is called
    std::vector<ReportLine> reportLines = lines();

    // Store report title
    json["title"] = title();

    // Convert each report line into JSON
    for (size_t i = 0; i < reportLines.size(); i++)
    {
        crow::json::wvalue item;

        item["label"] = reportLines[i].label;
        item["value"] = reportLines[i].value;

        // crow JSON objects cannot be copied, only moved
        jsonLines.push_back(std::move(item));
    }

    json["lines"] = std::move(jsonLines);

    return json;
}


// Convert report into normal text
std::string Report::toText() const
{
    std::string reportText = title() + "\n";

    // Add underline below the title
    reportText += std::string(title().size(), '=') + "\n";

    std::vector<ReportLine> reportLines = lines();

    // Add every label and value
    for (size_t i = 0; i < reportLines.size(); i++)
    {
        reportText += reportLines[i].label + ": ";
        reportText += reportLines[i].value + "\n";
    }

    return reportText;
}


// Short description: title and number of lines
std::string Report::describe() const
{
    return title() + " (" + std::to_string(lines().size()) + " lines)";
}


// ======================== CASE REPORT ========================

// Store all cases in the report
CaseReport::CaseReport(const std::vector<Case>& cases)
{
    cases_ = cases;
}


// Generate case report details
std::vector<ReportLine> CaseReport::lines() const
{
    std::vector<ReportLine> result;

    std::map<std::string, int> statusCount;
    std::map<std::string, int> typeCount;

    int openCases = 0;
    int urgentCases = 0;

    // Read every case
    for (size_t i = 0; i < cases_.size(); i++)
    {
        const Case& currentCase = cases_[i];

        // Count cases according to status
        statusCount[currentCase.status()]++;

        // Count cases according to type
        std::string type = currentCase.caseType();

        if (type.empty())
            type = "Unspecified";

        typeCount[type]++;

        // Count open cases
        if (currentCase.isOpen())
        {
            openCases++;

            // Priority 1 and 2 are urgent
            if (currentCase.priority() <= 2)
                urgentCases++;
        }
    }

    // Add main report information
    result.push_back({"Total cases", std::to_string(cases_.size())});

    result.push_back({
        "Open cases (pending + ongoing)",
        std::to_string(openCases)
    });

    result.push_back({
        "Urgent open cases (priority 1-2)",
        std::to_string(urgentCases)
    });

    // Add status-wise count
    for (std::map<std::string, int>::const_iterator it = statusCount.begin();
         it != statusCount.end(); it++)
    {
        result.push_back({
            "Status: " + it->first,
            std::to_string(it->second)
        });
    }

    // Add type-wise count
    for (std::map<std::string, int>::const_iterator it = typeCount.begin();
         it != typeCount.end(); it++)
    {
        result.push_back({
            "Type: " + it->first,
            std::to_string(it->second)
        });
    }

    return result;
}


// ======================== HEARING REPORT ========================

// Store all hearings in the report
HearingReport::HearingReport(const std::vector<Hearing>& hearings)
{
    hearings_ = hearings;
}


// Generate hearing report details
std::vector<ReportLine> HearingReport::lines() const
{
    std::vector<ReportLine> result;

    std::map<std::string, int> statusCount;
    std::string nextDate = "";

    // Read every hearing
    for (size_t i = 0; i < hearings_.size(); i++)
    {
        const Hearing& currentHearing = hearings_[i];

        // Count hearings according to status
        statusCount[currentHearing.status()]++;

        // Find the earliest upcoming hearing date
        if (currentHearing.isUpcoming())
        {
            if (nextDate.empty() || currentHearing.date() < nextDate)
            {
                nextDate = currentHearing.date();
            }
        }
    }

    // Add total number of hearings
    result.push_back({
        "Total hearings",
        std::to_string(hearings_.size())
    });

    // Add next upcoming hearing date
    if (nextDate.empty())
    {
        result.push_back({
            "Next upcoming hearing date",
            "None scheduled"
        });
    }
    else
    {
        result.push_back({
            "Next upcoming hearing date",
            nextDate
        });
    }

    // Add status-wise hearing count
    for (std::map<std::string, int>::const_iterator it = statusCount.begin();
         it != statusCount.end(); it++)
    {
        result.push_back({
            "Status: " + it->first,
            std::to_string(it->second)
        });
    }

    return result;
}


// ======================== NOTIFICATION CLASSES ========================

// Notification constructor (spaces at both ends are removed)
Notification::Notification(std::string title, std::string message)
{
    title_ = trimText(title);
    message_ = trimText(message);
}


// Virtual destructor
Notification::~Notification()
{
}


// Default maximum message length (child classes may change it)
size_t Notification::maxMessageLength() const
{
    return 2000;
}


// Check title and message. Returns "" if everything is fine.
// maxMessageLength() is virtual, so SMS gets its own 160 limit.
std::string Notification::validate() const
{
    if (title_.empty() || title_.size() > 255)
        return "Title is required (max 255 characters)";

    if (message_.empty() || message_.size() > maxMessageLength())
    {
        return "Message is required (max " +
               std::to_string(maxMessageLength()) + " characters)";
    }

    return "";
}


// One-line description, e.g.  [EMAIL] Hearing reminder
std::string Notification::describe() const
{
    return "[" + channel() + "] " + title_;
}


// Factory: create the correct notification according to the channel
std::unique_ptr<Notification> Notification::create(const std::string& channel,
                                                   const std::string& title,
                                                   const std::string& message)
{
    if (channel == "IN_APP")
        return std::unique_ptr<Notification>(new InAppNotification(title, message));

    if (channel == "EMAIL")
        return std::unique_ptr<Notification>(new EmailNotification(title, message));

    if (channel == "SMS")
        return std::unique_ptr<Notification>(new SmsNotification(title, message));

    // Unknown channel
    return nullptr;
}


// In-app: title and message are shown separately by the website
std::string InAppNotification::format() const
{
    return title() + " - " + message();
}


// Email: subject line followed by the body
std::string EmailNotification::format() const
{
    return "Subject: " + title() + "\n\n" + message() +
           "\n\n-- CourtCraft Court Management System";
}


// SMS: short text, cut to 160 characters if needed
std::string SmsNotification::format() const
{
    std::string text = title() + ": " + message();

    if (text.size() > maxMessageLength())
        text = text.substr(0, maxMessageLength() - 3) + "...";

    return text;
}

} // End of namespace cc
