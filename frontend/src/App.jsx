import { useEffect, useRef, useState } from "react";


function ScrollReveal({ children, className = "", delay = 0 }) {
  const ref = useRef(null);

  useEffect(() => {
    const element = ref.current;
    if (!element) return;

    const observer = new IntersectionObserver(
      ([entry]) => {
        if (entry.isIntersecting) {
          element.classList.add("is-visible");
          observer.unobserve(element);
        }
      },
      { threshold: 0.15, rootMargin: "0px 0px -60px 0px" }
    );

    observer.observe(element);
    return () => observer.disconnect();
  }, []);

  return (
    <div
      ref={ref}
      className={`scroll-reveal ${className}`}
      style={{ "--reveal-delay": `${delay}ms` }}
    >
      {children}
    </div>
  );
}

function App() {
  const [page, setPage] = useState("home");
  const [authMessage, setAuthMessage] = useState("");

  return (
    <>
      {page === "home" && (
        <HomePage onLogin={() => setPage("login")} />
      )}

      {page === "login" && (
        <LoginPage
          authMessage={authMessage}
          onBack={() => setPage("home")}
          onRegister={() => { setAuthMessage(""); setPage("register"); }}
          onUserLogin={(user) => {
            setAuthMessage("");
            setPage(String(user?.role || "").toUpperCase() === "ADMIN" ? "admin-dashboard" : "dashboard");
          }}
        />
      )}

      {page === "register" && (
        <RegisterPage onBack={() => { setAuthMessage(""); setPage("login"); }} onRegistered={(email) => { setAuthMessage(`Account created for ${email}. Please sign in.`); setPage("login"); }} />
      )}

      {page === "dashboard" && (
        <UserDashboard onLogout={() => setPage("home")} />
      )}

      {page === "admin-dashboard" && (
        <AdminDashboard onLogout={() => setPage("home")} />
      )}
    </>
  );
}


/* ================================================= */
/* HOME PAGE */
/* ================================================= */

function HomePage({ onLogin }) {
  const panelRef = useRef(null);

  const handlePanelMove = (event) => {
    const panel = panelRef.current;
    if (!panel) return;

    const rect = panel.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;

    const rotateY = ((x / rect.width) - 0.5) * 7;
    const rotateX = ((y / rect.height) - 0.5) * -7;

    panel.style.setProperty("--mouse-rotate-x", `${rotateX}deg`);
    panel.style.setProperty("--mouse-rotate-y", `${rotateY}deg`);
    panel.classList.add("mouse-active");
  };

  const handlePanelLeave = () => {
    const panel = panelRef.current;
    if (!panel) return;

    panel.style.setProperty("--mouse-rotate-x", "0deg");
    panel.style.setProperty("--mouse-rotate-y", "0deg");
    panel.classList.remove("mouse-active");
  };

  return (
    <div
      className="min-h-screen text-white overflow-hidden"
      style={{
        backgroundImage:
          'linear-gradient(90deg, rgba(3,4,7,0.42) 0%, rgba(3,4,7,0.18) 45%, rgba(3,4,7,0.10) 72%, rgba(3,4,7,0.28) 100%), linear-gradient(180deg, rgba(0,0,0,0.12) 0%, rgba(0,0,0,0.02) 52%, rgba(0,0,0,0.35) 100%), url("/courtroom-bg.png")',
        backgroundSize: "cover",
        backgroundPosition: "center top",
        backgroundRepeat: "no-repeat",
        backgroundAttachment: "fixed",
      }}
    >

      <nav className="relative z-50 mx-4 mt-4 rounded-2xl border border-white/10 bg-black/25 backdrop-blur-xl shadow-[0_10px_40px_rgba(0,0,0,0.25)]">

        <div className="max-w-7xl mx-auto px-5 md:px-7 py-3.5 flex items-center justify-between">

          {/* LOGO */}
          <button
            onClick={() => window.scrollTo({ top: 0, behavior: "smooth" })}
            className="flex items-center gap-3 group"
          >
            <div className="w-10 h-10 rounded-xl bg-white text-black flex items-center justify-center text-xl shadow-lg group-hover:scale-105 transition-transform duration-300">
              ⚖
            </div>

            <div className="text-left">
              <h1 className="font-bold text-lg tracking-wide text-white group-hover:tracking-wider transition-all duration-300">
                CourtCraft
              </h1>
              <p className="text-[10px] md:text-xs text-white/45">
                Digital Court Management
              </p>
            </div>
          </button>


          {/* NAV LINKS */}
          <div className="hidden md:flex items-center gap-2">

            <a
              href="#features"
              className="px-4 py-2 rounded-xl text-sm text-white/60 hover:text-white hover:bg-white/10 transition-all duration-300"
            >
              Features
            </a>

            <a
              href="#about"
              className="px-4 py-2 rounded-xl text-sm text-white/60 hover:text-white hover:bg-white/10 transition-all duration-300"
            >
              About
            </a>

            <a
              href="#contact"
              className="px-4 py-2 rounded-xl text-sm text-white/60 hover:text-white hover:bg-white/10 transition-all duration-300"
            >
              Contact
            </a>

          </div>


          {/* LOGIN */}
          <button
            onClick={onLogin}
            className="px-5 py-2.5 rounded-xl border border-white/20 bg-white/10 text-white font-semibold text-sm backdrop-blur-md shadow-lg hover:bg-white hover:text-black hover:border-white hover:-translate-y-0.5 hover:scale-105 transition-all duration-300"
          >
            Login
          </button>

        </div>

      </nav>


      <main className="max-w-7xl mx-auto px-6 md:px-8 pt-28 pb-28 relative">

        <div className="absolute top-10 left-10 w-96 h-96 bg-purple-500/10 blur-[120px] rounded-full animate-glow pointer-events-none" />

        <div className="absolute top-40 right-20 w-96 h-96 bg-blue-500/10 blur-[120px] rounded-full animate-glow pointer-events-none" />


        <div className="relative z-20 grid grid-cols-1 lg:grid-cols-[minmax(0,1fr)_400px] gap-12 lg:gap-16 items-center">

        <section className="max-w-3xl relative z-20">

          <div className="animate-fade-up inline-flex items-center gap-2 px-4 py-2.5 rounded-full border border-white/15 bg-black/25 backdrop-blur-xl text-xs md:text-sm text-white/75 mb-8 shadow-lg">
            <span className="w-2 h-2 rounded-full bg-emerald-400 shadow-[0_0_12px_rgba(52,211,153,0.8)]" />
            Modern Digital Court Management
          </div>


          <h1 className="animate-fade-up text-5xl sm:text-6xl md:text-8xl font-black tracking-[-0.04em] leading-[0.9]">
            <span className="block text-white">Justice.</span>
            <span className="block mt-2 bg-gradient-to-r from-white via-white/85 to-white/45 bg-clip-text text-transparent">
              Simplified.
            </span>
          </h1>


          <p className="animate-fade-up-delay mt-8 text-base md:text-xl text-white/60 max-w-2xl leading-relaxed">
            One secure digital platform for cases, hearings, lawyers,
            judges and court records — designed to make justice
            simpler, faster and more transparent.
          </p>


          <div className="animate-fade-up-delay-2 mt-10 flex flex-wrap gap-4">

            <button
              onClick={onLogin}
              className="group px-7 py-4 rounded-xl bg-white text-black font-semibold hover:scale-105 transition-all duration-300 shadow-[0_18px_45px_rgba(0,0,0,0.35)]"
            >
              Get Started <span className="inline-block ml-1 group-hover:translate-x-1 transition-transform">→</span>
            </button>

            <a
              href="#features"
              className="px-7 py-4 rounded-xl border border-white/15 bg-black/20 backdrop-blur-xl text-white/75 hover:bg-white/10 hover:text-white hover:-translate-y-0.5 transition-all duration-300"
            >
              Explore Platform
            </a>

          </div>

          <div className="animate-fade-up-delay-2 mt-8 flex flex-wrap items-center gap-x-6 gap-y-3 text-xs text-white/45">
            <span>✓ Secure records</span>
            <span>✓ Smart hearings</span>
            <span>✓ Centralized cases</span>
          </div>

        </section>


        {/* FLOATING COURT PANEL */}

        <div
          ref={panelRef}
          onMouseMove={handlePanelMove}
          onMouseLeave={handlePanelLeave}
          className="hidden lg:block w-[400px] justify-self-end relative z-30 mouse-tilt-wrapper"
        >

          <div className="animate-panel">

          <div className="court-panel relative rounded-3xl border border-white/15 bg-black/45 backdrop-blur-2xl p-6 shadow-[0_35px_100px_rgba(0,0,0,0.55)] overflow-hidden">

            <div className="flex items-center justify-between mb-7">

              <div>

                <p className="text-xs text-gray-500 uppercase tracking-widest">
                  Court Overview
                </p>

                <h3 className="text-xl font-semibold mt-1">
                  Dashboard
                </h3>

              </div>

              <div className="w-10 h-10 rounded-xl bg-white/10 flex items-center justify-center">
                ⚖
              </div>

            </div>


            <div className="grid grid-cols-2 gap-4">

              <div className="stat-card rounded-2xl bg-white/5 border border-white/5 p-4">

                <p className="text-sm text-gray-500">
                  Active Cases
                </p>

                <p className="text-3xl font-bold mt-2">
                  128
                </p>

                <p className="text-xs text-green-400 mt-1">
                  ↑ 12% this month
                </p>

              </div>


              <div className="stat-card rounded-2xl bg-white/5 border border-white/5 p-4">

                <p className="text-sm text-gray-500">
                  Hearings
                </p>

                <p className="text-3xl font-bold mt-2">
                  24
                </p>

                <p className="text-xs text-blue-400 mt-1">
                  This week
                </p>

              </div>

            </div>


            <div className="mt-5 rounded-2xl bg-white/5 border border-white/5 p-5">

              <div className="flex justify-between items-center mb-3">

                <p className="text-sm text-gray-400">
                  Case Progress
                </p>

                <p className="text-sm font-semibold">
                  76%
                </p>

              </div>


              <div className="h-2 rounded-full bg-white/10 overflow-hidden">

                <div className="h-full bg-white rounded-full animate-progress" />

              </div>

            </div>


            <div className="mt-5">

              <p className="text-sm text-gray-500 mb-3">
                Recent Activity
              </p>


              <div className="space-y-3">

                <div className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]">

                  <div className="w-9 h-9 rounded-lg bg-blue-500/10 flex items-center justify-center">
                    📅
                  </div>

                  <div className="flex-1">

                    <p className="text-sm font-medium">
                      Hearing Scheduled
                    </p>

                    <p className="text-xs text-gray-500">
                      Case #CC-2048
                    </p>

                  </div>

                  <span className="text-xs text-gray-600">
                    Today
                  </span>

                </div>


                <div className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]">

                  <div className="w-9 h-9 rounded-lg bg-green-500/10 flex items-center justify-center">
                    ✓
                  </div>

                  <div className="flex-1">

                    <p className="text-sm font-medium">
                      Record Updated
                    </p>

                    <p className="text-xs text-gray-500">
                      Case #CC-1920
                    </p>

                  </div>

                  <span className="text-xs text-gray-600">
                    5m
                  </span>

                </div>

              </div>

            </div>

          </div>

          </div>

        </div>

        </div>

      </main>


      {/* FEATURES */}

      <section
        id="features"
        className="relative z-10 max-w-7xl mx-auto px-8 py-24"
      >

        <ScrollReveal className="text-center mb-16">

          <p className="text-sm text-gray-500 uppercase tracking-widest">
            Platform
          </p>

          <h2 className="text-4xl md:text-5xl font-bold mt-3">
            Everything in one place.
          </h2>

          <p className="text-gray-500 mt-4 max-w-xl mx-auto">
            A centralized platform designed to make court
            management faster, simpler and more transparent.
          </p>

        </ScrollReveal>


        <ScrollReveal className="grid md:grid-cols-3 gap-6" delay={120}>

          <FeatureCard
            icon="📁"
            title="Case Management"
            description="Create, manage and track cases from registration to final judgment."
          />

          <FeatureCard
            icon="📅"
            title="Smart Hearings"
            description="Manage hearing schedules, courtroom assignments and upcoming dates."
          />

          <FeatureCard
            icon="🔐"
            title="Secure Records"
            description="Keep important court documents and records organized and protected."
          />

        </ScrollReveal>

      </section>


      {/* ABOUT */}

      <section
        id="about"
        className="relative z-10 border-t border-white/5"
      >

        <div className="max-w-7xl mx-auto px-8 py-24">

          <ScrollReveal className="max-w-3xl">

            <p className="text-sm text-gray-500 uppercase tracking-widest">
              About CourtCraft
            </p>

            <h2 className="text-4xl md:text-5xl font-bold mt-4">
              Building the future of digital justice.
            </h2>

            <p className="mt-6 text-gray-400 text-lg leading-relaxed">
              CourtCraft is designed to modernize traditional court
              management by bringing cases, hearings, people and
              documents into one connected system.
            </p>

          </ScrollReveal>

        </div>

      </section>


      {/* CONTACT */}

      <section
        id="contact"
        className="relative z-10 border-t border-white/5"
      >

        <div className="max-w-7xl mx-auto px-8 py-20">

          <ScrollReveal className="rounded-3xl border border-white/10 bg-black/25 backdrop-blur-xl p-10 md:p-14 shadow-[0_25px_80px_rgba(0,0,0,0.28)]" delay={80}>

            <p className="text-sm text-gray-500 uppercase tracking-widest">
              Contact
            </p>

            <h2 className="text-3xl md:text-4xl font-bold mt-3">
              Have questions?
            </h2>

            <p className="text-gray-500 mt-4">
              Get in touch with the CourtCraft team.
            </p>

            <button
              onClick={onLogin}
              className="mt-7 px-6 py-3 rounded-xl bg-white text-black font-semibold hover:scale-105 transition"
            >
              Enter CourtCraft
            </button>

          </ScrollReveal>

        </div>

      </section>


      <footer className="relative z-10 border-t border-white/5">

        <div className="max-w-7xl mx-auto px-8 py-8 flex flex-col md:flex-row justify-between gap-4">

          <p className="text-sm text-gray-600">
            © 2026 CourtCraft. All rights reserved.
          </p>

          <p className="text-sm text-gray-600">
            Digital Court Management System
          </p>

        </div>

      </footer>

    </div>
  );
}


