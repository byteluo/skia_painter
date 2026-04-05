#!/usr/bin/env node

const fs = require("fs");
const http = require("http");
const path = require("path");
const { URL } = require("url");
const { execFile } = require("child_process");

const rootDir = path.resolve(__dirname, "..");
const casesPath = path.join(rootDir, "web", "compare", "cases.json");
const compareCases = JSON.parse(fs.readFileSync(casesPath, "utf8"));

const port = Number.parseInt(process.env.PORT || "8787", 10);
const engineBinary =
  process.env.CANVAS_ENGINE_BIN || path.join(rootDir, "build", "canvas_engine");

const contentTypes = {
  ".css": "text/css; charset=utf-8",
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json; charset=utf-8",
  ".png": "image/png",
  ".svg": "image/svg+xml"
};

function sendJson(response, statusCode, payload) {
  response.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Cache-Control": "no-store"
  });
  response.end(JSON.stringify(payload));
}

function sendText(response, statusCode, payload) {
  response.writeHead(statusCode, {
    "Content-Type": "text/plain; charset=utf-8",
    "Cache-Control": "no-store"
  });
  response.end(payload);
}

function safeJoin(baseDir, unsafePath) {
  const targetPath = path.resolve(baseDir, unsafePath.replace(/^\/+/, ""));
  if (!targetPath.startsWith(baseDir)) {
    return null;
  }
  return targetPath;
}

function serveFile(response, filePath) {
  const extension = path.extname(filePath);
  const contentType = contentTypes[extension] || "application/octet-stream";

  fs.readFile(filePath, (error, data) => {
    if (error) {
      if (error.code === "ENOENT") {
        sendText(response, 404, "Not found");
        return;
      }

      sendText(response, 500, error.message);
      return;
    }

    response.writeHead(200, {
      "Content-Type": contentType,
      "Cache-Control": extension === ".png" ? "no-cache" : "no-store"
    });
    response.end(data);
  });
}

function renderCase(caseInfo) {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(engineBinary)) {
      reject(
        new Error(
          `canvas_engine not found at ${engineBinary}. Build the project first or set CANVAS_ENGINE_BIN.`
        )
      );
      return;
    }

    execFile(
      engineBinary,
      [path.join(rootDir, caseInfo.exampleScript)],
      {
        cwd: rootDir,
        timeout: 120000,
        maxBuffer: 16 * 1024 * 1024
      },
      (error, stdout, stderr) => {
        if (error) {
          const details = [stdout, stderr].filter(Boolean).join("\n");
          reject(new Error(details || error.message));
          return;
        }

        resolve({ stdout, stderr });
      }
    );
  });
}

const server = http.createServer(async (request, response) => {
  const url = new URL(request.url, `http://${request.headers.host || "127.0.0.1"}`);

  if (request.method === "GET" && url.pathname === "/api/cases") {
    sendJson(response, 200, compareCases);
    return;
  }

  if (request.method === "POST" && url.pathname.startsWith("/api/render/")) {
    const caseId = url.pathname.slice("/api/render/".length);
    const caseInfo = compareCases.find((item) => item.id === caseId);

    if (!caseInfo) {
      sendJson(response, 404, { ok: false, error: `unknown case: ${caseId}` });
      return;
    }

    try {
      await renderCase(caseInfo);
      sendJson(response, 200, {
        ok: true,
        imageUrl: `/${caseInfo.outputFile}?t=${Date.now()}`
      });
    } catch (error) {
      sendJson(response, 500, {
        ok: false,
        error: error.message
      });
    }
    return;
  }

  if (request.method === "GET" && url.pathname === "/") {
    response.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
    const html = fs.readFileSync(path.join(rootDir, "web", "compare", "index.html"), "utf8");
    response.end(html.replace("</head>", "<script>window.__devServer=true;</script></head>"));
    return;
  }

  if (request.method === "GET" && url.pathname.startsWith("/compare/")) {
    const targetPath = safeJoin(path.join(rootDir, "web", "compare"), url.pathname.slice("/compare/".length));
    if (!targetPath) {
      sendText(response, 400, "Bad request");
      return;
    }
    serveFile(response, targetPath);
    return;
  }

  if (request.method === "GET" && url.pathname.startsWith("/output/")) {
    const targetPath = safeJoin(path.join(rootDir, "output"), url.pathname.slice("/output/".length));
    if (!targetPath) {
      sendText(response, 400, "Bad request");
      return;
    }
    serveFile(response, targetPath);
    return;
  }

  if (request.method === "GET" && url.pathname.startsWith("/node_modules/")) {
    const targetPath = safeJoin(rootDir, url.pathname);
    if (!targetPath) {
      sendText(response, 400, "Bad request");
      return;
    }
    serveFile(response, targetPath);
    return;
  }

  // Serve compare files at root level (relative path support)
  {
    const compareDir = path.join(rootDir, "web", "compare");
    const targetPath = safeJoin(compareDir, url.pathname);
    if (targetPath && fs.existsSync(targetPath) && fs.statSync(targetPath).isFile()) {
      serveFile(response, targetPath);
      return;
    }
  }

  // Serve echarts.js at root (for relative path in index.html)
  if (request.method === "GET" && url.pathname === "/echarts.js") {
    serveFile(response, path.join(rootDir, "node_modules", "echarts", "dist", "echarts.js"));
    return;
  }

  sendText(response, 404, "Not found");
});

server.listen(port, "127.0.0.1", () => {
  console.log(`compare server: http://127.0.0.1:${port}`);
  console.log(`canvas engine: ${engineBinary}`);
});
