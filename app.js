const apiBase = "/api";

async function apiPOST(path, body) {
  const res = await fetch(apiBase + path, { method: "POST", body });
  return res.json();
}
async function apiGET(path) {
  const res = await fetch(apiBase + path);
  return res.json();
}

async function visit(url){
  const form = new URLSearchParams();
  form.append("url", url);
  await apiPOST("/visit", form);
  await refresh();
}

async function back(){ await apiPOST("/back"); await refresh(); }
async function forward(){ await apiPOST("/forward"); await refresh(); }
async function clearAll(){ if (!confirm("Clear history?")) return; await apiPOST("/clear"); await refresh(); }

async function refresh(){
  const history = document.getElementById("history");
  const cur = document.getElementById("current");
  history.textContent = "Loading…";
  try {
    const arr = await apiGET("/history");
    history.innerHTML = "";
    if (arr.length === 0){ history.innerHTML = "<em>No history</em>"; cur.textContent = "—"; return; }
    arr.forEach((it, idx) => {
      const div = document.createElement("div");
      div.className = "item" + (it.current ? " current" : "");
      div.innerHTML = `<div><div class="url">${it.url}</div><div class="meta">${it.time}</div></div>
                       <div class="small">${it.current ? "CURRENT" : ""}</div>`;
      history.appendChild(div);
      if (it.current) cur.textContent = it.url;
    });
  } catch (e) {
    history.innerHTML = "<em>Error fetching history (is server running?)</em>";
    cur.textContent = "—";
  }
}

async function doSearch(){
  const q = document.getElementById("q").value.trim();
  const out = document.getElementById("results");
  if (!q){ out.innerHTML = "<em>Enter query</em>"; return; }
  out.textContent = "Searching…";
  try {
    const j = await apiGET("/search?q=" + encodeURIComponent(q));
    out.innerHTML = "";
    if (!j.results || j.results.length === 0){ out.innerHTML = "<em>No matches</em>"; return; }
    j.results.forEach(r => {
      const div = document.createElement("div");
      div.className = "item";
      div.innerHTML = `<div><div class="url">${r.url}</div><div class="meta">${r.time}</div></div>
                       <div class="small">visits: ${r.count}</div>`;
      out.appendChild(div);
    });
  } catch (e) {
    out.innerHTML = "<em>Search failed</em>";
  }
}

/* hook UI */
document.getElementById("visit").addEventListener("click", async ()=>{
  const v = document.getElementById("url").value.trim();
  if (!v) return alert("Enter a URL");
  await visit(v);
  document.getElementById("url").value = "";
});
document.getElementById("back").addEventListener("click", back);
document.getElementById("forward").addEventListener("click", forward);
document.getElementById("clear").addEventListener("click", clearAll);
document.getElementById("refresh").addEventListener("click", refresh);
document.getElementById("searchBtn").addEventListener("click", doSearch);

/* initial load */
refresh();