/* ================================================= */
/* FEATURE CARD */
/* ================================================= */

function FeatureCard({ icon, title, description }) {

  return (
    <div className="group relative feature-card rounded-3xl border border-white/10 bg-black/25 backdrop-blur-xl p-7 hover:bg-white/[0.07] hover:-translate-y-2 transition-all duration-500 shadow-[0_20px_60px_rgba(0,0,0,0.18)]">

      <div className="w-12 h-12 rounded-xl bg-white/10 flex items-center justify-center text-xl">
        {icon}
      </div>

      <h3 className="text-xl font-semibold mt-6">
        {title}
      </h3>

      <p className="text-gray-500 mt-3 leading-relaxed">
        {description}
      </p>

      <div className="mt-7 text-sm text-gray-400 group-hover:text-white transition">
        Learn more →
      </div>

    </div>
  );
}


/* ================================================= */
/* LOGIN PAGE */
/* ================================================= */

function LoginPage({
  onBack,
  onRegister,
  onUserLogin,
  authMessage = "",
}) {

  const [role, setRole] = useState("user");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  return (
    <div className="relative min-h-screen text-white flex items-center justify-center px-6 py-10 overflow-hidden">
      <img
        src="/courtroom-bg.png"
        alt=""
        aria-hidden="true"
        className="fixed inset-0 w-full h-full object-cover object-center -z-20 pointer-events-none"
      />
      <div className="fixed inset-0 -z-10 pointer-events-none bg-[linear-gradient(90deg,rgba(3,4,7,0.72),rgba(3,4,7,0.42),rgba(3,4,7,0.72))]" />

      <div className="absolute top-[-150px] left-[-150px] w-[500px] h-[500px] bg-purple-500/10 blur-[140px] rounded-full animate-glow" />

      <div className="absolute bottom-[-180px] right-[-120px] w-[500px] h-[500px] bg-blue-500/10 blur-[140px] rounded-full animate-glow" />


      <div className="relative z-10 w-full max-w-5xl grid lg:grid-cols-2 gap-12 items-center">


        {/* LEFT */}

        <div className="hidden lg:block animate-fade-up">

          <div className="mb-8">

            <div className="w-14 h-14 rounded-2xl bg-white text-black flex items-center justify-center text-2xl shadow-xl">
              ⚖
            </div>

          </div>


          <p className="text-sm text-gray-500 uppercase tracking-[0.25em]">
            Welcome to CourtCraft
          </p>


          <h1 className="text-6xl font-bold tracking-tight mt-4 leading-tight">

            Justice.

            <br />

            <span className="text-gray-600">
              Starts Here.
            </span>

          </h1>


          <p className="text-gray-500 text-lg mt-7 max-w-md leading-relaxed">
            Access your cases, hearings, documents and
            court information through a secure digital platform.
          </p>


          <div className="mt-10 space-y-4">

            <MiniFeature
              icon="🔐"
              title="Secure Access"
              text="Your information stays protected."
            />

            <MiniFeature
              icon="📁"
              title="Centralized Records"
              text="Everything available in one place."
            />

            <MiniFeature
              icon="⚡"
              title="Faster Management"
              text="Manage your court journey digitally."
            />

          </div>

        </div>


        {/* LOGIN CARD */}

        <div className="animate-fade-up-delay">

          <div className="rounded-[2rem] border border-white/10 bg-[#111217]/90 backdrop-blur-2xl p-8 md:p-10 shadow-2xl">

            <button
              onClick={onBack}
              className="text-sm text-gray-500 hover:text-white transition mb-8"
            >
              ← Back to CourtCraft
            </button>


            <div className="mb-8">

              {authMessage && (
                <div className="mb-5 rounded-xl border border-emerald-400/20 bg-emerald-400/10 px-4 py-3 text-sm text-emerald-200">
                  {authMessage}
                </div>
              )}

              {error && (
                <div className="mb-5 rounded-xl border border-red-400/20 bg-red-400/10 px-4 py-3 text-sm text-red-200">
                  {error}
                </div>
              )}

              <p className="text-sm text-gray-500">
                Sign in
              </p>

              <h2 className="text-3xl font-bold mt-2">
                Welcome back.
              </h2>

              <p className="text-gray-600 text-sm mt-2">
                Enter your details to continue.
              </p>

            </div>


            {/* ROLE */}

            <div className="mb-7">

              <label className="text-xs text-gray-500 uppercase tracking-wider">
                Continue as
              </label>


              <div className="grid grid-cols-2 gap-3 mt-3">

                <button
                  onClick={() => setRole("user")}
                  className={`p-4 rounded-2xl border text-left transition-all duration-300 ${
                    role === "user"
                      ? "bg-white text-black border-white shadow-lg"
                      : "bg-white/[0.03] border-white/10 text-gray-400 hover:bg-white/[0.06]"
                  }`}
                >

                  <div className="text-xl mb-2">
                    👤
                  </div>

                  <p className="font-semibold text-sm">
                    User
                  </p>

                  <p className="text-xs mt-1 text-gray-600">
                    Manage your cases
                  </p>

                </button>


                <button
                  onClick={() => setRole("admin")}
                  className={`p-4 rounded-2xl border text-left transition-all duration-300 ${
                    role === "admin"
                      ? "bg-white text-black border-white shadow-lg"
                      : "bg-white/[0.03] border-white/10 text-gray-400 hover:bg-white/[0.06]"
                  }`}
                >

                  <div className="text-xl mb-2">
                    🛡️
                  </div>

                  <p className="font-semibold text-sm">
                    Admin
                  </p>

                  <p className="text-xs mt-1 text-gray-600">
                    Manage the platform
                  </p>

                </button>

              </div>

            </div>


            {/* EMAIL */}

            <div className="mb-5">

              <label className="text-sm text-gray-400">
                Email address
              </label>

              <input
                type="email"
                placeholder="you@example.com"
                value={email}
                onChange={(e) => { setEmail(e.target.value); setError(""); }}
                className="mt-2 w-full px-4 py-4 rounded-2xl bg-white/[0.04] border border-white/10 outline-none focus:border-white/30 focus:bg-white/[0.06] transition placeholder:text-gray-700"
              />

            </div>


            {/* PASSWORD */}

            <div className="mb-3">

              <div className="flex justify-between">

                <label className="text-sm text-gray-400">
                  Password
                </label>

                <button className="text-xs text-gray-600 hover:text-white transition">
                  Forgot password?
                </button>

              </div>


              <input
                type="password"
                placeholder="••••••••"
                value={password}
                onChange={(e) => { setPassword(e.target.value); setError(""); }}
                className="mt-2 w-full px-4 py-4 rounded-2xl bg-white/[0.04] border border-white/10 outline-none focus:border-white/30 focus:bg-white/[0.06] transition placeholder:text-gray-700"
              />

            </div>


            {/* LOGIN BUTTON */}

            <button
              onClick={async () => {
                const cleanEmail = email.trim().toLowerCase();

                if (!cleanEmail || !password) {
                  setError("Please enter your email and password.");
                  return;
                }

                setLoading(true);
                setError("");

                try {
                  const response = await fetch("http://127.0.0.1:8080/api/login", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ email: cleanEmail, password }),
                  });

                  const data = await response.json();

                  if (!response.ok) {
                    setError(data.message || "Invalid email or password.");
                    setLoading(false);
                    return;
                  }

                  if (data.status === "success" && data.user) {
                    localStorage.setItem("courtcraftSession", JSON.stringify({
                      id: data.user.id,
                      email: data.user.email,
                      name: data.user.full_name,
                      phone: data.user.phone,
                      role: data.user.role,
                    }));
                    setLoading(false);
                    onUserLogin(data.user);
                  } else {
                    setError("Login failed. Please try again.");
                    setLoading(false);
                  }
                } catch (err) {
                  console.error("Login error:", err);
                  setError("Unable to connect to CourtCraft server. Make sure the backend is running on port 8080.");
                  setLoading(false);
                }
              }}
              className="w-full mt-7 py-4 rounded-2xl bg-white text-black font-semibold hover:scale-[1.02] active:scale-[0.98] transition-all duration-300 shadow-xl"
            >

              {loading ? "Signing in..." : `Continue as ${role === "user" ? "User" : "Administrator"}`}

              {!loading && <span className="ml-2">→</span>}

            </button>


            {role === "user" && (

              <div className="text-center mt-7">

                <span className="text-sm text-gray-600">
                  New to CourtCraft?
                </span>

                <button
                  onClick={onRegister}
                  className="text-sm text-white font-semibold ml-2 hover:underline"
                >
                  Create account
                </button>

              </div>

            )}


            <div className="flex items-center justify-center gap-2 mt-8 text-xs text-gray-700">
              🔒 Secure encrypted connection
            </div>

          </div>

        </div>

      </div>

    </div>
  );
}


