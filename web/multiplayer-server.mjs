#!/usr/bin/env node
/*
 * Minimal WebSocket lobby/relay for browser-to-browser Contra co-op.
 *
 * It serves web/ as static files and relays per-frame input/hash JSON messages
 * between two clients in a room. It intentionally has no npm dependencies so it
 * can run anywhere Node is available:
 *
 *   node web/multiplayer-server.mjs
 *   CONTRA_WEB_PORT=8787 node web/multiplayer-server.mjs
 */

import { createHash } from "node:crypto";
import { createReadStream, existsSync, statSync } from "node:fs";
import { createServer } from "node:http";
import { extname, join, normalize, resolve, sep } from "node:path";
import { fileURLToPath } from "node:url";

const SCRIPT_DIR = resolve(fileURLToPath(new URL(".", import.meta.url)));
const PORT = Number(process.env.CONTRA_WEB_PORT || process.env.PORT || 8787);
const WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const ROOMS = new Map();

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".data": "application/octet-stream",
  ".json": "application/json; charset=utf-8"
};

function sendHttp(res, status, body, headers = {}) {
  res.writeHead(status, {
    "content-type": "text/plain; charset=utf-8",
    ...headers
  });
  res.end(body);
}

function staticPath(urlPath) {
  const cleanPath = normalize(decodeURIComponent(urlPath.split("?")[0])).replace(/^(\.\.[/\\])+/, "");
  const relativePath = cleanPath === "/" ? "index.html" : cleanPath.replace(/^[/\\]/, "");
  const fullPath = resolve(join(SCRIPT_DIR, relativePath));

  if (fullPath !== SCRIPT_DIR && !fullPath.startsWith(SCRIPT_DIR + sep)) {
    return null;
  }

  return fullPath;
}

function serveStatic(req, res) {
  const path = staticPath(req.url || "/");

  if (!path || !existsSync(path) || !statSync(path).isFile()) {
    sendHttp(res, 404, "not found\n");
    return;
  }

  res.writeHead(200, {
    "content-type": MIME[extname(path)] || "application/octet-stream",
    "cache-control": "no-store"
  });
  createReadStream(path).pipe(res);
}

function websocketAccept(key) {
  return createHash("sha1").update(key + WS_GUID).digest("base64");
}

function sendFrame(socket, payload) {
  const data = Buffer.from(payload, "utf8");
  let header;

  if (data.length < 126) {
    header = Buffer.from([0x81, data.length]);
  } else if (data.length < 65536) {
    header = Buffer.alloc(4);
    header[0] = 0x81;
    header[1] = 126;
    header.writeUInt16BE(data.length, 2);
  } else {
    throw new Error("payload too large");
  }

  socket.write(Buffer.concat([header, data]));
}

function sendJson(client, message) {
  if (!client.closed) {
    sendFrame(client.socket, JSON.stringify(message));
  }
}

function broadcast(room, message) {
  for (const client of room.clients) {
    sendJson(client, message);
  }
}

function roomReadyState(room) {
  return {
    1: Boolean(room.ready[1]),
    2: Boolean(room.ready[2])
  };
}

function broadcastPeerState(room) {
  broadcast(room, {
    type: "peer",
    players: room.clients.length,
    ready: roomReadyState(room)
  });
}

function randomRoomCode() {
  const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
  let code = "";

  do {
    code = "";
    for (let i = 0; i < 4; i += 1) {
      code += alphabet[Math.floor(Math.random() * alphabet.length)];
    }
  } while (ROOMS.has(code));

  return code;
}

function leaveRoom(client) {
  if (!client.room) {
    return;
  }

  const room = ROOMS.get(client.room);
  if (room) {
    room.clients = room.clients.filter((other) => other !== client);
    delete room.ready[client.player];
    room.started = false;
    if (room.clients.length === 0) {
      ROOMS.delete(client.room);
    } else {
      broadcast(room, { type: "error", message: "Other player disconnected. Ready up again when they rejoin." });
      broadcastPeerState(room);
    }
  }

  client.room = "";
  client.player = 0;
  client.ready = false;
}

