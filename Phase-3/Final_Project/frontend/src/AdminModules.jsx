// Admin modules (connected to the C++ backend) + a shared case-history view.
import { useMemo, useState } from "react";
import { apiFetch, errorMessage, useApi } from "./api";

/* ----------------------------- shared UI ----------------------------- */

const inputCls =
  "w-full rounded-xl bg-white/5 border border-white/10 px-4 py-3 outline-none focus:border-white/30 text-sm";
const primaryBtn =
  "px-5 py-3 rounded-xl bg-white text-black text-sm font-semibold hover:scale-[1.02] transition disabled:opacity-50 disabled:cursor-not-allowed disabled:hover:scale-100";
const ghostBtn =
  "px-3 py-2 rounded-lg border border-white/10 text-xs text-gray-300 hover:bg-white/5 transition disabled:opacity-50 disabled:cursor-not-allowed";

function PageHeader({ title, subtitle, action }) {
  return (
    <div className="flex flex-col md:flex-row md:items-end justify-between gap-4 mb-7">
      <div>
        <p className="text-gray-600 text-sm">Admin Management</p>
        <h2 className="text-3xl font-bold mt-1">{title}</h2>
        <p className="text-gray-600 mt-2">{subtitle}</p>
      </div>
      {action}
    </div>
  );
}

function Card({ children, className = "" }) {
  return (
    <div className={`rounded-3xl border border-white/10 bg-white/[0.03] p-5 md:p-6 ${className}`}>
      {children}
    </div>
  );
}

function ErrorBox({ children }) {
  if (!children) return null;
  return (
    <div className="mb-5 rounded-2xl border border-red-400/20 bg-red-400/10 px-5 py-4 text-sm text-red-200">
      {children}
    </div>
  );
}

function SuccessBox({ children }) {
  if (!children) return null;
  return (
    <div className="mb-5 rounded-2xl border border-emerald-400/20 bg-emerald-400/10 px-5 py-4 text-sm text-emerald-200">
      {children}
    </div>
  );
}

function LoadingBox({ text = "Loading from database..." }) {
  return <Card className="text-center text-gray-500 py-12">{text}</Card>;
}

function EmptyBox({ text }) {
  return <Card className="text-center text-gray-500 py-12">{text}</Card>;
}

function Field({ label, children }) {
  return (
    <label className="block">
      <span className="text-xs text-gray-500">{label}</span>
      <div className="mt-2">{children}</div>
    </label>
  );
}

const STATUS_STYLE = {
  PENDING: "text-yellow-300 bg-yellow-400/10 border-yellow-400/10",
  ONGOING: "text-green-300 bg-green-400/10 border-green-400/10",
  CLOSED: "text-blue-300 bg-blue-400/10 border-blue-400/10",
  DISMISSED: "text-red-300 bg-red-400/10 border-red-400/10",
  UPCOMING: "text-green-300 bg-green-400/10 border-green-400/10",
  COMPLETED: "text-blue-300 bg-blue-400/10 border-blue-400/10",
  POSTPONED: "text-yellow-300 bg-yellow-400/10 border-yellow-400/10",
};

function Badge({ value }) {
  return (
    <span className={`inline-block px-3 py-1 rounded-full border text-xs ${STATUS_STYLE[value] || "text-gray-300 bg-white/5 border-white/10"}`}>
      {value}
    </span>
  );
}

function PriorityBadge({ level }) {
  const styles = {
    1: "text-red-300 bg-red-400/10 border-red-400/20",
    2: "text-orange-300 bg-orange-400/10 border-orange-400/20",
    3: "text-yellow-300 bg-yellow-400/10 border-yellow-400/20",
    4: "text-blue-300 bg-blue-400/10 border-blue-400/20",
    5: "text-gray-300 bg-white/5 border-white/10",
  };
  const names = { 1: "Critical", 2: "High", 3: "Medium", 4: "Low", 5: "Routine" };
  return (
    <span className={`inline-block px-3 py-1 rounded-full border text-xs ${styles[level] || styles[5]}`}>
      P{level} · {names[level] || "Routine"}
    </span>
  );
}

