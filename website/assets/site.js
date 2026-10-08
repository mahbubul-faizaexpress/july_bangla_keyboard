// July Bangla Keyboard website. No tracking, no cookies, no network requests.
(function () {
  "use strict";

  var REPO = "https://github.com/mahbubul-faizaexpress/july_bangla_keyboard";
  var LATEST = REPO + "/releases/latest/download/";

  // Everything a download button needs, per platform.
  var PLATFORMS = {
    windows: { name: "Windows", url: LATEST + "JulyBanglaKeyboard-Setup.exe", meta: "Windows 10 ও 11 · ২.৮ MB" },
    android: { name: "Android", url: LATEST + "JulyBanglaKeyboard.apk", meta: "Android ৭ বা নতুন · ০.৬ MB" },
    linux: { name: "Linux", url: null, meta: "শীঘ্রই আসছে" }
  };
  var OTHER_NAMES = { mac: "macOS", ios: "iPhone ও iPad" };

  // ---- Device detection (only to choose the default button; nothing is stored or sent) ----
  function detect() {
    var ua = navigator.userAgent || "";
    var platform = (navigator.userAgentData && navigator.userAgentData.platform) || navigator.platform || "";
    if (/android/i.test(ua)) return "android";
    if (/iphone|ipad|ipod/i.test(ua) || (platform === "MacIntel" && navigator.maxTouchPoints > 1)) return "ios";
    if (/win/i.test(platform) || /windows/i.test(ua)) return "windows";
    if (/mac/i.test(platform)) return "mac";
    if (/linux|x11|cros/i.test(platform + " " + ua)) return "linux";
    return null;
  }
  var device = detect();
  document.documentElement.setAttribute("data-device", device || "unknown");

  // Smart buttons: <a data-smart-download> becomes "Download for <your device>".
  Array.prototype.forEach.call(document.querySelectorAll("[data-smart-download]"), function (btn) {
    var p = PLATFORMS[device];
    var title = btn.querySelector("[data-title]");
    var sub = btn.querySelector("[data-sub]");
    if (p && p.url) {
      btn.href = p.url;
      if (title) title.textContent = p.name + "-এর জন্য ডাউনলোড";
      if (sub) sub.textContent = p.meta;
    } else if (p || OTHER_NAMES[device]) {
      // Linux, macOS, iPhone: not ready yet -> send to the page with every download.
      btn.href = "download.html";
      if (title) title.textContent = "সব ডাউনলোড দেখুন";
      if (sub) sub.textContent = (p ? p.name : OTHER_NAMES[device]) + " সংস্করণ শীঘ্রই আসছে";
    }
  });

  // Download page: your device first and highlighted; the others stay available.
  var list = document.getElementById("platforms");
  if (list) {
    var mine = device && list.querySelector('[data-platform="' + device + '"]');
    if (mine) {
      mine.classList.add("is-yours");
      var badge = document.createElement("span");
      badge.className = "platform-badge";
      badge.textContent = "আপনার ডিভাইস";
      mine.insertBefore(badge, mine.firstChild);
      list.insertBefore(mine, list.firstChild);
    }
    var chip = document.getElementById("detected");
    var name = device && ((PLATFORMS[device] && PLATFORMS[device].name) || OTHER_NAMES[device]);
    if (chip && name) {
      chip.querySelector("b").textContent = name;
      chip.hidden = false;
    }
    Array.prototype.forEach.call(list.querySelectorAll("[data-download]"), function (a) {
      var p = PLATFORMS[a.getAttribute("data-download")];
      if (p && p.url) a.href = p.url;
    });
  }

  // ---- Mobile menu ----
  var menuBtn = document.querySelector(".menu-btn");
  var nav = document.getElementById("nav");
  if (menuBtn && nav) {
    menuBtn.addEventListener("click", function () {
      var open = nav.classList.toggle("is-open");
      menuBtn.setAttribute("aria-expanded", open ? "true" : "false");
    });
  }

  // ---- Hero typing demo (key sequences from tests/golden) ----
  var demoText = document.getElementById("demo-text");
  var demoKeys = document.getElementById("demo-keys");
  var reduced = window.matchMedia && window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  if (demoText && demoKeys) {
    var words = [
      { keys: "g f d m", text: "আমি" },
      { keys: "h f Q V f W", text: "বাংলায়" },
      { keys: "o f b", text: "গান" },
      { keys: "o f g d", text: "গাই" }
    ];
    var out = demoText.querySelector(".typed");
    if (reduced) {
      out.textContent = words.map(function (w) { return w.text; }).join(" ");
    } else {
      var w = 0;
      var typeWord = function () {
        var word = words[w];
        var keys = word.keys.split(" ");
        demoKeys.innerHTML = "";
        var caps = keys.map(function (k) {
          var el = document.createElement("kbd");
          el.textContent = /[A-Z]/.test(k) ? "⇧" + k.toLowerCase() : k;
          demoKeys.appendChild(el);
          return el;
        });
        var i = 0;
        var press = function () {
          if (i > 0) caps[i - 1].classList.remove("is-hit");
          if (i < caps.length) {
            caps[i].classList.add("is-hit");
            i++;
            setTimeout(press, 260);
            return;
          }
          out.textContent += (w === 0 ? "" : " ") + word.text;
          w++;
          if (w < words.length) {
            setTimeout(typeWord, 500);
          } else {
            setTimeout(function () { out.textContent = ""; w = 0; typeWord(); }, 3200);
          }
        };
        setTimeout(press, 300);
      };
      typeWord();
    }
  }

  // ---- Bijoy keyboard, drawn from window.BIJOY_LAYOUT (generated from the layout file) ----
  var kb = document.getElementById("kb");
  var layout = window.BIJOY_LAYOUT;
  if (kb && layout) {
    // Widths in 1/4 key units; each row is 60 (= 15 keys) wide, like a PC keyboard.
    var rows = [
      [["`"], ["1"], ["2"], ["3"], ["4"], ["5"], ["6"], ["7"], ["8"], ["9"], ["0"], ["-"], ["="], ["⌫", 8, "fn"]],
      [["Tab", 6, "fn"], ["q"], ["w"], ["e"], ["r"], ["t"], ["y"], ["u"], ["i"], ["o"], ["p"], ["["], ["]"], ["\\", 6]],
      [["Caps", 7, "fn"], ["a"], ["s"], ["d"], ["f"], ["g"], ["h"], ["j"], ["k"], ["l"], [";"], ["'"], ["Enter", 9, "fn"]],
      [["Shift", 9, "fn", "ShiftLeft"], ["z"], ["x"], ["c"], ["v"], ["b"], ["n"], ["m"], [","], ["."], ["/"], ["Shift", 11, "fn", "ShiftRight"]],
      [["Ctrl", 6, "fn", "ControlLeft"], ["Alt", 6, "fn", "AltLeft"], ["জুলাই বাংলা", 36, "space", "Space"], ["Alt", 6, "fn", "AltRight"], ["Ctrl", 6, "fn", "ControlRight"]]
    ];
    var codes = {
      Backquote: "`", Minus: "-", Equal: "=", BracketLeft: "[", BracketRight: "]", Backslash: "\\",
      Semicolon: ";", Quote: "'", Comma: ",", Period: ".", Slash: "/",
      Backspace: "⌫", Tab: "Tab", CapsLock: "Caps", Enter: "Enter"
    };
    var caps = {};
    var layer = "main";

    rows.forEach(function (row) {
      var r = document.createElement("div");
      r.className = "kb-row";
      row.forEach(function (spec) {
        var label = spec[0], span = spec[1] || 4, kind = spec[2];
        var el = document.createElement("div");
        el.className = "kb-key" + (kind === "fn" ? " is-fn" : kind === "space" ? " is-space" : "");
        el.style.gridColumn = "span " + span;
        el.innerHTML = '<span class="shift"></span><span class="main"></span><span class="latin"></span>';
        if (kind) {
          el.querySelector(".main").textContent = label;
        } else {
          el.setAttribute("data-key", label);
        }
        caps[spec[3] || label] = el;
        r.appendChild(el);
      });
      kb.appendChild(r);
    });

    var fill = function () {
      Array.prototype.forEach.call(kb.querySelectorAll("[data-key]"), function (el) {
        var label = el.getAttribute("data-key");
        var key = layout[label] || {};
        var big = layer === "main" ? key.n : key.l;
        var small = layer === "main" ? key.s : key.ls;
        el.querySelector(".main").textContent = big || (layer === "main" ? label : "");
        el.querySelector(".shift").textContent = small || "";
        el.querySelector(".latin").textContent = big ? label : "";
        el.classList.toggle("is-unmapped", !big);
      });
    };
    fill();

    Array.prototype.forEach.call(document.querySelectorAll("[data-layer]"), function (btn) {
      btn.addEventListener("click", function () {
        layer = btn.getAttribute("data-layer");
        Array.prototype.forEach.call(document.querySelectorAll("[data-layer]"), function (b) {
          b.setAttribute("aria-pressed", b === btn ? "true" : "false");
        });
        fill();
      });
    });

    var capFor = function (event) {
      var code = event.code || "";
      if (caps[code]) return caps[code];
      if (code.indexOf("Key") === 0) return caps[code.slice(3).toLowerCase()];
      if (code.indexOf("Digit") === 0) return caps[code.slice(5)];
      return caps[codes[code]];
    };
    var typing = function (event) {
      var t = event.target;
      return t && (t.tagName === "INPUT" || t.tagName === "TEXTAREA" || t.isContentEditable);
    };
    document.addEventListener("keydown", function (event) {
      if (typing(event) || event.metaKey) return;
      var el = capFor(event);
      if (el) el.classList.add("is-down");
      if (event.key === "Shift") kb.classList.add("show-shift");
    });
    document.addEventListener("keyup", function (event) {
      var el = capFor(event);
      if (el) el.classList.remove("is-down");
      if (event.key === "Shift") kb.classList.remove("show-shift");
    });
    window.addEventListener("blur", function () {
      kb.classList.remove("show-shift");
      Array.prototype.forEach.call(kb.querySelectorAll(".is-down"), function (el) { el.classList.remove("is-down"); });
    });
  }

  // Footer year.
  Array.prototype.forEach.call(document.querySelectorAll("[data-year]"), function (el) {
    el.textContent = String(new Date().getFullYear()).replace(/\d/g, function (d) { return "০১২৩৪৫৬৭৮৯"[d]; });
  });
})();
