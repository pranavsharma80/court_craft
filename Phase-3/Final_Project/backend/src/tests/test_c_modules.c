/*
 * test_c_modules.c -- self-checking tests for the C data-structure modules.
 * Build target: test_c_modules      Run: build\test_c_modules.exe
 * Exits with code 0 only if every check passes.
 */
#include <stdio.h>
#include <string.h>

#include "../c_modules/case_validation.h"
#include "../c_modules/case_list.h"
#include "../c_modules/hearing_queue.h"
#include "../c_modules/priority_queue.h"
#include "../c_modules/history_stack.h"

static int g_checks = 0, g_failed = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        g_checks++;                                                        \
        if (!(cond)) {                                                     \
            g_failed++;                                                    \
            printf("  FAIL  line %d: %s\n", __LINE__, #cond);              \
        }                                                                  \
    } while (0)

static void test_validation(void)
{
    char err[CC_ERR_BUF];
    printf("[validation]\n");

    CHECK(cc_validate_case_id("CC-3001", err, sizeof err) == 1);
    CHECK(cc_validate_case_id("CC-2026-001", err, sizeof err) == 1);
    CHECK(cc_validate_case_id("HC-12", err, sizeof err) == 1);
    CHECK(cc_validate_case_id("", err, sizeof err) == 0);
    CHECK(cc_validate_case_id("cc-3001", err, sizeof err) == 0);      /* lowercase */
    CHECK(cc_validate_case_id("C-3001", err, sizeof err) == 0);       /* 1 letter  */
    CHECK(cc_validate_case_id("CCCCC-1", err, sizeof err) == 0);      /* 5 letters */
    CHECK(cc_validate_case_id("CC3001", err, sizeof err) == 0);       /* no dash   */
    CHECK(cc_validate_case_id("CC-", err, sizeof err) == 0);
    CHECK(cc_validate_case_id("CC-1234567", err, sizeof err) == 0);   /* 7 digits  */
    CHECK(cc_validate_case_id("CC-1-2-3", err, sizeof err) == 0);     /* 3 parts   */
    CHECK(cc_validate_case_id("CC-1; DROP TABLE", err, sizeof err) == 0);
    CHECK(cc_validate_case_id(NULL, err, sizeof err) == 0);

    CHECK(cc_validate_date("2026-10-03") == 1);
    CHECK(cc_validate_date("2024-02-29") == 1);                      /* leap year */
    CHECK(cc_validate_date("2026-02-29") == 0);
    CHECK(cc_validate_date("2026-13-01") == 0);
    CHECK(cc_validate_date("2026-1-1") == 0);
    CHECK(cc_validate_date("abcd-ef-gh") == 0);

    CHECK(cc_validate_case_type("Civil") == 1);
    CHECK(cc_validate_case_type("Family Court") == 1);
    CHECK(cc_validate_case_type("") == 0);
    CHECK(cc_validate_case_type("Civil;--") == 0);

    CHECK(cc_case_type_priority("Criminal") == 1);
    CHECK(cc_case_type_priority("family matter") == 2);
    CHECK(cc_case_type_priority("Civil") == 3);
    CHECK(cc_case_type_priority("Consumer") == 4);
    CHECK(cc_case_type_priority("Tax") == 5);
}

static void test_linked_list(void)
{
    CaseList list, found;
    CaseNode *n;
    printf("[linked list]\n");

    case_list_init(&list);
    case_list_init(&found);

    CHECK(case_list_append(&list, 1, "CC-3001", "Property Dispute", "land", "Civil", "PENDING", "2026-09-18") != NULL);
    CHECK(case_list_append(&list, 2, "CC-3002", "Theft Case", "stolen bike", "Criminal", "ONGOING", "2026-08-01") != NULL);
    CHECK(case_list_prepend(&list, 3, "CC-3003", "Divorce", "family matter", "Family", "PENDING", "2026-10-01") != NULL);
    CHECK(case_list_size(&list) == 3);
    CHECK(list.head->id == 3);               /* prepend went to the front */
    CHECK(list.tail->id == 2);               /* append stayed at the back */

    n = case_list_find_by_number(&list, "cc-3002");
    CHECK(n != NULL && n->id == 2);
    CHECK(case_list_find_by_number(&list, "CC-9999") == NULL);

    CHECK(case_list_search(&list, "dispute", &found) == 1);
    CHECK(found.head && found.head->id == 1);
    case_list_free(&found);
    case_list_init(&found);
    CHECK(case_list_search(&list, "CC-300", &found) == 3);
    case_list_free(&found);

    CHECK(case_list_update_status(&list, 1, "CLOSED") == 1);
    CHECK(strcmp(case_list_find_by_number(&list, "CC-3001")->status, "CLOSED") == 0);
    CHECK(case_list_update_status(&list, 99, "CLOSED") == 0);

    CHECK(case_list_remove(&list, 3) == 1);  /* remove head */
    CHECK(list.head->id == 1);
    CHECK(case_list_remove(&list, 2) == 1);  /* remove tail */
    CHECK(list.tail->id == 1);
    CHECK(case_list_remove(&list, 2) == 0);
    CHECK(case_list_size(&list) == 1);

    case_list_free(&list);
    CHECK(case_list_size(&list) == 0 && list.head == NULL);
}