/* ------------------------ case history (stack) ------------------------ */

export function CaseHistory({ dbId }) {
  if (!dbId) return <p className="text-sm text-gray-600">No history available.</p>;
  return <CaseHistoryList dbId={dbId} />;
}

function CaseHistoryList({ dbId }) {
  const { loading, error, data } = useApi(`/api/case-history/${dbId}`, 0);

  if (loading) return <p className="text-sm text-gray-600">Loading history...</p>;
  if (error) return <p className="text-sm text-red-300">{error}</p>;

  const items = data?.history || [];

  if (items.length === 0) {
    return <p className="text-sm text-gray-600">No actions have been recorded for this case yet.</p>;
  }

  return (
    <div>
      {items.map((h, i) => (
        <div key={`${h.created_at}-${i}`} className="flex gap-4 pb-6 last:pb-0">
          <div className={`w-8 h-8 shrink-0 rounded-full flex items-center justify-center text-xs ${i === 0 ? "bg-white text-black" : "bg-white/5 text-gray-500"}`}>
            {i === 0 ? "●" : "•"}
          </div>
          <div className="min-w-0">
            <p className="text-sm font-medium">{h.description || h.action}</p>
            <p className="text-xs text-gray-600 mt-1">
              {h.created_at}{h.actor ? ` · ${h.actor}` : ""}
            </p>
          </div>
        </div>
      ))}
    </div>
  );
}

/* ----------------- case status control (admin case view) ----------------- */

export function AdminCaseStatusControl({ caseData, onChanged }) {
  const [status, setStatus] = useState(caseData.status);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const [historyKey, setHistoryKey] = useState(0);

  const update = async () => {
    if (busy || status === caseData.status) return;
    setBusy(true);
    setError("");
    setSuccess("");

    try {
      const { res, data } = await apiFetch(`/api/admin/cases/${caseData.id}/status`, {
        method: "PUT",
        body: { status },
      });

      if (!res.ok) {
        setError(errorMessage(res, data));
        return;
      }

      setSuccess(`Status changed to ${status}. The case owner was notified.`);
      setHistoryKey(k => k + 1);
      onChanged?.(status);
    } catch (e) {
      setError(e.message);
    } finally {
      setBusy(false);
    }
  };

  return (
    <>
      <Card className="mt-5">
        <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Update status</p>
        <ErrorBox>{error}</ErrorBox>
        <SuccessBox>{success}</SuccessBox>
        <div className="flex flex-col sm:flex-row gap-3 mt-3">
          <select value={status} onChange={e => { setStatus(e.target.value); setSuccess(""); }} className={`${inputCls} sm:max-w-xs bg-[#111217]`}>
            {["PENDING", "ONGOING", "CLOSED", "DISMISSED"].map(s => <option key={s} value={s}>{s}</option>)}
          </select>
          <button onClick={update} disabled={busy || status === caseData.status} className={primaryBtn}>
            {busy ? "Updating..." : "Update Status"}
          </button>
        </div>
      </Card>

      <Card className="mt-5">
        <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Case history</p>
        <h3 className="text-xl font-semibold mt-1 mb-5">Newest first</h3>
        <CaseHistory key={historyKey} dbId={caseData.id} />
      </Card>
    </>
  );
}

/* --------------------- priority queue panel (cases) --------------------- */

