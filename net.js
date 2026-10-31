/*
 * Online rooms for 2-player Contra.
 *
 * Peers find each other through the public PeerJS broker (WebRTC signalling
 * only); after that the two browsers talk directly over a reliable, ordered
 * WebRTC data channel. A room is the host's PeerJS id: a fixed prefix plus a
 * short code the host reads out or shares as an invite link.
 *
 *   ContraNet.host(handlers)        open a room; handlers.onCode(code) once it
 *                                   is registered, handlers.onConnected(link)
 *                                   when player 2 arrives
 *   ContraNet.join(code, handlers)  connect to a room; handlers.onConnected(link)
 *   handlers.onError(message)       anything fatal before or after connecting
 *
 * link = { send(obj), onMessage(fn), onClose(fn), close() }
 * The returned room object has close() to cancel/leave.
 */

(function () {
  "use strict";

  var PREFIX = "nes-contra-us-room-";
  var ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"; /* no 0/O/1/I */
  var CODE_LENGTH = 5;

  function newCode() {
    var code = "";
    var bytes = new Uint8Array(CODE_LENGTH);
    crypto.getRandomValues(bytes);
    for (var i = 0; i < CODE_LENGTH; i += 1) {
      code += ALPHABET[bytes[i] % ALPHABET.length];
    }
    return code;
  }

  function normalizeCode(code) {
    return String(code || "").toUpperCase().replace(/[^A-Z0-9]/g, "");
  }

  function wrapConnection(conn) {
    var messageHandler = function () {};
    var closeHandler = function () {};
    var closed = false;
    var fireClose = function () {
      if (!closed) {
        closed = true;
        closeHandler();
      }
    };
    conn.on("data", function (data) { messageHandler(data); });
    conn.on("close", fireClose);
    conn.on("error", fireClose);
    /* WebRTC can drop silently (laptop sleeps, network change). */
    if (conn.peerConnection) {
      conn.peerConnection.addEventListener("iceconnectionstatechange", function () {
        var state = conn.peerConnection.iceConnectionState;
        if (state === "failed" || state === "closed") {
          fireClose();
        }
      });
    }
    return {
      send: function (obj) {
        if (conn.open) {
          conn.send(obj);
        }
      },
      onMessage: function (fn) { messageHandler = fn; },
      onClose: function (fn) { closeHandler = fn; },
      close: function () { closed = true; conn.close(); }
    };
  }

  function describeError(err) {
    switch (err && err.type) {
      case "peer-unavailable": return "No game with that code. Check it and try again.";
      case "network":
      case "server-error":
      case "socket-error":
      case "socket-closed": return "Can't reach the matchmaking server. Check your connection.";
      case "browser-incompatible": return "This browser doesn't support WebRTC.";
      default: return "Connection error: " + ((err && (err.type || err.message)) || err);
    }
  }

  /* STUN finds each browser's public address; TURN relays the traffic when
     a network blocks direct WebRTC connections (mobile carriers, strict
     routers). PeerJS's own free TURN servers no longer answer, so the relay
     is Metered (metered.ca, app "nes-contra", free 500 MB/month trial). A
     TURN login is public by nature: every page that uses it ships it. */
  var TURN_USER = "473eeff290d5a4175b4ce537";
  var TURN_PASS = "85WFqv91LnAVBAdK";
  var ICE_SERVERS = [
    { urls: "stun:stun.l.google.com:19302" },
    { urls: "stun:stun.relay.metered.ca:80" },
    { urls: "turn:global.relay.metered.ca:80", username: TURN_USER, credential: TURN_PASS },
    { urls: "turn:global.relay.metered.ca:80?transport=tcp", username: TURN_USER, credential: TURN_PASS },
    { urls: "turn:global.relay.metered.ca:443", username: TURN_USER, credential: TURN_PASS },
    { urls: "turns:global.relay.metered.ca:443?transport=tcp", username: TURN_USER, credential: TURN_PASS }
  ];
  /* How long Player 2 waits for the direct/relayed connection to open. */
  var CONNECT_TIMEOUT_MS = 20000;

  function getIceServers() {
    return Promise.resolve(ICE_SERVERS);
  }

  function host(handlers) {
    var peer = null;
    var cancelled = false;
    var connected = false;

    function open(iceServers) {
      var code = newCode();
      peer = new Peer(PREFIX + code, { config: { iceServers: iceServers } });
      peer.on("open", function () {
        if (!cancelled) {
          handlers.onCode(code);
        }
      });
      peer.on("connection", function (conn) {
        if (connected || cancelled) {
          /* Room is full: turn away a third browser. */
          conn.on("open", function () { conn.close(); });
          return;
        }
        /* Only the first connection that actually opens takes the seat: one
           that never gets through (blocked network) must not lock the room. */
        conn.on("open", function () {
          if (connected || cancelled) {
            conn.close();
            return;
          }
          connected = true;
          /* Stop listening on the broker; the data channel stays up. */
          peer.disconnect();
          handlers.onConnected(wrapConnection(conn));
        });
      });
      peer.on("error", function (err) {
        if (err.type === "unavailable-id" && !connected) {
          peer.destroy();
          open(iceServers); /* code already taken: roll a new one */
          return;
        }
        if (!connected && !cancelled) {
          handlers.onError(describeError(err));
        }
      });
    }

    getIceServers().then(function (iceServers) {
      if (!cancelled) {
        open(iceServers);
      }
    });
    return {
      close: function () {
        cancelled = true;
        if (peer) {
          peer.destroy();
        }
      }
    };
  }

  function join(rawCode, handlers) {
    var code = normalizeCode(rawCode);
    var cancelled = false;
    var connected = false;
    var peer = null;
    var timer = 0;

    function fail(message) {
      if (!connected && !cancelled) {
        cancelled = true;
        clearTimeout(timer);
        if (peer) {
          peer.destroy();
        }
        handlers.onError(message);
      }
    }

    getIceServers().then(function (iceServers) {
      if (cancelled) {
        return;
      }
      peer = new Peer({ config: { iceServers: iceServers } });
      peer.on("open", function () {
        var conn = peer.connect(PREFIX + code, { reliable: true, serialization: "json" });
        timer = setTimeout(function () {
          fail("Found the room, but couldn't connect to Player 1. One of your networks " +
               "blocks direct connections. Try again, or switch networks (e.g. Wi-Fi " +
               "instead of mobile data).");
        }, CONNECT_TIMEOUT_MS);
        conn.on("open", function () {
          if (cancelled) {
            conn.close();
            return;
          }
          connected = true;
          clearTimeout(timer);
          peer.disconnect();
          handlers.onConnected(wrapConnection(conn));
        });
      });
      peer.on("error", function (err) {
        fail(describeError(err));
      });
    });
    return {
      close: function () {
        cancelled = true;
        clearTimeout(timer);
        if (peer) {
          peer.destroy();
        }
      }
    };
  }

  window.ContraNet = {
    host: host,
    join: join,
    normalizeCode: normalizeCode
  };
})();