function joinRoom(client, code) {
  const room = ROOMS.get(code);
  const occupied = new Set();
  let player = 0;

  if (!room) {
    sendJson(client, { type: "error", message: "Room " + code + " does not exist." });
    return;
  }
  if (room.started) {
    sendJson(client, { type: "error", message: "Room " + code + " is already playing." });
    return;
  }
  if (room.clients.length >= 2) {
    sendJson(client, { type: "error", message: "Room " + code + " is full." });
    return;
  }

  leaveRoom(client);

  for (const other of room.clients) {
    occupied.add(other.player);
  }
  player = occupied.has(1) ? 2 : 1;

  client.room = code;
  client.player = player;
  client.ready = false;
  room.clients.push(client);
  room.ready[player] = false;

  sendJson(client, {
    type: "room",
    room: code,
    player,
    players: room.clients.length,
    ready: roomReadyState(room)
  });
  broadcastPeerState(room);
}

function createRoom(client) {
  const code = randomRoomCode();

  ROOMS.set(code, {
    clients: [],
    ready: {},
    started: false
  });
  joinRoom(client, code);
}

function startIfReady(room) {
  if (room.clients.length === 2 && room.ready[1] && room.ready[2] && !room.started) {
    room.started = true;
    broadcast(room, { type: "start" });
  }
}

function handleClientMessage(client, message) {
  const room = client.room ? ROOMS.get(client.room) : null;

  if (message.type === "create") {
    createRoom(client);
    return;
  }
  if (message.type === "join") {
    joinRoom(client, String(message.room || "").toUpperCase().replace(/[^A-Z0-9]/g, ""));
    return;
  }
  if (message.type === "leave") {
    leaveRoom(client);
    return;
  }
  if (!room) {
    sendJson(client, { type: "error", message: "Join a room first." });
    return;
  }
  if (message.type === "ready") {
    client.ready = true;
    room.ready[client.player] = true;
    broadcastPeerState(room);
    startIfReady(room);
    return;
  }
  if (message.type === "input" && room.started) {
    broadcast(room, {
      type: "input",
      player: client.player,
      frame: Number(message.frame) | 0,
      input: Number(message.input) & 0xFF
    });
    return;
  }
  if (message.type === "hash" && room.started) {
    broadcast(room, {
      type: "hash",
      player: client.player,
      frame: Number(message.frame) | 0,
      hash: Number(message.hash) >>> 0
    });
  }
}

function parseClientFrames(client, chunk) {
  client.buffer = Buffer.concat([client.buffer, chunk]);

  while (client.buffer.length >= 2) {
    const first = client.buffer[0];
    const second = client.buffer[1];
    const opcode = first & 0x0f;
    const masked = (second & 0x80) !== 0;
    let length = second & 0x7f;
    let offset = 2;

    if (length === 126) {
      if (client.buffer.length < 4) {
        return;
      }
      length = client.buffer.readUInt16BE(2);
      offset = 4;
    } else if (length === 127) {
      client.socket.end();
      return;
    }

    if (!masked || client.buffer.length < offset + 4 + length) {
      return;
    }

    const mask = client.buffer.subarray(offset, offset + 4);
    offset += 4;

    const payload = Buffer.from(client.buffer.subarray(offset, offset + length));
    client.buffer = client.buffer.subarray(offset + length);

    for (let i = 0; i < payload.length; i += 1) {
      payload[i] ^= mask[i & 3];
    }

    if (opcode === 0x8) {
      client.socket.end();
      return;
    }
    if (opcode !== 0x1) {
      continue;
    }

    try {
      handleClientMessage(client, JSON.parse(payload.toString("utf8")));
    } catch (err) {
      sendJson(client, { type: "error", message: "Bad JSON: " + err.message });
    }
  }
}

const server = createServer(serveStatic);

server.on("upgrade", (req, socket) => {
  const key = req.headers["sec-websocket-key"];
  const url = new URL(req.url || "/", "http://localhost");

  if (url.pathname !== "/ws" || !key) {
    socket.end("HTTP/1.1 400 Bad Request\r\n\r\n");
    return;
  }

  socket.write(
    "HTTP/1.1 101 Switching Protocols\r\n" +
    "Upgrade: websocket\r\n" +
    "Connection: Upgrade\r\n" +
    "Sec-WebSocket-Accept: " + websocketAccept(key) + "\r\n" +
    "\r\n"
  );

  const client = {
    socket,
    buffer: Buffer.alloc(0),
    room: "",
    player: 0,
    ready: false,
    closed: false
  };

  socket.on("data", (chunk) => parseClientFrames(client, chunk));
  socket.on("close", () => {
    client.closed = true;
    leaveRoom(client);
  });
  socket.on("error", () => {
    client.closed = true;
    leaveRoom(client);
  });
});

server.listen(PORT, () => {
  console.log(`Contra web lobby listening on http://localhost:${PORT}/`);
});