export function AdminPriorityPanel() {
  const { loading, error, data } = useApi("/api/admin/priority-cases", 0);

  if (loading) return <LoadingBox text="Building priority queue..." />;
  if (error) return <ErrorBox>{error}</ErrorBox>;

  const cases = data?.cases || [];

  return (
    <>
      <Card className="mb-5">
        <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Priority rule</p>
        <p className="text-sm text-gray-400 mt-2">{data?.rule}</p>
        <p className="text-xs text-gray-600 mt-2">Computed with a min-heap priority queue (C module) over all pending and ongoing cases.</p>
      </Card>

      {cases.length === 0 ? (
        <EmptyBox text="No open cases." />
      ) : (
        <Card className="p-0 overflow-hidden">
          {cases.map(c => (
            <div key={c.id} className="flex flex-col sm:flex-row sm:items-center gap-3 px-5 py-4 border-b border-white/5 last:border-0">
              <div className="w-10 h-10 shrink-0 rounded-xl bg-white text-black flex items-center justify-center font-bold text-sm">{c.rank}</div>
              <div className="flex-1 min-w-0">
                <p className="text-sm font-semibold">{c.case_number} · {c.title}</p>
                <p className="text-xs text-gray-600 mt-1">{c.case_type || "—"} · Filed {c.filing_date || "—"}</p>
              </div>
              <PriorityBadge level={c.priority} />
              <Badge value={c.status} />
            </div>
          ))}
        </Card>
      )}
    </>
  );
}

/* ------------------------------ Users page ------------------------------ */

export function AdminUsersPage() {
  const [reload, setReload] = useState(0);
  const [search, setSearch] = useState("");
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const [busyId, setBusyId] = useState(null);
  const { loading, error: loadError, data } = useApi("/api/admin/users", reload);

  const users = useMemo(() => {
    const q = search.trim().toLowerCase();
    return (data?.users || []).filter(u => !q || `${u.full_name} ${u.email} ${u.role}`.toLowerCase().includes(q));
  }, [data, search]);

  const changeRole = async (user, role) => {
    if (busyId || role === user.role) return;
    setBusyId(user.id);
    setError("");
    setSuccess("");

    try {
      const { res, data: d } = await apiFetch(`/api/admin/users/${user.id}/role`, { method: "PUT", body: { role } });
      if (!res.ok) {
        setError(errorMessage(res, d));
      } else {
        setSuccess(`${user.full_name} is now ${role}.`);
        setReload(k => k + 1);
      }
    } catch (e) {
      setError(e.message);
    } finally {
      setBusyId(null);
    }
  };

  return (
    <div className="animate-fade-up">
      <PageHeader
        title="Manage Users"
        subtitle="Registered accounts loaded directly from the CourtCraft database."
        action={<button onClick={() => setReload(k => k + 1)} className={primaryBtn}>↻ Refresh</button>}
      />
      <ErrorBox>{loadError || error}</ErrorBox>
      <SuccessBox>{success}</SuccessBox>
      <input value={search} onChange={e => setSearch(e.target.value)} placeholder="Search name, email or role..." className={`${inputCls} mb-5`} />

      {loading ? <LoadingBox /> : users.length === 0 ? <EmptyBox text="No users found." /> : (
        <Card className="p-0 overflow-hidden">
          <div className="hidden md:grid grid-cols-[1.4fr_1.6fr_1fr_0.6fr_1fr] gap-4 px-5 py-4 border-b border-white/10 text-[10px] uppercase tracking-[0.15em] text-gray-600">
            <span>Name</span><span>Email</span><span>Phone</span><span>Cases</span><span>Role</span>
          </div>
          {users.map(u => (
            <div key={u.id} className="grid md:grid-cols-[1.4fr_1.6fr_1fr_0.6fr_1fr] gap-2 md:gap-4 items-center px-5 py-4 border-b border-white/5 last:border-0">
              <div>
                <p className="text-sm font-medium">{u.full_name}</p>
                <p className="text-[10px] text-gray-700 mt-1">ID #{u.id} · joined {u.created_at || "—"}</p>
              </div>
              <p className="text-sm text-gray-400 truncate">{u.email}</p>
              <p className="text-sm text-gray-500">{u.phone || "—"}</p>
              <p className="text-sm text-gray-400">{u.case_count}</p>
              <select
                value={u.role}
                disabled={busyId === u.id || u.id === data.current_admin_id}
                onChange={e => changeRole(u, e.target.value)}
                title={u.id === data.current_admin_id ? "You cannot change your own role" : "Change role"}
                className="rounded-xl bg-[#111217] border border-white/10 px-3 py-2 text-sm outline-none focus:border-white/30 disabled:opacity-50"
              >
                {["USER", "LAWYER", "JUDGE", "ADMIN"].map(r => <option key={r} value={r}>{r}</option>)}
              </select>
            </div>
          ))}
        </Card>
      )}
    </div>
  );
}

