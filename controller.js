/*
 * Browser front-end for the Contra WebAssembly port.
 *
 * Responsibilities:
 *   - boot the Emscripten module (web/dist/contra.js, factory `ContraModule`)
 *   - drive a 60 Hz fixed-step loop, reading the core's RGBA framebuffer straight
 *     out of wasm memory and blitting it to the <canvas> with putImageData
 *   - merge keyboard + on-screen NES controller input into the local player mask
 *   - optionally run two-browser co-op through WebSocket input lockstep
 *   - the Konami-code easter egg from the original CodePen (vanilla-JS rewrite)
 */

(function () {
  "use strict";

  /* NES button bits — must match port/contra_core/include/contra/buttons.h. */
  var BTN = {
    RIGHT: 0x01,
    LEFT: 0x02,
    DOWN: 0x04,
    UP: 0x08,
    START: 0x10,
    SELECT: 0x20,
    B: 0x40,
    A: 0x80
  };

  var FB_WIDTH = 256;
  var FB_HEIGHT = 240;
  var STEP_MS = 1000 / 60;
  var NET_INPUT_DELAY = 6;
  var NET_HASH_INTERVAL = 60;

  /* Input is the OR of the keyboard mask and the on-screen-controller mask, so a
     button held on either source counts as pressed. */
  var keyboardMask = 0;
  var pointerMask = 0;

  /* Keyboard mapping (mirrors platform_sdl/main.c). */
  var KEY_MAP = {
    ArrowRight: BTN.RIGHT,
    ArrowLeft: BTN.LEFT,
    ArrowDown: BTN.DOWN,
    ArrowUp: BTN.UP,
    Enter: BTN.START,
    ShiftLeft: BTN.SELECT,
    ShiftRight: BTN.SELECT,
    KeyA: BTN.A,   /* jump (NES A) — home-row friendly on QWERTZ */
    KeyS: BTN.B,   /* fire (NES B) */
    Space: BTN.A   /* alt jump   */
  };

  /* On-screen controller element id -> button bit. */
  var BUTTON_MAP = {
    "up-button": BTN.UP,
    "down-button": BTN.DOWN,
    "left-button": BTN.LEFT,
    "right-button": BTN.RIGHT,
    "a-button": BTN.A,
    "b-button": BTN.B,
    "select": BTN.SELECT,
    "start": BTN.START
  };

  var pressables = [];
  var lastVisualMask = -1;

  var net = {
    socket: null,
    wsUrl: "",
    room: "",
    player: 0,
    players: 0,
    ready: {},
    running: false,
    sentFrame: 0,
    simFrame: 0,
    inputs: Object.create(null),
    localHashes: Object.create(null),
    remoteHashes: Object.create(null),
    desynced: false,
    runtime: null
  };

  function localInputMask() {
    return keyboardMask | pointerMask;
  }

  function setupKeyboard() {
    window.addEventListener("keydown", function (e) {
      var bit = KEY_MAP[e.code];
      if (bit === undefined) {
        return;
      }
      keyboardMask |= bit;
      e.preventDefault();
    });
    window.addEventListener("keyup", function (e) {
      var bit = KEY_MAP[e.code];
      if (bit === undefined) {
        return;
      }
      keyboardMask &= ~bit;
      e.preventDefault();
    });
    /* Releasing focus shouldn't leave a key stuck down. */
    window.addEventListener("blur", function () {
      keyboardMask = 0;
    });
  }

  function setupOnScreenController() {
    Object.keys(BUTTON_MAP).forEach(function (id) {
      var el = document.getElementById(id);
      if (!el) {
        return;
      }
      var bit = BUTTON_MAP[id];
      pressables.push({ el: el, bit: bit });
      var press = function (e) {
        pointerMask |= bit;
        if (e.pointerId !== undefined && el.setPointerCapture) {
          try { el.setPointerCapture(e.pointerId); } catch (_) {}
        }
        e.preventDefault();
      };
      var release = function (e) {
        pointerMask &= ~bit;
        if (e && e.preventDefault) {
          e.preventDefault();
        }
      };
      el.addEventListener("pointerdown", press);
      el.addEventListener("pointerup", release);
      el.addEventListener("pointercancel", release);
      el.addEventListener("pointerleave", release);
      /* Block the synthetic context menu / text selection on long press. */
      el.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    });
  }

  /* Reflect the local button bitmask onto the on-screen controller. Only touches
     the DOM when the mask changes, so it's cheap to call every frame. */
  function syncPressedVisual(mask) {
    if (mask === lastVisualMask) {
      return;
    }
    lastVisualMask = mask;
    for (var i = 0; i < pressables.length; i += 1) {
      pressables[i].el.classList.toggle("pressed", (mask & pressables[i].bit) !== 0);
    }
  }

  function setupKonami() {
    var codeString = "";
    var on = function (id, fn) {
      var el = document.getElementById(id);
      if (el) {
        el.addEventListener("click", fn);
      }
    };
    on("up-button", function () {
      codeString = (codeString === "" || codeString === "u") ? codeString + "u" : "u";
    });
    on("down-button", function () {
      codeString = (codeString === "uu" || codeString === "uud") ? codeString + "d" : "";
    });
    on("left-button", function () {
      codeString = (codeString === "uudd" || codeString === "uuddlr") ? codeString + "l" : "";
    });
    on("right-button", function () {
      codeString = (codeString === "uuddl" || codeString === "uuddlrl") ? codeString + "r" : "";
    });
    on("b-button", function () {
      codeString = (codeString === "uuddlrlr") ? codeString + "b" : "";
    });
    on("a-button", function () {
      codeString = (codeString === "uuddlrlrb") ? codeString + "a" : "";
    });
    on("start", function () {
      if (codeString === "uuddlrlrba") {
        var el = document.getElementById("notSoSecretCode");
        if (el) {
          el.style.display = "block";
          el.classList.add("blink");
        }
      }
    });
  }

  function defaultWebSocketUrl() {
    var params = new URLSearchParams(window.location.search);
    var explicit = params.get("ws");
    if (explicit) {
      return explicit;
    }
    if (window.location.hostname.indexOf("github.io") !== -1) {
      return "";
    }
    if (!window.location.host || window.location.protocol === "file:") {
      return "";
    }
    return (window.location.protocol === "https:" ? "wss://" : "ws://") +
      window.location.host + "/ws";
  }

  function setNetStatus(text, className) {
    var el = document.getElementById("net-status");
    if (!el) {
      return;
    }
    el.textContent = text;
    el.className = "net-status " + className;
  }

  function setRoomInfo(text) {
    var el = document.getElementById("room-info");
    if (el) {
      el.textContent = text;
    }
  }

  function updateLobbyButtons(inRoom) {
    var create = document.getElementById("create-room");
    var join = document.getElementById("join-room");
    var room = document.getElementById("room-code");
    var ready = document.getElementById("ready-room");
    var leave = document.getElementById("leave-room");
    var canUseNetwork = net.wsUrl !== "";

    if (create) { create.disabled = !canUseNetwork || inRoom; }
    if (join) { join.disabled = !canUseNetwork || inRoom; }
    if (room) { room.disabled = !canUseNetwork || inRoom; }
    if (ready) { ready.disabled = !inRoom || net.running; }
    if (leave) { leave.disabled = !inRoom; }
  }

  function roomSummary() {
    if (!net.room) {
      return "Single-player mode. Run node web/multiplayer-server.mjs for local rooms.";
    }
    return "Room " + net.room + " · You are P" + net.player +
      " · players " + net.players + "/2 · ready P1:" +
      (net.ready[1] ? "yes" : "no") + " P2:" + (net.ready[2] ? "yes" : "no");
  }

  function sendNet(message) {
    if (net.socket && net.socket.readyState === WebSocket.OPEN) {
      net.socket.send(JSON.stringify(message));
    }
  }

  function resetNetSimulation() {
    if (!net.runtime) {
      return;
    }
    net.runtime.reset();
    net.runtime.applyDebugWarp();
    net.running = true;
    net.sentFrame = 0;
    net.simFrame = 0;
    net.inputs = Object.create(null);
    net.localHashes = Object.create(null);
    net.remoteHashes = Object.create(null);
    net.desynced = false;
    setNetStatus("Playing P" + net.player, "online");
    setRoomInfo(roomSummary() + " · lockstep delay " + NET_INPUT_DELAY + " frames");
  }

  function storeNetworkInput(player, frame, input) {
    if (frame < net.simFrame) {
      return;
    }
    if (!net.inputs[frame]) {
      net.inputs[frame] = [undefined, undefined];
    }
    net.inputs[frame][player - 1] = input & 0xFF;
  }

  function pruneNetworkInputs() {
    var cutoff = net.simFrame - 120;
    Object.keys(net.inputs).forEach(function (key) {
      if (Number(key) < cutoff) {
        delete net.inputs[key];
      }
    });
    Object.keys(net.localHashes).forEach(function (key) {
      if (Number(key) < cutoff) {
        delete net.localHashes[key];
      }
    });
    Object.keys(net.remoteHashes).forEach(function (key) {
      if (Number(key) < cutoff) {
        delete net.remoteHashes[key];
      }
    });
  }

  function networkTick(localMask) {
    var frameToSend;
    var pair;
    var hash;

    if (!net.running || !net.player) {
      return;
    }

    frameToSend = net.sentFrame;
    storeNetworkInput(net.player, frameToSend, localMask);
    sendNet({ type: "input", frame: frameToSend, input: localMask });
    net.sentFrame += 1;

    if (net.sentFrame <= NET_INPUT_DELAY) {
      return;
    }

    pair = net.inputs[net.simFrame];
    if (!pair || pair[0] === undefined || pair[1] === undefined) {
      setNetStatus("Waiting", "waiting");
      return;
    }

    net.runtime.setInputs(pair[0], pair[1]);
    net.runtime.step();
    net.simFrame += 1;

    if ((net.simFrame % NET_HASH_INTERVAL) === 0) {
      hash = net.runtime.stateHash() >>> 0;
      net.localHashes[net.simFrame] = hash;
      sendNet({ type: "hash", frame: net.simFrame, hash: hash });
      if (net.remoteHashes[net.simFrame] !== undefined &&
          net.remoteHashes[net.simFrame] !== hash) {
        net.desynced = true;
        setNetStatus("Desync", "error");
        setRoomInfo("Desync detected at frame " + net.simFrame + ". Leave and rejoin the room.");
      }
    }

    if (!net.desynced) {
      setNetStatus("Playing P" + net.player, "online");
    }
    pruneNetworkInputs();
  }

  function handleNetMessage(message) {
    if (message.type === "room") {
      net.room = message.room;
      net.player = message.player;
      net.players = message.players;
      net.ready = message.ready || {};
      setNetStatus("Room " + net.room, "waiting");
      setRoomInfo(roomSummary());
      updateLobbyButtons(true);
      return;
    }
    if (message.type === "peer") {
      net.players = message.players;
      net.ready = message.ready || {};
      setRoomInfo(roomSummary());
      return;
    }
    if (message.type === "start") {
      resetNetSimulation();
      return;
    }
    if (message.type === "input") {
      storeNetworkInput(message.player, message.frame, message.input);
      return;
    }
    if (message.type === "hash") {
      if (message.player !== net.player && net.runtime) {
        var remoteHash = message.hash >>> 0;
        net.remoteHashes[message.frame] = remoteHash;
        if (net.localHashes[message.frame] !== undefined &&
            net.localHashes[message.frame] !== remoteHash) {
          net.desynced = true;
          setNetStatus("Desync", "error");
          setRoomInfo("Desync detected at frame " + message.frame + ". Leave and rejoin the room.");
        }
      }
      return;
    }
    if (message.type === "error") {
      setNetStatus("Error", "error");
      setRoomInfo(message.message || "Network error");
    }
  }

  function connectAndSend(message) {
    if (!net.wsUrl) {
      setNetStatus("Offline", "error");
      setRoomInfo("No WebSocket relay configured. Run node web/multiplayer-server.mjs locally, or open with ?ws=wss://your-relay/ws.");
      return;
    }

    if (net.socket && net.socket.readyState === WebSocket.OPEN) {
      sendNet(message);
      return;
    }

    net.socket = new WebSocket(net.wsUrl);
    setNetStatus("Connecting", "waiting");
    setRoomInfo("Connecting to " + net.wsUrl + " ...");

    net.socket.addEventListener("open", function () {
      sendNet(message);
    });
    net.socket.addEventListener("message", function (event) {
      try {
        handleNetMessage(JSON.parse(event.data));
      } catch (err) {
        setNetStatus("Error", "error");
        setRoomInfo("Bad server message: " + err.message);
      }
    });
    net.socket.addEventListener("close", function () {
      net.socket = null;
      net.running = false;
      net.room = "";
      net.player = 0;
      net.players = 0;
      net.ready = {};
      setNetStatus("Offline", "offline");
      setRoomInfo("Disconnected. Single-player mode is active.");
      updateLobbyButtons(false);
    });
    net.socket.addEventListener("error", function () {
      setNetStatus("Error", "error");
      setRoomInfo("Could not connect to " + net.wsUrl);
    });
  }

  function setupMultiplayer(runtime) {
    var create = document.getElementById("create-room");
    var join = document.getElementById("join-room");
    var room = document.getElementById("room-code");
    var ready = document.getElementById("ready-room");
    var leave = document.getElementById("leave-room");

    net.runtime = runtime;
    net.wsUrl = defaultWebSocketUrl();
    updateLobbyButtons(false);

    if (!net.wsUrl) {
      setRoomInfo("Multiplayer needs a WebSocket relay. For local testing run node web/multiplayer-server.mjs, or deploy a relay and open this page with ?ws=wss://your-relay/ws.");
    } else {
      setRoomInfo("Multiplayer relay: " + net.wsUrl);
    }

    if (create) {
      create.addEventListener("click", function () {
        connectAndSend({ type: "create" });
      });
    }
    if (join) {
      join.addEventListener("click", function () {
        var code = room ? room.value.trim().toUpperCase() : "";
        if (!code) {
          setNetStatus("Error", "error");
          setRoomInfo("Enter a room code to join.");
          return;
        }
        connectAndSend({ type: "join", room: code });
      });
    }
    if (ready) {
      ready.addEventListener("click", function () {
        sendNet({ type: "ready" });
        ready.disabled = true;
      });
    }
    if (leave) {
      leave.addEventListener("click", function () {
        sendNet({ type: "leave" });
        if (net.socket) {
          net.socket.close();
        }
      });
    }
    if (room) {
      room.addEventListener("input", function () {
        room.value = room.value.toUpperCase().replace(/[^A-Z0-9]/g, "");
      });
    }
  }

  function start(Module) {
    var canvas = document.getElementById("screen");
    var ctx = canvas.getContext("2d");
    var loading = document.getElementById("loading");

    var setInput = Module.cwrap("contra_web_set_input", null, ["number"]);
    var setInputs = Module.cwrap("contra_web_set_inputs", null, ["number", "number"]);
    var step = Module.cwrap("contra_web_step", null, []);
    var reset = Module.cwrap("contra_web_reset", null, []);
    var stateHash = Module.cwrap("contra_web_state_hash", "number", []);
    var framebuffer = Module.cwrap("contra_web_framebuffer", "number", []);

    function applyDebugWarp() {
      if (location.hash === "#level2boss") {
        Module.ccall("contra_web_warp_level2_boss", null, [], []);
      } else if (location.hash === "#level4") {
        Module.ccall("contra_web_warp_level4", null, [], []);
      }
    }

    Module.ccall("contra_web_init", null, [], []);
    applyDebugWarp();

    var fbPtr = framebuffer();
    var fbBytes = FB_WIDTH * FB_HEIGHT * 4;
    var imageData = ctx.createImageData(FB_WIDTH, FB_HEIGHT);

    if (loading) {
      loading.style.display = "none";
    }

    setupKeyboard();
    setupOnScreenController();
    setupKonami();
    setupMultiplayer({
      setInput: setInput,
      setInputs: setInputs,
      step: step,
      reset: reset,
      stateHash: stateHash,
      applyDebugWarp: applyDebugWarp
    });

    var acc = 0;
    var last = performance.now();

    function frame(now) {
      var combined;
      var steps = 0;

      acc += now - last;
      last = now;

      combined = localInputMask();
      syncPressedVisual(combined);

      /* Catch up at 60 Hz, but never spiral (cap at 5 steps like the SDL host). */
      while (acc >= STEP_MS && steps < 5) {
        if (net.running) {
          networkTick(combined);
        } else {
          setInput(combined);
          step();
        }
        acc -= STEP_MS;
        steps += 1;
      }
      if (acc > 250) {
        acc = 0; /* tab was backgrounded; don't fast-forward on return */
      }

      /* Re-read HEAPU8 each frame: ALLOW_MEMORY_GROWTH can swap the view object. */
      imageData.data.set(Module.HEAPU8.subarray(fbPtr, fbPtr + fbBytes));
      ctx.putImageData(imageData, 0, 0);

      requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  }

  if (typeof ContraModule !== "function") {
    var msg = document.getElementById("loading");
    if (msg) {
      msg.textContent = "Build missing — run web/build.ps1 (or build.sh) first.";
    }
    return;
  }

  /* Resolve contra.wasm / contra.data relative to contra.js. Emscripten's
     preload-data loader otherwise fetches contra.data relative to the HTML page
     (a 404 when the build lives in dist/, which leaves the module promise
     pending forever). */
  var moduleBase = (function () {
    var s = document.querySelector('script[src$="contra.js"]');
    return s ? s.src.replace(/contra\.js(\?.*)?$/, "") : "";
  })();

  ContraModule({
    locateFile: function (path) { return moduleBase + path; }
  }).then(start).catch(function (err) {
    var msg = document.getElementById("loading");
    if (msg) {
      msg.textContent = "Failed to start: " + err;
    }
    /* eslint-disable-next-line no-console */
    console.error(err);
  });
})();
