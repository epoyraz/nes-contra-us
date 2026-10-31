/*
 * Browser front-end for the Contra WebAssembly port.
 *
 * Responsibilities:
 *   - boot the Emscripten module (web/dist/contra.js, factory `ContraModule`)
 *   - drive a 60 Hz fixed-step loop, reading the core's RGBA framebuffer straight
 *     out of wasm memory and blitting it to the <canvas> with putImageData
 *   - merge two input sources into this browser's button bitmask:
 *       (a) the physical keyboard
 *       (b) the on-screen NES controller (mouse + touch via Pointer Events)
 *   - the lobby: solo, or an online 2-player room (web/net.js) where each
 *     browser drives its own player and the two cores run in lockstep
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

  /* On-screen controller element class -> button bit. */
  var BUTTON_MAP = {
    "nes-up-button": BTN.UP,
    "nes-down-button": BTN.DOWN,
    "nes-left-button": BTN.LEFT,
    "nes-right-button": BTN.RIGHT,
    "nes-a-button": BTN.A,
    "nes-b-button": BTN.B,
    "nes-select": BTN.SELECT,
    "nes-start": BTN.START
  };

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

  /* One on-screen controller. Online, there are two side by side: this
     browser's own (local: pointer input drives the game) and the other
     player's (it only mirrors their presses). The render loop reflects a
     button bitmask onto each pad's `pressed` styling, so physical key presses
     light up the virtual controller too. */
  function makePad(el) {
    var pad = { el: el, label: el.querySelector(".pad-label"), local: true, pressables: [], lastMask: -1 };
    Object.keys(BUTTON_MAP).forEach(function (cls) {
      var btn = el.querySelector("." + cls);
      if (!btn) {
        return;
      }
      var bit = BUTTON_MAP[cls];
      pad.pressables.push({ el: btn, bit: bit });
      var press = function (e) {
        e.preventDefault();
        if (!pad.local) {
          return;
        }
        pointerMask |= bit;
        if (e.pointerId !== undefined && btn.setPointerCapture) {
          try { btn.setPointerCapture(e.pointerId); } catch (_) {}
        }
      };
      var release = function (e) {
        if (pad.local) {
          pointerMask &= ~bit;
        }
        if (e && e.preventDefault) {
          e.preventDefault();
        }
      };
      btn.addEventListener("pointerdown", press);
      btn.addEventListener("pointerup", release);
      btn.addEventListener("pointercancel", release);
      btn.addEventListener("pointerleave", release);
      /* Block the synthetic context menu / text selection on long press. */
      btn.addEventListener("contextmenu", function (e) { e.preventDefault(); });
    });
    return pad;
  }

  /* Only touches the DOM when the mask changes, so it's cheap every frame. */
  function syncPressedVisual(pad, mask) {
    if (mask === pad.lastMask) {
      return;
    }
    pad.lastMask = mask;
    for (var i = 0; i < pad.pressables.length; i += 1) {
      pad.pressables[i].el.classList.toggle("pressed", (mask & pad.pressables[i].bit) !== 0);
    }
  }

  /* The Konami-code easter egg from the original pen (up up down down left right
     left right b a, then Start), rewritten without jQuery. */
  function setupKonami(padEl) {
    var codeString = "";
    var on = function (cls, fn) {
      var el = padEl.querySelector("." + cls);
      if (el) {
        el.addEventListener("click", fn);
      }
    };
    on("nes-up-button", function () {
      codeString = (codeString === "" || codeString === "u") ? codeString + "u" : "u";
    });
    on("nes-down-button", function () {
      codeString = (codeString === "uu" || codeString === "uud") ? codeString + "d" : "";
    });
    on("nes-left-button", function () {
      codeString = (codeString === "uudd" || codeString === "uuddlr") ? codeString + "l" : "";
    });
    on("nes-right-button", function () {
      codeString = (codeString === "uuddl" || codeString === "uuddlrl") ? codeString + "r" : "";
    });
    on("nes-b-button", function () {
      codeString = (codeString === "uuddlrlr") ? codeString + "b" : "";
    });
    on("nes-a-button", function () {
      codeString = (codeString === "uuddlrlrb") ? codeString + "a" : "";
    });
    on("nes-start", function () {
      if (codeString === "uuddlrlrba") {
        var el = padEl.querySelector(".nes-notSoSecretCode");
        if (el) {
          el.style.display = "block";
          el.classList.add("blink");
        }
      }
    });
  }

  /* Online play is deterministic lockstep: both browsers run the same core
     and exchange only button masks. Local input for frame f is sampled at
     frame f - INPUT_DELAY and sent right away, so a frame steps as soon as
     the other player's mask for it has arrived; if it hasn't, that browser
     waits. INPUT_DELAY frames (~67 ms) hide normal internet latency. */
  var INPUT_DELAY = 4;
  var HASH_INTERVAL = 120; /* compare RAM hashes every 2 s to catch desyncs */

  function start(Module) {
    var canvas = document.getElementById("screen");
    var ctx = canvas.getContext("2d");
    var loading = document.getElementById("loading");
    var status = document.getElementById("net-status");

    var setInputs = Module.cwrap("contra_web_set_inputs", null, ["number", "number"]);
    var step = Module.cwrap("contra_web_step", null, []);
    var framebuffer = Module.cwrap("contra_web_framebuffer", "number", []);
    var stateHash = Module.cwrap("contra_web_state_hash", "number", []);

    var fbPtr = framebuffer();
    var fbBytes = FB_WIDTH * FB_HEIGHT * 4;
    var imageData = ctx.createImageData(FB_WIDTH, FB_HEIGHT);

    if (loading) {
      loading.style.display = "none";
    }

    setupKeyboard();

    var padsRow = document.getElementById("pads");
    var pad1El = padsRow.querySelector(".pad");
    var pad2El = pad1El.cloneNode(true);
    pad2El.setAttribute("data-player", "2");
    pad2El.hidden = true;
    padsRow.appendChild(pad2El);
    var pads = [makePad(pad1El), makePad(pad2El)];
    setupKonami(pad1El);

    /* Solo: one pad. Online: Player 1's pad left, Player 2's right. */
    function setPadLayout(online, me) {
      padsRow.classList.toggle("two-player", online);
      pad2El.hidden = !online;
      pads.forEach(function (pad, i) {
        pad.local = online ? (i === me - 1) : (i === 0);
        pad.el.classList.toggle("local", online && pad.local);
        pad.el.classList.toggle("remote", online && !pad.local);
        pad.label.hidden = !online;
        pad.label.textContent = "Player " + (i + 1) + (pad.local ? " (you)" : "");
        pad.lastMask = -1;
      });
      pointerMask = 0;
      syncPressedVisual(pads[online ? 2 - me : 1], 0);
    }

    /* null = solo; otherwise the lockstep session (see startOnline). */
    var net = null;
    var running = false;

    function setStatus(text, bad) {
      status.hidden = !text;
      status.textContent = text || "";
      status.classList.toggle("bad", !!bad);
    }

    function drawFrame() {
      /* Re-read HEAPU8 each frame: ALLOW_MEMORY_GROWTH can swap the view object. */
      imageData.data.set(Module.HEAPU8.subarray(fbPtr, fbPtr + fbBytes));
      ctx.putImageData(imageData, 0, 0);
    }

    function bootCore() {
      Module.ccall("contra_web_init", null, [], []);
    }

    function startSolo() {
      bootCore();
      /* Optional debug warps via URL hash, e.g. index.html#level2boss */
      if (location.hash === "#level2boss") {
        Module.ccall("contra_web_warp_level2_boss", null, [], []);
      } else if (location.hash === "#level4") {
        Module.ccall("contra_web_warp_level4", null, [], []);
      }
      net = null;
      setStatus("");
      run();
    }

    function startOnline(link, player, room) {
      var f;
      net = {
        link: link,
        room: room,
        player: player,          /* 1 = host, 2 = guest */
        frame: 0,                /* next frame to step */
        nextLocal: INPUT_DELAY,  /* next frame whose local input to sample */
        local: {},
        remote: {},
        myHashes: {},
        theirHashes: {},
        desynced: false,
        stallSince: 0,
        ping: 0,
        lastHeard: performance.now(),
        remoteShown: 0
      };
      for (f = 0; f < INPUT_DELAY; f += 1) {
        net.local[f] = 0;
        net.remote[f] = 0;
      }
      link.onMessage(function (msg) {
        if (!net || net.link !== link) {
          return;
        }
        net.lastHeard = performance.now();
        if (msg.t === "i") {
          net.remote[msg.f] = msg.m;
        } else if (msg.t === "h") {
          net.theirHashes[msg.f] = msg.h;
          checkHash(msg.f);
        } else if (msg.t === "p") {
          link.send({ t: "q", ts: msg.ts });
        } else if (msg.t === "q") {
          net.ping = Math.round(performance.now() - msg.ts);
        }
      });
      link.onClose(function () {
        if (net && net.link === link) {
          endOnline("The other player left the game.");
        }
      });
      net.pingTimer = setInterval(function () {
        /* WebRTC often never reports a closed tab; pings answer every second,
           so 10 s of silence means the other player is gone. */
        if (performance.now() - net.lastHeard > 10000) {
          endOnline("Lost the connection to the other player.");
          return;
        }
        link.send({ t: "p", ts: performance.now() });
      }, 1000);
      bootCore();
      setPadLayout(true, player);
      updateOnlineStatus();
      run();
    }

    function endOnline(message) {
      if (!net) {
        return;
      }
      clearInterval(net.pingTimer);
      net.link.close();
      net.room.close();
      net = null;
      running = false;
      setStatus("");
      setPadLayout(false, 1);
      showLobby(message);
    }

    function checkHash(f) {
      if (net.myHashes[f] === undefined || net.theirHashes[f] === undefined) {
        return;
      }
      if (net.myHashes[f] !== net.theirHashes[f]) {
        net.desynced = true;
      }
      delete net.myHashes[f];
      delete net.theirHashes[f];
    }

    function updateOnlineStatus() {
      var text = "Online · you are Player " + net.player;
      if (net.ping) {
        text += " · ping " + net.ping + " ms";
      }
      if (net.desynced) {
        setStatus(text + " · OUT OF SYNC: the two games differ, start a new one.", true);
      } else if (net.stallSince && performance.now() - net.stallSince > 500) {
        setStatus(text + " · waiting for the other player…", true);
      } else {
        setStatus(text);
      }
    }

    /* Advance the online game by one frame if the other player's input for it
       is here. Returns false when stalled. */
    function stepOnline(localMask) {
      var remoteMask;
      var h;
      /* Sample and send our input INPUT_DELAY frames ahead (once per frame). */
      if (net.nextLocal <= net.frame + INPUT_DELAY) {
        net.local[net.nextLocal] = localMask;
        net.link.send({ t: "i", f: net.nextLocal, m: localMask });
        net.nextLocal += 1;
      }
      remoteMask = net.remote[net.frame];
      if (remoteMask === undefined) {
        return false;
      }
      net.remoteShown = remoteMask;
      if (net.player === 1) {
        setInputs(net.local[net.frame], remoteMask);
      } else {
        setInputs(remoteMask, net.local[net.frame]);
      }
      step();
      delete net.local[net.frame];
      delete net.remote[net.frame];
      if (net.frame % HASH_INTERVAL === 0) {
        h = stateHash() >>> 0;
        net.myHashes[net.frame] = h;
        net.link.send({ t: "h", f: net.frame, h: h });
        checkHash(net.frame);
      }
      net.frame += 1;
      return true;
    }

    var STEP_MS = 1000 / 60;
    var acc = 0;
    var last = 0;

    function run() {
      acc = 0;
      last = performance.now();
      if (!running) {
        running = true;
        requestAnimationFrame(frame);
      }
    }

    function frame(now) {
      if (!running) {
        return;
      }
      acc += now - last;
      last = now;

      var combined = keyboardMask | pointerMask;

      /* Catch up at 60 Hz, but never spiral (cap at 5 steps like the SDL host). */
      var steps = 0;
      var stalled = false;
      while (acc >= STEP_MS && steps < 5) {
        if (net) {
          if (!stepOnline(combined)) {
            stalled = true;
            break;
          }
        } else {
          setInputs(combined, 0);
          step();
        }
        acc -= STEP_MS;
        steps += 1;
      }
      if (net) {
        if (stalled) {
          if (!net.stallSince) {
            net.stallSince = now;
          }
          /* Keep a few frames of budget so we catch up once input arrives. */
          acc = Math.min(acc, STEP_MS * 5);
        } else if (steps > 0) {
          net.stallSince = 0;
        }
        updateOnlineStatus();
      } else if (acc > 250) {
        acc = 0; /* tab was backgrounded; don't fast-forward on return */
      }

      if (net) {
        syncPressedVisual(pads[net.player - 1], combined);
        syncPressedVisual(pads[2 - net.player], net.remoteShown);
      } else {
        syncPressedVisual(pads[0], combined);
      }

      drawFrame();
      requestAnimationFrame(frame);
    }

    /* ---- Lobby ---------------------------------------------------------- */

    var lobby = document.getElementById("lobby");
    var menu = document.getElementById("lobby-menu");
    var wait = document.getElementById("lobby-wait");
    var waitTitle = document.getElementById("wait-title");
    var waitCode = document.getElementById("wait-code");
    var waitText = document.getElementById("wait-text");
    var copyBtn = document.getElementById("btn-copy");
    var joinCode = document.getElementById("join-code");
    var pendingRoom = null;
    var inviteLink = "";

    var slotP1 = document.getElementById("slot-p1");
    var slotP2 = document.getElementById("slot-p2");
    var players = document.getElementById("room-players");
    var inviteField = document.getElementById("invite-link");
    var startBtn = document.getElementById("btn-start");
    /* The connected-but-not-started room: { link, room, code }. */
    var waitingRoom = null;

    function leaveRoom() {
      if (waitingRoom) {
        waitingRoom.link.close();
        waitingRoom.room.close();
        waitingRoom = null;
      }
      if (pendingRoom) {
        pendingRoom.close();
        pendingRoom = null;
      }
    }

    function showLobby(message) {
      leaveRoom();
      lobby.hidden = false;
      menu.hidden = !!message;
      wait.hidden = !message;
      if (message) {
        showWaiting("Room closed", "", message);
      }
    }

    function showWaiting(title, code, text) {
      menu.hidden = true;
      wait.hidden = false;
      waitTitle.textContent = title;
      waitCode.textContent = code || "";
      waitText.textContent = text || "";
      copyBtn.hidden = true;
      inviteField.hidden = true;
      players.hidden = true;
      startBtn.hidden = true;
    }

    /* The room screen both players see before the game starts. */
    function showRoom(code, me, p2Here) {
      showWaiting("Room " + code, "", "");
      waitCode.textContent = "";
      players.hidden = false;
      slotP1.textContent = "Player 1" + (me === 1 ? " (you)" : "");
      slotP1.className = "here";
      slotP2.textContent = p2Here ? "Player 2" + (me === 2 ? " (you)" : "") : "Player 2: waiting to join…";
      slotP2.className = p2Here ? "here" : "";
      if (me === 1) {
        inviteField.hidden = false;
        inviteField.value = inviteLink;
        copyBtn.hidden = false;
        startBtn.hidden = !p2Here;
        waitText.textContent = p2Here
          ? "Everyone's here. Start when you're ready."
          : "Send Player 2 this link, or the code " + code + ".";
      } else {
        waitText.textContent = "Waiting for Player 1 to start the game…";
      }
    }

    function hideLobby() {
      lobby.hidden = true;
      /* Hand keyboard focus back to the game. */
      if (document.activeElement && document.activeElement.blur) {
        document.activeElement.blur();
      }
    }

    function beginOnline(player) {
      var r = waitingRoom;
      waitingRoom = null;
      if (location.hash) {
        /* A reload shouldn't try to rejoin a room that's in play. */
        history.replaceState(null, "", location.pathname + location.search);
      }
      hideLobby();
      startOnline(r.link, player, r.room);
    }

    function enterRoom(link, code, me) {
      waitingRoom = { link: link, room: pendingRoom, code: code };
      pendingRoom = null;
      link.onClose(function () {
        if (waitingRoom && waitingRoom.link === link) {
          showLobby(me === 1 ? "Player 2 left the room." : "Player 1 closed the room.");
        }
      });
      link.onMessage(function (msg) {
        if (me === 2 && msg.t === "go" && waitingRoom && waitingRoom.link === link) {
          beginOnline(2);
        }
      });
      showRoom(code, me, true);
    }

    function hostGame() {
      showWaiting("Opening a room…", "", "");
      pendingRoom = ContraNet.host({
        onCode: function (code) {
          inviteLink = location.origin + location.pathname + "#join=" + code;
          showRoom(code, 1, false);
          pendingRoom.code = code;
        },
        onConnected: function (link) {
          enterRoom(link, pendingRoom.code, 1);
        },
        onError: function (message) {
          showWaiting("Couldn't open a room", "", message);
        }
      });
    }

    function joinGame(code) {
      code = ContraNet.normalizeCode(code);
      if (!code) {
        joinCode.focus();
        return;
      }
      lobby.hidden = false;
      showWaiting("Joining room " + code + "…", "", "Connecting to Player 1.");
      pendingRoom = ContraNet.join(code, {
        onConnected: function (link) {
          enterRoom(link, code, 2);
        },
        onError: function (message) {
          showWaiting("Couldn't join room " + code, "", message);
        }
      });
    }

    startBtn.addEventListener("click", function () {
      if (waitingRoom) {
        /* The channel is ordered, so "go" reaches Player 2 before any of
           Player 1's frame inputs. */
        waitingRoom.link.send({ t: "go" });
        beginOnline(1);
      }
    });
    inviteField.addEventListener("focus", function () { inviteField.select(); });

    document.getElementById("btn-solo").addEventListener("click", function () {
      hideLobby();
      startSolo();
    });
    document.getElementById("btn-host").addEventListener("click", hostGame);
    document.getElementById("join-form").addEventListener("submit", function (e) {
      e.preventDefault();
      joinGame(joinCode.value);
    });
    document.getElementById("btn-cancel").addEventListener("click", function () {
      showLobby("");
    });
    copyBtn.addEventListener("click", function () {
      var done = function () { copyBtn.textContent = "Link copied!"; };
      if (navigator.clipboard) {
        navigator.clipboard.writeText(inviteLink).then(done, function () {
          window.prompt("Copy this invite link:", inviteLink);
        });
      } else {
        window.prompt("Copy this invite link:", inviteLink);
      }
      setTimeout(function () { copyBtn.textContent = "Copy invite link"; }, 2000);
    });
    /* Typing a room code shouldn't drive the game underneath. */
    joinCode.addEventListener("keydown", function (e) { e.stopPropagation(); });
    joinCode.addEventListener("keyup", function (e) { e.stopPropagation(); });

    var joinMatch = /^#join=([A-Za-z0-9]+)$/.exec(location.hash);
    if (typeof ContraNet === "undefined" || typeof Peer === "undefined") {
      /* PeerJS failed to load (offline / blocked): solo only. */
      document.getElementById("btn-host").disabled = true;
      document.getElementById("join-form").hidden = true;
      showLobby("");
    } else if (joinMatch) {
      joinGame(joinMatch[1]);
    } else if (location.hash === "#level2boss" || location.hash === "#level4") {
      startSolo(); /* debug warps skip the lobby */
    } else {
      showLobby("");
    }
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