/* ------------------------ Lawyers & Judges page ------------------------ */

export function AdminPeoplePage() {
  const [reload, setReload] = useState(0);
  const [selectedUser, setSelectedUser] = useState("");
  const [role, setRole] = useState("LAWYER");
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const { loading, error: loadError, data } = useApi("/api/admin/users", reload);

  const people = (data?.users || []).filter(u => u.role === "LAWYER" || u.role === "JUDGE");
  const candidates = (data?.users || []).filter(u => u.role === "USER");

  const assign = async () => {
    if (!selectedUser || busy) return;
    setBusy(true);
    setError("");
    setSuccess("");

    try {
      const { res, data: d } = await apiFetch(`/api/admin/users/${selectedUser}/role`, { method: "PUT", body: { role } });
      if (!res.ok) {
        setError(errorMessage(res, d));
      } else {
        setSuccess("Role assigned.");
        setSelectedUser("");
        setReload(k => k + 1);
      }
    } catch (e) {
      setError(e.message);
    } finally {
      setBusy(false);
    }
  };

  return (
    <div className="animate-fade-up">
      <PageHeader title="Lawyers & Judges" subtitle="Assign legal roles to registered users and review who holds them." />
      <ErrorBox>{loadError || error}</ErrorBox>
      <SuccessBox>{success}</SuccessBox>

      <Card className="mb-5">
        <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Assign a role</p>
        <div className="grid md:grid-cols-[2fr_1fr_auto] gap-3 mt-3">
          <select value={selectedUser} onChange={e => setSelectedUser(e.target.value)} className={`${inputCls} bg-[#111217]`}>
            <option value="">Select a registered user...</option>
            {candidates.map(u => <option key={u.id} value={u.id}>{u.full_name} ({u.email})</option>)}
          </select>
          <select value={role} onChange={e => setRole(e.target.value)} className={`${inputCls} bg-[#111217]`}>
            <option value="LAWYER">Lawyer</option>
            <option value="JUDGE">Judge</option>
          </select>
          <button onClick={assign} disabled={!selectedUser || busy} className={primaryBtn}>{busy ? "Saving..." : "Assign"}</button>
        </div>
      </Card>

      {loading ? <LoadingBox /> : people.length === 0 ? <EmptyBox text="No lawyers or judges yet. Assign a role above." /> : (
        <div className="grid md:grid-cols-2 gap-4">
          {people.map(u => (
            <Card key={u.id}>
              <div className="flex items-center gap-4">
                <div className="w-11 h-11 rounded-xl bg-white/5 flex items-center justify-center">{u.role === "JUDGE" ? "⚖" : "👤"}</div>
                <div className="min-w-0 flex-1">
                  <p className="font-semibold truncate">{u.full_name}</p>
                  <p className="text-xs text-gray-600 truncate">{u.email}</p>
                </div>
                <span className="px-3 py-1 rounded-full bg-white/5 text-xs text-gray-300">{u.role}</span>
              </div>
            </Card>
          ))}
        </div>
      )}
    </div>
  );
}

/* ----------------------------- Hearings page ----------------------------- */

const EMPTY_HEARING = { case_id: "", hearing_date: "", hearing_time: "", court_room: "", judge_name: "", purpose: "" };

