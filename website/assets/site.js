// Draws the Bijoy keyboard from window.BIJOY_LAYOUT (generated from layouts/bijoy.layout)
// and lights up keys pressed on the visitor's own keyboard. No tracking, no network.
(function () {
  "use strict";

  var layout = window.BIJOY_LAYOUT || {};
  var kb = document.getElementById("kb");
  if (!kb) return;

  // PC key rows by unshifted US label; "" is a spacer that keeps the stagger.
  var rows = [
    ["`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "="],
    ["q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", "\\"],
    ["a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", ""],
    ["z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "", ""]
  ];

  // KeyboardEvent.code -> US label.
  var codes = {
    Backquote: "`", Minus: "-", Equal: "=", BracketLeft: "[", BracketRight: "]",
    Backslash: "\\", Semicolon: ";", Quote: "'", Comma: ",", Period: ".", Slash: "/"
  };

  var layer = "main";
  var caps = {};

  function cap(label) {
    var el = document.createElement("div");
    el.className = "kb-key";
    if (label === "") {
      el.className += " is-spacer";
      return el;
    }
    el.innerHTML = '<span class="shift"></span><span class="main"></span><span class="latin"></span>';
    el.querySelector(".latin").textContent = label;
    caps[label] = el;
    return el;
  }

  function fill() {
    Object.keys(caps).forEach(function (label) {
      var key = layout[label] || {};
      var big = layer === "main" ? key.n : key.l;
      var small = layer === "main" ? key.s : key.ls;
      var el = caps[label];
      el.querySelector(".main").textContent = big || "";
      el.querySelector(".shift").textContent = small || "";
      el.classList.toggle("is-empty", !big && !small);
      if (!big && !small) el.querySelector(".main").textContent = layer === "main" ? label : "";
    });
  }

  rows.forEach(function (row) {
    var r = document.createElement("div");
    r.className = "kb-row";
    row.forEach(function (label) { r.appendChild(cap(label)); });
    kb.appendChild(r);
  });
  fill();

  Array.prototype.forEach.call(document.querySelectorAll(".layer-btn"), function (btn) {
    btn.addEventListener("click", function () {
      layer = btn.getAttribute("data-layer");
      Array.prototype.forEach.call(document.querySelectorAll(".layer-btn"), function (b) {
        b.classList.toggle("is-on", b === btn);
      });
      fill();
    });
  });

  function labelFor(event) {
    var code = event.code || "";
    if (code.indexOf("Key") === 0) return code.slice(3).toLowerCase();
    if (code.indexOf("Digit") === 0) return code.slice(5);
    return codes[code];
  }

  document.addEventListener("keydown", function (event) {
    if (event.ctrlKey || event.altKey || event.metaKey) return;
    var target = event.target;
    if (target && (target.tagName === "INPUT" || target.tagName === "TEXTAREA")) return;
    var el = caps[labelFor(event)];
    if (el) el.classList.add("is-down");
  });
  document.addEventListener("keyup", function (event) {
    var el = caps[labelFor(event)];
    if (el) el.classList.remove("is-down");
  });
  window.addEventListener("blur", function () {
    Object.keys(caps).forEach(function (label) { caps[label].classList.remove("is-down"); });
  });
})();
