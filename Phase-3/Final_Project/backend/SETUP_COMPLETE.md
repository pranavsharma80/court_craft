# CourtCraft Backend Setup Guide

## ✅ Setup Complete!

The CourtCraft backend has been successfully configured, built, and tested.

### Quick Start

**To run the server:**

```bash
cd backend\build
set PATH=C:\Program Files\MySQL\MySQL Server 8.0\bin;C:\Program Files\MySQL\MySQL Server 8.0\lib;%PATH%
.\courtcraft.exe
```

Or use the batch script:
```bash
cd backend\build
cmd /c run_server.bat
```

The server will start on **http://localhost:8080**

---

## 📋 What Was Fixed

### 1. **VS Code IntelliSense - MySQL Headers**
   - ✅ Created `.vscode/c_cpp_properties.json` with proper include paths
   - ✅ Added MySQL header directory: `C:\Program Files\MySQL\MySQL Server 8.0\include`
   - ✅ Added vcpkg include paths for Crow and ASIO

### 2. **CMake Configuration**
   - ✅ Updated `CMakeLists.txt` to set `CMAKE_PREFIX_PATH` for vcpkg
   - ✅ Properly linked Windows socket libraries: `ws2_32`, `mswsock`
   - ✅ Configured MySQL include directories and libraries
   - ✅ Added post-build command to copy `libmysql.dll` to executable directory

### 3. **Compiler Detection**
   - ✅ Detected MinGW g++ 15.1.0 (BrechtSanders build)
   - ✅ Set up MinGW Makefiles generator for CMake
   - ✅ Verified C++17 standard compatibility

### 4. **MySQL Integration**
   - ✅ Linked against `C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.lib`
   - ✅ Copied `libmysql.dll` to build directory
   - ✅ Resolved runtime DLL dependencies (libgcc, libstdc++, libmcfgthread)
   - ✅ Configured MySQL connection details in code

### 5. **Runtime Environment**
   - ✅ Added MySQL directories to PATH: 
     - `C:\Program Files\MySQL\MySQL Server 8.0\bin`
     - `C:\Program Files\MySQL\MySQL Server 8.0\lib`

---

## 🧪 Verified Endpoints

### 1. Root Endpoint
```
GET http://localhost:8080/
Response: "CourtCraft API is running!"
```

### 2. Health Check
```
GET http://localhost:8080/api/health
Response: {"status": "ok", "message": "CourtCraft backend is healthy"}
```

### 3. Database Connectivity Test
```
GET http://localhost:8080/api/db-test
Response: {"status": "connected", "database": "court_craft"}
```

---

## 📁 Project Structure

```
backend/
├── CMakeLists.txt              # Compiler config, MySQL/Crow setup
├── src/
│   └── main.cpp                # Crow API server + MySQL test
└── build/
    ├── courtcraft.exe          # Compiled executable ✅
    ├── libmysql.dll            # MySQL runtime library ✅
    ├── libgcc_s_seh-1.dll       # MinGW runtime ✅
    ├── libstdc++-6.dll          # MinGW C++ runtime ✅
    ├── libmcfgthread-2.dll      # MinGW threading ✅
    └── run_server.bat           # Server launcher script ✅
```

---

## 🔧 Environment Details

- **Compiler**: `g++.exe` (MinGW-W64 15.1.0, BrechtSanders MCF UCRT)
  - Path: `C:\Users\Rasmeet Sandhu\AppData\Local\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.MCF.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin`

- **Build System**: CMake 4.0.1 (same location)

- **Database**: MySQL Server 8.0.46
  - Headers: `C:\Program Files\MySQL\MySQL Server 8.0\include`
  - Libraries: `C:\Program Files\MySQL\MySQL Server 8.0\lib`

- **Web Framework**: Crow 1.3.4 (vcpkg x64-mingw-dynamic)
  - Location: `vcpkg/installed/x64-mingw-dynamic`

- **Async Library**: ASIO (header-only, via vcpkg)

---

## 🛠️ How to Rebuild

```bash
cd backend
rm -r build
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++ ..
cmake --build . --config Release
```

The build will:
1. Compile `src/main.cpp` with Crow and MySQL headers
2. Link against Crow::Crow (header-only), MySQL client library, and Windows socket libs
3. Copy runtime DLLs to the executable directory

---

## 📝 Key Configuration Files

### `.vscode/c_cpp_properties.json`
- Configures IntelliSense for the project
- Includes paths for MySQL headers, Crow, ASIO, and MinGW
- Compiler: MinGW g++.exe
- Language standard: C++17

### `backend/CMakeLists.txt`
- Sets C++17 standard
- Configures vcpkg prefix path
- Finds and links Crow package
- Defines MySQL include and library paths
- Links Windows socket libraries
- Post-build: copies libmysql.dll to executable directory

### `backend/src/main.cpp`
- Crow REST API server on port 8080
- MySQL connection pool with error handling
- Three endpoints: `/`, `/api/health`, `/api/db-test`

---

## 🚀 Next Steps

1. **Start the server**: Use the command above
2. **Test endpoints**: Use `curl`, Postman, or any HTTP client
3. **Develop API routes**: Add more endpoints to `src/main.cpp`
4. **Connect frontend**: Point React frontend to `http://localhost:8080`

---

## 🆘 Troubleshooting

### Server won't start (DLL not found)
**Solution**: Ensure MySQL directories are in PATH:
```bash
set PATH=C:\Program Files\MySQL\MySQL Server 8.0\bin;C:\Program Files\MySQL\MySQL Server 8.0\lib;%PATH%
```

### `#include <mysql.h>` shows errors in VS Code
**Solution**: IntelliSense is configured. Try:
- `Ctrl+Shift+P` → "C/C++: Rescan Solutions"
- Restart VS Code

### Build fails with linking errors
**Solution**: Verify:
- CMake configuration succeeded: `cmake -G "MinGW Makefiles" ..`
- MySQL lib directory exists: `C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.lib`
- All required packages in PATH

### Database connection fails (status: "failed")
**Solution**: Check:
- MySQL Server is running
- Credentials are correct (root/<your MySQL password>)
- Database "court_craft" exists with proper tables

---

## ✨ Summary

The CourtCraft backend is now fully functional with:
- ✅ C++ compilation with MinGW and CMake
- ✅ Crow REST API framework
- ✅ MySQL database connectivity
- ✅ Three working API endpoints
- ✅ Proper VS Code IntelliSense support
- ✅ Ready for production development

Enjoy building! 🎉