/* ================================================= */
/* MINI FEATURE */
/* ================================================= */

function MiniFeature({
  icon,
  title,
  text,
}) {

  return (
    <div className="flex items-center gap-4">

      <div className="w-10 h-10 rounded-xl bg-white/5 border border-white/10 flex items-center justify-center">
        {icon}
      </div>

      <div>

        <p className="text-sm font-medium">
          {title}
        </p>

        <p className="text-xs text-gray-600">
          {text}
        </p>

      </div>

    </div>
  );
}


/* ================================================= */
/* REGISTER PAGE */
/* ================================================= */

function RegisterPage({ onBack, onRegistered }) {

  const [form, setForm] = useState({
    name: "",
    email: "",
    phone: "",
    password: "",
    confirmPassword: "",
  });


  const handleChange = (e) => {

    setForm({
      ...form,
      [e.target.name]: e.target.value,
    });

  };


  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleRegister = async () => {
    const name = form.name.trim();
    const email = form.email.trim().toLowerCase();
    const phone = form.phone.trim();

    if (!name || !email || !phone || !form.password || !form.confirmPassword) {
      setError("Please fill all fields.");
      return;
    }
    if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
      setError("Please enter a valid email address.");
      return;
    }
    if (!/^[0-9+\-\s]{10,15}$/.test(phone)) {
      setError("Please enter a valid phone number.");
      return;
    }
    if (form.password.length < 8) {
      setError("Password must be at least 8 characters.");
      return;
    }
    if (form.password !== form.confirmPassword) {
      setError("Passwords do not match.");
      return;
    }

    setLoading(true);
    setError("");

    try {
      const response = await fetch("http://127.0.0.1:8080/api/register", {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          full_name: name,
          email: email,
          phone: phone,
          password: form.password,
        }),
      });

      if (response.status === 201) {
        // Success
        const data = await response.json();
        localStorage.setItem("courtcraftSession", JSON.stringify({ email: data.user.email, name: data.user.full_name, role: "user" }));
        setLoading(false);
        onRegistered?.(email);
      } else if (response.status === 409) {
        // Email already registered
        setLoading(false);
        setError("Email already registered. Please use a different email or sign in.");
      } else if (response.status === 400) {
        // Validation error
        const data = await response.json();
        setLoading(false);
        setError(data.message || "Please check your information and try again.");
      } else {
        // Other error
        setLoading(false);
        setError("An error occurred. Please try again.");
      }
    } catch (err) {
      setLoading(false);
      setError("Unable to connect to the server. Please check your connection and try again.");
    }
  };


  return (
    <div className="relative min-h-screen text-white flex items-center justify-center px-6 py-10 overflow-hidden">
      <img
        src="/courtroom-bg.png"
        alt=""
        aria-hidden="true"
        className="fixed inset-0 w-full h-full object-cover object-center -z-20 pointer-events-none"
      />
      <div className="fixed inset-0 -z-10 pointer-events-none bg-[linear-gradient(90deg,rgba(3,4,7,0.76),rgba(3,4,7,0.48),rgba(3,4,7,0.76))]" />

      <div className="absolute top-0 right-0 w-96 h-96 bg-purple-500/10 blur-[120px] rounded-full animate-glow" />

      <div className="absolute bottom-0 left-0 w-96 h-96 bg-blue-500/10 blur-[120px] rounded-full animate-glow" />


      <div className="relative z-10 w-full max-w-lg animate-fade-up">

        <div className="rounded-3xl border border-white/10 bg-[#111217]/95 backdrop-blur-xl p-8 shadow-2xl">

          <button
            onClick={onBack}
            className="text-sm text-gray-500 hover:text-white transition mb-7"
          >
            ← Back to Login
          </button>


          <div className="mb-7">

            {error && (
              <div className="mb-5 rounded-xl border border-red-400/20 bg-red-400/10 px-4 py-3 text-sm text-red-200">
                {error}
              </div>
            )}

            <div className="w-11 h-11 rounded-xl bg-white text-black flex items-center justify-center text-xl mb-5">
              ⚖
            </div>

            <h2 className="text-3xl font-bold">
              Create Account
            </h2>

            <p className="text-gray-500 mt-2">
              Join CourtCraft and manage your court cases digitally.
            </p>

          </div>


          <RegisterInput
            label="Full Name"
            name="name"
            value={form.name}
            onChange={handleChange}
            placeholder="Enter your full name"
          />


          <RegisterInput
            label="Email"
            name="email"
            type="email"
            value={form.email}
            onChange={handleChange}
            placeholder="Enter your email"
          />


          <RegisterInput
            label="Phone"
            name="phone"
            value={form.phone}
            onChange={handleChange}
            placeholder="Enter your phone number"
          />


          <RegisterInput
            label="Password"
            name="password"
            type="password"
            value={form.password}
            onChange={handleChange}
            placeholder="Create a password"
          />


          <RegisterInput
            label="Confirm Password"
            name="confirmPassword"
            type="password"
            value={form.confirmPassword}
            onChange={handleChange}
            placeholder="Confirm your password"
          />


          <button
            onClick={handleRegister}
            disabled={loading}
            className="w-full py-4 rounded-xl bg-white text-black font-semibold hover:scale-[1.02] transition disabled:opacity-50 disabled:scale-100 disabled:cursor-not-allowed"
          >
            {loading ? "Creating Account..." : "Create Account"}
          </button>


          <p className="text-center text-xs text-gray-600 mt-5">
            By creating an account, you agree to our terms and policies.
          </p>

        </div>

      </div>

    </div>
  );
}


/* ================================================= */
/* REGISTER INPUT */
/* ================================================= */

function RegisterInput({
  label,
  name,
  type = "text",
  value,
  onChange,
  placeholder,
}) {

  return (
    <div className="mb-4">

      <label className="text-sm text-gray-400">
        {label}
      </label>

      <input
        name={name}
        type={type}
        value={value}
        onChange={onChange}
        placeholder={placeholder}
        className="mt-2 w-full px-4 py-3.5 rounded-xl bg-white/5 border border-white/10 outline-none focus:border-white/30 transition"
      />

    </div>
  );
}



/* ================================================= */
/* ADMIN DASHBOARD */
/* ================================================= */

