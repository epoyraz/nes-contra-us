/*
 * Browser front-end for the Contra WebAssembly port.
 *
 * Responsibilities:
 *   - boot the Emscripten module (web/dist/contra.js, factory `ContraModule`)
 *   - drive a 60 Hz fixed-step loop, reading the core's RGBA framebuffer straight
 *     out of wasm memory and blitting it to the <canvas> with putImageData
 *   - merge two input sources into the player-1 button bitmask:
 *       (a) the physical keyboard
 *       (b) the on-screen NES controller (mouse + touch via Pointer Events)
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
    KeyZ: BTN.B,
    KeyY: BTN.B,
    KeyS: BTN.B,
    ControlLeft: BTN.B,
    Space: BTN.A,
    KeyX: BTN.A,
    KeyA: BTN.A
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
      /* The element actually styled with :active is the d-pad/red button itself
         (same node here), so toggle a .pressed class for visual feedback. */
      var press = function (e) {
        pointerMask |= bit;
        el.classList.add("pressed");
        if (e.pointerId !== undefined && el.setPointerCapture) {
          try { el.setPointerCapture(e.pointerId); } catch (_) {}
        }
        e.preventDefault();
      };
      var release = function (e) {
        pointerMask &= ~bit;
        el.classList.remove("pressed");
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

  /* The Konami-code easter egg from the original pen (up up down down left right
     left right b a, then Start), rewritten without jQuery. */
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

  function start(Module) {
    var canvas = document.getElementById("screen");
    var ctx = canvas.getContext("2d");
    var loading = document.getElementById("loading");

    var setInput = Module.cwrap("contra_web_set_input", null, ["number"]);
    var step = Module.cwrap("contra_web_step", null, []);
    var framebuffer = Module.cwrap("contra_web_framebuffer", "number", []);

    Module.ccall("contra_web_init", null, [], []);

    /* Optional debug warps via URL hash, e.g. index.html#level2boss */
    if (location.hash === "#level2boss") {
      Module.ccall("contra_web_warp_level2_boss", null, [], []);
    } else if (location.hash === "#level4") {
      Module.ccall("contra_web_warp_level4", null, [], []);
    }

    var fbPtr = framebuffer();
    var fbBytes = FB_WIDTH * FB_HEIGHT * 4;
    var imageData = ctx.createImageData(FB_WIDTH, FB_HEIGHT);

    if (loading) {
      loading.style.display = "none";
    }

    setupKeyboard();
    setupOnScreenController();
    setupKonami();

    var STEP_MS = 1000 / 60;
    var acc = 0;
    var last = performance.now();

    function frame(now) {
      acc += now - last;
      last = now;

      setInput(keyboardMask | pointerMask);

      /* Catch up at 60 Hz, but never spiral (cap at 5 steps like the SDL host). */
      var steps = 0;
      while (acc >= STEP_MS && steps < 5) {
        step();
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
