-- Optional, additive and safe to run more than once.
-- Gives cases that were created BEFORE audit logging existed one history entry
-- ("Case registered"), so the "Progress history" view is not empty for them.
USE court_craft;

INSERT INTO audit_logs (user_id, action, description, created_at)
SELECT c.user_id, 'CASE_CREATED',
       CONCAT('[', c.case_number, '] Case registered: ', c.title),
       COALESCE(c.created_at, NOW())
FROM cases c
WHERE NOT EXISTS (
    SELECT 1 FROM audit_logs a
    WHERE LEFT(a.description, CHAR_LENGTH(c.case_number) + 3) = CONCAT('[', c.case_number, '] ')
);