function AdminDashboard({ onLogout }) {
  const [activePage, setActivePage] = useState("overview");
  const [mobileMenu, setMobileMenu] = useState(false);
  const session = JSON.parse(localStorage.getItem("courtcraftSession") || "null");
  const adminName = session?.name || "Administrator";
  const initials = adminName.split(" ").map(x => x[0]).join("").slice(0, 2).toUpperCase();

  const navigate = (page) => {
    setActivePage(page);
    setMobileMenu(false);
  };

  const [stats, setStats] = useState([
    { icon: "▣", label: "Total Cases", value: "—", note: "Registered cases" },
    { icon: "👤", label: "Total Users", value: "—", note: "Registered accounts" },
    { icon: "◷", label: "Hearings", value: "—", note: "Scheduled hearings" },
    { icon: "⚖", label: "Active Cases", value: "—", note: "Currently ongoing" },
  ]);
  const [statsLoading, setStatsLoading] = useState(true);
  const [statsError, setStatsError] = useState("");

  useEffect(() => {
    let cancelled = false;

    const loadAdminStats = async () => {
      try {
        setStatsLoading(true);
        setStatsError("");

        const response = await fetch("http://127.0.0.1:8080/api/admin/stats");
        const data = await response.json();

        if (!response.ok || data.status !== "success") {
          throw new Error(data.message || "Could not load admin statistics.");
        }

        if (!cancelled) {
          const s = data.stats || {};
          setStats([
            { icon: "▣", label: "Total Cases", value: String(s.total_cases ?? 0).padStart(2, "0"), note: "Registered cases" },
            { icon: "👤", label: "Total Users", value: String(s.total_users ?? 0).padStart(2, "0"), note: "Registered accounts" },
            { icon: "◷", label: "Hearings", value: String(s.total_hearings ?? 0).padStart(2, "0"), note: "Scheduled hearings" },
            { icon: "⚖", label: "Active Cases", value: String(s.active_cases ?? 0).padStart(2, "0"), note: "Currently ongoing" },
          ]);
        }
      } catch (error) {
        console.error("Admin stats error:", error);
        if (!cancelled) setStatsError(error.message || "Unable to load admin statistics.");
      } finally {
        if (!cancelled) setStatsLoading(false);
      }
    };

    loadAdminStats();

    return () => {
      cancelled = true;
    };
  }, []);

  return (
    <div className="min-h-screen bg-[#07080b] text-white flex overflow-hidden">
      <aside className="hidden md:flex w-72 shrink-0 border-r border-white/10 bg-[#0b0c10] flex-col">
        <AdminBrand />
        <div className="p-4 flex-1 overflow-y-auto">
          <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600 px-3 mb-3">Administration</p>
          <DashboardNav icon="⌂" label="Overview" active={activePage === "overview"} onClick={() => navigate("overview")} />
          <DashboardNav icon="▣" label="Manage Cases" active={activePage === "cases"} onClick={() => navigate("cases")} />
          <DashboardNav icon="👤" label="Manage Users" active={activePage === "users"} onClick={() => navigate("users")} />
          <DashboardNav icon="◷" label="Hearings" active={activePage === "hearings"} onClick={() => navigate("hearings")} />
          <DashboardNav icon="⚖" label="Lawyers & Judges" active={activePage === "people"} onClick={() => navigate("people")} />
          <DashboardNav icon="◉" label="Notifications" active={activePage === "notifications"} onClick={() => navigate("notifications")} />
          <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600 px-3 mb-3 mt-8">System</p>
          <DashboardNav icon="▤" label="Reports & Analytics" active={activePage === "reports"} onClick={() => navigate("reports")} />
          <DashboardNav icon="⌁" label="Audit Logs" active={activePage === "audit"} onClick={() => navigate("audit")} />
        </div>
        <AdminUser name={adminName} initials={initials} onLogout={onLogout} />
      </aside>

      {mobileMenu && (
        <div className="fixed inset-0 z-[100] bg-black/70 backdrop-blur-sm md:hidden" onClick={() => setMobileMenu(false)}>
          <aside className="w-72 h-full bg-[#0b0c10] border-r border-white/10 p-4 overflow-y-auto" onClick={e => e.stopPropagation()}>
            <AdminBrand />
            <div className="mt-6">
              <DashboardNav icon="⌂" label="Overview" active={activePage === "overview"} onClick={() => navigate("overview")} />
              <DashboardNav icon="▣" label="Manage Cases" active={activePage === "cases"} onClick={() => navigate("cases")} />
              <DashboardNav icon="👤" label="Manage Users" active={activePage === "users"} onClick={() => navigate("users")} />
              <DashboardNav icon="◷" label="Hearings" active={activePage === "hearings"} onClick={() => navigate("hearings")} />
              <DashboardNav icon="⚖" label="Lawyers & Judges" active={activePage === "people"} onClick={() => navigate("people")} />
              <DashboardNav icon="◉" label="Notifications" active={activePage === "notifications"} onClick={() => navigate("notifications")} />
              <DashboardNav icon="▤" label="Reports & Analytics" active={activePage === "reports"} onClick={() => navigate("reports")} />
              <DashboardNav icon="⌁" label="Audit Logs" active={activePage === "audit"} onClick={() => navigate("audit")} />
            </div>
          </aside>
        </div>
      )}

      <main className="flex-1 min-w-0 overflow-y-auto">
        <header className="sticky top-0 z-40 h-20 border-b border-white/10 bg-[#07080b]/85 backdrop-blur-xl flex items-center justify-between px-4 md:px-10">
          <div className="flex items-center gap-3">
            <button className="md:hidden w-10 h-10 rounded-xl bg-white/5 border border-white/10" onClick={() => setMobileMenu(true)}>☰</button>
            <div>
              <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">Admin Portal</p>
              <h1 className="text-lg md:text-xl font-semibold">{getAdminTitle(activePage)}</h1>
            </div>
          </div>
          <div className="hidden sm:flex items-center gap-3">
            <div className="text-right">
              <p className="text-sm font-medium">{adminName}</p>
              <p className="text-xs text-gray-600">Administrator</p>
            </div>
            <div className="w-10 h-10 rounded-full bg-white text-black flex items-center justify-center font-bold">{initials}</div>
          </div>
        </header>

        <div className="p-4 md:p-8 lg:p-10 max-w-[1500px]">
          {activePage === "overview" && (
            <AdminOverview
              stats={stats}
              loading={statsLoading}
              error={statsError}
            />
          )}
          {activePage === "cases" && <AdminCasesPage />}
          {activePage === "users" && <AdminPlaceholderPage title="Manage Users" subtitle="View registered users and their account information." icon="👤" />}
          {activePage === "hearings" && <AdminPlaceholderPage title="Hearings" subtitle="Manage upcoming and completed court hearings." icon="◷" />}
          {activePage === "people" && <AdminPlaceholderPage title="Lawyers & Judges" subtitle="Manage lawyers, judges and court assignments." icon="⚖" />}
          {activePage === "notifications" && <AdminPlaceholderPage title="Notifications" subtitle="Send and manage court notifications." icon="◉" />}
          {activePage === "reports" && <AdminPlaceholderPage title="Reports & Analytics" subtitle="Review system activity and court statistics." icon="▤" />}
          {activePage === "audit" && <AdminPlaceholderPage title="Audit Logs" subtitle="Track important administrative actions." icon="⌁" />}
        </div>
      </main>
    </div>
  );
}

function AdminBrand() {
  return <div className="p-6 border-b border-white/10"><div className="flex items-center gap-3"><div className="w-10 h-10 rounded-xl bg-white text-black flex items-center justify-center text-lg">⚖</div><div><h2 className="font-bold tracking-wide">CourtCraft</h2><p className="text-xs text-gray-600">Admin Portal</p></div></div></div>;
}

function AdminUser({ name, initials, onLogout }) {
  return <div className="p-4 border-t border-white/10"><div className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]"><div className="w-9 h-9 rounded-full bg-white text-black flex items-center justify-center font-bold">{initials}</div><div className="flex-1 min-w-0"><p className="text-sm font-medium truncate">{name}</p><p className="text-xs text-gray-600">Administrator</p></div></div><button onClick={onLogout} className="w-full mt-3 py-2.5 rounded-xl text-sm text-gray-500 hover:text-white hover:bg-white/5 transition">↪ Logout</button></div>;
}

function getAdminTitle(page) {
  const titles = { cases:"Manage Cases", users:"Manage Users", hearings:"Hearings", people:"Lawyers & Judges", notifications:"Notifications", reports:"Reports & Analytics", audit:"Audit Logs" };
  return titles[page] || "Admin Overview";
}

function AdminOverview({ stats, loading = false, error = "" }) {
  return <div className="animate-fade-up">
    <div className="mb-8"><p className="text-gray-500">Administration 👋</p><h2 className="text-3xl md:text-4xl font-bold mt-1">CourtCraft Control Center</h2><p className="text-gray-600 mt-2">Manage cases, users, hearings and court operations from one place.</p></div>
    {error && (
      <div className="mb-5 rounded-2xl border border-red-400/20 bg-red-400/10 px-5 py-4 text-sm text-red-200">
        {error}
      </div>
    )}
    <div className="grid sm:grid-cols-2 xl:grid-cols-4 gap-4 md:gap-5">
      {stats.map(item => (
        <DashboardStat
          key={item.label}
          icon={item.icon}
          label={item.label}
          value={loading ? "…" : item.value}
          change={item.note}
        />
      ))}
    </div>
    <div className="grid xl:grid-cols-3 gap-5 mt-6">
      <section className="xl:col-span-2 rounded-3xl border border-white/10 bg-white/[0.03] p-6">
        <p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">System overview</p><h3 className="text-xl font-semibold mt-1">Administration workspace</h3>
        <div className="grid sm:grid-cols-2 gap-4 mt-6">
          <AdminActionCard icon="▣" title="Case Management" text="Review and manage registered cases." />
          <AdminActionCard icon="👤" title="User Management" text="View and manage CourtCraft accounts." />
          <AdminActionCard icon="◷" title="Hearing Management" text="Control upcoming court schedules." />
          <AdminActionCard icon="⚖" title="People Management" text="Manage lawyers and judges." />
        </div>
      </section>
      <section className="rounded-3xl border border-white/10 bg-white/[0.03] p-6">
        <p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">Recent activity</p><h3 className="text-xl font-semibold mt-1 mb-5">System Activity</h3>
        <div className="space-y-3">
          <AdminActivity icon="👤" title="Admin account created" text="CourtCraft Admin" time="Today" />
          <AdminActivity icon="◷" title="Hearing scheduled" text="Case #CC-3001" time="Today" />
          <AdminActivity icon="▣" title="Case registered" text="Property Dispute" time="Today" />
        </div>
      </section>
    </div>
    <div className="mt-6 rounded-3xl border border-emerald-400/10 bg-emerald-400/[0.04] p-5"><p className="text-sm font-semibold text-emerald-200">Admin access active</p><p className="text-xs text-gray-500 mt-1">You are signed in with the ADMIN role. Management modules can now be connected to the backend one by one.</p></div>
  </div>;
}

function AdminActionCard({ icon, title, text }) {
  return <div className="rounded-2xl border border-white/10 bg-white/[0.025] p-5 hover:bg-white/[0.05] hover:-translate-y-1 transition-all"><div className="w-10 h-10 rounded-xl bg-white/5 flex items-center justify-center">{icon}</div><p className="font-semibold mt-4">{title}</p><p className="text-xs text-gray-600 mt-1 leading-relaxed">{text}</p></div>;
}

function AdminActivity({ icon, title, text, time }) {
  return <div className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]"><div className="w-9 h-9 rounded-lg bg-white/5 flex items-center justify-center">{icon}</div><div className="flex-1 min-w-0"><p className="text-sm font-medium truncate">{title}</p><p className="text-xs text-gray-600 mt-1 truncate">{text}</p></div><span className="text-xs text-gray-700">{time}</span></div>;
}

