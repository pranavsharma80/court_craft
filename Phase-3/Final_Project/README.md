# CourtCraft — Court Case Management System

*Made by Vansh and team Vansh Infotech.*

B.Tech CSE (AI/ML) Project-Based Learning project.
A full-stack web app for registering court cases, scheduling hearings, notifications and administration.

```
React + Vite + Tailwind  ──HTTP/JSON──▶  C++ Crow API  ──▶  C data structures + C++ classes  ──▶  MySQL
        (frontend/)                        (backend/src/main.cpp)     (c_modules/, oop/)          (court_craft)
```

## Run it (Windows)

| Step | Command |
|---|---|
| 1. Start MySQL | make sure the MySQL80 service is running |
| 2. Create DB (first time only) | `mysql -u root -p < database/schema.sql` |
| 3. Build backend | `cd backend && mkdir build && cd build && cmake -G "MinGW Makefiles" .. && cmake --build .` (rebuild: `cmake --build .` inside `build`) |
| 4. Run backend (port 8080) | `cd backend\build && .\courtcraft.exe` |
| 5. Run frontend | `cd frontend && npm install && npm run dev` → open the URL Vite prints |
| 6. (optional) old cases history | `mysql -u root -p < database/backfill_case_history.sql` |

If `courtcraft.exe` says a DLL is missing, add `C:\Program Files\MySQL\MySQL Server 8.0\lib` and the MinGW `bin` folder to `PATH`.

**Configuration (environment variables, all optional):** `COURTCRAFT_DB_HOST`, `COURTCRAFT_DB_USER`, `COURTCRAFT_DB_PASSWORD`, `COURTCRAFT_DB_NAME`, `COURTCRAFT_DB_PORT`; frontend: `VITE_API_BASE` (default `http://127.0.0.1:8080`).
Set the password with an environment variable instead of editing source, and never commit it.

## What each C / C++ module does (and where it is used)

| Requirement | File | Used by (real endpoint) |
|---|---|---|
| Case ID validation (C) | `backend/src/c_modules/case_validation.c` | `POST /api/cases` (case number, date, type), `POST /api/admin/hearings` (date) |
| Linked list of cases (C) | `c_modules/case_list.c` | `GET /api/my/cases/search` — rows from MySQL are put in a list and searched |
| Hearing queue — FIFO (C) | `c_modules/hearing_queue.c` | `GET /api/admin/hearings` — "order of the day": enqueue by date/time, dequeue to assign positions |
| Priority queue — min-heap (C) | `c_modules/priority_queue.c` | `GET /api/admin/priority-cases` — which open case to handle first |
| Case history stack — LIFO (C) | `c_modules/history_stack.c` | `GET /api/case-history/<id>` — audit entries pushed oldest→newest, popped newest first |
| `User` + roles (C++) | `backend/src/oop/domain.h/.cpp` | every protected route: `authorize()` builds the right subclass and asks `hasPermission()` |
| `Case`, `Hearing` (C++) | `oop/domain.*` | built from MySQL rows in the admin routes, `toJson()` for responses, `priority()` calls the C module |
| `Report` (C++) | `oop/domain.*` | `GET /api/admin/report` — `CaseReport` / `HearingReport` through a base-class pointer |
| `Notification` (C++) | `oop/domain.*` | `POST /api/admin/notifications` — `InAppNotification` trims and validates the text; `EmailNotification` / `SmsNotification` show polymorphism (SMS limit 160) |
| `Printable` interface (C++) | `oop/domain.h` | `User`, `Case`, `Hearing`, `Report`, `Notification` all implement `describe()`; `std::cout << obj` works for all |

**Priority rule:** type → level (Criminal 1, Family 2, Property/Civil 3, Consumer/Labour 4, other 5); lower level first; ties go to the earlier filing date, then the smaller id.

**OOP concepts shown:** encapsulation (private members, `setStatus()` rejects invalid values), abstraction (`Printable`, `User`, `Report`, `Notification` are abstract), inheritance (`AdminUser`… from `User`; `CaseReport`… from `Report`; `EmailNotification`… from `Notification`), polymorphism (`roleName()`, `hasPermission()`, `lines()`, `format()`, `maxMessageLength()` are virtual), interface (`Printable`), factories (`User::create`, `Notification::create`), template method (`Report::toText`, `Notification::validate`), operator overloading (`Case::operator<` = more urgent, `operator==`, `operator<<`), static members (`Case::isValidStatus`), constructors/virtual destructors, RAII (`unique_ptr`). A class diagram is at the top of `oop/domain.h`.