export function AdminHearingsPage() {
  const [reload, setReload] = useState(0);
  const [form, setForm] = useState(EMPTY_HEARING);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const hearings = useApi("/api/admin/hearings", reload);
  const cases = useApi("/api/admin/cases", 0);

  const change = e => { setForm({ ...form, [e.target.name]: e.target.value }); setError(""); };

  const schedule = async () => {
    if (busy) return;
    if (!form.case_id) return setError("Please select a case.");
    if (!form.hearing_date) return setError("Please choose a hearing date.");

    setBusy(true);
    setError("");
    setSuccess("");

    try {
      const { res, data } = await apiFetch("/api/admin/hearings", {
        method: "POST",
        body: { ...form, case_id: Number(form.case_id) },
      });

      if (!res.ok) {
        setError(errorMessage(res, data));
        return;
      }

      setSuccess("Hearing scheduled. The case owner was notified.");
      setForm(EMPTY_HEARING);
      setReload(k => k + 1);
    } catch (e) {
      setError(e.message);
    } finally {
      setBusy(false);
    }
  };

  const setStatus = async (hearing, status) => {
    setError("");
    setSuccess("");
    try {
      const { res, data } = await apiFetch(`/api/admin/hearings/${hearing.id}/status`, { method: "PUT", body: { status } });
      if (!res.ok) return setError(errorMessage(res, data));
      setSuccess(`Hearing marked ${status}.`);
      setReload(k => k + 1);
    } catch (e) {
      setError(e.message);
    }
  };

  const list = hearings.data?.hearings || [];

  return (
    <div className="animate-fade-up">
      <PageHeader
        title="Hearings"
        subtitle="Schedule hearings and see the order of the day (FIFO queue by date and time)."
        action={<button onClick={() => setReload(k => k + 1)} className={primaryBtn}>↻ Refresh</button>}
      />
      <ErrorBox>{hearings.error || error}</ErrorBox>
      <SuccessBox>{success}</SuccessBox>

      <Card className="mb-6">
        <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Schedule a hearing</p>
        <div className="grid md:grid-cols-2 gap-4 mt-4">
          <Field label="Case *">
            <select name="case_id" value={form.case_id} onChange={change} className={`${inputCls} bg-[#111217]`}>
              <option value="">Select a case...</option>
              {(cases.data?.cases || []).map(c => <option key={c.id} value={c.id}>{c.case_number} — {c.title}</option>)}
            </select>
          </Field>
          <Field label="Hearing date *"><input type="date" name="hearing_date" value={form.hearing_date} onChange={change} className={inputCls} /></Field>
          <Field label="Time"><input type="time" name="hearing_time" value={form.hearing_time} onChange={change} className={inputCls} /></Field>
          <Field label="Court room"><input name="court_room" value={form.court_room} onChange={change} placeholder="e.g. Court Room 4" maxLength={100} className={inputCls} /></Field>
          <Field label="Judge"><input name="judge_name" value={form.judge_name} onChange={change} placeholder="e.g. Hon. A. Kumar" maxLength={150} className={inputCls} /></Field>
          <Field label="Purpose"><input name="purpose" value={form.purpose} onChange={change} placeholder="e.g. Arguments" maxLength={255} className={inputCls} /></Field>
        </div>
        <button onClick={schedule} disabled={busy} className={`${primaryBtn} mt-5`}>{busy ? "Scheduling..." : "Schedule Hearing"}</button>
      </Card>

      {hearings.loading ? <LoadingBox /> : list.length === 0 ? <EmptyBox text="No hearings scheduled yet." /> : (
        <Card className="p-0 overflow-hidden">
          {list.map(h => (
            <div key={h.id} className="flex flex-col lg:flex-row lg:items-center gap-3 px-5 py-4 border-b border-white/5 last:border-0">
              <div className="w-12 shrink-0 text-center">
                {h.queue_position > 0 ? (
                  <>
                    <p className="text-xl font-bold">#{h.queue_position}</p>
                    <p className="text-[9px] uppercase tracking-wider text-gray-600">in queue</p>
                  </>
                ) : <p className="text-gray-700">—</p>}
              </div>
              <div className="flex-1 min-w-0">
                <p className="text-sm font-semibold">{h.case_number} · {h.title}</p>
                <p className="text-xs text-gray-600 mt-1">
                  {h.hearing_date}{h.hearing_time ? ` at ${h.hearing_time}` : ""} · {h.court_room || "Room not assigned"} · {h.judge_name || "Judge not assigned"}
                  {h.purpose ? ` · ${h.purpose}` : ""}
                </p>
              </div>
              <Badge value={h.status} />
              {h.status === "UPCOMING" && (
                <div className="flex gap-2">
                  <button onClick={() => setStatus(h, "COMPLETED")} className={ghostBtn}>Complete</button>
                  <button onClick={() => setStatus(h, "POSTPONED")} className={ghostBtn}>Postpone</button>
                </div>
              )}
              {h.status === "POSTPONED" && <button onClick={() => setStatus(h, "UPCOMING")} className={ghostBtn}>Reschedule</button>}
            </div>
          ))}
        </Card>
      )}
    </div>
  );
}