static void test_queue(void)
{
    HearingQueue q;
    HearingItem a, b, out;
    printf("[hearing queue]\n");

    hq_init(&q);
    memset(&a, 0, sizeof a); memset(&b, 0, sizeof b);
    a.id = 10; strcpy(a.case_number, "CC-1");
    b.id = 20; strcpy(b.case_number, "CC-2");

    CHECK(hq_is_empty(&q));
    CHECK(hq_dequeue(&q, &out) == 0);
    CHECK(hq_enqueue(&q, &a) == 1);
    CHECK(hq_enqueue(&q, &b) == 1);
    CHECK(hq_size(&q) == 2);
    CHECK(hq_peek(&q, &out) == 1 && out.id == 10);
    CHECK(hq_dequeue(&q, &out) == 1 && out.id == 10);   /* FIFO */
    CHECK(hq_dequeue(&q, &out) == 1 && out.id == 20);
    CHECK(hq_is_empty(&q));

    hq_enqueue(&q, &a);
    hq_free(&q);
    CHECK(hq_size(&q) == 0);
}

static PriorityItem mk(int id, int pri, const char *num, const char *date)
{
    PriorityItem p;
    memset(&p, 0, sizeof p);
    p.id = id; p.priority = pri;
    strcpy(p.case_number, num);
    strcpy(p.filing_date, date);
    return p;
}

static void test_priority_queue(void)
{
    PriorityQueue pq;
    PriorityItem out;
    int i;
    printf("[priority queue]\n");

    pq_init(&pq);
    CHECK(pq_pop(&pq, &out) == 0);

    {
        PriorityItem a = mk(1, 3, "CC-1", "2026-01-01");
        PriorityItem b = mk(2, 1, "CC-2", "2026-05-01");
        PriorityItem c = mk(3, 1, "CC-3", "2026-02-01");   /* same priority, older */
        PriorityItem d = mk(4, 5, "CC-4", "2025-01-01");
        PriorityItem e = mk(5, 2, "CC-5", "");
        pq_push(&pq, &a); pq_push(&pq, &b); pq_push(&pq, &c);
        pq_push(&pq, &d); pq_push(&pq, &e);
    }

    CHECK(pq_size(&pq) == 5);
    CHECK(pq_peek(&pq, &out) == 1 && out.id == 3);
    CHECK(pq_pop(&pq, &out) == 1 && out.id == 3);  /* prio 1, older date */
    CHECK(pq_pop(&pq, &out) == 1 && out.id == 2);  /* prio 1             */
    CHECK(pq_pop(&pq, &out) == 1 && out.id == 5);  /* prio 2             */
    CHECK(pq_pop(&pq, &out) == 1 && out.id == 1);  /* prio 3             */
    CHECK(pq_pop(&pq, &out) == 1 && out.id == 4);  /* prio 5             */
    CHECK(pq_is_empty(&pq));

    /* growth beyond the initial capacity, popped in sorted order */
    for (i = 100; i > 0; i--) {
        PriorityItem p = mk(i, (i % 5) + 1, "CC-X", "2026-01-01");
        CHECK(pq_push(&pq, &p) == 1);
    }
    {
        int last = 0, ok = 1;
        while (pq_pop(&pq, &out)) {
            if (out.priority < last) ok = 0;
            last = out.priority;
        }
        CHECK(ok);
    }
    pq_free(&pq);
}

static void test_stack(void)
{
    HistoryStack s;
    HistoryEntry e, out;
    printf("[history stack]\n");

    hs_init(&s);
    memset(&e, 0, sizeof e);
    CHECK(hs_is_empty(&s));
    CHECK(hs_pop(&s, &out) == 0);

    strcpy(e.action, "Case created");  hs_push(&s, &e);
    strcpy(e.action, "Hearing set");   hs_push(&s, &e);
    strcpy(e.action, "Case closed");   hs_push(&s, &e);

    CHECK(hs_size(&s) == 3);
    CHECK(hs_peek(&s, &out) == 1 && strcmp(out.action, "Case closed") == 0);
    CHECK(hs_pop(&s, &out) == 1 && strcmp(out.action, "Case closed") == 0);   /* LIFO */
    CHECK(hs_pop(&s, &out) == 1 && strcmp(out.action, "Hearing set") == 0);
    CHECK(hs_pop(&s, &out) == 1 && strcmp(out.action, "Case created") == 0);
    CHECK(hs_is_empty(&s));

    hs_push(&s, &e);
    hs_free(&s);
    CHECK(hs_size(&s) == 0);
}

int main(void)
{
    test_validation();
    test_linked_list();
    test_queue();
    test_priority_queue();
    test_stack();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