**Persistent vs temporary:** MySQL holds everything that must survive (users, cases, hearings, notifications, audit logs). The C structures and C++ objects exist only for the duration of one request and are freed afterwards; login sessions are kept in memory on the server.

## API

All routes except register/login/health need `Authorization: Bearer <token>` (token is returned by login). JSON in, JSON out. Errors: `401` not logged in, `403` role not allowed, `400` invalid input, `404` not found, `409` conflict.

| Method & path | Who | Purpose |
|---|---|---|
| `GET /api/public/stats` | public | live counts for the home page (counts only, no personal data) |
| `POST /api/register` | public | create account |
| `POST /api/login` | public | returns `user` + `token` |
| `POST /api/logout` | logged in | invalidates the token |
| `GET/PUT /api/profile` | logged in | own profile (identity comes from the token only) |
| `GET /api/cases/<userId>` | self or admin | list cases |
| `POST /api/cases` | logged in | create case (owner = token user) |
| `GET /api/my/cases/search?q=` | logged in | linked-list search of own cases |
| `GET /api/case-history/<caseId>` | owner or admin | history, newest first (stack) |
| `GET /api/hearings/<userId>`, `GET /api/notifications/<userId>` | self or admin | lists |
| `GET /api/admin/stats`, `GET /api/admin/cases` | admin | dashboard data |
| `PUT /api/admin/cases/<id>/status` | admin | change status (audit log + notifies owner) |
| `GET/POST /api/admin/hearings`, `PUT /api/admin/hearings/<id>/status` | admin | list with queue position / schedule / complete / postpone |
| `GET /api/admin/priority-cases` | admin | priority-queue order |
| `GET /api/admin/users`, `PUT /api/admin/users/<id>/role` | admin | manage users and roles |
| `GET/POST /api/admin/notifications` | admin | recent + send to one user or everyone |
| `GET /api/admin/audit-logs`, `GET /api/admin/report` | admin | audit trail, C++ reports |

## Tests

* **C modules** (no database needed): `cd backend\build && cmake --build . --target test_c_modules && .\test_c_modules.exe` → expects `178 checks, 0 failed`.
* **C++ OOP classes** (no database needed): `cmake --build . --target test_oop && .\test_oop.exe` → expects `39 checks, 0 failed`.
* Security checklist worth demonstrating: call `/api/admin/users` without a token (401), with a normal user's token (403), and open `/api/cases/<another user's id>` (403).

## Known limitations (be upfront about these)

* **Password hashing** is a salted SHA-256 done by MySQL. That is fast to brute-force; production should use bcrypt/argon2 (e.g. libsodium).
* **Sessions are in memory:** restarting the backend logs everyone out (the app then sends them back to the login page).
* A **role change** applies to a user's existing session only after they log in again.
* CORS allows any origin (`*`) — fine for local development, restrict it for deployment.
* No HTTPS, rate limiting or account lockout.
* Uploading case *documents* is not implemented; "Upload Case" registers case details only.
* Lawyer/judge details per case (the "Lawyer"/"Judge" fields on a case) are not stored in the database yet and show "Not assigned".

## Folder map

```
backend/CMakeLists.txt         build config (C + C++17, Crow, MySQL client)
backend/src/main.cpp           server, sessions, auth, register/login/profile/cases/...
backend/src/extra_routes.hpp   search, history, priority queue, hearing queue, admin modules, reports
backend/src/db_helpers.hpp     prepared-statement runner, audit log, notifications, permission checks
backend/src/c_modules/         C: validation, linked list, queue, priority queue, stack
backend/src/oop/               C++: User/roles, Case, Hearing, Report
backend/src/tests/             C module tests
frontend/src/App.jsx           pages (home, login, register, user + admin dashboards)
frontend/src/AdminModules.jsx  admin pages + case history component
frontend/src/api.js            API helper (token header, session-expiry handling)
database/schema.sql            tables
```