function AdminCasesPage() {
  const [cases, setCases] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");
  const [search, setSearch] = useState("");
  const [filter, setFilter] = useState("All");
  const [selectedCase, setSelectedCase] = useState(null);

  const loadCases = async () => {
    try {
      setLoading(true);
      setError("");

      const response = await fetch("http://127.0.0.1:8080/api/admin/cases");
      const data = await response.json();

      if (!response.ok || data.status !== "success") {
        throw new Error(data.message || "Could not load cases.");
      }

      setCases(Array.isArray(data.cases) ? data.cases : []);
    } catch (err) {
      console.error("Admin cases error:", err);
      setError(err.message || "Unable to load cases.");
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadCases();
  }, []);

  const filteredCases = cases.filter((c) => {
    const matchesFilter =
      filter === "All" ||
      (filter === "Pending" && c.status === "PENDING") ||
      (filter === "Active" && c.status === "ONGOING") ||
      (filter === "Closed" && c.status === "CLOSED") ||
      (filter === "Dismissed" && c.status === "DISMISSED");

    const searchText =
      `${c.case_number || ""} ${c.title || ""} ${c.case_type || ""} ${c.user_name || ""} ${c.user_email || ""}`
        .toLowerCase();

    return matchesFilter && searchText.includes(search.toLowerCase());
  });

  const statusLabel = (status) => {
    const map = {
      PENDING: "Pending",
      ONGOING: "Active",
      CLOSED: "Closed",
      DISMISSED: "Dismissed",
    };
    return map[status] || status || "Unknown";
  };

  const statusClass = (status) => {
    if (status === "ONGOING") return "text-green-300 bg-green-400/10 border-green-400/10";
    if (status === "PENDING") return "text-yellow-300 bg-yellow-400/10 border-yellow-400/10";
    if (status === "CLOSED") return "text-blue-300 bg-blue-400/10 border-blue-400/10";
    if (status === "DISMISSED") return "text-red-300 bg-red-400/10 border-red-400/10";
    return "text-gray-300 bg-white/5 border-white/10";
  };

  if (selectedCase) {
    return (
      <div className="animate-fade-up">
        <button
          onClick={() => setSelectedCase(null)}
          className="text-sm text-gray-500 hover:text-white mb-5"
        >
          ← Back to cases
        </button>

        <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-6 md:p-8">
          <div className="flex flex-col md:flex-row md:items-start md:justify-between gap-4">
            <div>
              <p className="text-xs text-gray-600">{selectedCase.case_number}</p>
              <h2 className="text-3xl font-bold mt-1">{selectedCase.title}</h2>
              <p className="text-gray-500 mt-2">
                {selectedCase.case_type || "Case type not specified"}
              </p>
            </div>

            <span className={`px-4 py-2 rounded-full border text-sm w-fit ${statusClass(selectedCase.status)}`}>
              {statusLabel(selectedCase.status)}
            </span>
          </div>

          <p className="text-gray-400 mt-7 max-w-4xl">
            {selectedCase.description || "No description provided."}
          </p>

          <div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-3 mt-7">
            <AdminCaseInfo label="Case ID" value={selectedCase.id} />
            <AdminCaseInfo label="Filing Date" value={selectedCase.filing_date || "—"} />
            <AdminCaseInfo label="User" value={selectedCase.user_name || "Unassigned"} />
            <AdminCaseInfo label="User Email" value={selectedCase.user_email || "—"} />
          </div>

          <div className="mt-5 rounded-2xl border border-white/10 bg-black/20 p-5">
            <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">
              Database Record
            </p>
            <p className="text-xs text-gray-500 mt-2">
              Created: {selectedCase.created_at || "—"}
            </p>
          </div>
        </div>
      </div>
    );
  }

  return (
    <div className="animate-fade-up">
      <div className="flex flex-col md:flex-row md:items-end justify-between gap-4 mb-7">
        <div>
          <p className="text-gray-600 text-sm">Admin Management</p>
          <h2 className="text-3xl font-bold mt-1">Manage Cases</h2>
          <p className="text-gray-600 mt-2">
            All registered cases loaded directly from the CourtCraft database.
          </p>
        </div>

        <button
          onClick={loadCases}
          disabled={loading}
          className="px-5 py-3 rounded-xl bg-white text-black text-sm font-semibold hover:scale-[1.02] transition disabled:opacity-50"
        >
          {loading ? "Loading..." : "↻ Refresh Cases"}
        </button>
      </div>

      {error && (
        <div className="mb-5 rounded-2xl border border-red-400/20 bg-red-400/10 px-5 py-4 text-sm text-red-200">
          {error}
        </div>
      )}

      <div className="flex flex-col md:flex-row gap-3 mb-5">
        <input
          value={search}
          onChange={(e) => setSearch(e.target.value)}
          placeholder="Search case number, title, type, user..."
          className="flex-1 rounded-2xl bg-white/[0.04] border border-white/10 px-5 py-3.5 outline-none focus:border-white/30 text-sm"
        />

        <div className="flex gap-2 flex-wrap">
          {["All", "Pending", "Active", "Closed", "Dismissed"].map((item) => (
            <button
              key={item}
              onClick={() => setFilter(item)}
              className={`px-4 py-3 rounded-xl text-sm border transition ${
                filter === item
                  ? "bg-white text-black border-white"
                  : "bg-white/[0.03] text-gray-500 border-white/10 hover:text-white"
              }`}
            >
              {item}
            </button>
          ))}
        </div>
      </div>

      {!loading && !error && (
        <div className="mb-4 text-xs text-gray-600">
          Showing {filteredCases.length} of {cases.length} cases
        </div>
      )}

      {loading ? (
        <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">
          Loading cases from database...
        </div>
      ) : !error && filteredCases.length === 0 ? (
        <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">
          No cases found.
        </div>
      ) : (
        <div className="rounded-3xl border border-white/10 bg-white/[0.03] overflow-hidden">
          <div className="hidden md:grid grid-cols-[1.1fr_1.7fr_1fr_1.2fr_1fr_0.7fr] gap-4 px-5 py-4 border-b border-white/10 text-[10px] uppercase tracking-[0.15em] text-gray-600">
            <span>Case No.</span>
            <span>Title</span>
            <span>Type</span>
            <span>User</span>
            <span>Status</span>
            <span>View</span>
          </div>

          <div>
            {filteredCases.map((c) => (
              <button
                key={c.id}
                onClick={() => setSelectedCase(c)}
                className="w-full text-left px-5 py-5 border-b border-white/5 last:border-0 hover:bg-white/[0.035] transition"
              >
                <div className="grid md:grid-cols-[1.1fr_1.7fr_1fr_1.2fr_1fr_0.7fr] gap-4 items-center">
                  <div>
                    <p className="text-sm font-semibold">{c.case_number}</p>
                    <p className="text-[10px] text-gray-700 mt-1">ID #{c.id}</p>
                  </div>

                  <div>
                    <p className="text-sm text-gray-200">{c.title}</p>
                    <p className="text-xs text-gray-600 mt-1">
                      Filed {c.filing_date || "—"}
                    </p>
                  </div>

                  <p className="text-sm text-gray-500">{c.case_type || "—"}</p>

                  <div>
                    <p className="text-sm text-gray-300">{c.user_name || "Unassigned"}</p>
                    <p className="text-xs text-gray-700 mt-1 truncate">
                      {c.user_email || "—"}
                    </p>
                  </div>

                  <span className={`w-fit px-3 py-1.5 rounded-full border text-xs ${statusClass(c.status)}`}>
                    {statusLabel(c.status)}
                  </span>

                  <span className="text-sm text-white">Open →</span>
                </div>
              </button>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

function AdminCaseInfo({ label, value }) {
  return (
    <div className="rounded-xl bg-black/20 p-4">
      <p className="text-[10px] uppercase tracking-wider text-gray-700">{label}</p>
      <p className="text-sm text-gray-300 mt-1 truncate">{value}</p>
    </div>
  );
}

function AdminPlaceholderPage({ title, subtitle, icon }) {
  return <div className="animate-fade-up"><div className="mb-7"><p className="text-gray-600 text-sm">Admin Management</p><h2 className="text-3xl font-bold mt-1">{title}</h2><p className="text-gray-600 mt-2">{subtitle}</p></div><div className="rounded-3xl border border-white/10 bg-white/[0.03] p-10 md:p-14 text-center"><div className="w-16 h-16 mx-auto rounded-2xl bg-white/5 flex items-center justify-center text-2xl">{icon}</div><h3 className="text-xl font-semibold mt-5">{title}</h3><p className="text-sm text-gray-600 mt-2 max-w-md mx-auto">This module is ready for the next backend integration step.</p></div></div>;
}

/* ================================================= */
/* USER DASHBOARD */
/* ================================================= */

const COURTCRAFT_CASES = [
  { id:"CC-2048", title:"Property Dispute", court:"District Court", status:"Active", next:"18 Sep 2026", lawyer:"Adv. Rahul Sharma", judge:"Hon. A. Kumar", filing:"12 Jan 2026", description:"Property ownership dispute currently under judicial review." },
  { id:"CC-1987", title:"Civil Matter", court:"High Court", status:"Hearing", next:"24 Sep 2026", lawyer:"Adv. Priya Mehta", judge:"Hon. R. Singh", filing:"08 Feb 2026", description:"Civil matter involving contractual and financial claims." },
  { id:"CC-1920", title:"Contract Dispute", court:"District Court", status:"Resolved", next:"Closed", lawyer:"Adv. Amit Verma", judge:"Hon. S. Gupta", filing:"17 Nov 2025", description:"Contractual dispute successfully resolved by the court." },
  { id:"CC-1876", title:"Family Matter", court:"Family Court", status:"Active", next:"02 Oct 2026", lawyer:"Adv. Neha Kapoor", judge:"Hon. M. Sharma", filing:"23 Mar 2026", description:"Family court matter currently awaiting the next hearing." },
  { id:"CC-1765", title:"Consumer Complaint", court:"Consumer Court", status:"Hearing", next:"11 Oct 2026", lawyer:"Adv. Karan Singh", judge:"Hon. P. Verma", filing:"10 Apr 2026", description:"Consumer complaint regarding a disputed service transaction." },
];

const COURTCRAFT_HEARINGS = [
  { caseId:"CC-2048", title:"Property Dispute", date:"18 Sep 2026", time:"10:30 AM", room:"Court Room 4", judge:"Hon. A. Kumar", type:"Arguments" },
  { caseId:"CC-1987", title:"Civil Matter", date:"24 Sep 2026", time:"11:15 AM", room:"Court Room 2", judge:"Hon. R. Singh", type:"Evidence" },
  { caseId:"CC-1876", title:"Family Matter", date:"02 Oct 2026", time:"02:00 PM", room:"Family Court 1", judge:"Hon. M. Sharma", type:"Hearing" },
  { caseId:"CC-1765", title:"Consumer Complaint", date:"11 Oct 2026", time:"12:30 PM", room:"Consumer Court 3", judge:"Hon. P. Verma", type:"Arguments" },
];

const COURTCRAFT_NOTIFICATIONS = [
  { id:1, title:"Hearing scheduled", text:"CC-2048 has a hearing on 18 Sep 2026 at 10:30 AM.", time:"2 hours ago", unread:true },
  { id:2, title:"Case status updated", text:"CC-1920 has been marked as resolved.", time:"Yesterday", unread:true },
  { id:3, title:"Document reminder", text:"A document is pending for CC-1987.", time:"2 days ago", unread:false },
];

function UserDashboard({ onLogout }) {
  const [activePage, setActivePage] = useState("overview");
  const [selectedCase, setSelectedCase] = useState(null);
  const [mobileMenu, setMobileMenu] = useState(false);
  const [cases, setCases] = useState([]);
  const [showUpload, setShowUpload] = useState(false);
  const [casesLoading, setCasesLoading] = useState(false);
  const [hearings, setHearings] = useState([]);
  const [hearingsLoading, setHearingsLoading] = useState(false);
  const [hearingsError, setHearingsError] = useState("");
  const [notifications, setNotifications] = useState([]);
  const [notificationsLoading, setNotificationsLoading] = useState(false);
  const [notificationsError, setNotificationsError] = useState("");
  const session = JSON.parse(localStorage.getItem("courtcraftSession") || "null");
  const userName = session?.name || "User";
  const initials = userName.split(" ").map(x => x[0]).join("").slice(0,2).toUpperCase();

  const loadCases = async () => {
    if (!session?.id) return;
    setCasesLoading(true);
    try {
      const res = await fetch(`http://127.0.0.1:8080/api/cases/${session.id}`);
      const data = await res.json();
      if (res.ok && Array.isArray(data.cases)) setCases(data.cases.map(apiCaseToUi));
    } catch (e) {
      console.error("Failed to load cases", e);
    } finally { setCasesLoading(false); }
  };

  useEffect(() => { loadCases(); }, []);

  const loadHearings = async () => {
    if (!session?.id) return;
    setHearingsLoading(true);
    setHearingsError("");
    try {
      const res = await fetch(`http://127.0.0.1:8080/api/hearings/${session.id}`);
      const data = await res.json();

      if (!res.ok || data.status !== "success") {
        setHearingsError(data.message || "Could not load hearings.");
        return;
      }

      const mapped = (Array.isArray(data.hearings) ? data.hearings : []).map(h => ({
        caseId: h.case_number,
        title: h.title,
        date: formatHearingDate(h.hearing_date),
        time: formatHearingTime(h.hearing_time),
        room: h.court_room || "Not assigned",
        judge: h.judge_name || "Not assigned",
        type: h.purpose || "Hearing",
        status: h.status || "UPCOMING",
      }));

      setHearings(mapped);
    } catch (e) {
      console.error("Failed to load hearings:", e);
      setHearingsError("Unable to connect to CourtCraft server.");
    } finally {
      setHearingsLoading(false);
    }
  };

  useEffect(() => { loadHearings(); }, []);

  const loadNotifications = async () => {
    if (!session?.id) return;
    setNotificationsLoading(true);
    setNotificationsError("");
    try {
      const res = await fetch(`http://127.0.0.1:8080/api/notifications/${session.id}`);
      const data = await res.json();

      if (!res.ok || data.status !== "success") {
        setNotificationsError(data.message || "Could not load notifications.");
        return;
      }

      const mapped = (Array.isArray(data.notifications) ? data.notifications : []).map(n => ({
        id: n.id,
        title: n.title,
        text: n.message,
        time: formatNotificationTime(n.created_at),
        unread: !n.is_read,
      }));

      setNotifications(mapped);
    } catch (e) {
      console.error("Failed to load notifications:", e);
      setNotificationsError("Unable to connect to CourtCraft server.");
    } finally {
      setNotificationsLoading(false);
    }
  };

  useEffect(() => { loadNotifications(); }, []);

  const navigate = (page) => {
    setActivePage(page);
    setSelectedCase(null);
    setMobileMenu(false);
    if (page === "hearings") loadHearings();
  };
  const openCase = (caseData) => setSelectedCase(caseData);
  const unreadCount = notifications.filter(n => n.unread).length;

  return (
    <div className="min-h-screen bg-[#07080b] text-white flex overflow-hidden">
      <aside className="hidden md:flex w-64 shrink-0 border-r border-white/10 bg-[#0b0c10] flex-col">
        <DashboardBrand />
        <div className="p-4 flex-1">
          <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600 px-3 mb-3">Main Menu</p>
          <DashboardNav icon="⌂" label="Overview" active={activePage === "overview"} onClick={() => navigate("overview")} />
          <DashboardNav icon="▣" label="My Cases" active={activePage === "cases"} onClick={() => navigate("cases")} />
          <DashboardNav icon="◷" label="Hearings" active={activePage === "hearings"} onClick={() => navigate("hearings")} />
          <DashboardNav icon="◉" label="Notifications" active={activePage === "notifications"} badge={unreadCount} onClick={() => navigate("notifications")} />
          <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600 px-3 mb-3 mt-8">Account</p>
          <DashboardNav icon="◯" label="Profile" active={activePage === "profile"} onClick={() => navigate("profile")} />
        </div>
        <DashboardUser onLogout={onLogout} />
      </aside>

      {mobileMenu && (
        <div className="fixed inset-0 z-[100] bg-black/70 backdrop-blur-sm md:hidden" onClick={() => setMobileMenu(false)}>
          <aside className="w-72 h-full bg-[#0b0c10] border-r border-white/10 p-4" onClick={e => e.stopPropagation()}>
            <DashboardBrand />
            <div className="mt-6">
              <DashboardNav icon="⌂" label="Overview" active={activePage === "overview"} onClick={() => navigate("overview")} />
              <DashboardNav icon="▣" label="My Cases" active={activePage === "cases"} onClick={() => navigate("cases")} />
              <DashboardNav icon="◷" label="Hearings" active={activePage === "hearings"} onClick={() => navigate("hearings")} />
              <DashboardNav icon="◉" label="Notifications" active={activePage === "notifications"} badge={unreadCount} onClick={() => navigate("notifications")} />
              <DashboardNav icon="◯" label="Profile" active={activePage === "profile"} onClick={() => navigate("profile")} />
            </div>
          </aside>
        </div>
      )}

      <main className="flex-1 min-w-0 overflow-y-auto">
        <header className="sticky top-0 z-40 h-20 border-b border-white/10 bg-[#07080b]/85 backdrop-blur-xl flex items-center justify-between px-4 md:px-10">
          <div className="flex items-center gap-3">
            <button className="md:hidden w-10 h-10 rounded-xl bg-white/5 border border-white/10" onClick={() => setMobileMenu(true)}>☰</button>
            <div>
              <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">User Portal</p>
              <h1 className="text-lg md:text-xl font-semibold">{selectedCase ? "Case Details" : getDashboardTitle(activePage)}</h1>
            </div>
          </div>
          <div className="flex items-center gap-3">
            <button onClick={() => navigate("notifications")} className="relative w-10 h-10 rounded-xl bg-white/5 border border-white/10 hover:bg-white/10 transition">
              ◉{unreadCount > 0 && <span className="absolute top-2 right-2 w-2 h-2 rounded-full bg-red-400" />}
            </button>
            <button onClick={() => navigate("profile")} className="hidden sm:flex items-center gap-3 text-left hover:opacity-80 transition">
              <div><p className="text-sm font-medium">{userName}</p><p className="text-xs text-gray-600">User</p></div>
              <div className="w-10 h-10 rounded-full bg-white text-black flex items-center justify-center font-bold">{initials}</div>
            </button>
          </div>
        </header>

        <div className="p-4 md:p-8 lg:p-10 max-w-[1500px]">
          {selectedCase ? <CaseDetails caseData={selectedCase} onBack={() => setSelectedCase(null)} /> : (
            <>
              {activePage === "overview" && <OverviewPage onViewCases={() => navigate("cases")} onViewHearings={() => navigate("hearings")} onViewNotifications={() => navigate("notifications")} onViewCase={openCase} />}
              {activePage === "cases" && <CasesPage cases={cases} loading={casesLoading} onViewCase={openCase} onUpload={() => setShowUpload(true)} />}
              {activePage === "hearings" && (
                <HearingsPage
                  hearings={hearings}
                  loading={hearingsLoading}
                  error={hearingsError}
                />
              )}
              {activePage === "notifications" && <NotificationsPage notifications={notifications} loading={notificationsLoading} error={notificationsError} onReload={loadNotifications} />}
              {activePage === "profile" && <ProfilePage />}
            </>
          )}
        </div>
      </main>
      {showUpload && (
        <UploadCaseModal
          userId={session?.id}
          onClose={() => setShowUpload(false)}
          onCreated={(newCase) => { setCases(prev => [newCase, ...prev]); setShowUpload(false); setActivePage("cases"); }}
        />
      )}
    </div>
  );
}

function DashboardBrand() {
  return <div className="p-6 border-b border-white/10"><div className="flex items-center gap-3"><div className="w-10 h-10 rounded-xl bg-white text-black flex items-center justify-center text-lg">⚖</div><div><h2 className="font-bold tracking-wide">CourtCraft</h2><p className="text-xs text-gray-600">User Portal</p></div></div></div>;
}
function DashboardUser({ onLogout }) {
  return <div className="p-4 border-t border-white/10"><div className="flex items-center gap-3 p-3 rounded-xl bg-white/[0.03]"><div className="w-9 h-9 rounded-full bg-white text-black flex items-center justify-center font-bold">SV</div><div className="flex-1 min-w-0"><p className="text-sm font-medium truncate">Sudanshu Verma</p><p className="text-xs text-gray-600">User</p></div></div><button onClick={onLogout} className="w-full mt-3 py-2.5 rounded-xl text-sm text-gray-500 hover:text-white hover:bg-white/5 transition">↪ Logout</button></div>;
}
function DashboardNav({ icon, label, active, onClick, badge }) {
  return <button onClick={onClick} className={`w-full flex items-center gap-3 px-3 py-3 rounded-xl mb-1 text-sm transition-all ${active ? "bg-white text-black shadow-lg" : "text-gray-500 hover:text-white hover:bg-white/5"}`}><span className="w-6 text-center">{icon}</span><span className="flex-1 text-left">{label}</span>{badge > 0 && <span className={`min-w-5 h-5 px-1 rounded-full text-[10px] flex items-center justify-center ${active ? "bg-black text-white" : "bg-red-400/15 text-red-300"}`}>{badge}</span>}</button>;
}
function getDashboardTitle(page) { return page === "cases" ? "My Cases" : page === "hearings" ? "Hearings" : page === "notifications" ? "Notifications" : page === "profile" ? "My Profile" : "Overview"; }

function OverviewPage({ onViewCases, onViewHearings, onViewNotifications, onViewCase }) {
  const active = COURTCRAFT_CASES.filter(c => c.status === "Active").length;
  const resolved = COURTCRAFT_CASES.filter(c => c.status === "Resolved").length;
  return <div className="animate-fade-up">
    <div className="mb-8"><p className="text-gray-500">Good morning 👋</p><h2 className="text-3xl md:text-4xl font-bold mt-1">Welcome back, Sudanshu.</h2><p className="text-gray-600 mt-2">Here's a clear view of your legal activity.</p></div>
    <div className="grid sm:grid-cols-2 xl:grid-cols-4 gap-4 md:gap-5">
      <DashboardStat icon="▣" label="Total Cases" value={String(COURTCRAFT_CASES.length).padStart(2,"0")} change="Across all courts" />
      <DashboardStat icon="⚡" label="Active Cases" value={String(active).padStart(2,"0")} change="Currently active" />
      <DashboardStat icon="◷" label="Upcoming Hearings" value={String(COURTCRAFT_HEARINGS.length).padStart(2,"0")} change="Scheduled ahead" />
      <DashboardStat icon="✓" label="Resolved Cases" value={String(resolved).padStart(2,"0")} change="Successfully closed" />
    </div>
    <div className="grid xl:grid-cols-3 gap-5 mt-5">
      <section className="xl:col-span-2 rounded-3xl border border-white/10 bg-white/[0.03] p-5 md:p-6">
        <div className="flex items-center justify-between mb-4"><div><p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">Case activity</p><h3 className="text-xl font-semibold mt-1">Recent Cases</h3></div><button onClick={onViewCases} className="text-sm text-gray-500 hover:text-white">View all →</button></div>
        {COURTCRAFT_CASES.slice(0,4).map(c => <CaseRow key={c.id} {...c} onClick={() => onViewCase(c)} />)}
      </section>
      <section className="rounded-3xl border border-white/10 bg-white/[0.03] p-5 md:p-6">
        <p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">Next on calendar</p><h3 className="text-xl font-semibold mt-1 mb-5">Upcoming Hearing</h3>
        <div className="rounded-2xl bg-white/[0.045] border border-white/10 p-5"><div className="flex justify-between"><div><p className="text-[10px] text-gray-600">CASE</p><p className="font-semibold mt-1">{COURTCRAFT_HEARINGS[0].caseId}</p></div><div className="text-right"><p className="text-3xl font-bold">18</p><p className="text-[10px] text-gray-600">SEP 2026</p></div></div><div className="h-px bg-white/10 my-5"/><p className="text-sm text-gray-300">{COURTCRAFT_HEARINGS[0].title}</p><p className="text-xs text-gray-600 mt-2">◷ {COURTCRAFT_HEARINGS[0].time}</p><p className="text-xs text-gray-600 mt-1">⌖ {COURTCRAFT_HEARINGS[0].room}</p><button onClick={onViewHearings} className="w-full mt-5 py-3 rounded-xl bg-white text-black text-sm font-semibold hover:scale-[1.01] transition">View Hearings</button></div>
      </section>
    </div>
    <div className="mt-6"><p className="text-[10px] text-gray-600 uppercase tracking-[0.2em] mb-3">Quick Access</p><div className="grid sm:grid-cols-3 gap-4">
      <QuickCard icon="▣" title="View Cases" text="Track every case in one place" onClick={onViewCases}/><QuickCard icon="◷" title="Hearings" text="Check upcoming court dates" onClick={onViewHearings}/><QuickCard icon="◉" title="Notifications" text="Review your latest updates" onClick={onViewNotifications}/>
    </div></div>
  </div>;
}
function DashboardStat({icon,label,value,change}) { return <div className="rounded-2xl border border-white/10 bg-white/[0.03] p-5 hover:-translate-y-1 hover:bg-white/[0.05] transition-all"><div className="w-10 h-10 rounded-xl bg-white/5 flex items-center justify-center">{icon}</div><p className="text-sm text-gray-500 mt-5">{label}</p><p className="text-3xl font-bold mt-1">{value}</p><p className="text-xs text-gray-600 mt-2">{change}</p></div>; }
function QuickCard({icon,title,text,onClick}) { return <button onClick={onClick} className="text-left rounded-2xl border border-white/10 bg-white/[0.03] p-5 hover:bg-white/[0.06] hover:-translate-y-1 transition-all"><div className="w-10 h-10 rounded-xl bg-white/5 flex items-center justify-center">{icon}</div><p className="font-semibold mt-4">{title}</p><p className="text-xs text-gray-600 mt-1">{text}</p></button>; }
function CaseRow({id,title,court,status,date,onClick}) { const cls=status === "Active" ? "text-green-300 bg-green-400/10" : status === "Hearing" ? "text-blue-300 bg-blue-400/10" : "text-gray-400 bg-white/5"; return <button onClick={onClick} className="w-full text-left flex flex-col sm:flex-row sm:items-center gap-4 py-4 border-b border-white/5 last:border-0 hover:bg-white/[0.025] transition rounded-xl px-2"><div className="w-11 h-11 shrink-0 rounded-xl bg-white/5 flex items-center justify-center">▣</div><div className="flex-1"><p className="text-sm font-semibold">{title}</p><p className="text-xs text-gray-600 mt-1">{id} • {court}</p></div><div className="sm:text-right"><span className={`inline-block px-3 py-1 rounded-full text-xs ${cls}`}>{status}</span><p className="text-xs text-gray-700 mt-2">{date}</p></div></button>; }

function apiCaseToUi(c) {
  const statusMap = { PENDING:"Pending", ONGOING:"Active", CLOSED:"Resolved", DISMISSED:"Dismissed" };
  return { id:c.case_number, dbId:c.id, title:c.title, court:c.case_type || "Court", status:statusMap[c.status] || c.status || "Pending", next:"—", lawyer:"Not assigned", judge:"Not assigned", filing:c.filing_date || "—", description:c.description || "No description provided." };
}

function CasesPage({ cases, loading, onViewCase, onUpload }) {
  const [search,setSearch]=useState(""); const [filter,setFilter]=useState("All");
  const filtered=cases.filter(c=>(filter === "All" || c.status===filter) && `${c.id} ${c.title} ${c.court}`.toLowerCase().includes(search.toLowerCase()));
  return <div className="animate-fade-up"><div className="flex flex-col md:flex-row md:items-end justify-between gap-4 mb-7"><div><p className="text-gray-600 text-sm">Case Management</p><h2 className="text-3xl font-bold mt-1">My Cases</h2><p className="text-gray-600 mt-2">Your cases are loaded directly from CourtCraft database.</p></div><button onClick={onUpload} className="px-5 py-3 rounded-xl bg-white text-black text-sm font-semibold hover:scale-[1.02] transition">+ Upload Case</button></div>
    <div className="flex flex-col md:flex-row gap-3 mb-5"><input value={search} onChange={e=>setSearch(e.target.value)} placeholder="Search by case number, title, court..." className="flex-1 rounded-2xl bg-white/[0.04] border border-white/10 px-5 py-3.5 outline-none focus:border-white/30 text-sm"/><div className="flex gap-2 flex-wrap">{["All","Pending","Active","Resolved"].map(f=><button key={f} onClick={()=>setFilter(f)} className={`px-4 py-3 rounded-xl text-sm border ${filter===f?"bg-white text-black border-white":"bg-white/[0.03] text-gray-500 border-white/10 hover:text-white"}`}>{f}</button>)}</div></div>
    {loading ? <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">Loading your cases...</div> : filtered.length ? <div className="grid lg:grid-cols-2 gap-4">{filtered.map(c=><LargeCaseCard key={c.dbId || c.id} caseData={c} onClick={()=>onViewCase(c)}/>)}</div> : <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500"><p>No cases found in your account.</p><button onClick={onUpload} className="mt-4 px-5 py-3 rounded-xl bg-white text-black text-sm font-semibold">Upload your first case</button></div>}
  </div>;
}

function UploadCaseModal({ userId, onClose, onCreated }) {
  const [form,setForm]=useState({case_number:"",title:"",description:"",case_type:"Civil",filing_date:new Date().toISOString().slice(0,10)});
  const [loading,setLoading]=useState(false); const [error,setError]=useState("");
  const change=e=>setForm({...form,[e.target.name]:e.target.value});
  const submit=async()=>{
    if(!userId){setError("Please login again.");return;}
    if(!form.case_number.trim() || !form.title.trim()){setError("Case number and title are required.");return;}
    setLoading(true); setError("");
    try {
      const res=await fetch("http://127.0.0.1:8080/api/cases",{method:"POST",headers:{"Content-Type":"text/plain;charset=UTF-8"},body:JSON.stringify({...form,case_number:form.case_number.trim(),title:form.title.trim(),user_id:Number(userId)})});
      const data=await res.json();
      if(!res.ok){setError(data.message || "Could not upload case.");return;}
      onCreated(apiCaseToUi(data.case));
    } catch(e){setError("Backend se connection nahi ho pa raha. Check that CourtCraft backend is running.");}
    finally{setLoading(false);}
  };
  const Input=({name,label,type="text",placeholder})=><div><label className="text-xs text-gray-500">{label}</label><input name={name} type={type} value={form[name]} onChange={change} placeholder={placeholder} className="mt-2 w-full px-4 py-3 rounded-xl bg-white/5 border border-white/10 outline-none focus:border-white/30 text-sm"/></div>;
  return <div className="fixed inset-0 z-[200] bg-black/75 backdrop-blur-md flex items-center justify-center p-4" onClick={onClose}><div className="w-full max-w-2xl max-h-[90vh] overflow-y-auto rounded-3xl border border-white/10 bg-[#111217] p-6 md:p-8 shadow-2xl" onClick={e=>e.stopPropagation()}><div className="flex justify-between items-start mb-6"><div><p className="text-xs text-gray-600 uppercase tracking-[0.2em]">Case Management</p><h2 className="text-2xl font-bold mt-1">Upload New Case</h2><p className="text-sm text-gray-600 mt-2">This case will be saved in the CourtCraft database.</p></div><button onClick={onClose} className="w-9 h-9 rounded-xl bg-white/5 text-gray-400 hover:text-white">×</button></div>{error&&<div className="mb-5 rounded-xl border border-red-400/20 bg-red-400/10 px-4 py-3 text-sm text-red-200">{error}</div>}<div className="grid md:grid-cols-2 gap-4"><Input name="case_number" label="Case Number *" placeholder="e.g. CC-2050"/><Input name="title" label="Case Title *" placeholder="e.g. Property Dispute"/><Input name="case_type" label="Case Type" placeholder="e.g. Civil"/><Input name="filing_date" label="Filing Date" type="date"/></div><div className="mt-4"><label className="text-xs text-gray-500">Description</label><textarea name="description" value={form.description} onChange={change} rows="5" placeholder="Describe the case..." className="mt-2 w-full px-4 py-3 rounded-xl bg-white/5 border border-white/10 outline-none focus:border-white/30 text-sm resize-none"/></div><div className="flex gap-3 justify-end mt-6"><button onClick={onClose} className="px-5 py-3 rounded-xl border border-white/10 text-sm text-gray-400 hover:text-white">Cancel</button><button onClick={submit} disabled={loading} className="px-6 py-3 rounded-xl bg-white text-black text-sm font-semibold disabled:opacity-50">{loading?"Uploading...":"Upload Case"}</button></div></div></div>;
}

function LargeCaseCard({caseData,onClick}) { const c=caseData; return <button onClick={onClick} className="text-left rounded-3xl border border-white/10 bg-white/[0.03] p-5 md:p-6 hover:bg-white/[0.055] hover:-translate-y-1 transition-all"><div className="flex justify-between gap-4"><div><p className="text-xs text-gray-600">{c.id}</p><h3 className="text-lg font-semibold mt-1">{c.title}</h3></div><span className="h-fit px-3 py-1 rounded-full text-xs bg-white/5 text-gray-300">{c.status}</span></div><p className="text-sm text-gray-500 mt-4">{c.description}</p><div className="grid grid-cols-2 gap-3 mt-5"><MiniInfo label="Court" value={c.court}/><MiniInfo label="Next" value={c.next}/><MiniInfo label="Lawyer" value={c.lawyer}/><MiniInfo label="Judge" value={c.judge}/></div><div className="mt-5 pt-4 border-t border-white/10 flex justify-between text-xs"><span className="text-gray-600">Filed {c.filing}</span><span className="text-white">Open case →</span></div></button>; }
function MiniInfo({label,value}) { return <div className="rounded-xl bg-black/20 p-3"><p className="text-[10px] uppercase tracking-wider text-gray-700">{label}</p><p className="text-xs text-gray-300 mt-1 truncate">{value}</p></div>; }

function CaseDetails({caseData,onBack}) { const c=caseData; return <div className="animate-fade-up"><button onClick={onBack} className="text-sm text-gray-500 hover:text-white mb-5">← Back to cases</button><div className="rounded-3xl border border-white/10 bg-white/[0.03] p-6 md:p-8"><div className="flex flex-col md:flex-row md:items-start md:justify-between gap-4"><div><p className="text-xs text-gray-600">{c.id}</p><h2 className="text-3xl font-bold mt-1">{c.title}</h2><p className="text-gray-500 mt-2">{c.court}</p></div><span className="px-4 py-2 rounded-full bg-white/5 text-sm w-fit">{c.status}</span></div><p className="text-gray-400 mt-7 max-w-3xl">{c.description}</p><div className="grid sm:grid-cols-2 lg:grid-cols-4 gap-3 mt-7"><MiniInfo label="Filed" value={c.filing}/><MiniInfo label="Lawyer" value={c.lawyer}/><MiniInfo label="Judge" value={c.judge}/><MiniInfo label="Next Hearing" value={c.next}/></div></div><div className="grid lg:grid-cols-3 gap-5 mt-5"><div className="lg:col-span-2 rounded-3xl border border-white/10 bg-white/[0.03] p-6"><p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">Case timeline</p><h3 className="text-xl font-semibold mt-1 mb-6">Progress history</h3><TimelineItem title="Case registered" text={`Filed on ${c.filing}`} done/><TimelineItem title="Documents reviewed" text="Initial case documents were reviewed." done/><TimelineItem title={c.status === "Resolved" ? "Case resolved" : "Next hearing"} text={c.next === "Closed" ? "Matter successfully closed." : `Scheduled for ${c.next}.`} done={c.status === "Resolved"}/></div><div className="rounded-3xl border border-white/10 bg-white/[0.03] p-6"><p className="text-[10px] text-gray-600 uppercase tracking-[0.2em]">Case summary</p><div className="mt-5 space-y-3"><MiniInfo label="Case ID" value={c.id}/><MiniInfo label="Court" value={c.court}/><MiniInfo label="Status" value={c.status}/></div></div></div></div>; }
function TimelineItem({title,text,done}) { return <div className="flex gap-4 pb-6 last:pb-0"><div className={`w-8 h-8 shrink-0 rounded-full flex items-center justify-center text-xs ${done?"bg-white text-black":"bg-white/5 text-gray-500"}`}>{done?"✓":"•"}</div><div><p className="text-sm font-medium">{title}</p><p className="text-xs text-gray-600 mt-1">{text}</p></div></div>; }

function formatHearingDate(value) {
  if (!value) return "—";
  const parts = String(value).split("-");
  if (parts.length !== 3) return String(value);
  const [year, month, day] = parts;
  const date = new Date(Number(year), Number(month) - 1, Number(day));
  return date.toLocaleDateString("en-IN", {
    day: "2-digit",
    month: "short",
    year: "numeric",
  });
}

function formatHearingTime(value) {
  if (!value) return "—";
  const parts = String(value).split(":");
  if (parts.length < 2) return String(value);
  let hour = Number(parts[0]);
  const minute = parts[1];
  if (Number.isNaN(hour)) return String(value);
  const suffix = hour >= 12 ? "PM" : "AM";
  hour = hour % 12 || 12;
  return `${String(hour).padStart(2, "0")}:${minute} ${suffix}`;
}

function HearingsPage({ hearings = [], loading = false, error = "" }) {
  return (
    <div className="animate-fade-up">
      <div className="mb-7">
        <p className="text-gray-600 text-sm">Court Schedule</p>
        <h2 className="text-3xl font-bold mt-1">Hearings</h2>
        <p className="text-gray-600 mt-2">
          Your upcoming court hearings loaded directly from CourtCraft database.
        </p>
      </div>

      {loading ? (
        <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">
          Loading your hearings...
        </div>
      ) : error ? (
        <div className="rounded-3xl border border-red-400/20 bg-red-400/5 p-8 text-center">
          <p className="text-red-300">{error}</p>
          <p className="text-gray-600 text-sm mt-2">
            Make sure the CourtCraft backend is running on port 8080.
          </p>
        </div>
      ) : hearings.length === 0 ? (
        <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center">
          <div className="text-4xl mb-4">◷</div>
          <p className="text-gray-400">No hearings scheduled.</p>
          <p className="text-gray-700 text-sm mt-2">
            New hearings will appear here when added by the court.
          </p>
        </div>
      ) : (
        <div className="grid lg:grid-cols-2 gap-4">
          {hearings.map((h, index) => (
            <div
              key={`${h.caseId}-${h.date}-${h.time}-${index}`}
              className="rounded-3xl border border-white/10 bg-white/[0.03] p-5 md:p-6 hover:bg-white/[0.05] hover:-translate-y-1 transition-all"
            >
              <div className="flex items-start justify-between gap-4">
                <div>
                  <p className="text-[10px] uppercase tracking-[0.2em] text-gray-600">
                    Case
                  </p>
                  <p className="font-semibold mt-1">{h.caseId}</p>
                  <p className="text-gray-400 text-sm mt-1">{h.title}</p>
                </div>

                <span className="px-3 py-1 rounded-full text-xs text-blue-300 bg-blue-400/10 border border-blue-400/10">
                  {h.status === "UPCOMING" ? "Upcoming" : h.status}
                </span>
              </div>

              <div className="grid sm:grid-cols-2 gap-3 mt-6">
                <MiniInfo label="Date" value={h.date} />
                <MiniInfo label="Time" value={h.time} />
                <MiniInfo label="Court Room" value={h.room} />
                <MiniInfo label="Judge" value={h.judge} />
              </div>

              <div className="mt-4 rounded-2xl bg-white/[0.035] border border-white/5 p-4">
                <p className="text-[10px] uppercase tracking-[0.18em] text-gray-600">
                  Purpose
                </p>
                <p className="text-sm text-gray-300 mt-1">{h.type}</p>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}

function HearingCard({hearing:h,featured}) { return <div className={`rounded-3xl border p-5 md:p-6 ${featured?"border-white/25 bg-white/[0.06]":"border-white/10 bg-white/[0.03]"}`}><div className="flex flex-col md:flex-row md:items-center gap-5"><div className="w-20 h-20 shrink-0 rounded-2xl bg-white text-black flex flex-col items-center justify-center"><span className="text-2xl font-bold">{h.date.split(" ")[0]}</span><span className="text-[10px] font-semibold">{h.date.split(" ")[1]}</span></div><div className="flex-1"><div className="flex flex-wrap items-center gap-2"><span className="text-xs text-gray-600">{h.caseId}</span><span className="px-2 py-1 rounded-full bg-white/5 text-[10px] text-gray-400">{h.type}</span></div><h3 className="text-lg font-semibold mt-1">{h.title}</h3><p className="text-sm text-gray-500 mt-1">{h.judge} • {h.room}</p></div><div className="md:text-right"><p className="text-sm font-semibold">{h.time}</p><p className="text-xs text-gray-600 mt-1">{h.date}</p></div></div></div>; }

function formatNotificationTime(value) {
  if (!value) return "—";
  const date = new Date(String(value).replace(" ", "T"));
  if (Number.isNaN(date.getTime())) return String(value);
  return date.toLocaleString("en-IN", {
    day: "2-digit",
    month: "short",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
  });
}

function NotificationsPage({ notifications, loading, error, onReload }) {
  return <div className="animate-fade-up">
    <div className="flex items-end justify-between mb-7">
      <div>
        <p className="text-gray-600 text-sm">Updates</p>
        <h2 className="text-3xl font-bold mt-1">Notifications</h2>
        <p className="text-gray-600 mt-2">Stay informed about your cases and hearings.</p>
      </div>
      <button onClick={onReload} className="text-xs text-gray-500 hover:text-white">Refresh</button>
    </div>

    {loading && <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">Loading notifications...</div>}
    {!loading && error && <div className="rounded-3xl border border-red-400/20 bg-red-400/10 p-6 text-center text-red-200">{error}</div>}
    {!loading && !error && !notifications.length && <div className="rounded-3xl border border-white/10 bg-white/[0.03] p-12 text-center text-gray-500">No notifications yet.</div>}

    {!loading && !error && notifications.length > 0 && (
      <div className="space-y-3">
        {notifications.map(n =>
          <div key={n.id} className="w-full text-left rounded-2xl border border-white/10 bg-white/[0.03] p-5 transition flex gap-4">
            <div className={`w-10 h-10 shrink-0 rounded-xl flex items-center justify-center ${n.unread ? "bg-white text-black" : "bg-white/5 text-gray-500"}`}>◉</div>
            <div className="flex-1">
              <div className="flex justify-between gap-3">
                <p className="text-sm font-semibold">{n.title}</p>
                {n.unread && <span className="text-[10px] text-red-300">NEW</span>}
              </div>
              <p className="text-sm text-gray-500 mt-1">{n.text}</p>
              <p className="text-xs text-gray-700 mt-2">{n.time}</p>
            </div>
          </div>
        )}
      </div>
    )}
  </div>;
}

function ProfilePage() { const [editing,setEditing]=useState(false); const [saved,setSaved]=useState(false); return <div className="animate-fade-up max-w-4xl"><div className="mb-7"><p className="text-gray-600 text-sm">Account</p><h2 className="text-3xl font-bold mt-1">My Profile</h2><p className="text-gray-600 mt-2">Manage your personal information.</p></div><div className="rounded-3xl border border-white/10 bg-white/[0.03] p-6 md:p-8"><div className="flex flex-col sm:flex-row items-start sm:items-center gap-5 pb-7 border-b border-white/10"><div className="w-20 h-20 rounded-2xl bg-white text-black flex items-center justify-center text-2xl font-bold">SV</div><div className="flex-1"><h3 className="text-xl font-semibold">Sudanshu Verma</h3><p className="text-sm text-gray-600 mt-1">CourtCraft User</p></div><button onClick={()=>{setEditing(!editing);setSaved(false)}} className="px-4 py-2.5 rounded-xl bg-white text-black text-sm font-semibold">{editing?"Cancel":"Edit Profile"}</button></div><div className="grid md:grid-cols-2 gap-4 mt-7">{editing?<><EditableField label="Full Name" value="Sudanshu Verma"/><EditableField label="Email" value="sudanshu@example.com"/><EditableField label="Phone" value="+91 98XXXXXX00"/><EditableField label="City" value="Agra, Uttar Pradesh"/></>:<><ProfileField label="Full Name" value="Sudanshu Verma"/><ProfileField label="Email" value="sudanshu@example.com"/><ProfileField label="Phone" value="+91 98XXXXXX00"/><ProfileField label="City" value="Agra, Uttar Pradesh"/></>}</div>{editing&&<button onClick={()=>{setSaved(true);setEditing(false)}} className="mt-6 px-5 py-3 rounded-xl bg-white text-black text-sm font-semibold">Save Changes</button>}{saved&&<p className="text-xs text-green-300 mt-3">Profile changes saved locally.</p>}</div></div>; }
function EditableField({label,value}) { return <label className="rounded-2xl bg-white/[0.03] border border-white/10 p-4"><span className="text-xs text-gray-600">{label}</span><input defaultValue={value} className="block w-full bg-transparent outline-none text-sm mt-2"/></label>; }

/* ================================================= */
/* EXPORT */
/* ================================================= */

export default App;
