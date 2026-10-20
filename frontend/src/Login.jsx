import { useState } from "react";

function Login() {
  const [role, setRole] = useState("user");

  return (
    <div className="min-h-screen bg-[#08090d] text-white flex items-center justify-center">

      <div className="w-full max-w-md p-8">

        <h1 className="text-4xl font-bold text-center mb-8">
          CourtCraft Login
        </h1>

        <p className="text-gray-400 mb-3">
          Login as:
        </p>

        <div className="grid grid-cols-2 gap-4">

          <button
            onClick={() => setRole("user")}
            className={`p-4 rounded-xl border ${
              role === "user"
                ? "bg-white text-black"
                : "border-white/20 text-white"
            }`}
          >
            👤 User
          </button>

          <button
            onClick={() => setRole("admin")}
            className={`p-4 rounded-xl border ${
              role === "admin"
                ? "bg-white text-black"
                : "border-white/20 text-white"
            }`}
          >
            🛡️ Admin
          </button>

        </div>

        <p className="text-center mt-6 text-gray-400">
          Selected:{" "}
          <span className="text-white font-semibold">
            {role === "user" ? "User" : "Admin"}
          </span>
        </p>

      </div>

    </div>
  );
}

export default Login;