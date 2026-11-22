// Shared API helpers for CourtCraft.
// The backend URL can be overridden with VITE_API_BASE in frontend/.env.
import { useEffect, useState } from "react";

export const API_BASE = import.meta.env.VITE_API_BASE || "http://127.0.0.1:8080";

export function getSession() {
  try {
    return JSON.parse(localStorage.getItem("courtcraftSession") || "null");
  } catch {
    return null;
  }
}

// Tells the app that the server rejected our token (expired / server restarted),
// so it can send the user back to the login page.
export function notifyUnauthorized() {
  window.dispatchEvent(new Event("courtcraft:unauthorized"));
}

// { Authorization: "Bearer <token>" } or {} when not logged in
export function authHeaders() {
  const token = getSession()?.token;
  return token ? { Authorization: `Bearer ${token}` } : {};
}

// fetch() against the backend with the session token attached.
// A plain-object `body` is sent as JSON. Returns { res, data }.
// Throws an Error with a readable message if the server is unreachable.
export async function apiFetch(path, options = {}) {
  const headers = { ...authHeaders(), ...(options.headers || {}) };
  let body = options.body;

  if (body !== undefined && typeof body !== "string") {
    body = JSON.stringify(body);
    headers["Content-Type"] = "application/json";
  }

  let res;
  try {
    res = await fetch(`${API_BASE}${path}`, { ...options, headers, body });
  } catch {
    throw new Error("Unable to connect to CourtCraft server. Make sure the backend is running on port 8080.");
  }

  if (res.status === 401) notifyUnauthorized();

  const data = await res.json().catch(() => ({}));
  return { res, data };
}

export function errorMessage(res, data, fallback = "Request failed. Please try again.") {
  if (res.status === 401) return "Your session has expired. Please log in again.";
  return data?.message || fallback;
}

// GET a path and keep { loading, error, data } in state.
// Change `reloadKey` to refetch. No synchronous setState inside the effect.
export function useApi(path, reloadKey = 0) {
  const [state, setState] = useState({ loading: true, error: "", data: null });

  useEffect(() => {
    let cancelled = false;

    (async () => {
      try {
        const { res, data } = await apiFetch(path);
        if (cancelled) return;

        if (!res.ok || data.status !== "success") {
          setState({ loading: false, error: errorMessage(res, data), data: null });
        } else {
          setState({ loading: false, error: "", data });
        }
      } catch (e) {
        if (!cancelled) setState({ loading: false, error: e.message, data: null });
      }
    })();

    return () => { cancelled = true; };
  }, [path, reloadKey]);

  return state;
}
