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

  function host(handlers) {
    var peer = null;
    var cancelled = false;
    var connected = false;

    function open() {
      var code = newCode();
      peer = new Peer(PREFIX + code);
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
        connected = true;
        conn.on("open", function () {
          /* Stop listening on the broker; the data channel stays up. */
          peer.disconnect();
          handlers.onConnected(wrapConnection(conn));
        });
      });
      peer.on("error", function (err) {
        if (err.type === "unavailable-id" && !connected) {
          peer.destroy();
          open(); /* code already taken: roll a new one */
          return;
        }
        if (!connected && !cancelled) {
          handlers.onError(describeError(err));
        }
      });
    }

    open();
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
    var peer = new Peer();

    peer.on("open", function () {
      var conn = peer.connect(PREFIX + code, { reliable: true, serialization: "json" });
      conn.on("open", function () {
        if (cancelled) {
          conn.close();
          return;
        }
        connected = true;
        peer.disconnect();
        handlers.onConnected(wrapConnection(conn));
      });
    });
    peer.on("error", function (err) {
      if (!connected && !cancelled) {
        handlers.onError(describeError(err));
      }
    });
    return {
      close: function () {
        cancelled = true;
        peer.destroy();
      }
    };
  }

  window.ContraNet = {
    host: host,
    join: join,
    normalizeCode: normalizeCode
  };
})();