/* -------------------------- Notifications page -------------------------- */

export function AdminNotificationsPage() {
  const [reload, setReload] = useState(0);
  const [form, setForm] = useState({ user_id: "0", title: "", message: "" });
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState("");
  const [success, setSuccess] = useState("");
  const users = useApi("/api/admin/users", 0);
  const sent = useApi("/api/admin/notifications", reload);

  const change = e => { setForm({ ...form, [e.target.name]: e.target.value }); setError(""); };

  const send = async () => {
    if (busy) return;
    if (!form.title.trim() || !form.message.trim()) return setError("Title and message are required.");

    setBusy(true);
    setError("");
    setSuccess("");

    try {
      const { res, data } = await apiFetch("/api/admin/notifications", {
        method: "POST",
        body: { user_id: Number(form.user_id), title: form.title, message: form.message },
      });

      if (!res.ok) {
        setError(errorMessage(res, data));
        return;
      }

      setSuccess(`Notification sent to ${data.recipients} recipient${data.recipients === 1 ? "" : "s"}.`);
      setForm({ ...form, title: "", message: "" });
      setReload(k => k + 1);
    } catch (e) {
      setError(e.message);
    } finally {
      setBusy(false);
    }
  };

  const list = sent.data?.notifications || [];

  return (
    <div className="animate-fade-up">
      <PageHeader title="Notifications" subtitle="Send a notification to one user or to everyone." />
      <ErrorBox>{sent.error || error}</ErrorBox>
      <SuccessBox>{success}</SuccessBox>

      <Card className="mb-6">
        <div className="grid gap-4">
          <Field label="Recipient">
            <select name="user_id" value={form.user_id} onChange={change} className={`${inputCls} bg-[#111217]`}>
              <option value="0">Everyone (all users)</option>
              {(users.data?.users || []).map(u => <option key={u.id} value={u.id}>{u.full_name} ({u.email})</option>)}
            </select>
          </Field>
          <Field label="Title"><input name="title" value={form.title} onChange={change} maxLength={255} placeholder="e.g. Court closed on Friday" className={inputCls} /></Field>
          <Field label="Message"><textarea name="message" value={form.message} onChange={change} rows={4} maxLength={2000} placeholder="Write the notification..." className={`${inputCls} resize-none`} /></Field>
        </div>
        <button onClick={send} disabled={busy} className={`${primaryBtn} mt-5`}>{busy ? "Sending..." : "Send Notification"}</button>
      </Card>

      <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600 mb-3">Recent notifications</p>
      {sent.loading ? <LoadingBox /> : list.length === 0 ? <EmptyBox text="No notifications yet." /> : (
        <Card className="p-0 overflow-hidden">
          {list.slice(0, 30).map(n => (
            <div key={n.id} className="px-5 py-4 border-b border-white/5 last:border-0">
              <div className="flex justify-between gap-3">
                <p className="text-sm font-medium">{n.title}</p>
                <span className="text-xs text-gray-700 shrink-0">{n.created_at}</span>
              </div>
              <p className="text-xs text-gray-500 mt-1">{n.message}</p>
              <p className="text-[10px] text-gray-700 mt-2">To: {n.user_name || `User #${n.user_id}`} · {n.is_read ? "read" : "unread"}</p>
            </div>
          ))}
        </Card>
      )}
    </div>
  );
}

/* ------------------------------ Reports page ------------------------------ */

export function AdminReportsPage() {
  const [reload, setReload] = useState(0);
  const { loading, error, data } = useApi("/api/admin/report", reload);

  const download = () => {
    const blob = new Blob([data?.text || ""], { type: "text/plain" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = `courtcraft-report-${new Date().toISOString().slice(0, 10)}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="animate-fade-up">
      <PageHeader
        title="Reports & Analytics"
        subtitle="Generated by the C++ Report classes from live database records."
        action={
          <div className="flex gap-3">
            <button onClick={() => setReload(k => k + 1)} className={ghostBtn}>↻ Refresh</button>
            <button onClick={download} disabled={!data} className={primaryBtn}>Download .txt</button>
          </div>
        }
      />
      <ErrorBox>{error}</ErrorBox>

      {loading ? <LoadingBox text="Generating report..." /> : (
        <div className="grid lg:grid-cols-2 gap-5">
          {(data?.reports || []).map(r => (
            <Card key={r.title}>
              <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Report</p>
              <h3 className="text-xl font-semibold mt-1 mb-4">{r.title}</h3>
              {r.lines.map(l => (
                <div key={l.label} className="flex justify-between gap-4 py-2.5 border-b border-white/5 last:border-0 text-sm">
                  <span className="text-gray-500">{l.label}</span>
                  <span className="font-medium text-right">{l.value}</span>
                </div>
              ))}
            </Card>
          ))}
        </div>
      )}
    </div>
  );
}

/* ----------------------------- Audit log page ----------------------------- */

export function AdminAuditPage() {
  const [reload, setReload] = useState(0);
  const { loading, error, data } = useApi("/api/admin/audit-logs", reload);
  const logs = data?.logs || [];

  return (
    <div className="animate-fade-up">
      <PageHeader
        title="Audit Logs"
        subtitle="Every important action is recorded here (latest 200)."
        action={<button onClick={() => setReload(k => k + 1)} className={primaryBtn}>↻ Refresh</button>}
      />
      <ErrorBox>{error}</ErrorBox>

      {loading ? <LoadingBox /> : logs.length === 0 ? <EmptyBox text="No audit entries yet." /> : (
        <Card className="p-0 overflow-hidden">
          {logs.map(l => (
            <div key={l.id} className="flex flex-col md:flex-row md:items-center gap-2 md:gap-4 px-5 py-4 border-b border-white/5 last:border-0">
              <span className="md:w-44 shrink-0 text-xs text-gray-600">{l.created_at}</span>
              <span className="md:w-52 shrink-0 text-[11px] font-mono text-gray-400">{l.action}</span>
              <span className="flex-1 text-sm text-gray-300 min-w-0 break-words">{l.description}</span>
              <span className="md:w-36 shrink-0 text-xs text-gray-600 md:text-right">{l.actor}</span>
            </div>
          ))}
        </Card>
      )}
    </div>
  );
}

/* --------------------- overview "recent activity" panel --------------------- */

export function AdminRecentActivity() {
  const { loading, error, data } = useApi("/api/admin/audit-logs", 0);
  const logs = (data?.logs || []).slice(0, 4);

  if (loading) return <p className="text-sm text-gray-600">Loading activity...</p>;
  if (error) return <p className="text-sm text-red-300">{error}</p>;
  if (logs.length === 0) return <p className="text-sm text-gray-600">No activity recorded yet.</p>;

  return (
    <div className="space-y-3">
      {logs.map(l => (
        <div key={l.id} className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]">
          <div className="w-9 h-9 rounded-lg bg-white/5 flex items-center justify-center">⌁</div>
          <div className="flex-1 min-w-0">
            <p className="text-sm font-medium truncate">{l.action.replaceAll("_", " ").toLowerCase().replace(/^./, c => c.toUpperCase())}</p>
            <p className="text-xs text-gray-600 mt-1 truncate">{l.description}</p>
          </div>
          <span className="text-xs text-gray-700 shrink-0">{l.created_at.slice(5, 10)}</span>
        </div>
      ))}
    </div>
  );
}
